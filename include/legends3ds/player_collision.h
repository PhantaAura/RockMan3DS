#pragma once
#include "legends3ds/player_runtime.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable collision-response bridge recovered from the exact USA
 * ROCK_NEO.EXE.  Names deliberately describe byte/bit behavior instead of
 * assigning unproved floor/wall gameplay semantics.
 */

enum {
    L3_CONTACT_RESULT_AXIS_X_MASK = 0x0300,
    L3_CONTACT_RESULT_AXIS_Z_MASK = 0x0C00,
    L3_CONTACT_RESULT_AXIS_Y_MASK = 0x3000,
};

typedef struct L3RetailContactScratch {
    int16_t x;             /* retail scratch 0x1F8000C0 */
    int16_t y;             /* retail scratch 0x1F8000C2 */
    int16_t z;             /* retail scratch 0x1F8000C4 */
    uint8_t contact_state; /* retail scratch 0x1F8000F6 */
} L3RetailContactScratch;

/* Exact USA helper at 0x8002FE74.  The returned grouped class bits are used by
 * the player collision wrapper to choose its no-hit rollback path. */
uint8_t l3_obj_group_bits_8002fe74(uint8_t object_class);

/* Preserve the current short/fixed player pose into the exact rollback mirrors
 * consumed by the 0x8004B414 response wrapper (+0x2C..+0x40). */
void l3_pl00_snapshot_collision_pose(L3PlayerCompatWork *work);

/*
 * Exact work-mutation half of USA player collision wrapper 0x8004B414 after
 * its geometry solver (0x8004B6AC) has returned.
 *
 * contact_result is the solver's signed return value.  Positive values carry
 * the original axis snap masks.  Zero/negative values take the original
 * rollback branches.  The caller supplies the four scratch values written by
 * the geometry solver; no surface meaning is guessed here.
 */
int32_t l3_pl00_apply_contact_result_8004b414(L3PlayerCompatWork *work,
                                               int32_t contact_result,
                                               const L3RetailContactScratch *scratch);

/* Exact work mutation at USA 0x8003DAC0..0x8003DB08 for the normal jump launch
 * gate.  Returns 1 only when work+0xA7 is -1 and the launch mutation executes.
 * Animation/sound side effects are intentionally outside this helper. */
int l3_pl00_jump_launch_8003dac0(L3PlayerCompatWork *work);

/* Exact velocity branch at USA 0x8003DB50..0x8003DB78.  When held input
 * (+0x11C) intersects the jump-hold mask (+0x138), +0x46 gains 0x30.
 * Returns 0 when the hold mask is not active, 1 while the resulting velocity
 * is still negative, and 2 when the increment reaches/passes zero. */
int l3_pl00_jump_hold_gravity_8003db50(L3PlayerCompatWork *work);

/* Exact no-contact return-code tail of USA geometry solver 0x8004B6AC,
 * instructions 0x8004BC2C..0x8004BD90. The input and scratch coordinates use
 * the game's signed-16 world mirrors. Low bits 1..6 encode the dominant moved
 * axis/direction with retail tie ordering (X loses ties to Z; horizontal loses
 * ties to Y). High bits retain the exact per-axis movement-sign masks. */
int32_t l3_collision_no_contact_result_8004bc2c(int16_t input_x,
                                                 int16_t input_y,
                                                 int16_t input_z,
                                                 int16_t scratch_x,
                                                 int16_t scratch_y,
                                                 int16_t scratch_z);

#ifdef __cplusplus
}
#endif
