#include "demo.h"

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "config.h"
#include "variant.h"

/* One full cycle is: sweep up, hold, sweep down, hold. */
#define DEMO_HALF_MS  (DEMO_SWEEP_MS + DEMO_HOLD_MS)
#define DEMO_CYCLE_MS (2u * DEMO_HALF_MS)

float demo_value(void) {
  uint32_t t = (uint32_t)(to_ms_since_boot(get_absolute_time()) % DEMO_CYCLE_MS);

  const bool going_up = (t < DEMO_HALF_MS);
  if (!going_up) { t -= DEMO_HALF_MS; }

  /* 0 at the low end of the face, 1 at the high end. */
  float f = (t >= DEMO_SWEEP_MS) ? 1.f                                /* holding */
                                 : (float)t / (float)DEMO_SWEEP_MS;   /* sweeping */
  if (!going_up) { f = 1.f - f; }

  return gauge_variant.value_min
       + (gauge_variant.value_max - gauge_variant.value_min) * f;
}
