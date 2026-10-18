#pragma once
#include "legends3ds/st00_flow.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

// Contact/event sample returned by the common collision query before the switch in
// USA ROCK_NEO.EXE 0x80050C2C. The type is the halfword switched over at stack +0x18;
// position/facing are the four preceding halfwords used by the ST00 aux matcher.
struct St00ContactSample {
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    uint16_t facing = 0; // 0..0x0FFF full turn
    uint16_t type = 0;
    uint16_t detail = 0;
};

enum class St00AuxMatchKind : uint8_t {
    None,
    ExactPosition,
    ExactPositionFacingWindow,
    Type3NeedsObjectBounds,
    Type3DirectionalBounds,
};

// The type-3 branch reads two signed halfwords from the live object work at +0x14/+0x18.
// Their wider gameplay meaning is not named here; only the exact comparison behavior is preserved.
struct St00Type3ObjectBounds {
    int16_t work_14 = 0;
    int16_t work_18 = 0;
};

struct St00AuxMatch {
    size_t record_index = 0;
    St00AuxMatchKind kind = St00AuxMatchKind::None;
};

bool st00_aux_position_equal(const St00AuxRecord& r, const St00ContactSample& s);
// Exact original comparison from 0x80050DB8..0x80050DC4:
// ((sample - record + 0x200) & 0xFFF) < 0x401, i.e. inclusive +/-0x200.
bool st00_aux_facing_within_window(const St00AuxRecord& r, const St00ContactSample& s);

// Exact type-3 directional/bounds test reconstructed from USA ROCK_NEO.EXE
// 0x80050E74..0x80050F14. This is intentionally side-effect-free.
bool st00_aux_type3_directional_bounds_match(const St00AuxRecord& r,
                                              const St00ContactSample& sample,
                                              const St00Type3ObjectBounds& bounds);

// Reconstructs the aux-list selection portion of the common event switch without
// inventing side effects. Types 1 and 8 use exact position. Types 2/4/5/6 additionally
// use the proven facing window. Type 3 uses exact position plus a directional test; if
// live object bounds are unavailable, matching type-3 records are marked pending.
std::vector<St00AuxMatch> match_st00_aux_records(const St00AuxList& list,
                                                 const St00ContactSample& sample,
                                                 const St00Type3ObjectBounds* type3_bounds = nullptr);

} // namespace l3
