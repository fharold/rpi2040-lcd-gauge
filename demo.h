#ifndef DEMO_H
#define DEMO_H

/* Demo sweep. Compiled in only when the firmware is configured with
   -DUSE_DEMO=ON, see CMakeLists.txt; a normal build does not contain it.

   The needle walks the face from one end to the other and back, ignoring the
   sensor, which is how you check a gauge face, the needle geometry and the
   end stops with nothing wired to SENSOR1. */

/* Where the needle should sit right now, in engineering units. Derived from
   the time since boot, so it keeps no state of its own. */
float demo_value(void);

#endif /* DEMO_H */
