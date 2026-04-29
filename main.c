static __attribute__((section (".noinit")))char losabuf[4096];

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
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/interp.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include <float.h>
#include "pico/types.h"
#include "pico/bootrom/sf_table.h"
#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "lcd.h"
#include "w.h"
#include "lib/draw.h"
#include "QMI8658.h"
#include "CST816S.h"
#include "img/bg_trans_temp.h"
#include "img/bg_gauge_oil_t.h"
#include "img/bg_gauge_oil_p.h"
#include "img/font34.h"//touche pas à ça petit con
#include "img/font40.h"//touche pas à ça petit con
#include "lib/draw.h"
//#include "img/font48.h"

// Tested with the parts that have the height of 240 and 320
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SERIAL_CLK_DIV 1.f
#define UNIT_LSB 16

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((float) deg * (float) M_PI / 180.f)
#define THETA_MAX (2.f * (float) M_PI)

#define NEEDLE_MAX_ANGLE 135.f
#define NEEDLE_MIN_ANGLE 45.f
#define NEEDLE_UPPER_LENGTH 135.f
#define NEEDLE_LOWER_LENGTH 32.f

#define MIN_TEMP 50.f
#define MAX_TEMP 130.f

#define MIN_PRESS -0.5f
#define MAX_PRESS 6.5f

W* wn_background = NULL;
W* wn_content = NULL;
W* wn_draw_needle_temp = NULL;
W* wn_draw_needle_press = NULL;
W* wl[1] = {NULL};

#define MODE_OIL_P 0
#define MODE_OIL_T 1
#define MODE_TRANS_T 2
#define CURRENT_MODE MODE_OIL_P

typedef struct {
  Vec2 start;
  Vec2 end;
} NeedlePos;

//adc_read()

#define mcpy(d,s,sz) for(int i=0;i<sz;i++){d[i]=s[i];}

#define TFOWI 26
#define TFOSWI 14

float theta = 0.0f;
float theta1 = 0.0f;
float theta2 = 0.0f;
float theta3 = 0.0f;
float theta_d = 1.2f;
float current_pressure = 7.0f;
int16_t current_temperature = 0;

extern Vec2 vO;

extern uint8_t LCD_RST_PIN;
extern W wroot;

uint8_t* b0=NULL;
uint32_t* b1=NULL;

//ky-040
#define CCLK 16
#define CDT 17
#define CSW 19

//one button /
#define QMIINT1 23
#define CBUT_TOUCH 16

Vec2 center = {120, 195};
uint8_t CBUT0 = 22;
bool rp2040_touch = false;
bool clk,dt,sw,oclk,odt,osw;

void draw_pointer(Vec2 vs, Vec2 vts, int16_t tu, uint16_t color, const uint8_t* sr, uint16_t alpha){
  draw_pointer_mode(vs,tu,YELLOW);
}

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
  
  draw_line(center, upper_end, RED, 6);
  draw_line(center, lower_end, RED, 6);
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
  
  draw_line(center, upper_end, RED, 6);
  draw_line(center, lower_end, RED, 6);
}

void draw_background()
{
  switch (CURRENT_MODE) {
    case MODE_OIL_P:
    mcpy(b0,bg_gauge_oil_p,LCD_SZ);
    break;

    case MODE_OIL_T:
    mcpy(b0,bg_gauge_oil_t,LCD_SZ);
    break;

    case MODE_TRANS_T:
    mcpy(b0,bg_trans_temp,LCD_SZ);
    break;
  }
}

int main(void)
{
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

  i2c_scan();
  lcd_init();
  lcd_set_brightness(255);
  b0 = malloc(LCD_SZ);
  b1 = (uint32_t*)b0;
  if(b0==0){printf("b0==0!\n");}
  uint32_t o = 0;
  lcd_setimg((uint16_t*)b0);

  CST816S_init(CST816S_Gesture_Mode);

  gpio_init(QMIINT1);
  gpio_set_dir(QMIINT1,GPIO_IN);
  gpio_set_irq_enabled_with_callback(QMIINT1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

  if(rp2040_touch) {
    gpio_init(Touch_INT_PIN);
    gpio_pull_up(Touch_INT_PIN);
    gpio_set_dir(Touch_INT_PIN,GPIO_IN);
    gpio_set_irq_enabled(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    CST816S_init(CST816S_Point_Mode);
  } else {
    gpio_init(CBUT0);
    gpio_set_dir(CBUT0,GPIO_IN);
    gpio_pull_up(CBUT0);
    gpio_set_irq_enabled(CBUT0, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  }
  QMI8658_init();

  init_root();
  wn_background = wadd_none(&wroot,draw_background);
  if (CURRENT_MODE == MODE_OIL_P) {
    wn_draw_needle_press = wadd_none(&wroot,draw_needle_press);
  } else {
    wn_draw_needle_temp = wadd_none(&wroot,draw_needle_temp);
  }
  
  while(true){
    for(int i=0;i<LCD_SZ/4;i++){b1[i]=0x00;}  //clear buffer faster
    wdraw(&wroot);
    lcd_display(b0);
    current_temperature++;

    if (current_temperature > 150) {
      current_temperature = -10;
    }

    // current_pressure = current_pressure + 0.1f;

    if (current_pressure > 7.f) {
      current_pressure = 0.f;
    }

    sleep_ms(10);
  }
  return 0;
}