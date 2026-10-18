
#ifndef MML_RETAIL_MOTION_H
#define MML_RETAIL_MOTION_H
#include <stdint.h>
typedef struct {uint16_t heading12;int16_t turn_velocity;int16_t turn_increment;} MMLTurnState;
typedef struct {int32_t value;int32_t accel;} MMLSignedAccum;
void mml_turn_update(MMLTurnState *s,int direction,int16_t accel_step,int16_t max_turn);
void mml_signed_accum_update(MMLSignedAccum *s,int direction,int32_t accel_step,int32_t accel_cap,int32_t value_cap);
uint16_t mml_motion_projection_heading(uint16_t heading12,uint16_t offset12);
void mml_heading0_project(int32_t forward,int32_t vertical,int32_t lateral,int32_t *dx,int32_t *dy,int32_t *dz);
#endif
