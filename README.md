# universal_gauges

Basic and versatile gauges based on the WAVESHARE RP2040-LCD-1.28 module.
A small HAT enhances it:

- 2 analog sensor inputs (divider stuffed to suit the sender, 5 V supply)
- 2 digital inputs (level shifted)
- 1 ambient light sensor
- 1 button
- 2 relay outputs

The firmware sources are in the repository root; the HAT itself is a KiCad
project in `pcb/`, described in [The HAT board](#the-hat-board).

The target is the **RP2040-LCD-1.28**. The touch variant is not supported: its
user button sits on GP16, which the HAT uses for its first digital input, so
the two would share one pin.

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

## The HAT board

The HAT is a KiCad 10 project, `pcb/rpi2040-lcd-gauge.kicad_pro`: a two-layer
board, 1.6 mm, 35 x 38.7 mm, whose lower edge is a semicircle following the
module. It sits
behind the RP2040-LCD-1.28 and connects to it through J2, a 2x10 1.27 mm
header. J1 is the header on the other side of the module; it is mechanical
only, none of its pads is connected.

The schematic is hierarchical, one sheet per function:

| Sheet | File | Contents |
| ----- | ---- | -------- |
| psu | `PSU.kicad_sch` | input protection, 12 V -> 5 V buck |
| ain_1, ain_2 | `sensor_1.kicad_sch`, `sensor_2.kicad_sch` | analog input dividers |
| din_1, din_2 | `digital_in_1.kicad_sch`, `digital_in_2.kicad_sch` | level-shifted digital inputs |
| relay_1, relay_2 | `output_relay_1.kicad_sch`, `relay.kicad_sch` | low-side relay drivers |
| als | `als.kicad_sch` | ambient light sensor |
| switch | `switch.kicad_sch` | push button |

### Connectors

All field wiring is on JST XH connectors, 2.5 mm pitch.

| Ref | Type | Pin 1 | Pin 2 | Pin 3 | Pin 4 |
| --- | ---- | ----- | ----- | ----- | ----- |
| J5 | XH 4 | GND | digital_in1 | digital_in2 | +12 V in |
| J7 | XH 3 | sensor1 signal | GND | +5 V out | |
| J8 | XH 3 | sensor2 signal | GND | +5 V out | |
| J3 | XH 2 | +12 V | relay1 coil, low-side switched | | |
| J4 | XH 2 | +12 V | relay2 coil, low-side switched | | |

J2 carries GP16-GP22 to the module, +5 V to it, 3.3 V back from it, and GND.

### Power

The 12 V input passes through F1 (1 A), Q1 (P-channel MOSFET, reverse polarity
protection) and D2 (SMBJ36A TVS) onto the protected +12 V rail, which also
feeds the relay connectors. U1, an LMR16006 buck with L1 (22 uH) and the
R2/R3 feedback divider (54k9 / 10k), makes 5 V from that rail and supplies the
module through J2. The 3.3 V used by the ambient light sensor comes back from
the module's own regulator.

### Input and output stages

**Analog inputs.** Each is a divider to GND: R15 in series, the ADC pin, then
R16 to ground (R17/R18 on the second channel). **The two resistors are stuffed
to suit the sender the board is built for**, so a board and the firmware
variant flashed on it have to match.

*Pressure board, R15 = R16 = 100 k.* The sender has a voltage output and the
divider simply halves it: a 0.5-4.5 V sender spans 250-2250 mV at the pin.
Source impedance there is 50 k.

*Temperature board, R15 = 6 k, R16 = 10 k.* The sender is resistive and wired
between the connector's +5 V and its signal pin, so it becomes the top of the
divider and the pin reads

    V = 5 V x 10k / (R_sender + 16k)

which is 3.125 V with the sender shorted — comfortably under the 3.3 V full
scale — and 0 V with it open or its wire broken. Source impedance runs 4 to
7 k over the sender's range.

**Digital inputs.** Each is clamped by an SMAJ18A, divided 10 k / 100 k,
clamped again by a BZX84C12, and drives the gate of a BSS138 whose drain is
the GPIO, pulled up to 3.3 V through 10 k with 10 nF to GND. **The stage
inverts**: the GPIO reads low when the input is driven high, and idles high
when the input is open or grounded.

**Relay outputs.** GP19 and GP20 drive IRLML0060 MOSFETs through 1 k, with
100 k gate pulldowns, switching the low side of a coil fed from +12 V. There
is no flyback diode on the HAT: use a relay that has one built in, or fit one
across the coil.

**Button and light sensor.** SW1 pulls GP18 to GND, against the RP2040's
internal pull-up. U3, an SFH 5711 logarithmic light sensor, sinks its output
current into R11 (47 k) on GP26.

### Fabrication

`pcb/gerber/` holds the plot exported from KiCad 10.0.5, also zipped as
`pcb/gerber.zip`: two copper layers, mask, silkscreen, paste, and the PTH and
NPTH drill files. `pcb/rpi2040-lcd-gauge.csv` is the BOM.

## Source layout

| File | Contents |
| ---- | -------- |
| `main.c` | bring-up order and the main loop, nothing else |
| `config.h` | every number you would tune: sensor spans, scales, backlight, sampling |
| `variant.h`, `variant.c` | the gauge variants, and the one this build selects |
| `board.h`, `board.c` | pin map, I2C scan, which of the two modules is running |
| `sensors.h`, `sensors.c` | ADC readings and their conversion to engineering units |
| `gauge.h`, `gauge.c` | frame buffer, gauge face and needle |
| `demo.h`, `demo.c` | the demo sweep, only compiled into a `USE_DEMO` build |
| `w.c`, `lib/` | widgets, drawing and display, from picoclock |

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
feeds, and the gauge face drawn behind the needle. It also has to match how
the board's input divider is stuffed, see
[Input and output stages](#input-and-output-stages). SENSOR2 is not used by any
variant; its pad is only configured as an analog input so it stays
high-impedance.

Each target is declared in `CMakeLists.txt` by the `universal_gauges_variant`
function. What separates one variant from another — input span, scale, reading
granularity and gauge face — is a `gauge_variant` structure. Adding a variant
means adding a `MODE_xxx` value in `variant.h`, one structure in `variant.c`
and one `universal_gauges_variant` call in `CMakeLists.txt`. Nothing else in
the firmware tests `CURRENT_MODE`.

### Calibration

All in `config.h`. Input spans, in millivolts, measured at the ADC pin:

| Define | Default | Meaning |
| ------ | ------- | ------- |
| `TEMP_SENSOR_MV_MIN` / `_MAX` | 1470 / 2960 | ends of the temperature curve below |
| `PRESSURE_SENSOR_MV_MIN` / `_MAX` | 250 / 2250 | pressure sensor output range |
| `ALS_MV_MIN` / `_MAX` | 0 / 2700 | ambient light sensor output range |

The temperature sender is an NTC and is nowhere near linear, so it is
described by `TEMP_SENSOR_CURVE`, a table of measured (mV, °C) points ordered
by rising mV:

| mV at GP28 | °C | sender |
| ---------- | -- | ------ |
| 1470 | 50 | 17.9 k |
| 1770 | 60 | 12.3 k |
| 2030 | 70 | 8.61 k |
| 2260 | 80 | 6.14 k |
| 2450 | 90 | 4.43 k |
| 2590 | 100 | 3.32 k |
| 2700 | 110 | 2.55 k |
| 2780 | 120 | 1.91 k |
| 2860 | 130 | 1.51 k |
| 2910 | 140 | 1.17 k |
| 2960 | 150 | 0.90 k |

Between two points the reading is interpolated, outside the table it is
clamped. `TEMP_SENSOR_MV_MIN` and `_MAX` have to stay equal to the first and
last mV of the table: they are what the percentage readout uses. Re-measuring
the sender means replacing the rows, nothing else. A variant whose
`gauge_variant` carries no curve — the pressure one — keeps the plain linear
span.

Display scales, in engineering units:

| Define | Default |
| ------ | ------- |
| `MIN_TEMP` / `MAX_TEMP` | 50 / 140 °C |
| `MIN_PRESS` / `MAX_PRESS` | -0.5 / 6.5 bar |

A reading below `_MIN` or above `_MAX` is clamped, not extrapolated. Change the
mV values to match a different sensor; change the unit values only if the gauge
face graduations change too. Note that the sender reaches 150 °C while the face
stops at 140: the needle parks on the last graduation above that.

The ADC is read every `ADC_PERIOD_MS` (100 ms), oversampled `ADC_SAMPLES` (8)
times per reading. The ambient light sensor drives the backlight between
`MIN_BRIGHTNESS` and `MAX_BRIGHTNESS`.

### Day and night

The same ambient light reading picks the gauge face: the `_dark` faces and the
orange needle below `THEME_NIGHT_BELOW` (20 %), the day faces and the red
needle above `THEME_DAY_ABOVE` (25 %).

| Define | Default | Meaning |
| ------ | ------- | ------- |
| `THEME_NIGHT_BELOW` | 20 | % of the ALS span under which the night face is used |
| `THEME_DAY_ABOVE` | 25 | % over which the day face is used |
| `THEME_HOLD_MS` | 2000 | how long a reading has to disagree with the face on screen |

Between the two thresholds nothing changes, so a reading sitting on the edge
cannot flip the face back and forth; set them equal for a plain single
threshold. On top of that the new reading has to hold for `THEME_HOLD_MS`
before the face is swapped, so a bridge or a row of trees does not repaint the
screen. A demo build holds the light at `DEMO_LIGHT_PERCENT` (100 %) and so
always shows the day face; lower it below 20 to look at the night one.

## Building the image

The Pico SDK is required, and CMake has to be able to find it:

`export PICO_SDK_PATH=/path/to/pico-sdk`

Alternatively, `export PICO_SDK_FETCH_FROM_GIT=1` lets CMake clone it. Then:

`mkdir build;cd build;cmake ..;make`

This produces the three `.uf2` files in `build/`.

## Demo mode

A demo build sweeps the needle across the face, end to end and back, and
ignores the sensor. It is the quick way to check a gauge face, the needle
geometry and the end stops with nothing wired to SENSOR1. It is off by
default and has to be asked for when configuring:

`mkdir build-demo;cd build-demo;cmake -DUSE_DEMO=ON ..;make`

Its targets are named `main_<variant>_demo`, so a demo image never overwrites
a real one and `pilo oil_t` can never pick one up by mistake. `pilo` takes them
under that name, and says so before flashing:

`./tool/pilo oil_t_demo`

The sweep takes `DEMO_SWEEP_MS` (4 s) each way with a `DEMO_HOLD_MS` (500 ms)
pause at each end, and the backlight is held at `DEMO_LIGHT_PERCENT` (100 %)
so the needle stays visible whatever the ambient light. The three are in
`config.h`. The analog inputs are still read and printed on the serial link,
so a demo build also shows whether the ADC side is alive, and the first line
it prints says `demo build`.

## Flashing the image

Put the board in BOOTSEL — hold BOOT while plugging it in, or double-tap reset,
as the build links `pico_bootsel_via_double_reset` — then:

`./tool/pilo oil_t`     engine oil temperature
`./tool/pilo gearbox_t` gearbox oil temperature
`./tool/pilo oil_p`     engine oil pressure

`./tool/pilo` with no argument lists the variants, their `_demo` counterparts
and where each `.uf2` was found. It warns when the sources are newer than the binary, and takes an
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
`tool/tools.md`. The images and fonts themselves are in `img/`, and the KiCad
project for the HAT is in `pcb/`.

## Known issues

- battery display has to be adjusted depending on battery type
- a broken sender wire reads a clean 0 mV, R16 pulling the pin down, and on a
  temperature board a shorted sender reads 3125 mV. Both fall outside the
  calibration, so both are recognisable, but the firmware only clamps them to
  the ends of the scale instead of showing a fault
- the analog inputs are named `SENSOR*_FILTERED` but there is no filter: no
  capacitor sits across the divider. On a pressure board it also presents 50 k
  to the ADC pin, more than the RP2040 would like; the 6 k / 10 k of a
  temperature board is fine
- nothing on a board says which sender it is stuffed for, and nothing stops
  `pilo` from flashing the wrong variant onto it
- the layout is one annotation behind the schematic. The catch diode is D7 in
  the schematic and still D4 on the board, which therefore carries two D4s,
  and SW1 is `SW` there. The gerbers were plotted before the last schematic
  edits, so re-annotate and re-plot before ordering
- the schematic and the BOM give R15 to R18 as 100 k, the pressure stuffing;
  the 6 k / 10 k of a temperature board is only written down in `config.h`

## Credits

Based on [dawigit/picoclock](https://github.com/dawigit/picoclock) — the
display, widget, drawing and font code, and the tooling in `tool/`, come from
that project.
