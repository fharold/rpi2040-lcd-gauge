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
#include "img/b256.h"

#include "img/bega.h"
#include "img/sand.h"
#include "img/bg1_256.h"
#include "img/bg2_256.h"
#include "img/bg3_256.h"

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
#include "img/conf_rotozoom.h"
#include "img/conf_rotate.h"
#include "img/conf_save.h"
#include "img/conf_clock.h"
#include "img/conf_bender.h"

//textures (clock hands)
#include "img/w2.h"
#include "img/tiles_blue.h"
#include "img/flow.h"
#include "img/l3.h"
#include "img/gt.h"

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


typedef enum {
  GFX_NORMAL,
  GFX_ROTOZOOM,
  GFX_ROTATE,
} GFX_MODE;



typedef struct {
  char mode[8];
  datetime_t dt;
  uint8_t theme;
  uint8_t editpos;
  uint8_t BRIGHTNESS;

  bool sensors;
  bool gyrocross;
  bool bender;
  bool SMOOTH_BACKGROUND;
  bool INSOMNIA;
  bool DYNAMIC_CIRCLES;
  bool DEEPSLEEP;
  bool is_sleeping;
  bool highpointer;
  bool alphapointer;
  bool clock;
  bool pointerdemo;
  bool rotoz;
  bool rota;
  GFX_MODE gfxmode;
  float fspin;
  uint8_t pstyle;
  int8_t spin;
  uint8_t texture;
  uint8_t configpos;
  uint8_t conf_bg;
  uint8_t conf_phour;
  uint8_t conf_pmin;
  uint8_t scandir;
  bool dither;
  uint8_t dummy;
  uint8_t save_crc;
} LOSA_t;

static LOSA_t* plosa=(LOSA_t*)losabuf;

#define LOSASIZE (&plosa->dummy - &plosa->theme)

datetime_t default_time = {
  .year  = 2024,
  .month = 4,
  .day   = 22,
  .dotw  = 1, // 0 is Sunday, so 5 is Friday
  .hour  = 23,
  .min   = 39,
  .sec   = 0
};

char b_year[6];
char b_month[32];
char b_day[4];
char b_dotw[32];
char b_hour[4];
char b_min[4];
char b_sec[4];

char* fp_cn = NULL;

bool deepsleep=false;
int16_t flagdeg=90;
int16_t fdegs[7] = {90,30,60,30,60,130,260};
int16_t b2s=75;

int16_t config_deg=0;
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

W* w_hour = NULL;
W* w_min = NULL;
W* w_sec = NULL;
W* w_timed0 = NULL;
W* w_timed1 = NULL;
W* wsp_hour = NULL;
W* wsp_min = NULL;
W* wsp_sec = NULL;

W* w_day = NULL;
W* w_month = NULL;
W* w_year = NULL;
W* w_dated0 = NULL;
W* w_dated1 = NULL;
W* wsp_day = NULL;
W* wsp_month = NULL;
W* wsp_year = NULL;

W* cim_flags = NULL;
W* cim_config = NULL;

W* wl[14] = {NULL};

W* wb_dotw; //day of the week content box (lat/cn)

WBez2_t* w_move = NULL;

bool showfps = false;
uint16_t fps = 0;
uint8_t fps_sec = 0;

#define DEFAULT_THEME 0

#define MAX_CONF 8
#define MAX_CONFD (360/MAX_CONF)
#define MAX_FCONF 4
#define MAX_FCONFD (360/MAX_CONF)

#define MAX_SPIN 8
#define MAX_FSPIN (0.005f*8)

//forward decs [4WARD]
uint8_t crc(uint8_t *addr, uint32_t len);
void dosave();
void wtest();
void reblink(W* w, W* wt);
void reblink2(W* w0, W* w1, W* wt);
void blinker_off(W* w);
void blink_once(W* w, W* wt);
void brepos(W* w, W* wt);
//void draw_pointer(Vec2 vs, Vec2 vts, int16_t tu, uint16_t color, const uint8_t* sr, uint16_t alpha);
//void draw_pointer_mode(Vec2 vs, Vec2 vts, int16_t tu, uint16_t color, const uint8_t* sr, uint16_t alpha, PSTYLE cps);


//(uint16_t (*image_function)(void* self))config_functions[MAX_CONF];

void* config_functions[MAX_CONF];

//adc_read()

#define FKYR3x fkyr32

//font_t** fonts20[3] = {Font20,NULL,NULL};
font_t* f24[3] = {&fei24,&fkyr32,NULL};
font_t* f32[3] = {&fei32,&fkyr32,&fa32};
font_t* f40[3] = {&fei40,&fkyr40,&fa40};

#define TFONT fei24
//#define CNFONT fa32
//#define CNFONT font24a
#define TFO Font40
#define TFOS fei24
#define TFOW Font40

#define mcpy(d,s,sz) for(int i=0;i<sz;i++){d[i]=s[i];}

#define FRAME_DELAY 50
#define LOOPWAIT 25

#define DRAW_GFX_FIRST true //1 == text floating above clock
#define BUTTONGLASSC 300
#define BUTTONGLASS 1400
// 1st screensaver
#define SCRSAV 100
// 2nd screensaver
#define SCRSAV2 50
#define BRIGHTD 20
#define SWITCH_THEME_DELAY 10
#define THRS 12
#define THRLY 120

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

typedef enum CMode {
  CM_None = 0,
  CM_Config = 1,
  CM_Editpos = 2,
  CM_Changepos = 3,
  CM_Changetheme = 4
} CMode;

typedef enum CModeT {
  CMT_None = 0,
  CMT_Select = 1,
  CMT_EditDOTW,
  CMT_EditDOTW_CN,
  CMT_EditDOTWDone,
  CMT_EditHour,
  CMT_EditHourDone,
  CMT_EditMin,
  CMT_EditMinDone,
  CMT_EditSec,
  CMT_EditSecDone,
  CMT_EditDay,
  CMT_EditDayDone,
  CMT_EditMonth,
  CMT_EditMonthDone,
  CMT_EditYear,
  CMT_EditYearDone,
  CMT_MoveDone,
  CMT_BlinkOnce,
} CModeT;


typedef struct ColorTheme_t{
  uint16_t alpha;
  uint16_t col_h;
  uint16_t col_m;
  uint16_t col_s;
  uint16_t col_cs;
  uint16_t col_cs5;
  uint16_t col_dotw;
  uint16_t col_date;
  uint16_t col_time;
} ColorTheme_t;

typedef struct PXY_t {
  uint8_t x;
  uint8_t y;
} PXY_t;

typedef struct ThemePos_t {
  PXY_t pos_dow;
  PXY_t pos_day;
  PXY_t pos_month;
  PXY_t pos_year;
  PXY_t pos_h;
  PXY_t pos_m;
  PXY_t pos_s;
} ThemePos_t;

#define TFOWI 26
#define TFOSWI 14

#define EDITPOSITIONS 8
  PXY_t tpos[EDITPOSITIONS+1] =
  { POS_DOW_X,POS_DOW_Y,
    POS_DATE_X,POS_DATE_Y,
    POS_DATE_X+2*TFOWI+8,POS_DATE_Y,
    POS_DATE_X+4*TFOWI+16,POS_DATE_Y,
    POS_TIME_X,      POS_TIME_Y,
    POS_TIME_X+2*TFOSWI-8,POS_TIME_Y,
    POS_TIME_X+4*TFOSWI,POS_TIME_Y,
    POS_CX+88,POS_CY,
    POS_CX, POS_CY
  };

  #define EPOS_CENTER EDITPOSITIONS
  #define EPOS_CONFIG EDITPOSITIONS-1


  uint8_t pos_matrix_x=1; // start in center
  uint8_t pos_matrix_y=1; // start in center


/* Position Matrices
it's like a gear shifter
[ | / \] : up/down movement ways
[ -    ] : left/right movement ways

example us [3*3]:
DAY-MON-YEAR
 |   |  /
DOW-FLAG
 |   |  \
H  - M - S

example cn [4*3]:
DAY-MON-YEAR-DOW
  \  |  /     |
   FLAG    - DOW
  /  |  \     |
H  - M - S - DOW

*/

// pos points to position matrix, containing possible 'editpos' (positions)
  typedef struct PosMat_t {
    uint8_t dim_x;
    uint8_t dim_y;
    uint8_t* pos;
  } PosMat_t;

  int8_t pos_matrix_CN[] =
  {
    1,2,3,0,
    7,8,8,0,
    4,5,6,0
  };


  int8_t pos_matrix_US[] =
  {
    1,2,3,
    0,8,7,
    4,5,6
  };

  PosMat_t p_us = {3,3,pos_matrix_US};
  PosMat_t p_cn = {3,3,pos_matrix_US};
  //PosMat_t p_cn = {4,3,pos_matrix_CN};

