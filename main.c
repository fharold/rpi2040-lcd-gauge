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

typedef struct {
  uint8_t BRIGHTNESS;
  uint8_t scandir;
} LOSA_t;

static LOSA_t* plosa=(LOSA_t*)losabuf;

char* fp_cn = NULL;

int16_t flags_deg=0;
volatile uint8_t flag = 0;

bool center_enabled = false;

W* wn_background = NULL;
W* wn_content = NULL;
W* wn_drawclockhands = NULL;

W* wl[1] = {NULL};

//adc_read()

#define mcpy(d,s,sz) for(int i=0;i<sz;i++){d[i]=s[i];}

#define FRAME_DELAY 50
#define LOOPWAIT 25

#define POS_CX 120
#define POS_CY 120

#define POS_CX_S POS_CX-16
#define POS_CY_S POS_CY-16

#define POS_DATE_X 54
#define POS_DATE_Y 66

#define POS_DAY_X_S POS_DATE_X
#define POS_DAY_Y_S POS_DATE_Y

#define POS_MONTH_X_S POS_DATE_X+3*
#define POS_MONTH_Y_S POS_DATE_Y


#define POS_DOW_X 20
#define POS_DOW_Y 100
#define POS_CNDOW_X 18
#define POS_CNDOW_Y 68

#define POS_TIME_X 64
#define POS_TIME_Y 156

#define TFW 14

#define EYE_MAX 25-1

typedef struct PXY_t {
  uint8_t x;
  uint8_t y;
} PXY_t;

#define TFOWI 26
#define TFOSWI 14

int16_t xold,xoldt;
int16_t yold,yoldt;

// define number of backgrounds and backgrounds + extra data
uint16_t* tbg = NULL;

#define MAX_BG 1
const char* backgrounds[MAX_BG] = {bega};
const int16_t bg_size[MAX_BG] = {240};
const bool bg_dynamic[MAX_BG] = {false};

typedef enum {
  CP_EXIT=0,
  CP_BACKGROUND,
  CP_ROTATION,
  CP_SAVE,
  CP_PENSTYLE,
  CP_WAND,
  CP_PENCIL=7,
} CONF_POS;

float theta = 0.0f;
float theta1 = 0.0f;
float theta2 = 0.0f;
float theta3 = 0.0f;
float theta_d = 1.2f;

extern Vec2 vO;
extern Vec2 v32;

extern uint8_t LCD_RST_PIN;
extern W wroot;
//W* wb_time;
//W* wb_date;
//Vec2 v0 = {0,0};

uint16_t dcol = WHITE;
uint16_t editcol = YELLOW;
uint16_t changecol = YELLOW;
uint16_t acol=WHITE;

uint16_t blinker[2] = {BLUE,RED};

typedef enum Dir_t {
  D_NONE = 0,
  D_PLUS = 1,
  D_MINUS = 2
} Dir_t;


Dir_t dir_x;
Dir_t dir_y;
uint8_t no_pos_x=0;
uint8_t no_pos_y=0;

#define SLEEP_FRAME 250
uint32_t sleep_frame = SLEEP_FRAME;

char timebuffer[16] = {0};
char* ptimebuffer=timebuffer;
bool h24=true;

uint16_t comi=0;
char combufa[256]={0};
int16_t comt;
uint8_t comc;

uint8_t* b0=NULL;
uint32_t* b1=NULL;

//shell vars!
int16_t bcx0 = 80;
int16_t bcy0 = 80;
int16_t bcx1 = 80;
int16_t bcy1 = 80;

int16_t tpoy = 30;
int16_t tpox = 22;

int16_t tpol = -22;
int16_t tpor = 22;

//ky-040
#define CCLK 16
#define CDT 17
#define CSW 19

//one button /
#define QMIINT1 23
#define CBUT_TOUCH 16
uint32_t nopvar;
uint8_t CBUT0 = 22;
bool rp2040_touch = false;
bool fire_pressed=false;
bool analog_seconds=false;
uint32_t fire_counter=0;
bool fire=false;
bool ceasefire=false;
bool tcw=false;
bool tccw=false;
bool clk,dt,sw,oclk,odt,osw;
int gc=0;
char gch;
char gbuf[2] = {'c','d'};

