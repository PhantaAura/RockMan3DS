
#include "mml_retail_motion.h"
static int16_t clamp16(int32_t v,int16_t c){if(v>c)return c;if(v<-c)return (int16_t)-c;return (int16_t)v;}
void mml_turn_update(MMLTurnState *s,int d,int16_t step,int16_t cap){
 if(!s||!d)return;
 if((d>0&&s->turn_velocity<0)||(d<0&&s->turn_velocity>0)){s->turn_velocity=0;}
 else {s->turn_increment=(int16_t)(d>0?step:-step);s->turn_velocity=clamp16((int32_t)s->turn_velocity+s->turn_increment,cap);}
 s->heading12=(uint16_t)(((int32_t)s->heading12+s->turn_velocity)&0x0FFF);
}
void mml_signed_accum_update(MMLSignedAccum *s,int d,int32_t step,int32_t acap,int32_t vcap){
 if(!s) return;
 if(d==0){s->accel=0;return;}
 if((d>0&&s->accel<0)||(d<0&&s->accel>0))s->accel=0;
 else {s->accel+=d>0?step:-step;if(s->accel>acap)s->accel=acap;if(s->accel<-acap)s->accel=-acap;}
 s->value+=s->accel;if(s->value>vcap)s->value=vcap;if(s->value<-vcap)s->value=-vcap;
}
uint16_t mml_motion_projection_heading(uint16_t h,uint16_t o){return (uint16_t)((h-o)&0x0FFF);}
void mml_heading0_project(int32_t f,int32_t v,int32_t l,int32_t *dx,int32_t *dy,int32_t *dz){if(dx)*dx=l;if(dy)*dy=v;if(dz)*dz=-f;}
