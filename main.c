/* universal_gauges - entry point.
 *
 * Everything the firmware does is split across five modules:
 *   config.h   the numbers you would tune: sensor spans, scales, backlight
 *   variant.c  the three gauge variants, one of them selected at build time
 *   board.c    the pins, the I2C scan and which module we are running on
 *   sensors.c  the ADC readings and their conversion to engineering units
 *   gauge.c    the frame buffer, the face and the needle
 * What is left here is the bring-up order and the main loop.
 *
 * Built with -DUSE_DEMO=ON, demo.c takes the place of the sensor and sweeps
 * the needle across the face. That is a bench aid, not a normal build.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"

#include "lcd.h"
#include "QMI8658.h"
#include "CST816S.h"

#include "board.h"
#include "config.h"
#include "gauge.h"
#include "sensors.h"
#include "variant.h"

#ifdef DEMO_MODE
#include "demo.h"
#endif

static void init(void) {
  sleep_ms(100);  /* "Rain-wait": let the other chips come up */
  rtc_init();
  stdio_init_all();

  /* Give the USB host a couple of seconds to enumerate, but do not hang
     waiting for it: the gauge has to work without a computer attached. */
  for (int i = 0; i < 10 && !stdio_usb_connected(); i++) {
    sleep_ms(250);
  }
  sleep_ms(300);

  board_init_gpio();
  sensors_init();

  /* Before lcd_init(): the scan is what tells a touch board from a plain one,
     and the reset pin differs between the two. */
  board_i2c_scan();

  lcd_init();
  gauge_init_display();

  CST816S_init(CST816S_Gesture_Mode);
  board_init_module_inputs();
  QMI8658_init();

  gauge_init_widgets();
}

int main(void) {
  init();

#ifdef DEMO_MODE
  printf("[%s] demo build: the needle sweeps, the sensor is ignored\n",
         gauge_variant.name);
#endif

  absolute_time_t adc_next = get_absolute_time();

  while (true) {
    if (absolute_time_diff_us(get_absolute_time(), adc_next) <= 0) {
      adc_next = delayed_by_ms(get_absolute_time(), ADC_PERIOD_MS);

      sensors_update();

      printf("[%s] ALS raw=%u %umV %u%%   S1 raw=%u %umV %u%%\n",
             gauge_variant.name,
             als.raw, als.mv, als.percent,
             sensor1.raw, sensor1.mv, sensor1.percent);

#ifndef DEMO_MODE
      gauge_set_light(als.percent);
      gauge_set_value(sensors_value());
#endif
    }

#ifdef DEMO_MODE
    /* Refreshed every frame rather than every ADC period, so the needle
       sweeps smoothly. The inputs above are still read and logged. */
    gauge_set_light(DEMO_LIGHT_PERCENT);
    gauge_set_value(demo_value());
#endif

    gauge_draw();
    sleep_ms(FRAME_PERIOD_MS);
  }

  return 0;
}
