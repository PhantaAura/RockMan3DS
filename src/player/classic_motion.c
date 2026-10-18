#include "legends3ds/classic_motion.h"
static int16_t clamp16i(int v,int lo,int hi){if(v<lo)return(int16_t)lo;if(v>hi)return(int16_t)hi;return(int16_t)v;}
void mml_jump_enter(MMLClassicMotion *m,int high_variant_bit){m->vertical_velocity=(int16_t)(high_variant_bit?-0x320:-0x1C0);}
void mml_airborne_gravity_step(MMLClassicMotion *m){m->vertical_velocity=(int16_t)(m->vertical_velocity+0x30);}
void mml_turn_reset(MMLClassicMotion *m){m->turn_accum=0;}
void mml_turn_ramp(MMLClassicMotion *m,int direction,int16_t step,int16_t cap){
    if(direction==0){m->turn_accum=0;return;}
    if(step<0) step=(int16_t)-step;
    if(cap<0) cap=(int16_t)-cap;
    // ROCK_NEO 0x80042154 does a one-frame zero crossing when the requested turn direction
    // reverses: a negative accumulator becomes exactly 0 before positive acceleration begins,
    // and vice versa. It does not add/subtract the step on that reversal frame.
    if(direction>0){
        if(m->turn_accum<0) m->turn_accum=0;
        else m->turn_accum=clamp16i((int)m->turn_accum+step,0,cap);
    }else{
        if(m->turn_accum>0) m->turn_accum=0;
        else m->turn_accum=clamp16i((int)m->turn_accum-step,-cap,0);
    }
    m->yaw=(uint16_t)((m->yaw+(uint16_t)m->turn_accum)&0x0FFFu);
}
