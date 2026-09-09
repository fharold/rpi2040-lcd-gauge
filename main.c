#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "string.h"
#include "pico/time.h"
#include <math.h>
#include "pico/util/datetime.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "hardware/gpio.h"
#include <hardware/flash.h>
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/interp.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include <float.h>
#include "pico/types.h"
#include "pico/bootrom/sf_table.h"
#include "hardware/i2c.h"
#include "lcd.h"
#include "w.h"
#include "lib/draw.h"
#include "QMI8658.h"
#include "CST816S.h"
#include "img/bg_gearbox_temp.h"
#include "img/bg_gauge_oil_t.h"
#include "img/bg_gauge_oil_p.h"
#include "img/bg_gearbox_temp_dark.h"
#include "img/bg_gauge_oil_t_dark.h"
#include "img/bg_gauge_oil_p_dark.h"
/* Generated font data. These headers carry DEFINITIONS, not declarations:
   font34.h defines font_t Font12, Font16, Font20, Font24 and Font34, and
   font40.h defines Font40. w.c uses Font16, so dropping these includes
   breaks the link. Do not remove. */
#include "img/font34.h"
#include "img/font40.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((float) deg * (float) M_PI / 180.f)

#define NEEDLE_MAX_ANGLE 135.f
#define NEEDLE_MIN_ANGLE 45.f
#define NEEDLE_UPPER_LENGTH 128.f
#define NEEDLE_LOWER_LENGTH 32.f

#define MIN_TEMP 50.f
#define MAX_TEMP 140.f

#define MIN_PRESS -0.5f
#define MAX_PRESS 6.5f

#define RELAY2_COMMAND_PIN (20) //output
#define RELAY1_COMMAND_PIN (19) //output
#define BUTTON_INPUT (18) //input, pull-up
#define SENSOR1_FILTERED (28) //analog input
#define SENSOR2_FILTERED (27) //analog input
#define SHIFTED_DIGITAL_IN1 (16) //input 
#define SHIFTED_DIGITAL_IN2 (17) //input
#define ALS_OUT (26) //analog input

/* ---------- ADC inputs ----------
   RP2040 mapping : GP26=ADC0, GP27=ADC1, GP28=ADC2, GP29=ADC3 (VSYS/batt) */
#define ALS_ADC_CHANNEL      (0)       /* ALS_OUT           -> GP26 */
#define SENSOR2_ADC_CHANNEL  (1)       /* SENSOR2_FILTERED  -> GP27 */
#define SENSOR1_ADC_CHANNEL  (2)       /* SENSOR1_FILTERED  -> GP28 */

#define ADC_SAMPLES       (8)          /* oversampling to smooth out noise */
#define ADC_PERIOD_MS     (100)        /* read period in the main loop */
#define ADC_VREF_MV       (3300u)      /* ADC full scale, in mV */
#define ADC_RANGE         (1 << 12)    /* 12-bit ADC -> 0..4095 */
#define ADC_MAX           (ADC_RANGE - 1)

W* wn_background = NULL;
W* wn_draw_needle_temp = NULL;
W* wn_draw_needle_press = NULL;

/* ---------- build variant ----------
   Fourni par CMake (-DCURRENT_MODE=MODE_xxx), une cible par variante.
   La valeur ci-dessous n'est qu'un defaut pour une compilation manuelle. */
/* Values start at 1 on purpose: an unknown or stale mode name expands to 0
   in #if, so it fails the check below instead of silently matching a mode. */
#define MODE_OIL_P 1
#define MODE_OIL_T 2
#define MODE_GEARBOX_T 3

#ifndef CURRENT_MODE
#define CURRENT_MODE MODE_OIL_T
#endif

#if (CURRENT_MODE != MODE_OIL_P) && (CURRENT_MODE != MODE_OIL_T) && (CURRENT_MODE != MODE_GEARBOX_T)
#error "CURRENT_MODE doit valoir MODE_OIL_P, MODE_OIL_T ou MODE_GEARBOX_T"
#endif

#define NEEDLE_ORANGE 0xF840

#define mcpy(d,s,sz) for(int i=0;i<sz;i++){d[i]=s[i];}

#define DAY_THEME (uint8_t)0
#define NIGHT_THEME (uint8_t)1

#define MIN_BRIGHTNESS (uint8_t)1
#define MAX_BRIGHTNESS (uint8_t)100

#define ALS_MV_MAX 2700
#define ALS_MV_MIN 0

#define TEMP_SENSOR_MV_MAX 2900
#define TEMP_SENSOR_MV_MIN 640

