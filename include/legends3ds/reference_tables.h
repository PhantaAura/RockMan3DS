#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

// Version 1 carries the packed table only; version 2 appends the scalar
// quarter-wave table. Both versions load.
constexpr uint16_t kL3ReferenceVersion = 2;
constexpr size_t kReferenceTrigEntries = 4096;
constexpr size_t kReferenceQuarterEntries = 1025;

struct ReferenceTrigEntry { int16_t sin_q12=0, cos_q12=0; };
struct ReferenceTables {
    // Packed sin|cos at USA 0x80092EC4, read by RotMatrix* (0x8007B838 ...).
    std::array<ReferenceTrigEntry,kReferenceTrigEntries> trig{};
    // Quarter-wave halfwords at USA 0x80092348, read by rsin 0x8007A060 and
    // rcos 0x8007A12C. The two tables differ (by up to +-7) in 1944 of the
    // 4096 angles, so one never stands in for the other.
    std::array<int16_t,kReferenceQuarterEntries> scalar_quarter{};
    bool scalar_quarter_known=false;
};

struct L3ReferenceHeader {
    uint16_t version=0, header_bytes=0;
    uint32_t flags=0, trig_count=0, off_trig=0, file_bytes=0, payload_crc32=0;
    uint32_t off_quarter=0;   // version 2 only
};

// rsin 0x8007A060 / rcos 0x8007A12C, branch for branch, over the scalar
// quarter table. Callers must check scalar_quarter_known first.
int32_t reference_rsin_8007a060(const ReferenceTables& refs,int32_t angle) noexcept;
int32_t reference_rcos_8007a12c(const ReferenceTables& refs,int32_t angle) noexcept;

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