unsigned int tim_count = 0;
float last_z = 0.0f;

uint32_t ftid[128] = {0};
uint32_t ftid_kyr = 0;
uint32_t ftidi = 0;
uint32_t ftidik = 0;
uint16_t ftsti = 0;
uint16_t ftstik = 0;

bool do_reset = false;
bool force_no_load = false;
bool is_flashed = false;

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

    plosa->scandir&=0x03;
    // I2C Config
    i2c_init(I2C_PORT, 100 * 1000);
    //sleep_ms(500);

    gpio_set_function(DEV_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DEV_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DEV_SDA_PIN);
    gpio_pull_up(DEV_SCL_PIN);
    //sleep_ms(500);

    i2c_scan();
    lcd_init();
    lcd_setatt(plosa->scandir&0x03);
    lcd_make_cosin();
    lcd_set_brightness(10);
    b0 = malloc(LCD_SZ);
    b1 = (uint32_t*)b0;
    if(b0==0){printf("b0==0!\n");}
    uint32_t o = 0;
    lcd_setimg((uint16_t*)b0);

    ftid_kyr = ftidi;
    CST816S_init(CST816S_Gesture_Mode);

    gpio_init(QMIINT1);
    gpio_set_dir(QMIINT1,GPIO_IN);
    gpio_set_irq_enabled_with_callback(QMIINT1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    if(rp2040_touch){
      gpio_init(Touch_INT_PIN);
      gpio_pull_up(Touch_INT_PIN);
      gpio_set_dir(Touch_INT_PIN,GPIO_IN);
      gpio_set_irq_enabled(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
      CST816S_init(CST816S_Point_Mode);
    }else{
      gpio_init(CBUT0);
      gpio_set_dir(CBUT0,GPIO_IN);
      gpio_pull_up(CBUT0);
      gpio_set_irq_enabled(CBUT0, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    }
    QMI8658_init();

    interp_config lane0_cfg = interp_default_config();
    interp_config_set_shift(&lane0_cfg, UNIT_LSB - 1); // -1 because 2 bytes per pixel
    interp_config_set_mask(&lane0_cfg, 1, 1 + (8 - 1));
    interp_config_set_add_raw(&lane0_cfg, true); // Add full accumulator to base with each POP
    //interp_config_set_signed(&lane0_cfg, true);
    interp_config lane1_cfg = interp_default_config();
    interp_config_set_shift(&lane1_cfg, UNIT_LSB - (1 + 8));
    interp_config_set_mask(&lane1_cfg, 1 + 8, 1 + (2 * 8 - 1));
    interp_config_set_add_raw(&lane1_cfg, true);
    //interp_config_set_signed(&lane1_cfg, true);
    interp_set_config(interp0, 0, &lane0_cfg);
    interp_set_config(interp0, 1, &lane1_cfg);
    interp0->base[2] = (uint32_t) backgrounds[0];

    sleep_ms(100);
    init_root();
    sleep_ms(100);
    wn_background = wadd_none(&wroot,draw_background);
    wn_drawclockhands = wadd_none(&wroot,draw_clock_hands);
    sleep_ms(100);

    while(true){
      if(flag){ //#FLAG
        CST816S_Get_Point();
        printf("[%d] X:%d Y:%d\r\n", plosa->scandir, Touch_CTS816.x_point, Touch_CTS816.y_point);
        int16_t tx = (int16_t)Touch_CTS816.x_point;
        int16_t ty = (int16_t)Touch_CTS816.y_point;
        if(plosa->scandir==1){
          int16_t tt = tx; tx = ty; ty = tt;
          ty = LCD_H - ty;
        }else if(plosa->scandir==2){
          tx = LCD_W - tx;
          ty = LCD_H - ty;
        }else if(plosa->scandir==3){
          int16_t tt = tx; tx = ty; ty = tt;
          tx = LCD_W - tx;
        }
        W* wf = wfindxy(&wroot, tx, ty );
        flag = 0;
      } // if(flag) END

      //for(int i=0;i<LCD_SZ;i++){b0[i]=0x00;}  //clear buffer
      for(int i=0;i<LCD_SZ/4;i++){b1[i]=0x00;}  //clear buffer faster
      wdraw(&wroot);
      lcd_display(b0);
    }
    return 0;
}