#define PRESSURE_SENSOR_MV_MAX 2250
#define PRESSURE_SENSOR_MV_MIN 250

/* Le capteur de la variante est toujours cable sur SENSOR1 : seules ses
   bornes d'entree changent d'une variante a l'autre. */
#if CURRENT_MODE == MODE_OIL_P
  #define SENSOR_MV_MIN   PRESSURE_SENSOR_MV_MIN
  #define SENSOR_MV_MAX   PRESSURE_SENSOR_MV_MAX
  #define MODE_NAME       "OIL_P"
#elif CURRENT_MODE == MODE_OIL_T
  #define SENSOR_MV_MIN   TEMP_SENSOR_MV_MIN
  #define SENSOR_MV_MAX   TEMP_SENSOR_MV_MAX
  #define MODE_NAME       "OIL_T"
#else /* MODE_GEARBOX_T : meme sonde, meme echelle que OIL_T */
  #define SENSOR_MV_MIN   TEMP_SENSOR_MV_MIN
  #define SENSOR_MV_MAX   TEMP_SENSOR_MV_MAX
  #define MODE_NAME       "GEARBOX_T"
#endif

float current_pressure = 7.0f;
int16_t current_temperature = 130;
uint8_t current_brightness = MIN_BRIGHTNESS;
uint8_t current_theme = DAY_THEME;

/* last ALS measurement (ALS_OUT / GP26) */
uint16_t als_raw = 0;          /* 0..4095                        */
uint16_t als_mv = 0;           /* 0..3300 mV                     */
uint8_t  als_percent = 0;      /* 0..100 %, ALS_MV_MIN..ALS_MV_MAX */

/* last measurement of the variant's sensor (SENSOR1_FILTERED / GP28) */
uint16_t sensor1_raw = 0;      /* 0..4095                              */
uint16_t sensor1_mv = 0;       /* 0..3300 mV                           */
uint8_t  sensor1_percent = 0;  /* 0..100 %, SENSOR_MV_MIN..SENSOR_MV_MAX */

extern uint8_t LCD_RST_PIN;
extern W wroot;

uint8_t* b0=NULL;
uint32_t* b1=NULL;

//one button /
#define QMIINT1 23
#define CBUT_TOUCH 16

Vec2 center = {120, 195};
uint8_t CBUT0 = 22;

bool rp2040_touch = false;

bool reserved_addr(uint8_t addr) {
  return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan(){
  printf("\nI2C Bus Scan \n");
	printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	for (int addr = 0; addr < (1 << 7); ++addr) {
			if (addr % 16 == 0) {					printf("%02x ", addr);			}
			int ret;
			uint8_t rxdata;
			if (reserved_addr(addr))
					ret = PICO_ERROR_GENERIC;
			else
					ret = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);

      if(ret >= 0 && addr == CST816_ADDR){
        rp2040_touch = true;
        CBUT0 = CBUT_TOUCH;
        LCD_RST_PIN = 13;
      }
			printf(ret < 0 ? "." : "@");
			printf(addr % 16 == 15 ? "\n" : "  ");
	}
}

void gpio_callback(uint gpio, uint32_t events) {
}

void draw_needle_temp(){
  float angle_amplitude = NEEDLE_MAX_ANGLE - NEEDLE_MIN_ANGLE;
  float temperature_amplitude = MAX_TEMP - MIN_TEMP;
  float temp_difference = MAX(0.f, MIN(MAX(current_temperature, 0.f), MAX_TEMP) - MIN_TEMP);
  float factor = temp_difference / temperature_amplitude;
  float angle = NEEDLE_MIN_ANGLE + (angle_amplitude * factor);
  float angle_deg = PI - DEG_TO_RAD(angle);
  Vec2 upper_end;
  Vec2 lower_end;

  upper_end.x = center.x + (int)(NEEDLE_UPPER_LENGTH * cosf(angle_deg));
  upper_end.y = center.y - (int)(NEEDLE_UPPER_LENGTH * sinf(angle_deg));

  lower_end.x = center.x + (int)(NEEDLE_LOWER_LENGTH * cosf(angle_deg + PI));
  lower_end.y = center.y - (int)(NEEDLE_LOWER_LENGTH * sinf(angle_deg + PI));
  
  draw_line(center, upper_end, ((current_theme == DAY_THEME) ? RED : NEEDLE_ORANGE), 6);
  draw_line(center, lower_end, ((current_theme == DAY_THEME) ? RED : NEEDLE_ORANGE), 6);
}

