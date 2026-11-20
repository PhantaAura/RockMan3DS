#pragma once
// Stage 0x0C (the forest path, S2; docs/research_01516/STAGE_LOADING.md).
// One area. Its area-enter 0x801012D4 sets 0x1FF (with flag 0x21B set and
// 0x22A clear it also calls 0x8001EB98(0x8010D874): reported, not run). Its
// per-frame area-0 update 0x80100ED4 (byte +4) spawns the ruin guard from the
// record 0x8010CB70 (class 0x24, the generic NPC, message +0x0C = 11) and a
// 0x8003AC9C cell write at (2816, 0, 2304) (reported), then +4 = 255.
// The generic NPC tables are ST0C's own copies (ST03's addresses - 0x9A14).
// Original addresses are identity tokens.
#include "legends3ds/source_st03_event8.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

constexpr uint32_t kSt0CGuardRecord = 0x8010CB70u;
constexpr uint32_t kSt0CWalkSpeeds = 0x8010D398u, kSt0CIdleTimers = 0x8010D454u, kSt0CWalkTables = 0x8010D4C0u;

// A cooked stage-data file (stages/stNN/stage.l3sd): "L3SD", u16 version 1,
// u16 block count, then blocks of {u32 token, u32 length, bytes}.
struct StageDataBlock { uint32_t token = 0; std::vector<uint8_t> bytes; };
std::optional<std::vector<StageDataBlock>> load_stage_data(const std::vector<uint8_t>& bytes) noexcept;

// The field-object profile for ST0C (the generic NPC tables and set 0 = the guard).
bool bind_st0c_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out);

// 0x801012D4: true when the 0x8001EB98 spawn would also run (unported).
template <class FlagFn, class SetFn>
bool st0c_area0_enter_801012d4(uint8_t block_04, FlagFn flag, SetFn set_flag) {
    const bool spawn_unported = block_04 == 0 && flag(0x21B) && !flag(0x22A);
    set_flag(0x1FF, true);
    return spawn_unported;
}
// 0x80100ED4 (byte +4): returns true on the frame it spawns the guard.
inline bool st0c_area0_update_80100ed4(uint8_t& state_04) noexcept {
    if (state_04 != 0) return false;
    state_04 = 255;
    return true;
}

} // namespace l3
