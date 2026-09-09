cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
set(PICO_BOARD="waveshare_rp2040_lcd_1.28")
project(universal_gauges)
pico_sdk_init()

add_subdirectory(./lib/lcd)
include_directories(./lib/lcd)
add_subdirectory(./lib/QMI8658)
include_directories(./lib/QMI8658)

option(USE_DEVMODE "using DEVMODE" OFF)
if (USE_DEVMODE)
  add_definitions(-DDEVMODE)
endif()

set(UNIVERSAL_GAUGES_SOURCES
  main.c
  lib/draw.c
  w.c
  CST816S.c
)

# One executable per build variant. The variant is selected at compile time
# through CURRENT_MODE, which main.c reads (see its "build variant" block).
#   universal_gauges_variant(<target name> <MODE_xxx>)
function(universal_gauges_variant TARGET MODE)
  add_executable(${TARGET} ${UNIVERSAL_GAUGES_SOURCES})

  target_compile_definitions(${TARGET} PRIVATE CURRENT_MODE=${MODE})

  # enable usb output, disable uart output
  pico_enable_stdio_usb(${TARGET} 1)
  pico_enable_stdio_uart(${TARGET} 0)

  # create map/bin/hex/uf2 file etc.
  pico_add_extra_outputs(${TARGET})

  target_link_libraries(${TARGET}
    lcd QMI8658
    pico_stdlib
    hardware_dma
    hardware_spi
    hardware_i2c
    hardware_pwm
    hardware_rtc
    hardware_timer
    hardware_adc
    hardware_flash
    hardware_sync
    hardware_interp

    pico_bootsel_via_double_reset
  )
endfunction()

universal_gauges_variant(main_oil_t   MODE_OIL_T)    # engine oil temperature
universal_gauges_variant(main_gearbox_t MODE_GEARBOX_T) # gearbox oil temperature
universal_gauges_variant(main_oil_p   MODE_OIL_P)    # engine oil pressure
