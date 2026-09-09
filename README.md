# universal_gauges

Basic and versatile gauges based on the WAVESHARE RP2040-LCD-1.28 module.
A small HAT enhances it:

- 2 analog inputs (sensor, filtered)
- 2 digital inputs (level shifted)
- 1 ambient light sensor
- 1 button
- 2 relay outputs

Both the **RP2040-LCD-1.28** and the **RP2040-TOUCH-LCD-1.28** are supported:
the touch controller is detected on the I2C bus at boot and the reset pin is
adjusted accordingly, so the same `.uf2` runs on either board.

## Pinout

Signals added by the HAT:

| GPIO | Signal | Direction |
| ---- | ------ | --------- |
| GP16 | shifted_digital_in1 | input |
| GP17 | shifted_digital_in2 | input |
| GP18 | button_input | input, pull-up |
| GP19 | relay1_command_pin | output |
| GP20 | relay2_command_pin | output |
| GP26 | als_out | analog input, ADC0 |
| GP27 | sensor2_filtered | analog input, ADC1 |
| GP28 | sensor1_filtered | analog input, ADC2 |

The RP2040 ADC channel is not the GPIO number: GP26 is ADC0, GP27 is ADC1,
GP28 is ADC2, and GP29 (ADC3) is the module's own battery sense.

Pins used by the module itself (display, I2C, IMU, backlight, battery) are
defined in `lib/lcd/lcd.h`.

## Build variants

The firmware is built in three variants, selected at compile time by
`CURRENT_MODE`. One `cmake` + `make` builds all three:

| Variant | `CURRENT_MODE` | Gauge | Output |
| ------- | -------------- | ----- | ------ |
| engine oil temperature | `MODE_OIL_T` | 50-140 °C | `main_oil_t.uf2` |
| gearbox oil temperature | `MODE_GEARBOX_T` | 50-140 °C | `main_gearbox_t.uf2` |
| engine oil pressure | `MODE_OIL_P` | -0.5-6.5 bar | `main_oil_p.uf2` |

**The sensor is always wired to SENSOR1 (GP28), in every variant.** What the
variant changes is the input span applied to that reading, the quantity it
feeds, and the gauge face drawn behind the needle. SENSOR2 is not used by any
variant; its pad is only configured as an analog input so it stays
high-impedance.

Each target is declared in `CMakeLists.txt` by the `universal_gauges_variant`
function. Adding a variant means adding a `MODE_xxx` value in `main.c` and one
call there.

### Calibration

All in `main.c`. Input spans, in millivolts, measured at the ADC pin:

| Define | Default | Meaning |
| ------ | ------- | ------- |
| `TEMP_SENSOR_MV_MIN` / `_MAX` | 640 / 2900 | temperature sensor output range |
| `PRESSURE_SENSOR_MV_MIN` / `_MAX` | 250 / 2250 | pressure sensor output range |
| `ALS_MV_MIN` / `_MAX` | 0 / 2700 | ambient light sensor output range |

Display scales, in engineering units:

| Define | Default |
| ------ | ------- |
| `MIN_TEMP` / `MAX_TEMP` | 50 / 140 °C |
| `MIN_PRESS` / `MAX_PRESS` | -0.5 / 6.5 bar |

A reading below `_MIN` or above `_MAX` is clamped, not extrapolated. Change the
mV values to match a different sensor; change the unit values only if the gauge
face graduations change too.

The ADC is read every `ADC_PERIOD_MS` (100 ms), oversampled `ADC_SAMPLES` (8)
times per reading. The ambient light sensor drives the backlight between
`MIN_BRIGHTNESS` and `MAX_BRIGHTNESS`.

## Building the image

The Pico SDK is required, and CMake has to be able to find it:

`export PICO_SDK_PATH=/path/to/pico-sdk`

Alternatively, `export PICO_SDK_FETCH_FROM_GIT=1` lets CMake clone it. Then:

`mkdir build;cd build;cmake ..;make`

This produces the three `.uf2` files in `build/`.

## Flashing the image

Put the board in BOOTSEL — hold BOOT while plugging it in, or double-tap reset,
as the build links `pico_bootsel_via_double_reset` — then:

`./tool/pilo oil_t`     engine oil temperature
`./tool/pilo gearbox_t` gearbox oil temperature
`./tool/pilo oil_p`     engine oil pressure

`./tool/pilo` with no argument lists the variants and where each `.uf2` was
found. It warns when the sources are newer than the binary, and takes an
explicit file too: `./tool/pilo build/main_oil_p.uf2`. See
`tool/tools.md` for the environment overrides.

The equivalent by hand:

`picotool load ./build/main_oil_t.uf2 -x`

`sudo` is needed on Linux but not on macOS. With several boards connected,
list them with `picotool info -a` and target one with
`--bus <bus> --address <addr>`.

## Releases

Firmware images are not versioned in git; they are attached to GitHub
releases. Pushing a `v*` tag builds the three variants on GitHub and publishes
them:

`git tag v1.0.0 && git push origin v1.0.0`

The workflow is `.github/workflows/release.yml`. It also runs on
`workflow_dispatch`, and every run uploads the `.uf2` files as build
artifacts, so a build can be checked without publishing anything.

To publish from this machine instead — same three variants, built locally and
uploaded with the GitHub CLI:

`./tool/pirel v1.0.0`

`pirel` refuses to run on a dirty working tree, and passes extra arguments
through to `gh release create` (`--prerelease`, `--draft`, `--notes ...`).
Prefer the tag-triggered workflow for anything official: it does not depend on
this machine.

To flash an image downloaded from a release, give `pilo` its path:

`./tool/pilo ~/Downloads/main_oil_p.uf2`

## Tools

Scripts for flashing, releasing, serial shell, LCD screenshots, and converting
images and fonts into header files live in `tool/`, documented in
`tool/tools.md`. The images and fonts themselves are in `img/`.

## Known issues

- battery display has to be adjusted depending on battery type
- `NIGHT_THEME` and the `_dark` gauge faces exist but are never selected:
  `current_theme` stays on `DAY_THEME`
- a disconnected sensor leaves its input floating, which reads as a low but
  plausible value rather than as a fault

## Credits

Based on [dawigit/picoclock](https://github.com/dawigit/picoclock) — the
display, widget, drawing and font code, and the tooling in `tool/`, come from
that project.
