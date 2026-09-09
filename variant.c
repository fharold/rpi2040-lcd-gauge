#include "variant.h"
#include "config.h"

/* Only the two images this variant needs are included, so only they are
   compiled into the binary. Each face is LCD_SZ bytes (115200), which is why
   they are picked here rather than all six being carried around. */

#if CURRENT_MODE == MODE_OIL_P

#include "img/bg_gauge_oil_p.h"
#include "img/bg_gauge_oil_p_dark.h"

const gauge_variant_t gauge_variant = {
  .name          = "OIL_P",
  .sensor_mv_min = PRESSURE_SENSOR_MV_MIN,
  .sensor_mv_max = PRESSURE_SENSOR_MV_MAX,
  .value_min     = MIN_PRESS,
  .value_max     = MAX_PRESS,
  .value_step    = PRESS_STEP,
  .face_day      = bg_gauge_oil_p,
  .face_night    = bg_gauge_oil_p_dark,
};

#elif CURRENT_MODE == MODE_OIL_T

#include "img/bg_gauge_oil_t.h"
#include "img/bg_gauge_oil_t_dark.h"

const gauge_variant_t gauge_variant = {
  .name          = "OIL_T",
  .sensor_mv_min = TEMP_SENSOR_MV_MIN,
  .sensor_mv_max = TEMP_SENSOR_MV_MAX,
  .value_min     = MIN_TEMP,
  .value_max     = MAX_TEMP,
  .value_step    = TEMP_STEP,
  .face_day      = bg_gauge_oil_t,
  .face_night    = bg_gauge_oil_t_dark,
};

#else /* MODE_GEARBOX_T: same probe and same scale as OIL_T, other face */

#include "img/bg_gearbox_temp.h"
#include "img/bg_gearbox_temp_dark.h"

const gauge_variant_t gauge_variant = {
  .name          = "GEARBOX_T",
  .sensor_mv_min = TEMP_SENSOR_MV_MIN,
  .sensor_mv_max = TEMP_SENSOR_MV_MAX,
  .value_min     = MIN_TEMP,
  .value_max     = MAX_TEMP,
  .value_step    = TEMP_STEP,
  .face_day      = bg_gearbox_temp,
  .face_night    = bg_gearbox_temp_dark,
};

#endif