#define USA_Old_Glory_Red  0xB0C8 //0xB31942
#define USA_Old_Glory_Blue 0x098C //0x0A3161

#define CN_Red 0xE8E4 //0xee1c25
#define CN_Gold 0xFFE0 //0xffff00

#define GER_Gold 0xFE60
#define GER_Red 0xF800

#define TR_Red 0xF800
#define TR_White 0xFFFF

//#define GB_Blue 0x012169
#define GB_Blue 0x010D
//#define GB_Red  0xC8102E
#define GB_Red  0xC885
#define GB_White 0xFFFF

//#define CH_Red 0xDA291C
#define CH_Red 0xD943
#define CH_White 0xFFFF

#define KR_White 0xffff
#define KR_Red RGB565(0xCD,0x2E,0x3A)
#define KR_Blue RGB565(0x00,0x47,0xA0)

#define JP_Red   RGB565(0xEE,0x00,0x00)
#define JP_White RGB565(0xff,0xff,0xff)

#define HR_Yellow RGB565(0xf7,0xdb,0x17)
#define HR_Blue   RGB565(0x17,0x17,0x96)
#define HR_Cyan   RGB565(0x00,0x93,0xdd)
#define HR_Red    RGB565(0xff,0x00,0x00)

#define RU_Blue   RGB565(0x00,0x36,0xA7)
#define RU_Red    RGB565(0xD6,0x27,0x18)
#define RU_White  0xffff

#define UA_Yellow RGB565(0xFF,0xD8,0x00)
#define UA_StrongAzure RGB565(0x00,0x56,0xB9)


#define THEMES 11

typedef struct theme_t {
  const uint8_t* flag16;
  const uint8_t* flag32;
  ColorTheme_t colt;
  char* week[7];
  font_t* afont;
  uint16_t editcolor;
  uint16_t changecolor;
} theme_t;

CMode cmode = CM_None;
int16_t xold,xoldt;
int16_t yold,yoldt;

//uint8_t theme = DEFAULT_THEME;
const PosMat_t* positions[THEMES] = {&p_cn,&p_cn,&p_cn,&p_us,&p_us,&p_us,&p_us,&p_us,&p_us,&p_us,&p_us};
const uint8_t* flags[THEMES] = {cn32,flag_jp32,flag_kr32,usa32,ger32,tr32,flag_gb32,flag_ch32,flag_hr32,flag_ru32,flag_ua32};
const uint8_t* stars[THEMES] = {cn16,flag_jp16,flag_kr16,usa16,ger16,tr16,flag_gb16,flag_ch16,flag_hr16,flag_ru16,flag_ua16};

// define number of backgrounds and backgrounds + extra data
uint16_t* tbg = NULL;

#define MAX_BG 8
const char* backgrounds[MAX_BG] = {e8,i8,b256,bega,sand,
  //bg1_256,bg2_256,bg3_256, bg4_256,bg5_256,bg6_256
  bg1_256,bg2_256,bg3_256, //bg4_256
};
const int16_t bg_size[MAX_BG] = {256,256,256,240,240,256,256,256};
const bool bg_dynamic[MAX_BG] = {true,true,true,false,false,false,false,false};


#define TEXTURES 5
#define MAX_TEXTURE TEXTURES
uint16_t pd_tex = 0;
const char* textures[TEXTURES] ={w2,flow,tiles_blue,l3,gt};
Vec2 texsize[TEXTURES] = {128,20, 128,20, 128,19, 128,25, 120,26  };
Vec2 psize_h[TEXTURES] = {75,20,   75,20,  75,19,  80,25,  75,20  };
Vec2 psize_m[TEXTURES] = {102,16,  102,10, 102,10, 118,25, 102,10 };

const uint16_t edit_colors[THEMES] = {ORANGE,YELLOW,ORANGE,ORANGE,ORANGE,ORANGE  ,ORANGE,ORANGE,ORANGE,ORANGE,ORANGE};
const uint16_t change_colors[THEMES] = {YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW,YELLOW};

uint8_t theme_bg_dynamic_mode = 0;


theme_t themes[THEMES] = {
  //cn
  cn16,cn32,(ColorTheme_t){
    BLACK,CN_Red,CN_Red,CN_Gold,CN_Red,CN_Gold,WHITE,WHITE,WHITE},
    "星期日\0","星期一\0","星期二\0","星期三\0","星期四\0","星期五\0","星期六\0", &fa32,YELLOW,ORANGE
    //"三二四\0","星期一\0","星期二\0","星期三\0","星期四\0","星期五\0","星期六\0", &fa32,YELLOW,ORANGE
  //jp
  ,flag_jp16,flag_jp32, {BLACK,JP_Red,JP_Red,JP_White,NWHITE,JP_Red,WHITE,WHITE,WHITE},
    "日曜日\0","月曜日\0","火曜日\0","水曜日\0","木曜日\0","金曜日\0","土曜日\0",&fa32,YELLOW,ORANGE
  //kr
  ,flag_kr16,flag_kr32,{BLACK,KR_Red,KR_Blue,KR_White,NWHITE,KR_Red,WHITE,WHITE,WHITE},
  {"일요일\0","월요일\0","화요일\0","수요일\0","목요일\0","금요일\0","토요일\0"},&fa32,YELLOW,ORANGE,
 usa16, usa32,{BLACK,USA_Old_Glory_Red,USA_Old_Glory_Blue,NWHITE,USA_Old_Glory_Red,WHITE,WHITE,WHITE,WHITE}
 ,{"Sun\0","Mon\0","Tue\0","Wed\0","Thu\0","Fri\0","Sat\0"}, NULL, YELLOW, ORANGE,
//de
 ger16, ger32,{BLACK,GER_Red,BLACK,GER_Gold,GER_Red,GER_Gold,WHITE,WHITE,WHITE}
 ,{"Son\0","Mon\0","Die\0","Mit\0","Don\0","Fre\0","Sam\0"}, NULL, YELLOW, ORANGE,
//tr
 tr16, tr32,{BLACK,TR_Red,TR_Red,TR_White,NWHITE,TR_Red,WHITE,WHITE,WHITE},
 {"PAZ\0","PZT\0","SAL\0","CAR\0","PER\0","CUM\0","CMT\0"}, NULL, YELLOW, ORANGE,
//gb
 flag_gb16, flag_gb32,{BLACK,GB_Blue,GB_Red,GB_White,NWHITE,GB_Red,WHITE,WHITE,WHITE},
 {"Sun\0","Mon\0","Tue\0","Wed\0","Thu\0","Fri\0","Sat\0"}, NULL, YELLOW, ORANGE,
//ch
 flag_ch16, flag_ch32, {BLACK,CH_Red,CH_Red,CH_White,NWHITE,GB_Red,WHITE,WHITE,WHITE},
 {"Son\0","Mon\0","Die\0","Mit\0","Don\0","Fre\0","Sam\0"}, NULL, YELLOW, ORANGE,
 //hr
  flag_hr16, flag_hr32, {BLACK,HR_Red,HR_Blue,HR_Cyan,HR_Yellow,HR_Yellow,HR_Cyan,WHITE,WHITE},
  {"ned\0","pon\0","uto\0","sri\0","čet\0","pet\0","sub\0"}, &fkyr40, YELLOW, ORANGE,
 //ru
  flag_ru16, flag_ru32, {BLACK,RU_Red,RU_Blue,RU_White,NWHITE,RU_Red,RU_Blue,WHITE,WHITE},
  {"Вс","Пн","Вт","Ср","Чт","Пт","Сб"}, &fkyr40, YELLOW, ORANGE,
 //ua
  flag_ua16, flag_ua32, {BLACK,UA_Yellow,UA_StrongAzure,UA_StrongAzure,NWHITE,UA_Yellow,UA_StrongAzure,WHITE,WHITE},
  {"Няд","Пнд","Аўт","Сер","Чцв","Пят","Суб"}, &fkyr40, YELLOW, ORANGE
};


typedef enum {
  CP_EXIT=0,
  CP_BACKGROUND,
  CP_ROTOZOOM,
  CP_ROTATION,
  CP_SAVE,
  CP_PENSTYLE,
  CP_WAND,
  CP_PENCIL=7,
} CONF_POS;

const uint8_t* config_images[MAX_CONF+1] = {conf_exit,conf_background,conf_rotozoom,conf_rotate,conf_save,conf_handstyle,conf_clock,conf_bender};

