# Pico SI5351 + 47L16 Frequency Synthesizer

RP Pico VFO / clock generator using a Silicon Labs **SI5351A**: tune from **2 kHz to 250 MHz** in **1 Hz** steps (firmware clamps are listed below). Dual I2C buses drive the SI5351, OLED displays, and a Microchip **47L16** EERAM so frequency and step settings survive power cycles.

**Status:** working bench firmware. Project page: [k9dtv.com/si5351a.html](https://k9dtv.com/si5351a.html)

## Hardware

| Item | Role |
|------|------|
| Raspberry Pi Pico (RP2040) | MCU |
| SI5351A module (25 MHz XTAL) | Clock / synthesizer output (CLK0) |
| SH1106 128x64 OLED | Main frequency / step display (I2C0) |
| SSD1306 64x32 OLED | Mini readout (I2C1) |
| 47L16 EERAM | Non-volatile settings (I2C1) |
| Rotary encoder + switch | Tune / step / auto-step |
| Back + Confirm buttons | Recall / save settings |

### Pins (`vfo.h`)

| Signal | GPIO |
|--------|------|
| I2C0 SDA / SCL (SI5351, SH1106) | GP0 / GP1 |
| I2C1 SDA / SCL (47L16, SSD1306) | GP26 / GP27 |
| Back | GP11 |
| Confirm | GP12 |
| Encoder A / B / SW | GP13 / GP14 / GP15 |

I2C runs at 800 kHz on both buses.

## Firmware range and steps

| Item | Value |
|------|-------|
| Product target | 2 kHz – 250 MHz, 1 Hz resolution |
| Current clamps (`minFreq` / `maxFreq`) | 5 kHz – 160 MHz |
| Step sizes | 1 Hz, 10 Hz, 100 Hz, 1 kHz, 10 kHz, 100 kHz, 1 MHz, 10 MHz |
| Default boot frequency | 10 MHz |

Widen the clamps in `vfo.h` when you are ready to push the SI5351 harder; the Multisynth / R-div / DIVBY4 path in `si5351.cpp` already covers low and VHF bands.

## Controls

- **Rotate encoder** — change frequency by the current step.
- **Short press encoder** — next step size.
- **Long press encoder** (~350 ms) — toggle auto-step (step size grows when you roll past a decade digit).
- **Confirm** — force-save settings to 47L16 SRAM (encoder changes also auto-save when dirty).
- **Back** — reload last saved settings from SRAM and re-apply the SI5351.

## How the SI5351 path works

1. **CLK0 only** — CLK1/CLK2 stay powered down; output enabled after init.
2. **Low frequencies** — an R-divider (1…128) keeps the Multisynth target above ~500 kHz.
3. **Mid band** — even Multisynth divider targeting ~750 MHz VCO; fractional PLLA feedback from the calibrated XTAL.
4. **≥ 100 MHz** — Multisynth DIVBY4 (integer mode) for VHF.
5. **Calibration** — `OFFSET_AT_150MHZ` in `vfo.h` scales a measured error at 150 MHz into a corrected 25 MHz XTAL constant (`CALIBRATED_XTAL`).

## 47L16 settings

Packed 32-byte record with signature, version, sequence, frequency, step index, auto-step flag, tail magic, and CRC32. Written to EERAM SRAM on change; validated on boot. Invalid / missing data falls back to 10 MHz, 1 Hz step, auto-step on.

## Libraries

- [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110x)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) (dependency)

Board package: [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico)

## Build

1. Install Arduino IDE + Arduino-Pico (RP2040) board package.
2. Install the Adafruit libraries above.
3. Open `Pico_SI5351_47l16.ino` from this folder (folder name must match the `.ino` name).
4. Select **Raspberry Pi Pico**, compile, and upload.
5. Set `OFFSET_AT_150MHZ` after measuring CLK0 at 150 MHz if you need tighter absolute accuracy.

GitHub Actions compiles the sketch on push (compile check only).

## Layout

| File | Role |
|------|------|
| `Pico_SI5351_47l16.ino` | `setup` / `loop` |
| `main.cpp` | Init and main loop logic |
| `vfo.h` | Pins, limits, shared state |
| `si5351.cpp` | SI5351 register math and I2C writes |
| `frequency.cpp` | Tuning, steps, auto-step |
| `memory.cpp` | 47L16 load / save / CRC |
| `display.cpp` | SH1106 + SSD1306 UI |
| `encoder.cpp` | Quadrature + long-press |
| `buttons.cpp` | Back / Confirm debounce |

## License

Project files are published for personal / educational use under K9DTV. See the site write-up for the story and wiring notes.
