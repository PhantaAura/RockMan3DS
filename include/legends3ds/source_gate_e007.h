#pragma once
#include "legends3ds/l3map.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// ST00 E0/07 (0x80115A4C, states 0x8011CCE4), the Area 1 trap room:
//  - parameter != 0: floor switch. State 5 (0x80115D38) sets 0x607 when the
//    player (+0x14/+0x16/+0x18) is within dx,dz -60..60 and dy -10..160 of
//    +0x38; sound 0x11E; 16-frame press (6), then idle (7, 8).
//  - parameter 0: gate. Init stores its cell (0x80048C60 of +0x38) and that
//    cell's IDX descriptor (+0x2C) plus the one whose word0 is 0x40500
//    (0x8003AC24, +0x30). State 1 waits 0x607, closes (writes +0x30), sound
//    0x119; 2 counts 8 frames, then 0x800398E0(0x1E) and sound 0x118; 3 waits
//    0x61F, sets 0x60A, sound 0x119; 4 counts 64 frames, restores +0x2C at 16
//    and at 0 calls 0x8001B2F0(0xF) and releases.
// Init (0x80115A88) releases both when 0x607 is already set.
struct SourceE007Object {
    bool gate = false;
    std::array<int16_t,3> position{};  // +0x38/+0x3A/+0x3C
    uint8_t state = 0;                 // +8
    uint8_t timer = 0;                 // +0xB
    uint16_t idx_cell = 0;             // +0x44
    uint16_t open_descriptor = 0;      // +0x2C
    uint16_t closed_descriptor = 0;    // +0x30
    uint16_t anim_20 = 0;              // +0x20 (model animation; not drawn)
    bool released = false;
};
struct SourceE007Step {
    bool set_607 = false, set_60a = false;
    std::optional<uint16_t> descriptor_write;  // for idx_cell
    uint16_t sound = 0;                         // 0x8001A050 id
    uint8_t shake_frames = 0;                   // 0x800398E0 argument
    bool bgm_8001b2f0 = false;                  // 0x8001B2F0(0xF)
};
// nullopt: the object releases at init (0x607 set) or the gate's cell or
// closed descriptor cannot be found (held, not guessed).
std::optional<SourceE007Object> source_e007_init_80115a88(const RuntimeMap& map, size_t area,
    std::array<int16_t,3> position, uint8_t parameter, bool flag_607);
SourceE007Step source_e007_step(SourceE007Object& o, bool flag_607, bool flag_61f,
    std::array<int16_t,3> player) noexcept;

} // namespace l3
