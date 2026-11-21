#pragma once
// Stage 9 (the first ruin, S5; docs/research_01516/STAGE_LOADING.md).
// The area-setup routine 0x80100398 ends with 0x801005F4, which sets flag
// 0x16 on every area load. ST09 has no area-enter / 0x1FF gate: its
// per-frame dispatcher 0x8010061C runs the area's update directly. Area 0's
// update 0x80100670: flag 0x7B0 starts event 0 (the door event, already run
// by the runtime's door path); byte +4 = 0: without flag 12 it spawns set 0
// (0x80100B20(0): one record at 0x8011DD68, the trapped junk-shop man, class
// 0x13 variant 1, ST09 code 0x80107CDC, not ported) and goes to 1, else 255;
// state 1 waits for flag 0x60C, then starts event 0x17 (reported) and goes
// to 255. Original addresses are identity tokens.
#include "legends3ds/source_st0c.h"
#include <cstdint>
#include <optional>

namespace l3 {

constexpr uint32_t kSt09Set0Record = 0x8011DD68u;

struct St09Area0Step {
    std::optional<uint8_t> object_set;
    std::optional<uint8_t> event_unported;   // 0x8001DDE4(0x17)
};
template <class FlagFn>
St09Area0Step st09_area0_update_80100670(uint8_t& state_04, FlagFn flag) {
    St09Area0Step out;
    if (state_04 == 0) {
        if (flag(12)) state_04 = 255;
        else { out.object_set = 0; state_04 = 1; }
    } else if (state_04 == 1 && flag(0x60C)) {
        out.event_unported = 0x17; state_04 = 255;
    }
    return out;
}

// Set 0 (the one record at 0x8011DD68) from the cooked stage.l3sd.
bool bind_st09_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out);

} // namespace l3
