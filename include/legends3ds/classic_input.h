#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MMLControlProfile { uint16_t raw[16]; uint16_t rotated[4][4]; } MMLControlProfile;
typedef struct MMLMotionAccums { int16_t axis44; int16_t axis48; } MMLMotionAccums;
void mml_build_rotated_masks(MMLControlProfile *out, const uint16_t raw[16]);
void mml_update_direction_accums(MMLMotionAccums *m, const MMLControlProfile *p, unsigned orientation, uint16_t pressed);
uint16_t mml_circlepad_to_dpad(float x, float y, float deadzone);
#ifdef __cplusplus
}
#endif
