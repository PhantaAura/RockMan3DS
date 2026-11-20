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
#include <utility>
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
// overlay. Layout: "L3DL", u16 version (1 or 2), u8 row count, u8 override
// count (v2; 0 in v1), list[8], rows[count][8], then (v2) overrides of
// {u16 flag, u8 row, u8 0}: when 0x800C1B62 is 0 the stage init reapplies the
// row of the first override whose flag is set (ST04 0x8010044C: flag 2 -> 13,
// flag 70 -> 12).
struct StageDoorLocks {
    std::array<uint8_t,8> doors{};
    std::vector<std::array<uint8_t,8>> rows;
    std::vector<std::pair<uint16_t,uint8_t>> overrides;
};
// The stage init: row 0x800C1B62, then (byte 0) the first matching override.
template <class FlagFn, class SetFn>
void apply_stage_door_locks(const StageDoorLocks& d, uint8_t byte_800c1b62, FlagFn flag, SetFn set_flag) {
    if (byte_800c1b62 >= d.rows.size()) return;
    source_door_locks_8001e4c4(d.doors, d.rows[byte_800c1b62], set_flag);
    if (byte_800c1b62 != 0) return;
    for (const auto& o : d.overrides)
        if (flag(o.first)) { if (o.second < d.rows.size()) source_door_locks_8001e4c4(d.doors, d.rows[o.second], set_flag); return; }
}
std::optional<StageDoorLocks> load_stage_door_locks(const std::vector<uint8_t>& bytes) noexcept;

} // namespace l3
