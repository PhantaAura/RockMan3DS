#include "legends3ds/player_runtime.h"

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static int16_t rds16(const uint8_t *p) { return (int16_t)rd16(p); }
static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)(v >> 8);
}
static int16_t abs16_nonneg(int16_t v) {
    if (v >= 0) return v;
    if (v == (int16_t)0x8000) return (int16_t)0x7fff;
    return (int16_t)-v;
}

void l3_pl00_shot_enable_off(L3PlayerCompatWork *work) {
    const uint8_t old_ac = work->bytes[0xAC];
    const uint8_t old_a4 = work->bytes[0xA4];
    work->bytes[0x174] = 0;
    work->bytes[0x177] = 0;
    work->bytes[0x175] = (uint8_t)(work->bytes[0x175] & 0xFEu);
    work->bytes[0x104] = old_ac;
    work->bytes[0x100] = old_a4;
}

void l3_pl00_shot_enable_on(L3PlayerCompatWork *work) {
    work->bytes[0x175] = (uint8_t)(work->bytes[0x175] | 0x01u);
}

int l3_pl00_shot_is_enabled(const L3PlayerCompatWork *work) {
    return (work->bytes[0x175] & 0x01u) != 0;
}

int l3_pl00_fire_state_80040e9c(L3PlayerCompatWork *work,
                                uint16_t action,
                                int helper_known,
                                uint8_t helper_request) {
    static const uint8_t flags[8] = {0x86u,0x82u,0x45u,0x41u,0x86u,0x82u,0x45u,0x41u};
    if (!work || action > 7u) return 0;
    if (action >= 4u && !helper_known) return 0;
    work->bytes[0x174] = flags[action];
    if (action < 4u) return 1;
    work->bytes[0x104] = helper_request;
    wr16(&work->bytes[0x178], 0u);
    work->bytes[0x105] = 0;
    work->bytes[0x100] = 0;
    if ((work->bytes[0x174] & 4u) == 0u && work->bytes[0xAC] == work->bytes[0xAD])
        l3_pl00_anim_request_80041ddc(work, helper_request, 0u, 1u);
    return 1;
}

void l3_pl00_anim_request_80041ddc(L3PlayerCompatWork *work,
                                   uint8_t request,
                                   uint8_t substate,
                                   uint8_t force) {
    uint8_t old;
    int16_t t;
    if (!work) return;
    if (force != 0u) {
        work->bytes[0xAD] = 0xFFu;
    } else {
        old = work->bytes[0xAD];
        if (request == old) return;
    }

    if (request == 0x01u) {
        if (work->bytes[0xAD] == 0x0Cu) {
            t = (int16_t)((int16_t)work->bytes[0xA4] - 4);
            if (t < 0) t = (int16_t)((int16_t)work->bytes[0xA4] + 0x1C);
            substate = (uint8_t)(int16_t)(t >> 1);
        }
    } else if (request == 0x0Cu && work->bytes[0xAD] == 0x01u) {
        t = (int16_t)((int16_t)work->bytes[0xA4] * 2 + 4);
        if (t >= 0x20) t = (int16_t)((int16_t)work->bytes[0xA4] * 2 - 0x1C);
        substate = (uint8_t)t;
    }
    work->bytes[0xAC] = request;
    work->bytes[0xA4] = substate;
}

void l3_pl00_axis_step_80041f54(L3PlayerCompatWork *work,
                                uint16_t mask,
                                int16_t step,
                                int16_t cap) {
    int16_t accel = rds16(&work->bytes[0x10C]);
    int16_t axis = rds16(&work->bytes[0x108]);
    step = abs16_nonneg(step);
    cap = abs16_nonneg(cap);

    if ((rd16(&work->bytes[0x11C]) & mask) != 0u) {
        if (accel < 0) {
            accel = 0;
        } else {
            int32_t next = (int32_t)accel + (int32_t)step;
            if (next > cap) next = cap;
            accel = (int16_t)next;
        }
    } else {
        if (accel > 0) {
            accel = 0;
        } else {
            int32_t next = (int32_t)accel - (int32_t)step;
            if (next < -(int32_t)cap) next = -(int32_t)cap;
            accel = (int16_t)next;
        }
    }

    {
        int32_t next_axis = (int32_t)axis + (int32_t)accel;
        if (next_axis > 0x200) next_axis = 0x200;
        if (next_axis < -0x200) next_axis = -0x200;
        axis = (int16_t)next_axis;
    }
    wr16(&work->bytes[0x10C], (uint16_t)accel);
    wr16(&work->bytes[0x108], (uint16_t)axis);
}

