// OPTION Turn (0x800C1B64) -> control profile 0x800425FC (table 0x8008A04C):
// with the port's 1:1 pad (3DS D-pad = PS1 directions, L/R = L1/R1), profile
// 0 turns on L1/R1 and slides on the D-pad (0x8003D124 strafe +0x48 = +-0x140),
// profile 1 the reverse. Turn helper 0x80042044 accumulates +0xB4.
#include "legends3ds/player_runtime.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {
constexpr uint16_t kLeft = 0x0080, kRight = 0x0020, kL1 = 0x0400, kR1 = 0x0800;
int16_t rd(const L3PlayerCompatWork& w, size_t o) { return int16_t(w.bytes[o] | (w.bytes[o + 1] << 8)); }
void held(L3PlayerCompatWork& w, uint16_t m) { w.bytes[0x11C] = uint8_t(m); w.bytes[0x11D] = uint8_t(m >> 8); }
int16_t turn(unsigned profile, uint16_t pad) {
    L3PlayerCompatWork w; std::memset(&w, 0, sizeof w);
    assert(l3_pl00_install_control_profile_800425fc(&w, profile));
    held(w, pad);
    (void)l3_pl00_turn_pair_80042044(&w, 4, 0x20);
    return rd(w, 0xB4);
}
int16_t slide(unsigned profile, uint16_t pad) {
    L3PlayerCompatWork w; std::memset(&w, 0, sizeof w);
    assert(l3_pl00_install_control_profile_800425fc(&w, profile));
    held(w, pad);
    assert(l3_pl00_directional_setup_8003d124(&w, 0));
    return rd(w, 0x48);
}
} // namespace

int main() {
    // Profile 0 (boot default: 0x80013360 zeroes 0x800C1B64; oracle New Game 0).
    assert(turn(0, kL1) != 0 && turn(0, kR1) != 0 && turn(0, kL1) == -turn(0, kR1));
    assert(turn(0, kLeft) == 0 && turn(0, kRight) == 0);
    assert(slide(0, kLeft) == 0x140 && slide(0, kRight) == -0x140);
    assert(slide(0, kL1) == 0 && slide(0, kR1) == 0);
    // Profile 1 ("Turn: Directional button / Slide: L1 R1").
    assert(turn(1, kLeft) != 0 && turn(1, kLeft) == -turn(1, kRight));
    assert(turn(1, kL1) == 0 && turn(1, kR1) == 0);
    assert(slide(1, kL1) == 0x140 && slide(1, kR1) == -0x140);
    assert(slide(1, kLeft) == 0 && slide(1, kRight) == 0);
    // Same physical direction either way: D-pad left in profile 1 turns like L1 in profile 0.
    assert(turn(1, kLeft) == turn(0, kL1) && slide(1, kL1) == slide(0, kLeft));
    std::cout << "player_turn_profile: ok\n";
    return 0;
}
