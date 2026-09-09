#ifndef GAUGE_H
#define GAUGE_H

#include <stdint.h>

#define DAY_THEME   (uint8_t)0
#define NIGHT_THEME (uint8_t)1

/* Which set of faces and needle colours is drawn. Follows the ambient light,
   see gauge_set_light(). */
extern uint8_t gauge_theme;

/* Allocates the frame buffer and hands it to the display. Call after
   lcd_init(). */
void gauge_init_display(void);

/* Puts the background and the needle on the widget tree. */
void gauge_init_widgets(void);

/* What the needle points at, in engineering units. Quantised to the variant's
   value_step. */
void gauge_set_value(float value);

/* The ambient light reading, in percent. Sets the backlight level, and
   swaps between the day and the night face at the thresholds in config.h. */
void gauge_set_light(uint8_t als_percent);

/* Clears the frame buffer, draws the widget tree, pushes it to the display. */
void gauge_draw(void);

#endif /* GAUGE_H */
