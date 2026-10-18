#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3ReferenceVersion = 1;
constexpr size_t kReferenceTrigEntries = 4096;

struct ReferenceTrigEntry { int16_t sin_q12=0, cos_q12=0; };
struct ReferenceTables { std::array<ReferenceTrigEntry,kReferenceTrigEntries> trig{}; };

struct L3ReferenceHeader {
    uint16_t version=0, header_bytes=0;
    uint32_t flags=0, trig_count=0, off_trig=0, file_bytes=0, payload_crc32=0;
};

L3ReferenceHeader inspect_l3reference(const std::vector<uint8_t>& bytes,bool verify_crc=true);
ReferenceTables load_l3reference(const std::vector<uint8_t>& bytes,bool verify_crc=true);

// Exact fixed-point coordinate update corresponding to the proved mathematical
// path through USA 0x80041E90 -> 0x8002FEA4 -> RotMatrixY/ApplyMatrixLV, when
// supplied the user's cooked original trig table. Inputs are the signed values
// passed as player work +0x44/+0x46/+0x48 (forward/vertical/strafe).
struct ReferenceMotionDelta { int32_t dx_fixed=0,dy_fixed=0,dz_fixed=0; };
ReferenceMotionDelta reference_player_motion_delta(const ReferenceTables& refs,
                                                    uint16_t yaw,
                                                    int16_t forward,
                                                    int16_t vertical,
                                                    int16_t strafe);

// Matches the rounding step at USA 0x80030150 when converting movement fixed
// accumulators to the short world-position mirror.
int16_t reference_fixed_to_short_80030150(int32_t value);

} // namespace l3
