#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Byte-compatible scratch mirror for the original USA Player_work object.
 * Unknown offsets stay unnamed on purpose. Native gameplay state can migrate
 * away from this mirror once each field has been proven.
 */
enum { L3_PLAYER_COMPAT_WORK_BYTES = 0x5F4 };
typedef struct L3PlayerCompatWork {
    uint8_t bytes[L3_PLAYER_COMPAT_WORK_BYTES];
} L3PlayerCompatWork;

/* Exact translations of USA ROCK_NEO.EXE 0x800423A0 / 0x800423C8. */
void l3_pl00_shot_enable_off(L3PlayerCompatWork *work);
void l3_pl00_shot_enable_on(L3PlayerCompatWork *work);
int l3_pl00_shot_is_enabled(const L3PlayerCompatWork *work);

/* Exact player-work mutation at USA ROCK_NEO.EXE 0x80040E9C.
 * action 0..3 commits without a helper. Actions 4..7 require the byte returned
 * by 0x800423DC (4/5) or 0x80042458 (6/7); pass helper_known=0 to hold without
 * mutation until that independent owner is available. Returns 1 on commit,
 * 0 for a missing helper receipt or unsupported action.
 */
int l3_pl00_fire_state_80040e9c(L3PlayerCompatWork *work,
                                uint16_t action,
                                int helper_known,
                                uint8_t helper_request);

/* Exact animation/request-state mutation at USA ROCK_NEO.EXE 0x80041DDC.
 * Names stay neutral until the downstream animation dispatcher is fully traced.
 * request is stored at +0xAC and substate at +0xA4. force!=0 seeds +0xAD=0xFF
 * before the original request-transition logic.
 */
void l3_pl00_anim_request_80041ddc(L3PlayerCompatWork *work,
                                   uint8_t request,
                                   uint8_t substate,
                                   uint8_t force);

/*
 * Exact accumulator branch at USA ROCK_NEO.EXE 0x80041F54.
 * It updates signed work halfword +0x10C by +/-step, clamps it to +/-cap,
 * integrates it into signed work halfword +0x108, then clamps +0x108 to
 * [-0x200,+0x200]. The positive branch is selected when held_mask & mask != 0.
 */
void l3_pl00_axis_step_80041f54(L3PlayerCompatWork *work,
                                uint16_t mask,
                                int16_t step,
                                int16_t cap);

/*
 * Exact turn accumulator at USA ROCK_NEO.EXE 0x80042154.
 * Uses held input at work+0x11C, signed accumulator +0xB4 and yaw +0x56.
 * Reversal has the original one-frame zero crossing. Return value is the
 * original branch code: 1 when held&mask is non-zero, otherwise 2.
 */
int l3_pl00_turn_step_80042154(L3PlayerCompatWork *work,
                               uint16_t mask,
                               int16_t step,
                               int16_t cap);


/*
 * Exact input-pair wrapper at USA 0x80042044. Masks are read from work
 * +0x128/+0x12A. No input or both inputs clears turn accumulator +0xB4.
 * A single input delegates to 0x80042154 using the +0x12A mask.
 */
int l3_pl00_turn_pair_80042044(L3PlayerCompatWork *work,
                               int16_t step,
                               int16_t cap);

/*
 * Exact directional-axis wrapper at USA 0x80041EF4. Masks come from
 * work +0x124/+0x126; no directional input clears +0x10C. The original
 * global at 0x800C1B65 chooses which half of the pair is treated positive,
 * so the portable bridge takes that value explicitly instead of hiding a global.
 */
void l3_pl00_axis_pair_80041ef4(L3PlayerCompatWork *work,
                                int16_t step,
                                int16_t cap,
                                int alternate_orientation);


/* Exact work mutation portion of USA 0x8003D124. Direction is the original
 * work+0x112 value in the proved 0..3 domain. It selects one of the four
 * derived directional mask pairs at +0x148..+0x162, writes strafe +0x48,
 * and conditionally writes turn accumulator +0xB4. The original routine then
 * calls 0x80041E90 with forward=work+0x44, vertical=0, strafe=work+0x48;
 * native code performs that final motion through the cooked exact trig table.
 */
int l3_pl00_directional_setup_8003d124(L3PlayerCompatWork *work,
                                        int8_t direction);

/* Exact predicate at USA ROCK_NEO.EXE 0x8004324C. */
int l3_pl00_state_busy_8004324c(const L3PlayerCompatWork *work);

/*
 * Exact 0x48-byte class-0x80-family seed performed by USA routine 0x80042528.
 * The broader gameplay meaning of this work object remains deliberately
 * unnamed until its owning caller/dispatcher is fully proved.
 */
enum { L3_CLASS80_COMPAT_WORK_BYTES = 0x48 };
typedef struct L3Class80CompatWork {
    uint8_t bytes[L3_CLASS80_COMPAT_WORK_BYTES];
} L3Class80CompatWork;

/* selector 0/1/2 chooses player XYZ at +0x14/+0x418/+0x400 respectively.
 * Returns non-zero for those proved selector values. Y receives +0x10.
 * Header bytes are always seeded exactly: +0=3, +2=0x83, +3=0.
 */
int l3_pl00_seed_class83_80042528(const L3PlayerCompatWork *player,
                                  int16_t selector,
                                  L3Class80CompatWork *out);

/*
 * Exact USA control-profile copy at 0x800425FC. The original table at
 * 0x8008A04C contains two 16-halfword player profiles before the following
 * function-pointer data begins. Profile 0 is the USA default at boot.
 * This translation stores the factual button-mask table and also rebuilds the
 * derived masks at +0x144..+0x150 exactly like the original routine.
 * Returns zero for an unsupported profile index.
 */
int l3_pl00_install_control_profile_800425fc(L3PlayerCompatWork *work,
                                             unsigned profile_index);

#ifdef __cplusplus
}
#endif
