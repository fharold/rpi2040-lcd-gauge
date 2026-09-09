#include "sensors.h"

#include "hardware/adc.h"

#include "board.h"
#include "config.h"
#include "variant.h"

analog_reading_t als     = {0};
analog_reading_t sensor1 = {0};

/* Oversampled raw reading. The channel is selected on every call because
   lcd_module_init() selects BAR_CHANNEL (3) for the battery measurement. */
static uint16_t read_raw(uint8_t channel) {
  adc_select_input(channel);
  uint32_t sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += adc_read();
  }
  return (uint16_t)(sum / ADC_SAMPLES);
}

/* raw count -> mV (max 4095 * 3300 = 13.5e6, fits in uint32_t). */
static uint16_t raw_to_mv(uint16_t raw) {
  return (uint16_t)(((uint32_t)raw * ADC_VREF_MV) / ADC_MAX);
}

/* mV -> 0..100 % over [mv_min, mv_max], clamped. */
static uint8_t mv_to_percent(uint16_t mv, uint16_t mv_min, uint16_t mv_max) {
  if (mv_max <= mv_min) { return 0; }
  if (mv <= mv_min)     { return 0; }
  if (mv >= mv_max)     { return 100; }
  return (uint8_t)(((uint32_t)(mv - mv_min) * 100u) / (uint32_t)(mv_max - mv_min));
}

/* mV -> engineering units, linear over [mv_min, mv_max] -> [out_min, out_max].
   A reading outside the span is clamped, not extrapolated. */
static float mv_to_range(uint16_t mv, uint16_t mv_min, uint16_t mv_max,
                         float out_min, float out_max) {
  if (mv_max <= mv_min) { return out_min; }
  if (mv <= mv_min)     { return out_min; }
  if (mv >= mv_max)     { return out_max; }
  float factor = (float)(mv - mv_min) / (float)(mv_max - mv_min);
  return out_min + (out_max - out_min) * factor;
}

/* Reads one channel and fills the three fields in one go. */
static void read_input(analog_reading_t* r, uint8_t channel,
                       uint16_t mv_min, uint16_t mv_max) {
  r->raw     = read_raw(channel);
  r->mv      = raw_to_mv(r->raw);
  r->percent = mv_to_percent(r->mv, mv_min, mv_max);
}

void sensors_init(void) {
  adc_init();
  adc_gpio_init(ALS_OUT);
  adc_gpio_init(SENSOR1_FILTERED);
  adc_gpio_init(SENSOR2_FILTERED);
}

void sensors_update(void) {
  read_input(&als, ALS_ADC_CHANNEL, ALS_MV_MIN, ALS_MV_MAX);
  read_input(&sensor1, SENSOR1_ADC_CHANNEL,
             gauge_variant.sensor_mv_min, gauge_variant.sensor_mv_max);
}

float sensors_value(void) {
  return mv_to_range(sensor1.mv,
                     gauge_variant.sensor_mv_min, gauge_variant.sensor_mv_max,
                     gauge_variant.value_min, gauge_variant.value_max);
}