void draw_needle_press(){
  float angle_amplitude = NEEDLE_MAX_ANGLE - NEEDLE_MIN_ANGLE;
  float pressure_amplitude = MAX_PRESS - MIN_PRESS;
  float pressure_difference = MAX(0.f, MIN(MAX(current_pressure, 0.f), MAX_PRESS) - MIN_PRESS);
  float factor = pressure_difference / pressure_amplitude;
  float angle = NEEDLE_MIN_ANGLE + (angle_amplitude * factor);
  float angle_deg = PI - DEG_TO_RAD(angle);
  Vec2 upper_end;
  Vec2 lower_end;

  upper_end.x = center.x + (int)(NEEDLE_UPPER_LENGTH * cosf(angle_deg));
  upper_end.y = center.y - (int)(NEEDLE_UPPER_LENGTH * sinf(angle_deg));

  lower_end.x = center.x + (int)(NEEDLE_LOWER_LENGTH * cosf(angle_deg + PI));
  lower_end.y = center.y - (int)(NEEDLE_LOWER_LENGTH * sinf(angle_deg + PI));
  
  draw_line(center, upper_end, ((current_theme == DAY_THEME) ? RED : NEEDLE_ORANGE), 6);
  draw_line(center, lower_end, ((current_theme == DAY_THEME) ? RED : NEEDLE_ORANGE), 6);
}

void draw_background()
{
  switch (CURRENT_MODE) {
    case MODE_OIL_P:
    mcpy(b0, ((current_theme == DAY_THEME) ? bg_gauge_oil_p : bg_gauge_oil_p_dark), LCD_SZ);
    break;

    case MODE_OIL_T:
    mcpy(b0, ((current_theme == DAY_THEME) ? bg_gauge_oil_t : bg_gauge_oil_t_dark), LCD_SZ);
    break;

    case MODE_GEARBOX_T:
    mcpy(b0, ((current_theme == DAY_THEME) ? bg_gearbox_temp : bg_gearbox_temp_dark), LCD_SZ);
    break;
  }
}

/* ---------- ADC helpers ---------- */

/* Configure the analog pins (disables digital I/O + pulls on the pads). */
void pc_adc_init(void) {
  adc_init();
  adc_gpio_init(ALS_OUT);
  adc_gpio_init(SENSOR1_FILTERED);
  adc_gpio_init(SENSOR2_FILTERED);
}

/* Oversampled raw reading. The channel is selected on every call because
   lcd_module_init() selects BAR_CHANNEL (3) for the battery measurement. */
uint16_t pc_adc_read_raw(uint8_t channel) {
  adc_select_input(channel);
  uint32_t sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += adc_read();
  }
  return (uint16_t)(sum / ADC_SAMPLES);
}

/* raw count -> mV (max 4095 * 3300 = 13.5e6, fits in uint32_t). */
uint16_t pc_adc_raw_to_mv(uint16_t raw) {
  return (uint16_t)(((uint32_t)raw * ADC_VREF_MV) / ADC_MAX);
}

/* mV -> 0..100 % over the sensor's own [mv_min, mv_max] span, clamped. */
uint8_t pc_adc_mv_to_percent(uint16_t mv, uint16_t mv_min, uint16_t mv_max) {
  if (mv_max <= mv_min) { return 0; }
  if (mv <= mv_min)     { return 0; }
  if (mv >= mv_max)     { return 100; }
  return (uint8_t)(((uint32_t)(mv - mv_min) * 100u) / (uint32_t)(mv_max - mv_min));
}

/* mV -> engineering units, linear over [mv_min, mv_max] -> [out_min, out_max]. */
float pc_adc_mv_to_range(uint16_t mv, uint16_t mv_min, uint16_t mv_max,
                         float out_min, float out_max) {
  if (mv_max <= mv_min) { return out_min; }
  if (mv <= mv_min)     { return out_min; }
  if (mv >= mv_max)     { return out_max; }
  float factor = (float)(mv - mv_min) / (float)(mv_max - mv_min);
  return out_min + (out_max - out_min) * factor;
}

/* Refresh the als_* globals. */
void als_update(void) {
  als_raw     = pc_adc_read_raw(ALS_ADC_CHANNEL);
  als_mv      = pc_adc_raw_to_mv(als_raw);
  als_percent = pc_adc_mv_to_percent(als_mv, ALS_MV_MIN, ALS_MV_MAX);
}

