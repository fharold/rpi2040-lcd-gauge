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
   In millivolts, measured at the ADC pin. The HAT halves the sensor voltage
   with a 100k/100k divider, so a 0-5 V sender lands in 0-2500 mV here. */

#define TEMP_SENSOR_MV_MIN     640
#define TEMP_SENSOR_MV_MAX     2900

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
