#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

namespace platform {

#define CLOCK_IN_PIN 3
#define CLOCK_OUT_PIN 18
#define MUTE_PIN 22

#define S0_PIN 12
#define S1_PIN 13
#define S2_PIN 14
#define S3_PIN 15
// #define COM 26 # ADC0

bool getBootButton() {
    // Unfortunately, reading the boot button is unreliable on my second batch
    // of boards and it can crash the firmware. It is also really slow (taking
    // about a quarter of all available time in my setup at the time of writing)
    // and kinda unreliable - you have to "slowly" press the button to make sure
    // it gets checked in time.
    return false;
}
/*
bool __no_inline_not_in_flash_func(getBootButton)() {
    const uint CS_PIN_INDEX = 1;

    // Must disable interrupts, as interrupt handlers may be in flash, and we
    // are about to temporarily disable flash access!
    uint32_t flags = save_and_disable_interrupts();

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
#if PICO_RP2040
    #define CS_BIT (1u << 1)
#else
    #define CS_BIT SIO_GPIO_HI_IN_QSPI_CSN_BITS
#endif
    bool button_state = !(sio_hw->gpio_hi_in & CS_BIT);

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}
*/

}  // namespace platform

#endif // PLATFORM_GPIO_H