int l3_pl00_turn_step_80042154(L3PlayerCompatWork *work,
                               uint16_t mask,
                               int16_t step,
                               int16_t cap) {
    int16_t accum = rds16(&work->bytes[0xB4]);
    uint16_t yaw = rd16(&work->bytes[0x56]);
    int branch;
    step = abs16_nonneg(step);
    cap = abs16_nonneg(cap);

    if ((rd16(&work->bytes[0x11C]) & mask) != 0u) {
        branch = 1;
        if (accum < 0) {
            accum = 0;
        } else {
            int32_t next = (int32_t)accum + (int32_t)step;
            if (next > cap) next = cap;
            accum = (int16_t)next;
            wr16(&work->bytes[0xB6], (uint16_t)step);
        }
    } else {
        branch = 2;
        if (accum > 0) {
            accum = 0;
        } else {
            int32_t next = (int32_t)accum - (int32_t)step;
            if (next < -(int32_t)cap) next = -(int32_t)cap;
            accum = (int16_t)next;
            wr16(&work->bytes[0xB6], (uint16_t)(int16_t)-step);
        }
    }

    yaw = (uint16_t)((yaw + (uint16_t)accum) & 0x0fffu);
    wr16(&work->bytes[0xB4], (uint16_t)accum);
    wr16(&work->bytes[0x56], yaw);
    return branch;
}


int l3_pl00_turn_pair_80042044(L3PlayerCompatWork *work,
                               int16_t step,
                               int16_t cap) {
    const uint16_t mask_a = rd16(&work->bytes[0x128]);
    const uint16_t mask_b = rd16(&work->bytes[0x12A]);
    const uint16_t pair = (uint16_t)(mask_a | mask_b);
    const uint16_t hit = (uint16_t)(rd16(&work->bytes[0x11C]) & pair);
    if (hit == 0u || hit == pair) {
        wr16(&work->bytes[0xB4], 0u);
        return 0;
    }
    return l3_pl00_turn_step_80042154(work, mask_b, step, cap);
}

void l3_pl00_axis_pair_80041ef4(L3PlayerCompatWork *work,
                                int16_t step,
                                int16_t cap,
                                int alternate_orientation) {
    const uint16_t mask_a = rd16(&work->bytes[0x124]);
    const uint16_t mask_b = rd16(&work->bytes[0x126]);
    const uint16_t pair = (uint16_t)(mask_a | mask_b);
    const uint16_t hit = (uint16_t)(rd16(&work->bytes[0x11C]) & pair);
    if (hit == 0u) {
        wr16(&work->bytes[0x10C], 0u);
        return;
    }
    l3_pl00_axis_step_80041f54(work,
                               alternate_orientation ? mask_b : mask_a,
                               step, cap);
}

int l3_pl00_directional_setup_8003d124(L3PlayerCompatWork *work,
                                        int8_t direction) {
    uint16_t held, mask_all, mask_side, turn_pair, hit;
    size_t off;
    int16_t strafe = 0;
    if (!work || direction < 0 || direction > 3) return 0;
    off = (size_t)(uint8_t)direction * 8u;
    held = rd16(&work->bytes[0x11C]);
    mask_all = rd16(&work->bytes[0x148u + off]);
    mask_side = rd16(&work->bytes[0x14Au + off]);
    hit = (uint16_t)(held & mask_all);
    if (hit != 0u && hit != mask_all) {
        turn_pair = (uint16_t)(rd16(&work->bytes[0x128]) | rd16(&work->bytes[0x12A]));
        if ((held & mask_side) != 0u) {
            strafe = (int16_t)-0x140;
            if ((held & turn_pair) == 0u) wr16(&work->bytes[0xB4], 0x0040u);
        } else {
            strafe = (int16_t)0x140;
            if ((held & turn_pair) == 0u) wr16(&work->bytes[0xB4], (uint16_t)(int16_t)-0x40);
        }
    }
    wr16(&work->bytes[0x48], (uint16_t)strafe);
    return 1;
}

