/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include <stdio.h>

#define SAMPLE_RATE 48000
#define HALF_SAMPLE_RATE 24000.f

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/structs/clocks.h"

#include "pico/audio_i2s.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "pico/rand.h"

#include "lib/gpio.hpp"
#include "lib/pots.hpp"
#include "lib/buttons.hpp"
#include "firmware/sds/sds-instrument.hpp"

using namespace platform;

bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN,
                            "I2S DIN",
                            PICO_AUDIO_I2S_CLOCK_PIN_BASE,
                            "I2S BCK",
                            PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1,
                            "I2S LRCK"));


#define SAMPLES_PER_BUFFER 256

struct audio_buffer_pool* init_audio() {

  static audio_format_t audio_format = {
    .sample_freq = SAMPLE_RATE,
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .channel_count = 2,
  };

  static struct audio_buffer_format producer_format = {.format = &audio_format, .sample_stride = 4};

  struct audio_buffer_pool* producer_pool =
    audio_new_producer_pool(&producer_format,
                            3,
                            SAMPLES_PER_BUFFER);  // todo correct size
  bool __unused ok;
  const struct audio_format* output_format;
  struct audio_i2s_config config = {
    .data_pin = PICO_AUDIO_I2S_DATA_PIN,
    .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
    .dma_channel = 0,
    .pio_sm = 0,
  };

  output_format = audio_i2s_setup(&audio_format, &config);
  if (!output_format) {
    panic("PicoAudio: Unable to open audio device.\n");
  }

  ok = audio_i2s_connect(producer_pool);
  assert(ok);
  audio_i2s_set_enabled(true);
  return producer_pool;
}


int main() {
  // we could also just use get_rand_32() everywhere, but I'm just using it to
  // get a random seed for the c rand() function for now
  srand(get_rand_32());

  // TODO: can we make this optional and only enable for debugging?
  stdio_init_all();

  adc_init();
  // TODO: do we need this?
  adc_set_temp_sensor_enabled(true);
  adc_gpio_init(26);
  adc_select_input(0);

  gpio_init(MUTE_PIN);
  gpio_set_dir(MUTE_PIN, GPIO_OUT);
  gpio_put(MUTE_PIN, true);

  gpio_init(CLOCK_IN_PIN);
  gpio_set_dir(CLOCK_IN_PIN, GPIO_IN);
  
  gpio_init(CLOCK_OUT_PIN);
  gpio_set_dir(CLOCK_OUT_PIN, GPIO_OUT);
  gpio_put(CLOCK_OUT_PIN, false);

  platform::Pots pots(S0_PIN, S1_PIN, S2_PIN, S3_PIN);
  pots.init();
  platform::ButtonInput bootButton;
  platform::SDSInstrument instrument(pots, bootButton);
  instrument.init(SAMPLE_RATE);

  struct audio_buffer_pool* ap = init_audio();
  auto tickStart = time_us_64();
  uint64_t total = 0;

  while (true) {
    bool bootButtonState = false;
    pots.process();
    instrument.update();

    auto start = time_us_64();
    struct audio_buffer* buffer = take_audio_buffer(ap, true);
    auto end = time_us_64();
    auto timeTaken = end - start;
    total += timeTaken;

    int16_t* samples = (int16_t*)buffer->buffer->bytes;
    for (uint i = 0; i < buffer->max_sample_count; i++) {
      // checking the boot button is quite slow, so how frequently we check it
      // is a compromise and unfortunately that means we can miss quick presses
      if (i % 16 == 0) {
        bootButtonState = getBootButton();
      }
      bootButton.update(bootButtonState);
      //printf("%d, %d, %d\n", bootButtonState, bootButton.isDown, bootButton.isPressed);
      float sample = instrument.process();
      //bootButtonState = true;
      int16_t sampleInt = (int16_t)(sample * 32767.f);
      samples[i * 2] = sampleInt;
      samples[i * 2 + 1] = sampleInt;
    }
    buffer->sample_count = buffer->max_sample_count;

    start = time_us_64();
    give_audio_buffer(ap, buffer);
    end = time_us_64();
    timeTaken = end - start;
    total += timeTaken;
    
    if (end - tickStart > 1000000) {
      // this is how long we busy-waited for the audio buffers to drain in the
      // last second because they were all full. ie. roughly how much of each
      // second we have "spare"
      printf("%.2fms\n", total/1000.f);
      //printf("%.2f\n", instrument.getState()->shape.value);
      //printf("%.2f\n", instrument.getState()->filterResonance.getScaled());
      //printf("%d\n", instrument.getState()->shape.getScaled());
      // TODO: should we somehow take into account the remainder?
      total = 0;
      tickStart = end; 
    }
  }

  puts("\n");
  return 0;
}