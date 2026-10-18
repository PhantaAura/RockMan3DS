#include "legends3ds/st00_event.h"

namespace l3 {
bool st00_aux_position_equal(const St00AuxRecord& r, const St00ContactSample& s) {
    return r.halfwords[4] == s.x && r.halfwords[5] == s.y && r.halfwords[6] == s.z;
}

bool st00_aux_facing_within_window(const St00AuxRecord& r, const St00ContactSample& s) {
    const uint16_t record_facing = static_cast<uint16_t>(r.halfwords[7]);
    const uint16_t delta = static_cast<uint16_t>(
        (static_cast<uint32_t>(s.facing) - static_cast<uint32_t>(record_facing) + 0x200u) & 0x0FFFu);
    return delta < 0x0401u;
}

bool st00_aux_type3_directional_bounds_match(const St00AuxRecord& r,
                                              const St00ContactSample& sample,
                                              const St00Type3ObjectBounds& bounds) {
    // First direction gate: ((sample_facing - record_facing + 0x200) & 0x400) == 0.
    // Note this is deliberately not the +/-0x200 window used by types 2/4/5/6.
    const int32_t record_facing = static_cast<int16_t>(r.halfwords[7]);
    const int32_t sample_facing = static_cast<int16_t>(sample.facing);
    const uint32_t gate = static_cast<uint32_t>(sample_facing - record_facing + 0x200) & 0x0400u;
    if (gate != 0u) return false;

    // The original converts the contact facing into one of four quarter-turn sectors
    // with a signed floor-style divide by 0x400, then masks to 0..3. For the native
    // 12-bit angle domain this is equivalent to ((facing + 0x200) >> 10) & 3.
    const uint16_t f = static_cast<uint16_t>(sample.facing & 0x0FFFu);
    const unsigned quadrant = static_cast<unsigned>(((static_cast<uint32_t>(f) + 0x200u) >> 10u) & 3u);
    switch (quadrant) {
        case 0: return bounds.work_18 >= sample.z;
        case 1: return bounds.work_14 >= sample.x;
        case 2: return sample.z >= bounds.work_18;
        case 3: return sample.x >= bounds.work_14;
        default: return false;
    }
}

std::vector<St00AuxMatch> match_st00_aux_records(const St00AuxList& list,
                                                 const St00ContactSample& sample,
                                                 const St00Type3ObjectBounds* type3_bounds) {
    std::vector<St00AuxMatch> out;
    St00AuxMatchKind kind = St00AuxMatchKind::None;
    switch (sample.type) {
        case 1:
        case 8:
            kind = St00AuxMatchKind::ExactPosition;
            break;
        case 2:
        case 4:
        case 5:
        case 6:
            kind = St00AuxMatchKind::ExactPositionFacingWindow;
            break;
        case 3:
            kind = St00AuxMatchKind::Type3NeedsObjectBounds;
            break;
        default:
            return out;
    }

    for (size_t i = 0; i < list.records.size(); ++i) {
        const auto& r = list.records[i];
        if (!st00_aux_position_equal(r, sample)) continue;
        if (kind == St00AuxMatchKind::ExactPositionFacingWindow &&
            !st00_aux_facing_within_window(r, sample)) continue;
        if (kind == St00AuxMatchKind::Type3NeedsObjectBounds) {
            if (!type3_bounds) {
                out.push_back({i, kind});
                continue;
            }
            if (!st00_aux_type3_directional_bounds_match(r, sample, *type3_bounds)) continue;
            out.push_back({i, St00AuxMatchKind::Type3DirectionalBounds});
            continue;
        }
        out.push_back({i, kind});
    }
    return out;
}
} // namespace l3
