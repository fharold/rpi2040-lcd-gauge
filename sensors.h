#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

/* One analog input, in the three forms the firmware uses. */
typedef struct {
  uint16_t raw;      /* 0..ADC_MAX, oversampled                      */
  uint16_t mv;       /* 0..ADC_VREF_MV                               */
  uint8_t  percent;  /* 0..100 over that input's own [min, max] span */
} analog_reading_t;

/* Ambient light sensor, ALS_OUT / GP26. Drives the backlight. */
extern analog_reading_t als;

/* The variant's sensor, SENSOR1_FILTERED / GP28. Its span comes from
   gauge_variant, so the same code serves every variant. */
extern analog_reading_t sensor1;

/* Sets up the ADC block and the three analog pads. SENSOR2 is not used by any
   variant; its pad is configured only so it stays high-impedance. */
void sensors_init(void);

/* Refreshes als and sensor1. */
void sensors_update(void);

/* sensor1 in engineering units, clamped to the gauge face. */
float sensors_value(void);

#endif /* SENSORS_H */
