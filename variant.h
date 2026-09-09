#ifndef VARIANT_H
#define VARIANT_H

#include <stdint.h>

/* ---------- build variant ----------
   Supplied by CMake (-DCURRENT_MODE=MODE_xxx), one target per variant. The
   value below is only a default for a hand-made compilation.

   Values start at 1 on purpose: an unknown or stale mode name expands to 0
   in #if, so it fails the check below instead of silently matching a mode. */

#define MODE_OIL_P     1
#define MODE_OIL_T     2
#define MODE_GEARBOX_T 3

#ifndef CURRENT_MODE
#define CURRENT_MODE MODE_OIL_T
#endif

#if (CURRENT_MODE != MODE_OIL_P) && (CURRENT_MODE != MODE_OIL_T) && (CURRENT_MODE != MODE_GEARBOX_T)
#error "CURRENT_MODE doit valoir MODE_OIL_P, MODE_OIL_T ou MODE_GEARBOX_T"
#endif

/* What separates one gauge from another: the span the sensor delivers, the
   scale the needle sweeps, and the face drawn behind it. The sensor is always
   wired to SENSOR1 (GP28), in every variant.

   Adding a gauge means adding a MODE_xxx above, one instance in variant.c and
   one universal_gauges_variant() call in CMakeLists.txt. Nothing else in the
   firmware tests CURRENT_MODE. */
typedef struct {
  const char*    name;           /* variant name, shown in the serial log   */
  uint16_t       sensor_mv_min;  /* sensor output at value_min, in mV       */
  uint16_t       sensor_mv_max;  /* sensor output at value_max, in mV       */
  float          value_min;      /* left end of the gauge face              */
  float          value_max;      /* right end of the gauge face             */
  float          value_step;     /* reading granularity, 0 = continuous     */
  const uint8_t* face_day;       /* background image, day theme             */
  const uint8_t* face_night;     /* background image, night theme           */
} gauge_variant_t;

/* The one variant this firmware was built for. */
extern const gauge_variant_t gauge_variant;

#endif /* VARIANT_H */
