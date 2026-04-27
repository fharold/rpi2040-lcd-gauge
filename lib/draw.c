#include "draw.h"


//Vec2 vO = {120,120};
//Vec2 v0 = {0,0};
Vec2 vO = {LCD_W>>1,LCD_H>>1};
Vec2 v32 = {32,32};


DOImage* DOImage_newv2(  Vec2 vpos,  Vec2 vs,  uint16_t alpha,  const uint8_t* data){
  DOImage* doi = malloc(sizeof(DOImage));
  doi->vpos=vpos;
  doi->vsize=vs;
  doi->alpha=alpha;
  doi->data =data ;
  return doi;
}
DOImage* DOImage_new(int16_t x, int16_t y, int16_t x1, int16_t y1,  uint16_t alpha,  const uint8_t* data){
  printf("sizeof DOImage: %d\n",sizeof(DOImage));
  DOImage* doi = malloc(sizeof(DOImage));
  doi->vpos.x=x;
  doi->vpos.y=y;
  doi->vsize.x=x1;
  doi->vsize.y=y1;
  doi->alpha=alpha;
  doi->data =data ;
  return doi;
}

void draw_doimage(DOImage* doi){
  lcd_blit((uint8_t)doi->vpos.x,(uint8_t)doi->vpos.y,
           (uint8_t)doi->vsize.x,(uint8_t)doi->vsize.y,doi->alpha,doi->data);
}
void draw_dotexture(DOTexture* doi, uint16_t deg){
  lcd_blit_deg(doi->vpos,doi->vs,doi->vts,deg,doi->data,doi->alpha,false);
}
void draw_pointer_mode(Vec2 vs, int16_t tu, uint16_t color){
  lcd_line_deg(vO, tu, vs.x, color, vs.y);
}