float theta = 0.0f;
float theta1 = 0.0f;
float theta2 = 0.0f;
float theta3 = 0.0f;
float theta_d = 1.2f;

void update_pos_matrix(){
  if(pos_matrix_x>=positions[plosa->theme]->dim_x){
    pos_matrix_x=positions[plosa->theme]->dim_x-1;
  }
  if(pos_matrix_y>=positions[plosa->theme]->dim_y){
    pos_matrix_y=positions[plosa->theme]->dim_y-1;
  }
}

void repos(uint8_t id){
  for(uint8_t j=0;j<positions[plosa->theme]->dim_y;j++){
    for(uint8_t i=0;i<positions[plosa->theme]->dim_x;i++){
      if(id == positions[plosa->theme]->pos[j*(positions[plosa->theme]->dim_x)+i]){
        pos_matrix_x = i;
        pos_matrix_y = j;
        break;
      }
    }
  }
}

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
uint16_t colors[EDITPOSITIONS+1]  = {WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE};
uint16_t dcolors[EDITPOSITIONS+1] = {WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE};

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


//Bez2_t* bezt[8] = {NULL};
//
//Bez2_t* tbez = NULL;
//int16_t bfc = 0;

//char datetime_buf[256];
//char *datetime_str = &datetime_buf[0];
//char* dt_date;
//char* dt_time;

//uint32_t dps=0;
//uint32_t dpsc=0;

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
bool temp_read=false;
int gc=0;
char gch;
char gbuf[2] = {'c','d'};
uint32_t last_wait;
uint32_t stime;
uint8_t tseco;

bool hg_enabled=false;
int16_t hgx=0;
int16_t hgy=0;

int16_t buttonglass=BUTTONGLASS;
int16_t screensaver=SCRSAV;

int flagsdelay = SWITCH_THEME_DELAY;
int blink_counter = 0;
bool bmode = false;

uint32_t last_blink;

extern float tsin[DEGS];
extern float tcos[DEGS];
//float tfsin[600];
//float tfcos[600];

bool edittime=false;
bool changetime=false;
char dbuf[8];
float temperature = -99.99f;

float mag[3];
bool draw_gfx_first = DRAW_GFX_FIRST;
bool draw_gfx_enabled = true;
bool draw_config_enabled = false;
bool draw_flagconfig_enabled = false;

// config symbol
DOImage* doi_config;
DOImage* doi_config_cn;

DOImage** adoi_config[THEMES] = {&doi_config_cn,&doi_config_cn,&doi_config_cn,&doi_config,&doi_config,&doi_config,&doi_config,&doi_config,&doi_config,&doi_config,&doi_config};

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

