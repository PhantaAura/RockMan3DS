#pragma once
// Door locks (shared EXE code; docs/research_01516/STAGE_LOADING.md, "Door
// locks"). A stage init passes its door list (up to 8 door ids, 0 ends it)
// and one row of 8 state bytes to 0x8001E4C4. For each door, a state other
// than 0xFF goes to 0x8001E460(id, bit 0, bit 1): bit 0 sets or clears flag
// 0x7E0 + id (the lock 0x800519E4 tests), bit 1 flag 0x7C0 + id.
// Original addresses are identity tokens.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

// 0x8001E460(id, lock, alt).
template <class FlagFn>
void source_door_lock_8001e460(uint8_t id, bool lock, bool alt, FlagFn set_flag) {
    set_flag(uint16_t(0x7E0u + id), lock);
    set_flag(uint16_t(0x7C0u + id), alt);
}

// 0x8001E4C4(doors, states).
template <class FlagFn>
void source_door_locks_8001e4c4(const std::array<uint8_t,8>& doors, const std::array<uint8_t,8>& states, FlagFn set_flag) {
    for (std::size_t i = 0; i < 8 && doors[i] != 0; ++i) {
        const uint8_t s = states[i];
        if (s != 0xFFu) source_door_lock_8001e460(doors[i], (s & 1u) != 0, ((s >> 1) & 1u) != 0, set_flag);
    }
}

// A stage's cooked door-lock data (stages/stNN/doors.l3door): the door list
// and the state rows its stage init passes to 0x8001E4C4, copied from the
// overlay. Layout: "L3DL", u16 version 1, u8 row count, u8 0, list[8],
// rows[count][8].
struct StageDoorLocks {
    std::array<uint8_t,8> doors{};
    std::vector<std::array<uint8_t,8>> rows;
};
std::optional<StageDoorLocks> load_stage_door_locks(const std::vector<uint8_t>& bytes) noexcept;

} // namespace l3
