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
#include "img/bega.h"
#include "img/font34.h"//touche pas à ça petit con
#include "img/font40.h"//touche pas à ça petit con
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

#define THETA_MAX (2.f * (float) M_PI)

W* wn_background = NULL;
W* wn_content = NULL;
W* wn_drawclockhands = NULL;
W* wl[1] = {NULL};

//adc_read()

#define mcpy(d,s,sz) for(int i=0;i<sz;i++){d[i]=s[i];}

#define TFOWI 26
#define TFOSWI 14

float theta = 0.0f;
float theta1 = 0.0f;
float theta2 = 0.0f;
float theta3 = 0.0f;
float theta_d = 1.2f;

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

void draw_clock_hands(){
  uint8_t x1,y1,xt,yt;
  int xi,yi;
  Vec2 dp0 = {102,10};
  dp0 = vset(65,6);
  draw_pointer_mode(dp0,90,RED);
}

void draw_background()
{
  mcpy(b0,bega,LCD_SZ);
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
  lcd_set_brightness(10);
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
  wn_drawclockhands = wadd_none(&wroot,draw_clock_hands);

  while(true){
    for(int i=0;i<LCD_SZ/4;i++){b1[i]=0x00;}  //clear buffer faster
    wdraw(&wroot);
    lcd_display(b0);
  }
  return 0;
}