/* Refresh the sensor1_* globals, with the current variant's input span. */
void sensor1_update(void) {
  sensor1_raw     = pc_adc_read_raw(SENSOR1_ADC_CHANNEL);
  sensor1_mv      = pc_adc_raw_to_mv(sensor1_raw);
  sensor1_percent = pc_adc_mv_to_percent(sensor1_mv, SENSOR_MV_MIN, SENSOR_MV_MAX);
}

void init() {
  sleep_ms(100);  // "Rain-wait" wait 100ms after booting (for other chips to initialize)
  rtc_init();
	stdio_init_all();

	//stdio_usb_init();
	int i = 0;
	while (i++ < 10) {
		if (stdio_usb_connected())
			break;
		sleep_ms(250);
	}

  sleep_ms(300);

  // I2C Config
  i2c_init(I2C_PORT, 100 * 1000);

  gpio_set_function(DEV_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(DEV_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(DEV_SDA_PIN);
  gpio_pull_up(DEV_SCL_PIN);

  gpio_init(RELAY1_COMMAND_PIN);
  gpio_set_dir(RELAY1_COMMAND_PIN, GPIO_OUT);

  gpio_init(RELAY2_COMMAND_PIN);
  gpio_set_dir(RELAY2_COMMAND_PIN, GPIO_OUT);
  
  gpio_init(BUTTON_INPUT);
  gpio_set_dir(BUTTON_INPUT, GPIO_IN);
  gpio_pull_up(BUTTON_INPUT);

  gpio_init(SHIFTED_DIGITAL_IN1);
  gpio_set_dir(SHIFTED_DIGITAL_IN1, GPIO_IN);

  gpio_init(SHIFTED_DIGITAL_IN2);
  gpio_set_dir(SHIFTED_DIGITAL_IN2, GPIO_IN);

  /* ALS_OUT (GP26), SENSOR1_FILTERED (GP28), SENSOR2_FILTERED (GP27)
     are analog inputs -> ADC, not digital GPIOs. SENSOR2 is not wired in
     any variant, it is only configured so the pad stays high-impedance. */
  pc_adc_init();
  
  i2c_scan();
  lcd_init();
  b0 = malloc(LCD_SZ);
  b1 = (uint32_t*)b0; 
  if(b0==0){printf("b0==0!\n");}
  lcd_setimg((uint16_t*)b0);

  CST816S_init(CST816S_Gesture_Mode);

  gpio_init(QMIINT1);
  gpio_set_dir(QMIINT1,GPIO_IN);
  gpio_set_irq_enabled_with_callback(QMIINT1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

  gpio_init(CBUT0);
  gpio_set_dir(CBUT0,GPIO_IN);
  gpio_pull_up(CBUT0);
  gpio_set_irq_enabled(CBUT0, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

  QMI8658_init();

  init_root();

  wn_background = wadd_none(&wroot,draw_background);
#if CURRENT_MODE == MODE_OIL_P
  wn_draw_needle_press = wadd_none(&wroot,draw_needle_press);
#else
  wn_draw_needle_temp = wadd_none(&wroot,draw_needle_temp);
#endif
}

int main(void)
{
  init();

  absolute_time_t adc_next = get_absolute_time();

  while(true){
    /* ---- read the analog inputs ---- */
    if (absolute_time_diff_us(get_absolute_time(), adc_next) <= 0) {
      adc_next = delayed_by_ms(get_absolute_time(), ADC_PERIOD_MS);

      als_update();      /* ALS_OUT          -> ADC0 / GP26 */
      sensor1_update();  /* SENSOR1_FILTERED -> ADC2 / GP28 */

      printf("[" MODE_NAME "] ALS raw=%u %umV %u%%   S1 raw=%u %umV %u%%\n",
             als_raw, als_mv, als_percent,
             sensor1_raw, sensor1_mv, sensor1_percent);

      current_brightness = MIN_BRIGHTNESS + (uint8_t)(((uint32_t)als_percent * (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) / 100u);

#if CURRENT_MODE == MODE_OIL_P
      current_pressure = pc_adc_mv_to_range(sensor1_mv,
                             SENSOR_MV_MIN, SENSOR_MV_MAX,
                             MIN_PRESS, MAX_PRESS);
#else
      current_temperature = (int16_t)pc_adc_mv_to_range(sensor1_mv,
                                SENSOR_MV_MIN, SENSOR_MV_MAX,
                                MIN_TEMP, MAX_TEMP);
#endif
    }

    for(int i=0;i<LCD_SZ/4;i++){b1[i]=0x00;}  //clear buffer faster
    lcd_set_brightness(current_brightness);
    wdraw(&wroot);
    lcd_display(b0);
    sleep_ms(10);
  }
  return 0;
}