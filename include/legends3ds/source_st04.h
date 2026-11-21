#pragma once
// Stage 4 (Apple Market, S4; docs/research_01516/STAGE_LOADING.md).
// Areas: 0 the town, 1..5 the shops. Area-enter: 0x80101AC4 (area 0: with
// flag 0x235, locks 0x7E2..0x7E7) and 0x80101B24 (shops); both set 0x1FF.
// Area 0's per-frame update 0x80100594 (byte +4), first pass: set 0xD, clear
// 0x16, with flag 12 set 0x609..0x60D; then by the first set flag of 9 / 15 /
// 6 / 1 / 32 / 70 / 12 the object set 4 / 4 / 3 / 33 / 34 / 2 / 1, else 0
// (0x80101E68). Flag 9 and 32 also call 0x8010207C (reported); flag 70
// without 32 spawns four more objects (0x8001EB98, reported) and goes to
// state 10; states 10..12 (the walk past z 3584 and flags 0x60F / 0x610) are
// reported. The generic NPC tables are ST04's copies (ST03's - 0xA6EC).
// Original addresses are identity tokens.
#include "legends3ds/source_st0c.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

template <class FlagFn, class SetFn>
void st04_area_enter(uint8_t area, FlagFn flag, SetFn set_flag) {
    if (area == 0 && flag(0x235))
        for (uint16_t f = 0x7E2; f <= 0x7E7; ++f) set_flag(f, true);
    set_flag(0x1FF, true);
}

struct St04Area0Step {
    std::array<uint16_t,6> flags_set{};
    uint8_t flag_set_count = 0;
    std::optional<uint16_t> flag_clear;
    std::optional<uint8_t> object_set;
    bool call_8010207c_unported = false;   // flags 9 / 32
    bool extra_spawns_unported = false;    // flag 70 without 32 (state 10)
    bool state_unported = false;           // states 10..12
};
template <class FlagFn>
St04Area0Step st04_area0_setup_step(uint8_t& state_04, FlagFn flag) {
    St04Area0Step out;
    if (state_04 == 255) return out;
    if (state_04 != 0) { out.state_unported = true; return out; }
    const auto set = [&](uint16_t f) { if (out.flag_set_count < out.flags_set.size()) out.flags_set[out.flag_set_count++] = f; };
    set(0x0D); out.flag_clear = 0x16;
    if (flag(12)) for (uint16_t f = 0x609; f <= 0x60D; ++f) set(f);
    state_04 = 255;
    if (flag(9)) { out.object_set = 4; out.call_8010207c_unported = true; }
    else if (flag(15)) out.object_set = 4;
    else if (flag(6)) out.object_set = 3;
    else if (flag(1)) out.object_set = 33;
    else if (flag(32)) { out.object_set = 34; out.call_8010207c_unported = true; }
    else if (flag(70)) { out.object_set = 2; out.extra_spawns_unported = true; state_04 = 10; }
    else out.object_set = flag(12) ? 1 : 0;
    return out;
}

// Area 1 (the junk shop) update 0x80100890, first pass: by flag 1 / 70 / 12
// the object set 7 / 7 / 6, else 5 (the owner's wife, class 0x09, a generic
// NPC: message 181 asks, its Yes choice opens 182, which sets flag 0xE); flag
// 1 also calls 0x8010207C(1), (2) (reported); with flag 34 also set 54.
struct St04Area1Step {
    std::array<uint8_t,2> object_sets{};
    uint8_t object_set_count = 0;
    bool call_8010207c_unported = false;
};
template <class FlagFn>
St04Area1Step st04_area1_setup_step(uint8_t& state_04, FlagFn flag) {
    St04Area1Step out;
    if (state_04 != 0) return out;
    const auto add = [&](uint8_t n) { if (out.object_set_count < out.object_sets.size()) out.object_sets[out.object_set_count++] = n; };
    if (flag(1)) { add(7); out.call_8010207c_unported = true; }
    else if (flag(70)) add(7);
    else add(flag(12) ? 6 : 5);
    if (flag(34)) add(54);
    state_04 = 255;
    return out;
}

// The field-object profile of a stage from its cooked stage.l3sd: the object
// set table (8 bytes per set) and its records, the generic NPC tables and the
// class mask the pack derived from the stage's class table.
struct FieldProfileSpec {
    uint32_t set_table = 0;
    uint8_t set_count = 0;
    uint32_t walk_speeds = 0, idle_timers = 0, walk_tables = 0, class_mask = 0;
};
constexpr FieldProfileSpec kSt04FieldProfile{0x8010C010u, 58, 0x8010C6C0u, 0x8010C77Cu, 0x8010C7E8u, 0x8010A570u};
bool bind_field_profile(const std::vector<StageDataBlock>& blocks, const FieldProfileSpec& spec, St03Event8Profile& out);

} // namespace l3
