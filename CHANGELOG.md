### Changelog:

#### universal_gauges (fork)
- main.c split into modules: config.h (calibration), variant.c (the gauge
  variants, as a gauge_variant structure instead of scattered #if), board.c
  (pins and board detection), sensors.c (ADC), gauge.c (face and needle).
  main.c is now the bring-up order and the main loop
- the needle is clamped to the two ends of the gauge face instead of to zero,
  so the -0.5 to 0 bar part of the pressure scale is finally reachable
- support for the RP2040-TOUCH-LCD-1.28 dropped, with the CST816S driver: its
  button is on GP16, which the HAT uses as digital input 1. The board is now
  the RP2040-LCD-1.28, the I2C scan is only a boot diagnostic
- 'pilo' takes the demo targets as <variant>_demo, looks in build-demo/ as well
  as build/, and its stale-build check now covers every module, not just main.c
- demo mode, off by default, built with cmake -DUSE_DEMO=ON: the needle sweeps
  the face instead of following the sensor, targets named main_<variant>_demo
- 3 build variants (engine oil temp / gearbox oil temp / engine oil pressure),
  selected at compile time by CURRENT_MODE, one CMake target each
- sensor reading on SENSOR1 (ADC2) with oversampling, mV conversion and
  per-sensor calibration spans; ambient light sensor drives the backlight
- 'pilo' rewritten: takes a variant name, locates the .uf2, warns on stale
  builds, sudo on Linux only
- firmware images are no longer in git: attached to GitHub releases, built by
  .github/workflows/release.yml on a v* tag, or locally with 'pirel'

#### upstream (dawigit/picoclock)
- all tools/scripts moved to folder 'tool'
- img2data.md -> tool/tools.md
- spi transfer (at full speed) via dma -> 20fps
- raspberry texture added
- some seamless textures added (good with rotozoom)
- texture mapping (using hardware_interp)
  based on: [https://github.com/raspberrypi/pico-examples/tree/master/pio/st7789_lcd]
  (quadruple draw, mirroring) (rotate / rotozoom)
- gyroscope error correction
- 'main.uf2' for  RP2040-TOUCH-LCD-1.28 /  RP2040-LCD-1.28
- 'scandir' command added [0-3] (hardware) rotate display by 90°
- 'editpos' command added [0-8]
- load/save flash fixed. [restore save ram from flash now works]
- added new icons
- added config (next to center, opposite dotw)
- added more textures
- removed pointerdemo
- added textured pointers
- added alpha pointers
- added rotating background (themes 0,1)
- added rotozoom (from hagl [https://github.com/tuupola/hagl]) (cannon travel!)
- added "pico_bootsel_via_double_reset" in CMakeLists.txt
- more vars (values) now resist in 'noinit' ram
- battery display will flicker a lot at start (or when bat is lo) as it finds new min/max values
- when sleeping and a new lo-bat minimum was found, it saves (new minimum value)
- changed shell commands
- added flash ram
- added save: press the button for >3 sec when in center position (cursor/flag) to save save ram to flash ram
- gyrocross improved, shows ghosted old position
- battery display improved
- bending second pointer fixed at the star
- lcd_rect -> lcd_frame (and other changes in lcd)
- changed button system (internal)
- added battery types
- changed shell commands, see below
- snapshots with 'snaps' script: `snaps 0 mysnapfilename` [/dev/ttyACM0] (minicom must be stopped before)
- added shell: file 'pigsh' contains a simple bash script sending your input to /dev/ttyACMx (minicom has to be started before)
- 'pigsh 0' to connect to  /dev/ttyACM0
- 'pigsh 1' to connect to  /dev/ttyACM1
