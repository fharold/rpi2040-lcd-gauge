#include "gauge.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "lcd.h"
#include "w.h"
#include "lib/draw.h"

#include "config.h"
#include "variant.h"

/* Generated font data. These headers carry DEFINITIONS, not declarations:
   font34.h defines font_t Font12, Font16, Font20, Font24 and Font34, and
   font40.h defines Font40. w.c uses Font16, so dropping these includes
   breaks the link. They belong in exactly one translation unit. Do not
   remove. */
#include "img/font34.h"
#include "img/font40.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Parentheses matter: the argument is an expression at the call site. */
#define DEG_TO_RAD(deg) ((float)(deg) * (float)M_PI / 180.f)

/* Needle geometry, in pixels and degrees on the 240x240 face. These match the
   graduations drawn on the background images. */
#define NEEDLE_MIN_ANGLE    45.f
#define NEEDLE_MAX_ANGLE    135.f
#define NEEDLE_UPPER_LENGTH 128.f
#define NEEDLE_LOWER_LENGTH 32.f
#define NEEDLE_THICKNESS    6
#define NEEDLE_ORANGE       0xF840

static const Vec2 needle_pivot = {120, 195};

uint8_t gauge_theme = DAY_THEME;

/* Frame buffer. b1 is the same memory seen as words, to clear it four bytes
   at a time. */
static uint8_t*  b0 = NULL;
static uint32_t* b1 = NULL;

static float   gauge_value        = 0.f;
static uint8_t current_brightness = MIN_BRIGHTNESS;

/* When the light reading first started asking for the other face. */
static uint32_t theme_pending_since = 0;

static void draw_background(void) {
  memcpy(b0,
         (gauge_theme == DAY_THEME) ? gauge_variant.face_day
                                    : gauge_variant.face_night,
         LCD_SZ);
}

static void draw_needle(void) {
  const float value_span = gauge_variant.value_max - gauge_variant.value_min;
  const float angle_span = NEEDLE_MAX_ANGLE - NEEDLE_MIN_ANGLE;

  /* The reading is clamped to the face before it is turned into an angle, so
     an out-of-range sensor parks the needle on a graduation instead of
     sweeping past it. Clamping is to the two ends of the face, which is what
     lets a scale that starts below zero use its lower part. */
  const float clamped = MIN(MAX(gauge_value, gauge_variant.value_min),
                            gauge_variant.value_max);
  const float amount  = clamped - gauge_variant.value_min;
  const float factor  = (value_span > 0.f) ? (amount / value_span) : 0.f;
  const float degrees = NEEDLE_MIN_ANGLE + angle_span * factor;
  const float angle   = PI - DEG_TO_RAD(degrees);

  Vec2 upper_end;
  Vec2 lower_end;

  upper_end.x = needle_pivot.x + (int)(NEEDLE_UPPER_LENGTH * cosf(angle));
  upper_end.y = needle_pivot.y - (int)(NEEDLE_UPPER_LENGTH * sinf(angle));

  lower_end.x = needle_pivot.x + (int)(NEEDLE_LOWER_LENGTH * cosf(angle + PI));
  lower_end.y = needle_pivot.y - (int)(NEEDLE_LOWER_LENGTH * sinf(angle + PI));

  const uint16_t colour = (gauge_theme == DAY_THEME) ? RED : NEEDLE_ORANGE;

  draw_line(needle_pivot, upper_end, colour, NEEDLE_THICKNESS);
  draw_line(needle_pivot, lower_end, colour, NEEDLE_THICKNESS);
}

void gauge_init_display(void) {
  b0 = malloc(LCD_SZ);
  b1 = (uint32_t*)b0;
  if (b0 == NULL) { printf("b0==0!\n"); }
  lcd_setimg((uint16_t*)b0);
}

void gauge_init_widgets(void) {
  init_root();
  wadd_none(&wroot, draw_background);
  wadd_none(&wroot, draw_needle);
}

void gauge_set_value(float value) {
  const float step = gauge_variant.value_step;
  gauge_value = (step > 0.f) ? truncf(value / step) * step : value;
}

/* Which face a reading calls for. Between the two thresholds it asks for
   whatever is already on screen, which is what makes the switch hysteretic. */
static uint8_t theme_wanted(uint8_t als_percent, uint8_t current) {
  if (als_percent < THEME_NIGHT_BELOW) { return NIGHT_THEME; }
  if (als_percent > THEME_DAY_ABOVE)   { return DAY_THEME; }
  return current;
}

/* Swaps the face once the reading has disagreed with it for long enough. */
static void theme_update(uint8_t als_percent) {
  const uint8_t  wanted = theme_wanted(als_percent, gauge_theme);
  const uint32_t now    = to_ms_since_boot(get_absolute_time());

  if (wanted == gauge_theme) {
    theme_pending_since = now;   /* the reading agrees, restart the wait */
    return;
  }
  if (now - theme_pending_since >= THEME_HOLD_MS) {
    gauge_theme         = wanted;
    theme_pending_since = now;
  }
}

void gauge_set_light(uint8_t als_percent) {
  current_brightness = MIN_BRIGHTNESS
    + (uint8_t)(((uint32_t)als_percent * (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) / 100u);

  theme_update(als_percent);
}

void gauge_draw(void) {
  for (int i = 0; i < LCD_SZ / 4; i++) { b1[i] = 0x00; }  /* clear, 4 bytes at a time */
  lcd_set_brightness(current_brightness);
  wdraw(&wroot);
  lcd_display(b0);
}
