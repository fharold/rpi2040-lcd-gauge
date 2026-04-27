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
#include "img/font34.h"
#include "img/font40.h"
//#include "img/font48.h"
#include "img/fa32.h"
#include "img/fa40.h"
#include "img/fkyr32.h"
#include "img/fkyr40.h"
#include "img/fei24.h"
#include "img/fei32.h"
#include "img/fei40.h"

#include "img/e8.h"
#include "img/i8.h"

#include "img/bega.h"

//#include "img/maple.h"
#include "img/cn32.h"
#include "img/cn16.h"
#include "img/flag_jp16.h"
#include "img/flag_jp32.h"
#include "img/flag_kr16.h"
#include "img/flag_kr32.h"
#include "img/usa32.h"
#include "img/usa16.h"
#include "img/ger32.h"
#include "img/ger16.h"
#include "img/tr32.h"
#include "img/tr16.h"
#include "img/flag_gb16.h"
#include "img/flag_gb32.h"
#include "img/flag_ch16.h"
#include "img/flag_ch32.h"
#include "img/flag_hr16.h"
#include "img/flag_hr32.h"
#include "img/flag_ru16.h"
#include "img/flag_ru32.h"
#include "img/flag_ua16.h"
#include "img/flag_ua32.h"

#include "img/config.h"
//#include "img/conf_accept.h"
#include "img/conf_exit.h"
#include "img/conf_background.h"
#include "img/conf_handstyle.h"
#include "img/conf_rotate.h"
#include "img/conf_save.h"
#include "img/conf_clock.h"
#include "img/conf_bender.h"

// Tested with the parts that have the height of 240 and 320
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH2 120
#define SCREEN_HEIGHT2 120
#define SCREEN_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)
#define IMAGE_SIZE 256
#define LOG_IMAGE_SIZE 8
#define SERIAL_CLK_DIV 1.f
#define UNIT_LSB 16

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define THETA_MAX (2.f * (float) M_PI)


typedef struct {
  char mode[8];
  uint8_t BRIGHTNESS;

  bool sensors;
  bool gyrocross;
  bool bender;
  uint8_t scandir;
  bool dither;
  uint8_t dummy;
  uint8_t save_crc;
} LOSA_t;

static LOSA_t* plosa=(LOSA_t*)losabuf;

#define LOSASIZE (&plosa->dummy - &plosa->theme)

char* fp_cn = NULL;

int16_t flags_deg=0;
volatile uint8_t flag = 0;

bool center_enabled = false;

W* wn_background = NULL;
W* wn_content = NULL;
W* wn_drawclockhands = NULL;
W* img_center = NULL;
W* img_config = NULL;
W* wblinker = NULL;
W* wblinkerg = NULL;
W* wblinker_once = NULL;
W* wblinker_ref = NULL;

W* w_dotw = NULL;
W* w_dotw_cn = NULL;
W* wsp_dotw = NULL;
W* wsp_dotw_cn = NULL;

W* cim_flags = NULL;
W* cim_config = NULL;

W* wl[1] = {NULL};

W* wb_dotw; //day of the week content box (lat/cn)

WBez2_t* w_move = NULL;

bool showfps = false;
uint16_t fps = 0;
uint8_t fps_sec = 0;


#define MAX_CONF 8
#define MAX_CONFD (360/MAX_CONF)
#define MAX_FCONF 4
#define MAX_FCONFD (360/MAX_CONF)

#define MAX_SPIN 8
#define MAX_FSPIN (0.005f*8)

//forward decs [4WARD]
uint8_t crc(uint8_t *addr, uint32_t len);
void dosave();

void* config_functions[MAX_CONF];

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

const uint8_t* config_images[MAX_CONF+1] = {conf_exit,conf_background,conf_rotate,conf_save,conf_handstyle,conf_clock,conf_bender};

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
uint32_t last_wait;
uint32_t stime;
uint8_t tseco;

int blink_counter = 0;
bool bmode = false;

uint32_t last_blink;

extern float tsin[DEGS];
extern float tcos[DEGS];

bool edittime=false;
bool changetime=false;

// config symbol
DOImage* doi_config;

unsigned int tim_count = 0;
float last_z = 0.0f;

uint16_t cn_chars=0;
char ftst[128*4] = {0};
uint32_t ftid[128] = {0};
uint32_t ftid_kyr = 0;
uint32_t ftidi = 0;
uint32_t ftidik = 0;
uint16_t ftsti = 0;
uint16_t ftstik = 0;

