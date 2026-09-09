#ifndef CONFIG_H
#define CONFIG_H

/* Every value you would touch to fit a different sensor, a different gauge
   face or a different backlight behaviour lives here. Nothing in this file
   depends on the build variant: variant.c picks which of these spans and
   scales the firmware is built with. */

#include <stdint.h>

/* ---------- ADC sampling ---------- */

#define ADC_SAMPLES   (8)          /* oversampling to smooth out noise */
#define ADC_PERIOD_MS (100)        /* read period in the main loop */
#define ADC_VREF_MV   (3300u)      /* ADC full scale, in mV */
#define ADC_RANGE     (1 << 12)    /* 12-bit ADC -> 0..4095 */
#define ADC_MAX       (ADC_RANGE - 1)

/* ---------- sensor input spans ----------
   In millivolts, measured at the ADC pin. The HAT's input divider, R15 then
   R16 (R17/R18 on the second channel), is stuffed to suit the sender the
   board is built for, so the firmware variant and the board have to match. */

/* Temperature board: R15 6k, R16 10k. The sender is resistive and wired
   between the connector's +5 V and its signal pin, so it becomes the top of
   the divider and the pin reads 5 V * 10k / (R + 16k) - 3125 mV with the
   sender shorted, 0 mV with it open or its wire broken.

   The sender is an NTC: its output is not remotely linear, so it is described
   by the curve below rather than by a span. Measured at the ADC pin, GP28,
   with the sender resistance shown for the record.

   Points must be ordered by rising mV, and MV_MIN / MV_MAX must stay equal to
   the first and last of them - they are what the percentage readout and the
   clamps use. Between two points the reading is interpolated; outside the
   table it is clamped, not extrapolated. */
#define TEMP_SENSOR_MV_MIN     1470
#define TEMP_SENSOR_MV_MAX     2960

#define TEMP_SENSOR_CURVE {   \
  { 1470,  50.f },  /* 17.9 k */ \
  { 1770,  60.f },  /* 12.3 k */ \
  { 2030,  70.f },  /*  8.61 k */ \
  { 2260,  80.f },  /*  6.14 k */ \
  { 2450,  90.f },  /*  4.43 k */ \
  { 2590, 100.f },  /*  3.32 k */ \
  { 2700, 110.f },  /*  2.55 k */ \
  { 2780, 120.f },  /*  1.91 k */ \
  { 2860, 130.f },  /*  1.51 k */ \
  { 2910, 140.f },  /*  1.17 k */ \
  { 2960, 150.f },  /*  0.90 k */ \
}

/* Pressure board: R15 and R16 both 100k. The sender has a voltage output, the
   divider simply halves it, and a 0.5-4.5 V sender lands here. Linear, so no
   curve. */
#define PRESSURE_SENSOR_MV_MIN 250
#define PRESSURE_SENSOR_MV_MAX 2250

#define ALS_MV_MIN             0
#define ALS_MV_MAX             2700

/* ---------- display scales ----------
   Engineering units at the two ends of the gauge face. Change these only if
   the graduations drawn on the face change too. */

#define MIN_TEMP   50.f
#define MAX_TEMP   140.f

#define MIN_PRESS  -0.5f
#define MAX_PRESS  6.5f

/* Reading granularity, in engineering units. The temperature gauges read in
   whole degrees; 0 lets the needle follow the raw conversion. */
#define TEMP_STEP  1.f
#define PRESS_STEP 0.f

/* ---------- backlight ----------
   Driven by the ambient light sensor, between these two limits. */

#define MIN_BRIGHTNESS (uint8_t)1
#define MAX_BRIGHTNESS (uint8_t)100

/* ---------- day and night faces ----------
   The same ambient light reading picks the gauge face. Two thresholds rather
   than one, so a reading sitting right on the edge cannot flip the face back
   and forth; set them equal for a plain single threshold. On top of that the
   new reading has to hold for THEME_HOLD_MS, so a bridge or a row of trees
   does not repaint the screen. */

#define THEME_NIGHT_BELOW (20)    /* % of the ALS span: under it, night face */
#define THEME_DAY_ABOVE   (25)    /* over it, day face; between the two, no
                                     change */
#define THEME_HOLD_MS     (2000)  /* how long the reading has to disagree with
                                     the face on screen before it is swapped */

/* ---------- main loop ---------- */

#define FRAME_PERIOD_MS (10)

/* ---------- demo mode ----------
   Only used by builds configured with -DUSE_DEMO=ON. The needle sweeps the
   face end to end instead of following the sensor. */

#define DEMO_SWEEP_MS       (4000)  /* one end of the face to the other */
#define DEMO_HOLD_MS        (500)   /* pause at each end, to see the stops */
#define DEMO_LIGHT_PERCENT  (100)   /* backlight, so the sweep stays visible
                                       whatever the ambient light */

#endif /* CONFIG_H */
