#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MMLClassicMotion { int16_t vertical_velocity; int16_t turn_accum; uint16_t yaw; } MMLClassicMotion;
void mml_jump_enter(MMLClassicMotion *m, int high_variant_bit);
void mml_airborne_gravity_step(MMLClassicMotion *m);
void mml_turn_reset(MMLClassicMotion *m);
void mml_turn_ramp(MMLClassicMotion *m, int direction, int16_t step, int16_t cap);
#ifdef __cplusplus
}
#endif