bool do_reset = false;
bool force_no_load = false;
bool is_flashed = false;
char crcstatus[32] = {"\0"};
char flashstatus[32] = {"\0"};

void __no_inline_not_in_flash_func(flash_data_load)(){
	uint32_t xip_offset = 0xb0000;
	char *p = (char *)XIP_BASE+xip_offset;
	for(size_t i=0;i<FLASH_SECTOR_SIZE;i++){ losabuf[i]=p[i];	}
}

void __no_inline_not_in_flash_func(flash_data)(){
	printf("FLASHING SAVE (c%d)\n",get_core_num());
	uint32_t xip_offset = 0xb0000;
	char *p = (char *)XIP_BASE+xip_offset;
	uint32_t ints = save_and_disable_interrupts();
	flash_range_erase (xip_offset, FLASH_SECTOR_SIZE);
	flash_range_program (xip_offset, (uint8_t*)losabuf, FLASH_SECTOR_SIZE);
	for(size_t i=0;i<FLASH_SECTOR_SIZE;i++){ losabuf[i]=p[i];	}
	restore_interrupts(ints);
  //sleep_ms(500);
	printf("FLASHED!\n");

}

uint8_t crc(uint8_t *addr, uint32_t len){
    uint8_t crc = 0;
    while (len != 0){
        uint8_t i;
        uint8_t in_byte = *addr++;
        for (i = 8; i != 0; i--){
            uint8_t carry = (crc ^ in_byte ) & 0x80;        /* set carry */
            crc <<= 1;                                      /* left shift 1 */
            if (carry != 0){                crc ^= 0x7;            }
            in_byte <<= 1;                                  /* left shift 1 */
        }
        len--;                                              /* len-- */
  }
  return crc;                                               /* return crc */
}

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

uint16_t to_rgb565(uint8_t r,uint8_t g,uint8_t b){
  r>>=3;
  g>>=2;
  b>>=3;
  return ((r<<11)+(g<<5)+b);
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b){
  return ((r<<11)+(g<<5)+b);
}

void to_rgb(uint16_t rgb, uint8_t* r, uint8_t* g, uint8_t* b){
  *r=((rgb>>11)&0x1f)<<3;
  *g=((rgb>>5)&0x3f)<<3;
  *b=(rgb&0x1f)<<3;
}

inline uint32_t get_ac(char** p){
  char n;
  uint32_t r=0;
  char* pc=*p;
  n=*pc;
  //printf("pc=%08x %02x ",pc,*pc);
  if(     (0b11000000&n)==0b10000000){ r= n; *p+=1;}
  else if((0b11100000&n)==0b11000000){ r= (pc[0]<<8) + pc[1]; *p+=2;}
  else if((0b11110000&n)==0b11100000){ r= (pc[0]<<16)+(pc[1]<<8) +pc[2]; *p+=3;}
  else if((0b11111000&n)==0b11110000){ r= (pc[0]<<24)+(pc[1]<<16)+(pc[2]<<8)+pc[3]; *p+=4;}
  //printf("%08x -> pc=%08x\n",r,*p);
  return r;
}

uint8_t get_acid(char** p){
  char** pt = p;
  uint32_t r = get_ac(pt);
  for(uint8_t i=0;i<ftidi;++i){
    if(ftid[i]==r)return i+1;
  }
  return 0;
}

uint32_t find_ac(uint32_t c){
  for(uint32_t i=0;i<ftidi;++i){ if(ftid[i]==c)return i; }
  return 0xffffffff;
}
void convert_as(char* source, char* target){
  uint32_t si=0, ti=0;
  printf("c_as: '%s'\n",source);
  while(*source){target[ti++]=get_acid(&source);}
  target[ti]=0;
  printf("convert_as: ");
  for(int i=0;i<ti;++i){
    if(target[i]>=ftid_kyr)target[i]-=ftid_kyr;
    printf("%02x ",target[i]);
  }
  printf("\n");
}

