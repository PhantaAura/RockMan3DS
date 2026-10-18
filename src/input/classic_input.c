#include "legends3ds/classic_input.h"
#include <math.h>
#include <string.h>
enum { PSX_UP=0x0010, PSX_RIGHT=0x0020, PSX_DOWN=0x0040, PSX_LEFT=0x0080 };
static int16_t clamp16(int v, int lo, int hi) { if(v<lo)return(int16_t)lo; if(v>hi)return(int16_t)hi; return(int16_t)v; }
void mml_build_rotated_masks(MMLControlProfile *out, const uint16_t raw[16]) {
    memcpy(out->raw, raw, sizeof(out->raw));
    const uint16_t up=raw[0], down=raw[1], a=raw[4], b=raw[5];
    out->rotated[0][0]=up|down; out->rotated[0][1]=up;   out->rotated[0][2]=a|b;     out->rotated[0][3]=b;
    out->rotated[1][0]=a|b;     out->rotated[1][1]=a;    out->rotated[1][2]=up|down; out->rotated[1][3]=up;
    out->rotated[2][0]=up|down; out->rotated[2][1]=down; out->rotated[2][2]=a|b;     out->rotated[2][3]=a;
    out->rotated[3][0]=a|b;     out->rotated[3][1]=b;    out->rotated[3][2]=up|down; out->rotated[3][3]=down;
}
void mml_update_direction_accums(MMLMotionAccums *m, const MMLControlProfile *p, unsigned orientation, uint16_t pressed) {
    orientation &= 3u;
    const uint16_t pair44=p->rotated[orientation][0], pos44=p->rotated[orientation][1];
    const uint16_t pair48=p->rotated[orientation][2], neg48=p->rotated[orientation][3];
    uint16_t hit=(uint16_t)(pressed & pair44);
    if (hit && hit != pair44) m->axis44=clamp16(m->axis44 + ((pressed & pos44)?0x28:-0x28), -0x280, 0x280);
    hit=(uint16_t)(pressed & pair48);
    if (hit == pair48 && hit != 0) m->axis48=0;
    else if (hit) m->axis48=clamp16(m->axis48 + ((pressed & neg48)?-0x20:0x20), -0x100, 0x100);
}
uint16_t mml_circlepad_to_dpad(float x, float y, float deadzone) {
    uint16_t r=0; if(deadzone<0)deadzone=0; if(deadzone>0.95f)deadzone=0.95f;
    if (fabsf(x) < deadzone) x = 0;
    if (fabsf(y) < deadzone) y = 0;
    if(x<0)r|=PSX_LEFT; else if(x>0)r|=PSX_RIGHT;
    if(y<0)r|=PSX_DOWN; else if(y>0)r|=PSX_UP;
    return r;
}
