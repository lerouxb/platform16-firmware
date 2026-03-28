# Platform 16

A Raspberry Pi Pico 2-based digital synthesizer platform with 16 potentiometers,
clock in/out, and I2S stereo audio output. It hosts swappable generative
instrument firmwares that are selected at compile time.

## Hardware

- Raspberry Pi Pico 2
- 16 potentiometers read through a CD4067-style 4-to-16 analog multiplexer
- I2S DAC (data on GPIO 9, clock base on GPIO 10)
- Clock input with normalling detection (for syncing to external gear)
- Clock output (Teenage Engineering-style, every second 16th note)

Audio runs at 24 kHz stereo with 256-sample buffers.

## Firmwares

Only one firmware is active at a time — uncomment the desired `#include` in
platform16.cpp and rebuild.

### SDS — Stochastic Decay (Subtractive)

A generative subtractive monosynth. A PolyBLEP saw oscillator and optional
coloured noise are fed through a Moog-style 4-pole ladder filter (LP24/HP24)
with resonance and a soft-clip overdrive stage.

Sequencing is built around 32 random steps with probability-based skips. Each
step carries per-step pitch and filter modulation amounts. Nine "algorithm"
modes sort the pitch data into melodic contours (triangles, interleaved
patterns, etc.). An "evolve" knob gradually mutates the sequence over time.
Pitch is quantized to one of eight musical scales (or left unquantized).
Bidirectional attack-or-decay envelopes control both volume and filter cutoff.

### PMD — Phase Modulation with Decay

A generative 3-voice FM/PM chord synthesizer. Three parallel 2-operator phase
modulation voices form a triad (root, 3rd, 5th) derived from the harmonic minor
chord scale. Attack-or-decay envelopes shape each note. Two sine LFOs modulate
the FM modulation depth ("timbre") and the envelope time.

Sequencing uses a procedural sequencer driven by density, complexity, spread, and
bias parameters. A "scramble" knob probabilistically mutates the sequence seeds.

### TEP — Two-tone Euclidean Polymeters

A dual-oscillator arpeggiator driven by three independent Euclidean rhythms.
Two detunable PolyBLEP saw oscillators feed a Moog ladder filter (LP24) with a
soft-clip overdrive stage.

The three Euclidean rhythms independently control volume accents, cutoff accents,
and when the arpeggio advances to the next chord tone. Chords are 4-note
voicings (triad + 6th) from the natural minor scale. Ten arpeggio modes are
available: up, down, up-down, down-up, converge, diverge, converge-diverge,
diverge-converge, random, and off. Per-step glide smooths frequency, cutoff, and
volume transitions.

## Library

Reusable DSP and utility modules shared across firmwares, found in `lib/`:

| Module            | Description                                                            |
|-------------------|------------------------------------------------------------------------|
| oscillator.hpp    | Multi-waveform oscillator with PolyBLEP anti-aliasing (from DaisySP)  |
| ladder.hpp        | Moog-style 4-pole ladder filter, Huovilainen model (from DaisySP)     |
| variablesawosc.hpp| Variable-shape saw/notch oscillator (from Mutable Instruments Plaits) |
| pm2.hpp           | 2-operator phase modulation (FM) synth (from DaisySP)                 |
| decay.hpp         | Simple exponential decay envelope                                      |
| attackordecay.hpp | Bidirectional attack-or-decay envelope                                 |
| sequencer.hpp     | Procedural 32-step gate + CV sequencer                                 |
| rhythms.hpp       | Pre-computed Euclidean rhythm pattern lookup table                      |
| arpeggio.hpp      | 10-mode arpeggiator (up, down, converge, diverge, random, etc.)        |
| quantize.hpp      | Pitch quantization to scales, chord type lookup, 88-key frequency table|
| metro.hpp         | Metronome / clock tick generator (from DaisySP)                        |
| inoutclock.hpp    | Internal/external clock manager with division and multiplication       |
| pots.hpp          | 16-pot multiplexer reader with interpolation                           |
| buttons.hpp       | Debounced button input with single/double/long press detection         |
| gpio.hpp          | Pin definitions and boot button helper                                 |
| parameters.hpp    | Typed parameter mappings (bipolar, integer range, exponential, etc.)   |
| utils.hpp         | DSP utilities: soft clip, PolyBLEP helpers, clamping, random          |

## Architecture

Each firmware follows a three-part pattern:

- **State** (`*-state.hpp`) — a plain struct holding all parameter values.
- **Controller** (`*-controller.hpp`) — maps the 16 physical pots to state
  parameters using the typed parameter abstractions from `lib/parameters.hpp`.
- **Instrument** (`*-instrument.hpp`) — the DSP engine. Exposes `init()`,
  `update()` (called once per buffer to read state), and `process()` (called
  once per sample to produce audio).

The main loop in `platform16.cpp` ties it together: read one pot per iteration,
update the controller, fill an audio buffer sample-by-sample, and hand it off
to I2S DMA.

## Building

Requires the Raspberry Pi Pico SDK (v2.1.0) and Pico Extras. The VS Code
Raspberry Pi Pico extension handles toolchain setup. To build:

1. Configure with CMake (the extension does this automatically).
2. Run the "Compile Project" task, or from the terminal:
   ```
   ninja -C build
   ```
3. Flash `build/platform16.uf2` to the Pico 2 via USB or use the "Run Project"
   task with picotool.

## Project Structure

```
├── platform16.cpp              Main application entry point
├── CMakeLists.txt              Build configuration
├── firmware/
│   ├── sds/                    Stochastic Decay (Subtractive)
│   ├── pmd/                    Phase Modulation with Decay
│   └── tep/                    Two-tone Euclidean Polymeters
├── lib/                        Shared DSP and utility modules
├── build/                      Build output (generated)
├── pico_sdk_import.cmake       Pico SDK integration
└── pico_extras_import.cmake    Pico Extras integration (I2S audio)
```