uint8_t find_cc(uint8_t a, uint8_t b, uint8_t c){
  uint fo=0;
  for(int i=0; i<cn_chars+1;i++){
    if( (ftst[fo+0]==a) && (ftst[fo+1]==b) && (ftst[fo+2]==c) ){ return i; }
    fo+=4;
  }
}
void convert_cs(char* source, char* target){
  uint32_t si=0, ti=0;
  while(source[si]){
    target[ti]=find_cc(source[si],source[si+1],source[si+2]);
    si+=3;
    target[ti]+=(256-32);
    ++ti;
    target[ti]+='\n';
  }
  target[ti]=0;
}

void print_font_table4(char* pc){
  uint8_t fts=0;
  uint8_t n=0;
  uint8_t nbytes=0;
  uint32_t ft[128];
  uint32_t sti=0;
  char ftc[5] = {0};
  printf("symcheck1\n");
  uint32_t c = 0;
  while(*pc){
    ft[fts]=get_ac(&pc);
    bool dupe=false;
    for(int j=0;j<fts;j++){
      if(ft[j]==ft[fts]){dupe=true;break;}
    }
    if(!dupe){++fts;}
  }
  uint32_t i,k;
  uint32_t temp;
  n=fts;
  for(i = 0; i<n-1; i++) {
    for(k = 0; k<n-1-i; k++) {
      if(ft[k] > ft[k+1]) {
        temp = ft[k];
        ft[k] = ft[k+1];
        ft[k+1] = temp;
      }
    }
  }
  for(i=0;i<fts;++i){    printf("1: (%02d) %08x\n",i,ft[i]);  }
  char* ppc = (char*)&ft[0];
  sti=0;
  char cls[512] = {0};
  uint32_t clsi = 0;
  for(i=0;i<fts;i++){
    //printf("%02d : %d %02x %02x %02x %02x\n",i,ft[i],pc[0],pc[1],pc[2],pc[3]);
    ftc[0]=ppc[3];
    ftc[1]=ppc[2];
    ftc[2]=ppc[1];
    ftc[3]=ppc[0];
    printf("S1[%02d]: %02x %02x %02x %02x %s\n",i,ftc[0],ftc[1],ftc[2],ftc[3],&ftc[1]);
    cls[clsi+0]=ftc[1];
    cls[clsi+1]=ftc[2];
    cls[clsi+2]=ftc[3];
    clsi+=3;  //ftid[ftidi++]=(uint32_t)ftc[0]<<24+ftc[1]<<16+ftc[2]<<8+ftc[3];
    ftid[ftidi++]=(uint32_t)ft[i];
    ppc+=4;
  }
  if(!cn_chars){
    //ftst[ftsti]=0;
    cls[clsi]=0;
    cn_chars=fts;
  }
  printf("CHARLIST:\n%s (%d)\n\n\n",cls,strlen(cls));
  char* clst = &cls[0];
  while(*clst){
    printf("B %08x 0x%02x\n",clst,get_acid(&clst));
  }

}

#define MS 1000
#define US 1000000
#define BUTD 500  // delay between possible button presses (default: 500, half of a second)
#define REBOOT US*3 // 30 second/10
uint32_t rebootcounter = 0;
uint32_t rebootcounterold = 0;

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

void draw_gfx(){
  uint8_t x1,y1,xt,yt;
  uint8_t x0=120;
  uint8_t y0=120;
    
  int xi,yi;
  Vec2 vc_s,vc_e;
  float scf = DEGS/360.0f;
  float mindeg = 1024.0f/60.0f;
  for(int16_t i=0;i<60;i++){
    vc_e = gvdl((int16_t)i*mindeg,119);
    vc_e = vadd(vc_e,vO);
    if(!(i%5)){
      vc_s = gvdl((int16_t)i*mindeg,110);
      vc_s = vadd(vc_s,vO);
      // lcd_linev2(vc_s,vc_e, null, 1);
    }else{
      vc_s = gvdl((int16_t)i*mindeg,115);
      vc_s = vadd(vc_s,vO);
    }
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
    interp_config_set_mask(&lane0_cfg, 1, 1 + (LOG_IMAGE_SIZE - 1));
    interp_config_set_add_raw(&lane0_cfg, true); // Add full accumulator to base with each POP
    //interp_config_set_signed(&lane0_cfg, true);
    interp_config lane1_cfg = interp_default_config();
    interp_config_set_shift(&lane1_cfg, UNIT_LSB - (1 + LOG_IMAGE_SIZE));
    interp_config_set_mask(&lane1_cfg, 1 + LOG_IMAGE_SIZE, 1 + (2 * LOG_IMAGE_SIZE - 1));
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