#include "legends3ds/player_collision.h"
#include <stddef.h>
#include <string.h>

static uint16_t rd16c(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static int16_t rds16c(const uint8_t *p) { return (int16_t)rd16c(p); }
static void wr16c(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}
static void wr32c(uint8_t *p, int32_t v) {
    const uint32_t u = (uint32_t)v;
    p[0] = (uint8_t)(u & 0xFFu);
    p[1] = (uint8_t)((u >> 8) & 0xFFu);
    p[2] = (uint8_t)((u >> 16) & 0xFFu);
    p[3] = (uint8_t)((u >> 24) & 0xFFu);
}

uint8_t l3_obj_group_bits_8002fe74(uint8_t object_class) {
    uint8_t v = object_class;
    if (v < 0x20u) {
        return (uint8_t)(v & 0x10u);
    }
    v = (uint8_t)(v & 0xE0u);
    if (v == 0xA0u) return 0x80u;
    return v;
}

void l3_pl00_snapshot_collision_pose(L3PlayerCompatWork *work) {
    if (!work) return;
    memcpy(&work->bytes[0x2C], &work->bytes[0x14], 8u);
    memcpy(&work->bytes[0x34], &work->bytes[0x1C], 16u);
}

int32_t l3_pl00_apply_contact_result_8004b414(L3PlayerCompatWork *work,
                                               int32_t contact_result,
                                               const L3RetailContactScratch *scratch) {
    if (!work) return contact_result;
    /* Original8004B414 branches out on zero before either correction or
     * negative-result rollback. Zero is successful unchanged movement. */
    if (contact_result == 0) return contact_result;

    if (contact_result > 0) {
        if (!scratch) return contact_result;
        if ((contact_result & L3_CONTACT_RESULT_AXIS_X_MASK) != 0) {
            wr16c(&work->bytes[0x14], (uint16_t)scratch->x);
            wr32c(&work->bytes[0x1C], (int32_t)scratch->x * 65536);
        }
        if ((contact_result & L3_CONTACT_RESULT_AXIS_Z_MASK) != 0) {
            wr16c(&work->bytes[0x18], (uint16_t)scratch->z);
            wr32c(&work->bytes[0x24], (int32_t)scratch->z * 65536);
        }
        if ((contact_result & L3_CONTACT_RESULT_AXIS_Y_MASK) != 0) {
            wr16c(&work->bytes[0x16], (uint16_t)scratch->y);
            wr32c(&work->bytes[0x20], (int32_t)scratch->y * 65536);
        }
        if ((int8_t)work->bytes[0x74] != 2) {
            work->bytes[0x74] = scratch->contact_state;
        }
        return contact_result;
    }

    /* 0x8004B434 calls 0x8002FE74(work+1), then converts a zero return into
     * the 0x8000 flag held in s1. */
    if (l3_obj_group_bits_8002fe74(work->bytes[0x01]) == 0u) {
        wr16c(&work->bytes[0x14], rd16c(&work->bytes[0x2C]));
        wr16c(&work->bytes[0x18], rd16c(&work->bytes[0x30]));
        memcpy(&work->bytes[0x1C], &work->bytes[0x34], 4u);
        memcpy(&work->bytes[0x24], &work->bytes[0x3C], 4u);
        wr16c(&work->bytes[0x48], 0u);
        wr16c(&work->bytes[0x44], 0u);

        if ((int8_t)work->bytes[0x74] != 0) {
            wr16c(&work->bytes[0x16], rd16c(&work->bytes[0x2E]));
            memcpy(&work->bytes[0x20], &work->bytes[0x38], 4u);
            work->bytes[0x74] = 0u;
            wr16c(&work->bytes[0x46], 0u);
        }
    } else {
        memcpy(&work->bytes[0x14], &work->bytes[0x2C], 8u);
        memcpy(&work->bytes[0x1C], &work->bytes[0x34], 16u);
    }
    return contact_result;
}

int l3_pl00_jump_launch_8003dac0(L3PlayerCompatWork *work) {
    int16_t velocity;
    if (!work || (int8_t)work->bytes[0xA7] != -1) return 0;
    work->bytes[0x74] = 1u;
    work->bytes[0x0A] = (uint8_t)(work->bytes[0x0A] + 1u);
    work->bytes[0xAD] = work->bytes[0xAC];
    velocity = (work->bytes[0x16B] & 0x01u) ? (int16_t)-0x320 : (int16_t)-0x1C0;
    wr16c(&work->bytes[0x46], (uint16_t)velocity);
    l3_pl00_shot_enable_on(work);
    return 1;
}

int l3_pl00_jump_hold_gravity_8003db50(L3PlayerCompatWork *work) {
    const uint16_t held = work ? rd16c(&work->bytes[0x11C]) : 0u;
    const uint16_t hold_mask = work ? rd16c(&work->bytes[0x138]) : 0u;
    int16_t v;
    if (!work || (held & hold_mask) == 0u) return 0;
    v = (int16_t)(rds16c(&work->bytes[0x46]) + 0x30);
    wr16c(&work->bytes[0x46], (uint16_t)v);
    return v < 0 ? 1 : 2;
}


int32_t l3_collision_no_contact_result_8004bc2c(int16_t input_x,
                                                 int16_t input_y,
                                                 int16_t input_z,
                                                 int16_t scratch_x,
                                                 int16_t scratch_y,
                                                 int16_t scratch_z) {
    const int32_t dx = (int32_t)input_x - (int32_t)scratch_x;
    const int32_t dz = (int32_t)input_z - (int32_t)scratch_z;
    const int32_t dy = (int32_t)input_y - (int32_t)scratch_y;
    int32_t result;
    int32_t adx, adz, ady;

    if (dx == 0 && dz == 0 && dy == 0) return 0;
    adx = dx < 0 ? -dx : dx;
    adz = dz < 0 ? -dz : dz;
    ady = dy < 0 ? -dy : dy;

    // The MIPS path uses strict signed less-than comparisons. Therefore ties
    // deliberately fall through from X -> Z and from the horizontal winner -> Y.
    if (adz < adx) {
        if (ady < adx) result = dx > 0 ? 1 : 2;
        else result = dy > 0 ? 5 : 6;
    } else {
        if (ady < adz) result = dz > 0 ? 3 : 4;
        else result = dy > 0 ? 5 : 6;
    }

    if (dx > 0) result |= 0x0100;
    else if (dx < 0) result |= 0x0200;
    if (dz > 0) result |= 0x0400;
    else if (dz < 0) result |= 0x0800;
    if (dy > 0) result |= 0x1000;
    else if (dy < 0) result |= 0x2000;
    return result;
}