//char** nlatc[4]={week_cn,week_jp,week_ru,week_ua};
// dummy month0
uint8_t last[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

char* hours[24] = {"00","01","02","03","04","05","06","07","08","09",
               "10","11","12","13","14","15","16","17","18","19",
               "20","21","22","23"};
char* numbers[100] = {
  "00","01","02","03","04","05","06","07","08","09",
  "10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29",
  "30","31","32","33","34","35","36","37","38","39",
  "40","41","42","43","44","45","46","47","48","49",
  "50","51","52","53","54","55","56","57","58","59",
  "60","61","62","63","64","65","66","67","68","69",
  "70","71","72","73","74","75","76","77","78","79",
  "80","82","81","83","84","85","86","87","88","89",
  "90","91","92","93","94","95","96","97","98","99"
};

char cn_buffer[32] = {0};

bool do_reset = false;
bool force_no_load = false;
bool is_flashed = false;
char crcstatus[32] = {"\0"};
char flashstatus[32] = {"\0"};

//objdump -x main.elf | grep binary_info_end
//1006f7d8 g       .binary_info   00000000 __binary_info_end
// 0x90000 == xip_offset (must be bigger then the above value from objdump)

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

void check_save_data(){
  uint8_t acrc = crc(&plosa->theme, LOSASIZE);
  bool crc_status = (acrc==plosa->save_crc);
  sprintf(crcstatus,"CRC: [%02x][%02x] %s\n\0",acrc,plosa->save_crc,(crc_status)?"OK":"ERR");
  if(strstr((char*)plosa->mode,"LOAD")!=plosa->mode){
    plosa->dt.year  = default_time.year ;
    plosa->dt.month = default_time.month;
    plosa->dt.day   = default_time.day  ;
    plosa->dt.dotw  = default_time.dotw ;
    plosa->dt.hour  = default_time.hour ;
    plosa->dt.min   = default_time.min  ;
    plosa->dt.sec   = default_time.sec  ;
    plosa->theme = DEFAULT_THEME;
    plosa->editpos = EDITPOSITIONS; //center
    plosa->is_sleeping = false;
    plosa->highpointer = false;
    plosa->alphapointer = true;
    plosa->pointerdemo = false;
    plosa->pstyle = PS_NORMAL;
    plosa->clock = true;
    plosa->spin = 0;
    plosa->fspin = 0.0f;
    plosa->texture = 0;
    plosa->configpos = 0;
    plosa->conf_bg = 0;
    plosa->conf_pmin = 0;
    plosa->conf_phour = 0;
    plosa->rota = false;
    plosa->rotoz = false;
    plosa->gfxmode = GFX_NORMAL;
    plosa->sensors = false;
    plosa->bender = false;
    plosa->gyrocross = true;
    plosa->DYNAMIC_CIRCLES = false;
    plosa->DEEPSLEEP = true;
    plosa->INSOMNIA = true;
    sprintf(plosa->mode,"LOAD\0");
    printf("settings reset to defaults");
  }else{ // do a few sanity checks
    plosa->dt.sec+=1;
    if(plosa->dt.month > 12 ){plosa->dt.month = default_time.month;}
    if(plosa->dt.day   > 31) {plosa->dt.day   = default_time.day  ;}
    if(plosa->dt.dotw  > 6)  {plosa->dt.dotw  = default_time.dotw ;}
    if(plosa->dt.hour  > 23) {plosa->dt.hour  = default_time.hour ;}
    if(plosa->dt.min   > 59) {plosa->dt.min   = default_time.min  ;}
    if(plosa->dt.sec   > 59) {plosa->dt.sec   = default_time.sec  ;}
    if(plosa->theme>=THEMES){plosa->theme=0;}
    if(plosa->editpos>EDITPOSITIONS){plosa->editpos=EDITPOSITIONS;}
    if(plosa->conf_bg>=MAX_BG){plosa->conf_bg=0;}
    if(plosa->gfxmode>GFX_ROTATE){plosa->gfxmode=GFX_NORMAL;}
    if(plosa->spin>7){plosa->spin=7;}
    if(plosa->fspin>THETA_MAX){plosa->fspin=0.0f;}
    if(plosa->fspin<-THETA_MAX){plosa->fspin=0.0f;}
    if(plosa->texture>=TEXTURES){plosa->texture=0;}
    if(plosa->configpos>=MAX_CONF){plosa->configpos = 0;}
    if(plosa->pstyle>=PS_TEXTURE){plosa->pstyle = PS_TEXTURE;}
    if(plosa->scandir>3){plosa->scandir = 0;}
    if(plosa->bender>1){plosa->bender = 0;}
    if(plosa->BRIGHTNESS > 100 || plosa->BRIGHTNESS <= 10)plosa->BRIGHTNESS = 30;
    printf("MODE:='%s'\n",plosa->mode);
  }



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

void empty_deinit(){
  //printf("REBOOTING...\n");
  plosa->save_crc = crc(&plosa->theme,LOSASIZE);
}

void doreset(){
  spi_deinit(SPI_PORT);
  i2c_deinit(I2C_PORT);
  watchdog_reboot((uint32_t)&empty_deinit,0,1);
  watchdog_enable(1, 1);
}

void dosave(){
  // do other stuff here
  sprintf((char*)&plosa->mode,"SAVE");
  doreset();
}

void draw_pointer(Vec2 vs, Vec2 vts, int16_t tu, uint16_t color, const uint8_t* sr, uint16_t alpha){
  draw_pointer_mode(vs,vts,tu,color,sr,alpha,plosa->pstyle);
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

void set_dcolors(){
  for(int i=0;i<7;i++){
    colors[i]=dcolors[i];
  }
}

void set_colt_colors(){
  dcolors[0]=themes[plosa->theme].colt.col_dotw;
  dcolors[1]=themes[plosa->theme].colt.col_date;
  dcolors[2]=themes[plosa->theme].colt.col_date;
  dcolors[3]=themes[plosa->theme].colt.col_date;
  dcolors[4]=themes[plosa->theme].colt.col_time;
  dcolors[5]=themes[plosa->theme].colt.col_time;
  dcolors[6]=themes[plosa->theme].colt.col_time;
  dcolors[7]=themes[plosa->theme].colt.col_time;
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
uint32_t button0_time=0;
uint32_t button1_time=0;
uint32_t button0_dif=0;
uint32_t button1_dif=0;

void gpio_callback(uint gpio, uint32_t events) {
    if(events&GPIO_IRQ_EDGE_RISE){
      if(gpio==CBUT0){ceasefire=true;fire_pressed=false;rebootcounter=0;}
      if(gpio==QMIINT1){ deepsleep=false; }
      if(rp2040_touch){
        if(gpio==Touch_INT_PIN){
          deepsleep=false;
          flag = 1;
        }
      }
    }

    if(events&GPIO_IRQ_EDGE_FALL){
      if(gpio==CBUT0 && !fire && (((time_us_32()-button0_time)/MS)>=BUTD)){ceasefire=false;fire=true;button0_time = time_us_32();fire_pressed=true;}
    }

}
char C_SET[4]="set ";
char C_GET[4]="get ";
void command(char* c){
    bool tc=false;  // time changed
    char* left=c;
    if(strstr(left," ")){
      char* space = strstr(left," ");
      space[0] = 0;
      char* right = space+1;
      if(strstr(left,"b2s")){   b2s = (int16_t)atoi(right);}
      if(strstr(left,"dither")){   plosa->dither = (bool)atoi(right);}
      if(strstr(left,"spos")){
        W_spinner* wsp = (W_spinner*)wsp_dotw_cn->d;
        wsp->fpos = (uint16_t)atoi(right);
        wsp->fixed = (wsp->fpos)?true:false;
      }
      //if(strstr(left,"setfsh")){   Font38.h = (uint16_t)atoi(right);}
      //if(strstr(left,"setfsw")){   Font38.w = (uint16_t)atoi(right);}
      if(strstr(left,"scandir")){   plosa->scandir = ((uint8_t)atoi(right))&0x03;lcd_setatt(plosa->scandir);}
      if(strstr(left,"ori")){   plosa->scandir = ((uint8_t)atoi(right))&0x03;lcd_setatt(plosa->scandir);}
      if(strstr(left,"sensors")){   plosa->sensors = (bool)atoi(right);}
      if(strstr(left,"gyro")){ plosa->gyrocross = (bool)atoi(right);}
      if(strstr(left,"bender")){    plosa->bender = (bool)atoi(right);}
      if(strstr(left,"smooth")){  plosa->SMOOTH_BACKGROUND = (bool)atoi(right);}
      if(strstr(left,"inso")){  plosa->INSOMNIA = true;}
      if(strstr(left,"insomnia")){  plosa->INSOMNIA = true;}
      if(strstr(left,"circle")){ plosa->DYNAMIC_CIRCLES = (bool)atoi(right);}
      if(strstr(left,"light")){     plosa->BRIGHTNESS = (uint8_t)atoi(right);lcd_set_brightness(plosa->BRIGHTNESS);}
      if(strstr(left,"deep")){ plosa->DEEPSLEEP = (bool)atoi(right);}
      if(strstr(left,"high")){ plosa->highpointer = (bool)atoi(right);}
      if(strstr(left,"alpha")){ plosa->alphapointer = (bool)atoi(right);}
      if(strstr(left,"pstyle")){ plosa->pstyle = (PSTYLE)atoi(right);}
      if(strstr(left,"theme")){
        plosa->theme = (uint8_t)atoi(right);
        if(plosa->theme>=THEMES){
          plosa->theme=THEMES-1;
        }
        if(plosa->theme<3){  wshow(w_dotw_cn);whide(w_dotw); // china
        }else{ whide(w_dotw_cn); wshow(w_dotw); }

        blinker_off(wblinker);
        if(wblinker_ref){wblinker_ref=NULL;}
      }
      if(strstr(left,"thd")){ theta_d = (float)atof(right);return;}
      if(strstr(left,"tht")){ theta = (float)atof(right);return;}
      if(strstr(left,"texture")){
        plosa->texture = (uint8_t)atoi(right);
        if(plosa->texture>=TEXTURES){plosa->texture=TEXTURES-1;}
      }
      if(strstr(left,"editpos")){
        plosa->editpos = (uint8_t)atoi(right);
        if(plosa->editpos>EDITPOSITIONS){plosa->editpos=EDITPOSITIONS;}
        repos(plosa->editpos);
      }
      if(strstr(left,"spin")){
        plosa->spin = (uint8_t)atoi(right);
        plosa->fspin = 0.005f*plosa->spin;
      }
      if(strstr(left,"fspin")){ plosa->fspin = (float)atof(right);}
      if(strstr(left,"clock")){ plosa->clock = (bool)atoi(right);}
      if(strstr(left,"pointerdemo")){ plosa->pointerdemo = (bool)atoi(right);}
      if(strstr(left,"pd")){ plosa->pointerdemo = (bool)atoi(right);}
      if(strstr(left,"bg")){ plosa->conf_bg = (uint8_t)atoi(right);
        if(plosa->conf_bg >= MAX_BG){plosa->conf_bg=0;}
      }
      if(strstr(left,"deg")){
        flagdeg=(int16_t)atoi(right);
        theta=to_rad(flagdeg);
      }
      if(strstr(left,"blit")){
        int16_t d = (int16_t)atoi(right);
        Vec2 dp1 = {128,20};
        Vec2 dp0 = {102,20};
        draw_pointer_mode(dp0,dp1,d, themes[plosa->theme].colt.col_m,textures[plosa->texture],BLACK,PS_TEXTURE);
      }
      if(strstr(left,"hour")){ uint8_t tv = (uint8_t)atoi(right);if(tv>=0&&tv<24){plosa->dt.hour = tv;tc=true;}}
      if(strstr(left,"min")){  uint8_t tv = (uint8_t)atoi(right);if(tv>=0&&tv<60){plosa->dt.min  = tv;tc=true;}}
      if(strstr(left,"sec")){  uint8_t tv = (uint8_t)atoi(right);if(tv>=0&&tv<60){plosa->dt.sec  = tv;tc=true;}}

      if(strstr(left,"day")){  uint8_t tv = (uint8_t)atoi(right);if(tv>0&&tv<=last[plosa->dt.month]){plosa->dt.day = tv;tc=true;}}
      if(strstr(left,"mon")){  uint8_t tv = (uint8_t)atoi(right);if(tv>0&&tv<13){plosa->dt.month  = tv;tc=true;}}
      if(strstr(left,"year")){ uint16_t tv = (uint16_t)atoi(right);if(tv>=0){plosa->dt.year  = tv;tc=true;}}
      if(strstr(left,"dotw")||strstr(left,"WDAY")){ uint8_t tv = (uint8_t)atoi(right);if(tv>=0&&tv<7){plosa->dt.dotw  = tv;tc=true;}}
      if(tc){rtc_set_datetime(&plosa->dt);}
      return;
    }
    if(strstr(left,"cir0")){plosa->DYNAMIC_CIRCLES=false;printf("DYCI0\n");return;}
    if(strstr(left,"cir1")){plosa->DYNAMIC_CIRCLES=true;printf("DYCI1\n");return;}
    if(strstr(left,"save")){ dosave(); }
    if(strstr(left,"deg+")){ flagdeg++; }
    if(strstr(left,"deg-")){ flagdeg--; }

    if(strstr(left,"rota")){ plosa->gfxmode=GFX_ROTATE;}
    if(strstr(left,"roto")){ plosa->gfxmode=GFX_ROTOZOOM;}
    if(strstr(left,"norm")){ plosa->gfxmode=GFX_NORMAL;}
    if(strstr(left,"fps")){ showfps = (showfps)?false:true;}
    if(strstr(left,"stat")){
      if(plosa->theme>=THEMES){plosa->theme=0;}
      if(plosa->editpos>8){plosa->editpos=0;}
      if(plosa->dt.dotw>6){plosa->dt.dotw=0;}
      printf("\n- STATUS [%s]-\n\nmode[8]: %s\n",(rp2040_touch)?"WS_TOUCH_LCD_1.28":"WS_LCD_1.28",plosa->mode);
      printf("dt: %02d:%02d:%04d\r\n",plosa->dt.day,plosa->dt.month,plosa->dt.year);
      printf("dt: %s %02d:%02d:%02d\r\n",themes[plosa->theme].week[plosa->dt.dotw],plosa->dt.hour,plosa->dt.min,plosa->dt.sec);
      printf("editpos: %d\r\n",plosa->editpos);
      printf("theme: %d\r\n",plosa->theme);
      printf("BRIGHTNESS : %d\r\n",plosa->BRIGHTNESS);
      printf("is_sleeping: %s\r\n",(plosa->is_sleeping)?"1":"0");
      printf("sensors: %s\r\n",plosa->sensors?"1":"0");
      printf("gyrocross: %s\r\n",plosa->gyrocross?"1":"0");
      printf("bender: %s\r\n",plosa->bender?"1":"0");
      printf("SMOOTH_BACKGROUND: %s\r\n",plosa->SMOOTH_BACKGROUND?"1":"0");
      printf("INSOMNIA : %s\r\n",plosa->INSOMNIA?"1":"0");
      printf("DYNAMIC_CIRCLES: %s\r\n",plosa->DYNAMIC_CIRCLES?"1":"0");
      printf("DEEPSLEEP: %s\r\n",plosa->DEEPSLEEP?"1":"0");
      printf("HIGHPOINTER: %s\r\n",plosa->highpointer?"1":"0");
      printf("%s\r\n",crcstatus);
      printf("%s\r\n",flashstatus);
    }
    if(strstr(left,"i2c_scan")){   i2c_scan();}
    if(strstr(left,"boot")){reset_usb_boot(0,0);}
    if(strstr(left,"PFT4")){print_font_table4(fp_cn);}
    if(strstr(left,"narkose")){QMI8658_enableWakeOnMotion();}
    if(strstr(left,"qmiinit")){QMI8658_init();sleep_ms(2500);printf("init done\n");}
    if(strstr(left,"qmireset")){QMI8658_reset();}
    if(strstr(left,"qmireenable")){QMI8658_reenable();}
    if(strstr(left,"scrs")){printf("SCRS: %d [%d] {%d}\n",screensaver,theme_bg_dynamic_mode,plosa->is_sleeping);}
    if(strstr(left,"SNAPSHOT")){
      //printf("-----------------------> CUT HERE <---------------------\n\nuint8_t imagedata[138+  240*240*2] = {\n");
      if(b0==NULL){return;}
      uint8_t snaphead[] = {
      //  ID   ||  SIZE  |                                                                     | WIDTH   |          | HEIGHT |
      0x4d,0x42,0xc2,0x8a,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x8a,0x00,0x00,0x00,0x7c,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0xf0,
      0x00,0x00,0x00,0x01,0x00,0x10,0x00,0x03,0x00,0x00,0xc2,0x00,0x00,0x01,0x0b,0x12,0x00,0x00,0x0b,0x12,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x07,0xe0,0x00,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x47,0x42,
      0x73,0x52,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
      for(uint32_t i=0;i<138;i+=2){
        putchar_raw(snaphead[i+1]);
        putchar_raw(snaphead[i]);
      }
      for(uint32_t i=0;i<LCD_SZ;i+=2){
        putchar_raw(b0[i+1]);
        putchar_raw(b0[i]);
      }
      stdio_flush();
      //stdio_usb_init();
    }
}

void draw_init(){
  doi_config = DOImage_new(120-16, 16, 32,32 ,BLACK,config);
  doi_config_cn = DOImage_new(120-16, 16, 32,32 ,BLACK,config);
  //doi_config_cn = DOImage_new(18, 120-16, 32,32 ,BLACK,config);
}


void fx_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t c, uint16_t ps, uint16_t xo, uint16_t yo)
{
  if(plosa->DYNAMIC_CIRCLES){
    lcd_bez3circ(x,y,r,c,ps,xo,yo);
  }else{
    lcd_circle(x,y,r,c,ps,0);
  }
}

#define CIRCMENU_RADIUS 88

int16_t draw_circmenu(int16_t cdf, uint8_t num_items, const uint8_t** src_menuitems)
{
  int16_t cdeg;
  int16_t cdegc;
  int16_t maxcd = (int16_t)(DEGS/num_items);
  cdeg=chkdeg(cdf);
  cdf=chkdeg(cdf);
  for(uint16_t i=0;i<num_items;i++){
    Vec2 cv = gvdl(cdeg,CIRCMENU_RADIUS);
    lcd_blit(cv.x-16+LCD_W2,cv.y-16+LCD_H2,32,32,BLACK,src_menuitems[i]);
    cdeg=chkdeg(cdeg+maxcd);
  }

  int16_t d = (cdf+(DEGS/4))%maxcd;
  if(d!=0){
    if(d>(maxcd/2)){  return d-maxcd;
    }else{            return d;    }
  }
  return 0;

}

void draw_clock_hands(){
  uint8_t x1,y1,xt,yt;
  int xi,yi;
  uint8_t x0=120;
  uint8_t y0=120;
  float mindeg = 1024.0f/60.0f;
  Vec2 dp1 = texsize[plosa->texture];
  int16_t tu=(int16_t)plosa->dt.min*mindeg;
  Vec2 dp0 = {102,10};
  dp0 = psize_m[plosa->texture];
  if(plosa->pstyle==PS_NORMAL){    dp0 = vset(102,3);
  }else if(plosa->pstyle==PS_ALPHA){    dp0 = vset(102,4);  }
  draw_pointer_mode(dp0,dp1,tu,themes[plosa->theme].colt.col_m,textures[plosa->texture],BLACK,plosa->pstyle);
  tu=(int16_t)plosa->dt.hour;
  if(tu>=12){tu-=12;}
  tu=(int16_t)(tu*mindeg*5);
  tu+=(int16_t)((float)256/3/60*plosa->dt.min);
  dp0=vset(75, 20);
  dp0 = psize_h[plosa->texture];
  if(plosa->pstyle==PS_NORMAL){    dp0 = vset(65,6);
  }else if(plosa->pstyle==PS_ALPHA){    dp0 = vset(65,6);  }
  draw_pointer_mode(dp0,dp1,tu, themes[plosa->theme].colt.col_h,textures[plosa->texture],BLACK,plosa->pstyle);

  tu=(int16_t)(plosa->dt.sec*mindeg);
  //if(!analog_seconds){
    // 'jump' seconds
    int16_t seci = ((int16_t)plosa->dt.sec*mindeg);
    xi = (int8_t)(tcos[seci]*114);
    yi = (int8_t)(tsin[seci]*114);
    x1 = (uint8_t)x0+xi;
    y1 = (uint8_t)y0+yi;
    if(plosa->bender==true){
        int16_t xit=x1;
        int16_t yit=y1;
        xi = (int8_t)(tcos[seci]*106);
        yi = (int8_t)(tsin[seci]*106);
        x1 = (uint8_t)x0+xi;
        y1 = (uint8_t)y0+yi;
        Vec2 v1={x1,y1};
        lcd_alpha_on();
        lcd_alpha_off();
        if(plosa->pstyle == PS_ALPHA){          lcd_alpha_line_deg(v1, tu, 7, themes[plosa->theme].colt.col_s, 1);
        }else{          lcd_line_deg(v1, tu, 7, themes[plosa->theme].colt.col_s, 1);        }
    }else{
      dp0.x=114;
      dp0.y=1;
      draw_pointer_mode(dp0,dp0,tu, themes[plosa->theme].colt.col_s,textures[plosa->texture],BLACK,PS_NORMAL);
    }
    lcd_blit((int)(x0-8+tcos[seci]*100),(int)(y0-8+tsin[seci]*100),16,16,themes[plosa->theme].colt.alpha,themes[plosa->theme].flag16);
  //}
}

void draw_background()
{
  if(plosa->gfxmode==GFX_NORMAL||plosa->gfxmode==GFX_ROTATE){
    if(bg_size[plosa->conf_bg] > 240) {
      uint16_t* psrc = (uint16_t*)backgrounds[plosa->conf_bg];
      uint16_t* ptgt = (uint16_t*)b0;
      int i=0;
      for(int y=0;y<LCD_H;y++){
        for(int x=0;x<LCD_W;x++){              ptgt[x] = *psrc++;          }
        psrc+=bg_size[plosa->conf_bg]-LCD_W;
        ptgt+=LCD_W;
      }
    } else {
      mcpy(b0,backgrounds[plosa->conf_bg],LCD_SZ);
    }
  }
}

void draw_gfx(){
  if(!draw_gfx_enabled){return;}
  uint8_t x1,y1,xt,yt;
  uint8_t x0=120;
  uint8_t y0=120;
  if(!plosa->clock){return;}
    
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
      lcd_linev2(vc_s,vc_e, themes[plosa->theme].colt.col_cs, 1);
    }else{
      vc_s = gvdl((int16_t)i*mindeg,115);
      vc_s = vadd(vc_s,vO);
      lcd_linev2(vc_s,vc_e, themes[plosa->theme].colt.col_cs5, 1);
    }
  }
}

void make_buffers(){
  sprintf(b_hour,"%02d",plosa->dt.hour);
  sprintf(b_min,"%02d",plosa->dt.min);
  sprintf(b_sec,"%02d",plosa->dt.sec);

  sprintf(b_day,"%02d",plosa->dt.day);
  sprintf(b_month,"%02d",plosa->dt.month);
  sprintf(b_year,"%04d",plosa->dt.year);
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

    plosa->dummy=0;
    if(strstr((char*)plosa->mode,"SAVE")){
    		sprintf((char*)plosa->mode,"LOAD");
        plosa->save_crc = crc(&plosa->theme,LOSASIZE);
        flash_data();
        sprintf(flashstatus,"flash: saved\0");
    }else{
    		if(!force_no_load && !strstr((char*)plosa->mode,"LOAD")){
    			flash_data_load();
          sprintf(flashstatus,"flash: loaded\0");
    		}else{
          sprintf(flashstatus,"flash: normal\0");
        }
    }
    check_save_data(); // init, increase time by 1 second
    // reboot takes about 0.6 sec. + 0.1 sec "Rain-wait" -> wait 0.3sec
    sleep_ms(300);

    if(plosa->BRIGHTNESS < 10)plosa->BRIGHTNESS = 10;
    if(plosa->BRIGHTNESS > 100)plosa->BRIGHTNESS = 100;

    plosa->pointerdemo=false;
    config_deg = 90;
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
    draw_init();
    lcd_set_brightness(100);
    b0 = malloc(LCD_SZ);
    b1 = (uint32_t*)b0;
    if(b0==0){printf("b0==0!\n");}
    uint32_t o = 0;
    lcd_setimg((uint16_t*)b0);
    
    bool o_clk;
    bool o_dt;
    bool o_sw;
    //rtc_init();
    printf("init realtime clock\n");
    if(!(plosa->dt.year%4)){last[2]=29;}else{last[2]=28;}
    rtc_set_datetime(&plosa->dt);
    printf("init realtime clock done\n");
    fp_cn = malloc(512);

    // 16 bit characters (kyrillic)

    fp_cn[0] = 0;
    for(int i=0;i<7;i++){ fp_cn=strcat(fp_cn,themes[9].week[i]); }
    for(int i=0;i<7;i++){ fp_cn=strcat(fp_cn,themes[10].week[i]); }
    fp_cn=strcat(fp_cn,"č");
    lcd_makeutf8table(fp_cn);

    // 24 bit characters (asian)
    fp_cn[0] = 0;
    for(int i=0;i<7;i++){ fp_cn=strcat(fp_cn,themes[0].week[i]); }
    for(int i=0;i<7;i++){ fp_cn=strcat(fp_cn,themes[1].week[i]); }
    for(int i=0;i<7;i++){ fp_cn=strcat(fp_cn,themes[2].week[i]); }
    lcd_makeutf8table(fp_cn);
    //printf("PFT: '%s' [%d]\n",fp_cn,strlen(fp_cn));
    //print_font_table4(fp_cn);

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

    command("stat");
    sleep_ms(100);
    init_root();
    sleep_ms(100);
    wtest();
    sleep_ms(100);
    #define WL_SIZE 14
    CModeT cmt = CMT_None;
    bool set_time = false;
    uint8_t qmis = 0;
    int16_t last_tx, last_ty;
    int16_t diff_tx, diff_ty;
    W_spinner* spinrx = NULL;
    W_spinner* spinry = NULL;
    uint32_t flag_last_time = 0;

    while(true){
      if(flag){ //#FLAG
        //printf("FLAG!\n");
        screensaver=SCRSAV;
        if(plosa->is_sleeping){
          lcd_set_brightness(plosa->BRIGHTNESS);
          lcd_sleepoff();
        }
        plosa->is_sleeping=false;
        theme_bg_dynamic_mode = 0;
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
        if(flag_last_time && ((time_us_32()-flag_last_time)/MS) < 300){
          printf("FLT: %08x %d\n",flag_last_time,((time_us_32()-flag_last_time)/MS));
          //flag_last_time = 0;
          wf = NULL;
          fire = false;
        }
        if(wf){
          //printf("*");
          if(wf->t == wt_spinner){
            //printf("SP %08x %08x {%08x}\n",time_us_32(),wf->lt, (time_us_32()-wf->lt)/MS);
            if(!fire && (((time_us_32()-wf->lt)/MS)<100)){
              fire=true;
              if(wf->st == st_spinner_char_v){
                spinry = (W_spinner*)wf->d;
              }else if(wf->st == st_spinner_char_h){
                spinrx = (W_spinner*)wf->d;
              }
            }else{
              fire = false;
              last_tx = 0;
              last_ty = 0;
              diff_tx = 0;
              diff_ty = 0;
            }
            wf->lt = time_us_32();
          }else{
            if(!fire && (((time_us_32()-wf->lt)/MS)>=BUTD)){
              fire=true;wf->lt = time_us_32();
            }else{ fire = false; }
          }
        }
        if(fire){
          if(cmt == CMT_EditDOTW && wf == wsp_dotw){
            W_spinner* wsp = (W_spinner*)wsp_dotw->d;
            spinry = wsp;
            if(last_ty && ty > last_ty){
              wsp->fpos -= (int16_t)(ty - last_ty); printf("+%d",(ty - last_ty));
              diff_ty-= (ty - last_ty)>>2;
            }else if(last_ty && ty < last_ty){
              wsp->fpos += (int16_t)(last_ty - ty); printf("-%d",(last_ty - ty));
              diff_ty += (last_ty-ty)>>2;
            }
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.dotw,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(cmt == CMT_EditDOTW_CN && wf == wsp_dotw_cn){
            W_spinner* wsp = (W_spinner*)wsp_dotw_cn->d;
            spinrx = wsp;
            if(last_tx && tx > last_tx){
              wsp->fpos -= (int16_t)(tx - last_tx); printf("+%d",(tx - last_tx));
              diff_tx-= (tx - last_tx)>>2;
            }else if(last_tx && tx < last_tx){
              wsp->fpos += (int16_t)(last_tx - tx); printf("-%d",(last_tx - tx));
              diff_tx += (last_tx-tx)>>2;
            }
            wspinner_adjust_h(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.dotw,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(cmt == CMT_EditDay && wf == wsp_day){
            W_spinner* wsp = (W_spinner*)wsp_day->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>2;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>2;}
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.day,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(cmt == CMT_EditMonth && wf == wsp_month){
            W_spinner* wsp = (W_spinner*)wsp_month->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>2;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>2;}
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.month,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);

          }
          if(cmt == CMT_EditYear && wf == wsp_year){
            W_spinner* wsp = (W_spinner*)wsp_year->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>1;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>1;}
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.year,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }

          if(cmt == CMT_EditHour && wf == wsp_hour){
            W_spinner* wsp = (W_spinner*)wsp_hour->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>2;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>2;}
            wspinner_adjust(wsp);
            printf("wsp_hour %d [%d %d] {%d %d} %d / %d\n",plosa->dt.hour,tx,ty,last_tx,last_ty,wsp->pos,wsp->fpos);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.hour,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(cmt == CMT_EditMin && wf == wsp_min){
            W_spinner* wsp = (W_spinner*)wsp_min->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>1;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>1;}
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.min,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(cmt == CMT_EditSec && wf == wsp_sec){
            W_spinner* wsp = (W_spinner*)wsp_sec->d;
            if(last_ty && ty > last_ty){       wsp->fpos -= (int16_t)(ty - last_ty); diff_ty-= (ty - last_ty)>>1;
            }else if(last_ty && ty < last_ty){   wsp->fpos += (int16_t)(last_ty - ty); diff_ty += (last_ty-ty)>>1;}
            wspinner_adjust(wsp);
            last_tx = tx; last_ty = ty;
            printf("wsp_dotw %d [%d %d] (%d %d) {%d %d} %d / %d\n",plosa->dt.sec,tx,ty,diff_tx,diff_ty,last_tx,last_ty,wsp->pos,wsp->fpos);
          }
          if(wf->t == wt_text){
            W_text* wt = (W_text*)wf->d;
            //printf("wf = 0x%08x %d %d [%d W_text %d] '%s'\n",wf,wf->x,wf->y,wf->t,wf->ws,wt->text);
            reblink2(wblinker,wblinkerg,wf);
            if(cmt == CMT_None){ cmt = CMT_Select; }

            if(wf == w_day && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              if(plosa->dt.day > last[plosa->dt.month]){
                plosa->dt.day = last[plosa->dt.month];
                sprintf(b_day,"%02d",plosa->dt.day);
                rtc_set_datetime(&plosa->dt);
              }
              wspinner_set(wsp_day,plosa->dt.day-1);
              wspinner_set_max(wsp_day,last[plosa->dt.month]);
              wshow(w_day);
              wshow(wsp_day);
              
              cmt = CMT_EditDay;
            }else if(wf == w_day && wblinkerg->ws == ws_hidden){
              whide(wsp_day);
              wbez2_reset(w_move);
              cmt = CMT_EditDayDone;
              if(plosa->dt.day > last[plosa->dt.month]){
                plosa->dt.day = last[plosa->dt.month];
                sprintf(b_day,"%02d",plosa->dt.day);
                rtc_set_datetime(&plosa->dt);
              }
            }
            if(wf == w_month && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              wspinner_set(wsp_month,plosa->dt.month-1);
              wshow(w_month);
              wshow(wsp_month);
              cmt = CMT_EditMonth;
            }else if(wf == w_month && wblinkerg->ws == ws_hidden){
              whide(wsp_month);
              wbez2_reset(w_move);
              cmt = CMT_EditMonthDone;
              if(plosa->dt.day > last[plosa->dt.month]){
                plosa->dt.day = last[plosa->dt.month];
                sprintf(b_day,"%02d",plosa->dt.day);
                rtc_set_datetime(&plosa->dt);
              }
            }
            if(wf == w_year && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              wspinner_set(wsp_year,plosa->dt.year-2000);
              wshow(w_year);
              wshow(wsp_year);
              cmt = CMT_EditYear;
            }else if(wf == w_year && wblinkerg->ws == ws_hidden){
              whide(wsp_year);
              wbez2_reset(w_move);
              cmt = CMT_EditYearDone;
            }
            if(wf == w_hour && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              wspinner_set(wsp_hour,plosa->dt.hour);

              wshow(w_hour);
              wshow(wsp_hour);
              cmt = CMT_EditHour;
            }else if(wf == w_hour && wblinkerg->ws == ws_hidden){
              whide(wsp_hour);
              wbez2_reset(w_move);
              cmt = CMT_EditHourDone;
            }
            if(wf == w_min && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              wshow(w_min);
              wspinner_set(wsp_min,plosa->dt.min);
              wshow(wsp_min);
              cmt = CMT_EditMin;
            }else if(wf == w_min && wblinkerg->ws == ws_hidden){
              whide(wsp_min);
              wbez2_reset(w_move);
              cmt = CMT_EditMinDone;
            }
            if(wf == w_sec && wblinkerg->ws == ws_shown){
              whidem(wl,WL_SIZE);
              wshow(w_sec);
              wspinner_set(wsp_sec,plosa->dt.sec);
              wshow(wsp_sec);
              cmt = CMT_EditSec;
            }else if(wf == w_sec && wblinkerg->ws == ws_hidden){
              whide(wsp_sec);
              wbez2_reset(w_move);
              cmt = CMT_EditSecDone;
            }

          }else if(wf->t == wt_textr){
            W_textr* wt = (W_textr*)wf->d;
            printf("wf = 0x%08x %d %d [%d W_textr %d] '%s'\n",wf,wf->x,wf->y,wf->t,wf->ws,wt->get_text());
            reblink2(wblinker,wblinkerg,wf);
            if(cmt == CMT_None){
              cmt = CMT_Select;
            }
            if(wf == w_dotw && wblinkerg->ws == ws_shown){
              wspinner_set(wsp_dotw,plosa->dt.dotw);
              whidem(wl,WL_SIZE);
              wshow(wb_dotw);
              wshow(wsp_dotw);
              cmt = CMT_EditDOTW;
            }else if(wf == w_dotw && wblinkerg->ws == ws_hidden){
              wshowm(wl,WL_SIZE);
              whide(wsp_dotw);
              cmt = CMT_EditDOTWDone;
            }

            if(wf == w_dotw_cn && wblinkerg->ws == ws_shown){
              wspinner_set(wsp_dotw_cn,plosa->dt.dotw);
              whidem(wl,WL_SIZE);
              wshow(wb_dotw);
              wshow(wsp_dotw_cn);
              cmt = CMT_EditDOTW_CN;
            }else if(wf == w_dotw_cn && wblinkerg->ws == ws_hidden){
              wshowm(wl,WL_SIZE);
              whide(wsp_dotw_cn);
              cmt = CMT_EditDOTWDone;
            }

          }else if(wf->t == wt_imager){
            reblink2(wblinker,wblinkerg,wf);
            if(wf == img_center && wblinkerg->ws == ws_shown){
              printf("img_center\n");
              whidem(wl,WL_SIZE);
              wshow(img_center);
              wshow(cim_flags);
            }else if(wf == img_center && wblinkerg->ws == ws_hidden){
              wshowm(wl,WL_SIZE);
              //whide(cim_config);
              whide(cim_flags);
            }

          }else if(wf->t == wt_image){
            reblink2(wblinker,wblinkerg,wf);
            if(wf == img_config && wblinkerg->ws == ws_shown){
              printf("img_config\n");
              blinker_off(wblinkerg);
              wshow(cim_config);
              wshow(wn_drawclockhands);
              whidem(wl,WL_SIZE);
              flag_last_time = time_us_32();
            //}else if(wf == img_config && wblinkerg->ws == ws_hidden){
            //  wshowm(wl,WL_SIZE);
            //  whide(cim_config);
            //  whide(wn_drawclockhands);
            }else if(wf->p == cim_flags && wblinkerg->ws == ws_shown){ // flag selected
              bool flag_found = false;
              uint8_t fi = 0;
              W_box* wb = (W_box*)cim_flags->d;
              for(;fi<wb->nc;fi++){
                if(wb->ch[fi] == wf){
                  flag_found = true;
                  break;
                }
              }
              if(flag_found){
                plosa->theme = fi;
                if(plosa->theme<3){ // china/jp/ko alt fonts
                  wshow(w_dotw_cn);
                  whide(w_dotw);
                }else{
                  whide(w_dotw_cn);
                  wshow(w_dotw);
                }
                flag_last_time = time_us_32();
                whide(cim_flags);
                wshowm(wl,WL_SIZE);
                whide(wblinkerg);
              }
              printf("cim_flags(%d) [%d] %08x\n",plosa->theme,flag_found?fi:-1,flag_last_time);
            }
          }else if(wf->t == wt_imagef){
            blink_once(wblinker_once,wf);
            wshow(wblinker_once);
            W_imagef* wif = (W_imagef*)wf->d;
            if(wif){
              //*wif->index += 1;
              //if(*wif->index >= wif->max_index)*wif->index = 0;
              if(wif->image_function) wif->image_function();
              flag_last_time = time_us_32();
              last_blink = time_us_32();
              printf("imagef w:%08x %08x %08x f:%8x\n",wf,flag_last_time, last_blink, wif->image_function);
            }

          }else{  printf("wf = NONE FOUND\n"); }
          fire = false;
        }
        flag = 0;
      } // if(flag) END

      // SLEEP/DEEPSLEEP
      if(plosa->is_sleeping){
        sleep_ms(sleep_frame);
        if(plosa->DEEPSLEEP){
          deepsleep=true;
          QMI8658_disableWakeOnMotion();
          sleep_ms(10);
          QMI8658_reenable();
          screensaver=SCRSAV;
          plosa->is_sleeping=false;
          theme_bg_dynamic_mode = 0;
          lcd_set_brightness(plosa->BRIGHTNESS);
          lcd_sleepoff();
        }
        continue;
      }

      uint32_t t = time_us_32();
      if(button0_time){ button0_dif = t-button0_time; }
      if(button1_time){ button1_dif = t-button1_time; }
      if((button0_dif||button1_dif)&&(plosa->editpos==EPOS_CENTER)){ // only from central position (flag)
        //printf("%d %d %d\n",t,button0_time,button0_dif);
        if(button0_dif>=US){ printf("REBOOT: [%d->%d] s\n", button0_dif/MS, REBOOT/MS); }
        if(button1_dif>=US){ printf("REBOOT: [%d->%d] s\n", button1_dif/MS, REBOOT/MS); }
        if(button0_dif>=REBOOT || button1_dif >= REBOOT){
          printf("SAVING...\n");
          dosave();
        }
      }
      //for(int i=0;i<LCD_SZ;i++){b0[i]=0x00;}  //clear buffer
      for(int i=0;i<LCD_SZ/4;i++){b1[i]=0x00;}  //clear buffer faster
      
      // draw-bg

      if(cmode!=CM_Editpos || plosa->editpos==EPOS_CENTER ){
        rtc_get_datetime(&plosa->dt);
      }

      make_buffers();
      wdraw(&wroot);
      lcd_display(b0);
      // show fps
      /*
      */
      if(showfps){
        if(plosa->dt.sec != fps_sec){
          printf("FPS: %d\n",fps);
          fps = 0;
          fps_sec = plosa->dt.sec;
        }
        ++fps;
      }
        
      // show fps

      plosa->save_crc=crc(&plosa->theme,LOSASIZE);
    }
    return 0;
}

void blinker_off(W* w){
    if(w->ws == ws_shown){ w->ws = ws_hidden; }
}

void brepos(W* w, W* wt){
  if( ((wt->w>>1) >= wt->h) || ((wt->h>>1) >= wt->w) ){ // rect
    w->x = wt->x-6;
    w->y = wt->y-6;
    w->w = wt->w+9;
    w->h = wt->h+9;
    w->t = wt_blinker_rect;
  }else{
    w->w = (wt->w>>1)+6;
    w->h = (wt->h>>1)+6;
    if( (wt->h>>1) > wt->w ){
      w->w = w->h;
    }
    w->x = wt->x+(wt->w>>1);
    w->y = wt->y+(wt->h>>1);
    w->t = wt_blinker_circle;
  }

}

void blink_once(W* w, W* wt){
  w->ws = ws_shown;
  wblinker_ref = wt;
  w->st = st_blink_once;
  brepos(w,wt);
}

void reblink(W* w, W* wt){ // w = wblinker, wt = wtarget
  if(wblinker_ref == wt){
    printf("SAME!\n");
    w->ws = ws_hidden;
    wblinker_ref = NULL;
  }else{
    w->ws = ws_shown;
    wblinker_ref = wt;
    brepos(w,wt);
  }
}

#define W1C_MAX 2
uint16_t w1c = 0;
void reblink2(W* w0, W* w1, W* wt){
  if(!wblinker_ref || wblinker_ref != wt){
    //++w1c;
    //if(w1c>=W1C_MAX){
      //w1c = 0;
      w0->ws = ws_shown;
      w1->ws = ws_hidden;
      wblinker_ref = wt;
      brepos(w0,wt);
    //}
  }else if(w0->ws == ws_shown){
    w0->ws = ws_hidden;
    w1->ws = ws_shown;
    wblinker_ref = wt;
    brepos(w1,wt);
  }else if(w1->ws == ws_shown){
    w1->ws = ws_hidden;
    wblinker_ref = NULL;
  }
}

char* get_dotw_all(uint8_t index){
  if(index>=7){
    printf("*Iai:%d\n",index);
    index-=7;
  }
  return themes[plosa->theme].week[index];
}

char* get_dotw(){
  return themes[plosa->theme].week[plosa->dt.dotw];
}

char* get_dotw_cn(){
  return themes[plosa->theme].week[plosa->dt.dotw];
}

char* get_dotw_cn_i(uint8_t i){
  if(i>=7){
    printf("*Icni:%d\n",i);
    i-=7;
  }
  return themes[plosa->theme].week[i];
}

uint16_t* get_theme_image(){
  return (uint16_t*)themes[plosa->theme].flag32;
}

void doit_bg(){
  if(++plosa->conf_bg >= MAX_BG){ plosa->conf_bg = 0; }
  printf("doit_bg %d\n",plosa->conf_bg);
}

void doit_rotate(){
  if(plosa->rota){
    plosa->spin+=1;
    plosa->fspin+=0.005f;
    //if(plosa->fspin >= MAX_FSPIN){
    if(plosa->fspin >= 0.035f){
      plosa->fspin = 0.0f;
      plosa->spin = 0;
      plosa->gfxmode = GFX_NORMAL;
      plosa->rota = false;
    }
  }else{
    plosa->rota = true;
    plosa->gfxmode = GFX_ROTATE;
    if(plosa->fspin==0){ plosa->fspin = 0.005f; plosa->spin=1;}
  }
  printf("doit_rota: [%d] %0.6f\n",plosa->rota?1:0,plosa->fspin);
}
void doit_clockhand_style(){
  if(plosa->pstyle<PS_TEXTURE){ // rotate normal/alpha/texture
    plosa->pstyle++;
  }else{
    plosa->pstyle=PS_NORMAL;
  }
}

void doit_rotozoom(){
  plosa->rotoz = (bool)!plosa->rotoz;
  if(plosa->rotoz){
    plosa->gfxmode=GFX_ROTOZOOM;
  }else{
    plosa->gfxmode=GFX_NORMAL;
  }
}
void doit_wand(){
  plosa->texture++;
  if(plosa->texture==TEXTURES){ plosa->texture=0; }
}
void doit_bender(){
  uint8_t b = plosa->bender;
  if(plosa->bender){    plosa->bender = false;
  }else{                plosa->bender = true; }
  //printf("doit_bender [%d / %d]\n", plosa->bender,b);
}
void doit_save(){
  dosave();
}

void doit_exit(){
  wshowm(wl,WL_SIZE);
  whide(cim_config);
  whide(wn_drawclockhands);
}


void wtest(){

  uint8_t config_maxdex[8] = {0,MAX_BG,1,MAX_SPIN,0,3,MAX_TEXTURE,1};
  uint8_t* config_dex[8] = {0,(uint8_t*)&plosa->conf_bg,(uint8_t*)&plosa->rotoz,&plosa->spin,0,(uint8_t*)&plosa->pstyle,&plosa->texture,(uint8_t*)&plosa->bender};

  config_functions[0] = (void*)doit_exit;
  config_functions[1] = (void*)doit_bg;
  config_functions[2] = (void*)doit_rotozoom;
  config_functions[3] = (void*)doit_rotate;
  config_functions[4] = (void*)doit_save;
  config_functions[5] = (void*)doit_clockhand_style;
  config_functions[6] = (void*)doit_wand;
  config_functions[7] = (void*)doit_bender;

  W* w;
  wn_background = wadd_none(&wroot,draw_background);
  wn_drawclockhands = wadd_none(&wroot,draw_clock_hands);
  img_center = wadd_imager(&wroot,get_theme_image,120-16,120-16,32,32);
  wblinker = wadd_blinker(&wroot,120,120,24,24,5,RED,BLUE,5,false);
  whide(wblinker);

  //cim_flags = wcim_make(THEMES,0);
  //wb_time = wadd_box(&wroot,TIME_X,TIME_Y,7*f40[0]->w,f40[0]->h);
  int16_t maxcd = (int16_t)(DEGS/THEMES);
  int16_t cdeg = 0;
  cim_flags = wadd_box(&wroot,0,0,240,240);
  for(uint16_t i=0;i<THEMES;i++){
    Vec2 cv = gvdl(cdeg,CIRCMENU_RADIUS);
    cdeg=chkdeg(cdeg+maxcd);
    wadd_image(cim_flags,(uint16_t*)themes[i].flag32,cv.x-16+LCD_W2,cv.y-16+LCD_H2,32,32);
  }
  whide(cim_flags);

  cim_config = wadd_box(&wroot,0,0,240,240);
  resize(cim_config,MAX_CONF);
  maxcd = (int16_t)(DEGS/MAX_CONF);
  Vec2 cv;
  for(uint16_t i=0;i<MAX_CONF;i++){
    cv = gvdl(cdeg,CIRCMENU_RADIUS);
    cdeg=chkdeg(cdeg+maxcd);
    W* w = wadd_imagef(cim_config,cv.x-16+LCD_W2,cv.y-16+LCD_H2,32,32,(uint16_t*)config_images[i],config_functions[i],config_dex[i],config_maxdex[i]);
    //printf("CIMC: %d %08x [%08x]\n",i,w,config_functions[i]);
  }
  W_box* wb = (W_box*)cim_config->d;
  printf("wb %d\n",wb->nc);
  whide(cim_config);

  uint16_t i=0;
  wl[i++] = wn_content;
  wl[i++] = w_hour;
  wl[i++] = w_min;
  wl[i++] = w_sec;
  wl[i++] = w_timed0;
  wl[i++] = w_timed1;
  wl[i++] = w_day;
  wl[i++] = w_month;
  wl[i++] = w_year;
  wl[i++] = w_dated0;
  wl[i++] = w_dated1;
  wl[i++] = img_center;
  wl[i++] = img_config;
  wl[i++] = wb_dotw;

  if(plosa->theme<3){  wshow(w_dotw_cn);whide(w_dotw); // china
  }else{ whide(w_dotw_cn); wshow(w_dotw); }

}