int l3_pl00_state_busy_8004324c(const L3PlayerCompatWork *work) {
    if ((work->bytes[0x174] & 0xC0u) != 0u) return 1;
    return ((int8_t)work->bytes[0x177]) != 0;
}

int l3_pl00_seed_class83_80042528(const L3PlayerCompatWork *player,
                                  int16_t selector,
                                  L3Class80CompatWork *out) {
    size_t base;
    int16_t x, y, z;
    if (!player || !out) return 0;

    out->bytes[0] = 3;
    out->bytes[2] = 0x83;
    out->bytes[3] = 0;

    if (selector == 0) base = 0x14;
    else if (selector == 1) base = 0x418;
    else if (selector == 2) base = 0x400;
    else return 0;

    x = rds16(&player->bytes[base + 0]);
    y = rds16(&player->bytes[base + 2]);
    z = rds16(&player->bytes[base + 4]);
    y = (int16_t)(y + 0x10);
    wr16(&out->bytes[0x1C], (uint16_t)x);
    wr16(&out->bytes[0x1E], (uint16_t)y);
    wr16(&out->bytes[0x20], (uint16_t)z);
    return 1;
}


int l3_pl00_install_control_profile_800425fc(L3PlayerCompatWork *work,
                                             unsigned profile_index) {
    static const uint16_t profiles[2][16] = {
        {0x0010,0x0040,0x0400,0x0800,0x0080,0x0020,0x0080,0x0020,
         0x2000,0x2000,0x4000,0x8000,0x1000,0x0100,0x0200,0x0001},
        {0x0010,0x0040,0x0080,0x0020,0x0400,0x0800,0x0080,0x0020,
         0x2000,0x2000,0x4000,0x8000,0x1000,0x0100,0x0200,0x0001},
    };
    unsigned i;
    if (!work || profile_index >= 2u) return 0;
    for (i = 0; i < 16u; ++i) wr16(&work->bytes[0x124u + i * 2u], profiles[profile_index][i]);
    wr16(&work->bytes[0x144], (uint16_t)(rd16(&work->bytes[0x124]) | rd16(&work->bytes[0x126])));
    wr16(&work->bytes[0x146], rd16(&work->bytes[0x124]));
    wr16(&work->bytes[0x148], (uint16_t)(rd16(&work->bytes[0x12C]) | rd16(&work->bytes[0x12E])));
    wr16(&work->bytes[0x14A], rd16(&work->bytes[0x12E]));
    wr16(&work->bytes[0x14C], (uint16_t)(rd16(&work->bytes[0x12C]) | rd16(&work->bytes[0x12E])));
    wr16(&work->bytes[0x14E], rd16(&work->bytes[0x12C]));
    wr16(&work->bytes[0x150], (uint16_t)(rd16(&work->bytes[0x124]) | rd16(&work->bytes[0x126])));
    wr16(&work->bytes[0x152], rd16(&work->bytes[0x124]));
    wr16(&work->bytes[0x154], (uint16_t)(rd16(&work->bytes[0x124]) | rd16(&work->bytes[0x126])));
    wr16(&work->bytes[0x156], rd16(&work->bytes[0x126]));
    wr16(&work->bytes[0x158], (uint16_t)(rd16(&work->bytes[0x12C]) | rd16(&work->bytes[0x12E])));
    wr16(&work->bytes[0x15A], rd16(&work->bytes[0x12C]));
    wr16(&work->bytes[0x15C], (uint16_t)(rd16(&work->bytes[0x12C]) | rd16(&work->bytes[0x12E])));
    wr16(&work->bytes[0x15E], rd16(&work->bytes[0x12E]));
    wr16(&work->bytes[0x160], (uint16_t)(rd16(&work->bytes[0x124]) | rd16(&work->bytes[0x126])));
    wr16(&work->bytes[0x162], rd16(&work->bytes[0x126]));
    return 1;
}
