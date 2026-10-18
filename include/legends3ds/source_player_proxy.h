#pragma once

#include "legends3ds/source_hit_proxy.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {

// Owner-bound PS1 state, independent of the renderer's floating-point camera.
// Only the reached RTPS(sf=1,lm=0) command is supported. Timing and the other
// commands are outside this bounded native prerequisite.
struct SourceGteRtpsState {
    // Raw storage agrees with original-emulator snapshots. Register15's raw
    // backing is retained; its architectural read mirrors14 through the API.
    std::array<std::uint32_t,32> data{};
    std::array<std::uint32_t,32> control{};
    bool matrix_translation_bound = false;
    bool projection_controls_bound = false; // resident OFX,OFY,H,DQA,DQB
};
enum class SourcePlayerProxyStatus : std::uint8_t {
    SourceBackedPartial, NeedsMatrixTranslation, NeedsProjectionControls,
    NeedsMask, NeedsOwnerPoint, NeedsBoundedArithmetic
};
struct SourceGteProjectionResult {
    SourcePlayerProxyStatus status = SourcePlayerProxyStatus::NeedsMatrixTranslation;
    std::uint32_t screen_xy = 0;
    std::uint16_t depth = 0;
    std::uint32_t reciprocal = 0;
    std::uint32_t flags = 0;
};

// Hardware UNR division: intentionally differs from ordinary rounded division.
// Result has no heap allocation; bit17 is true only for H>=2*SZ (including zero).
std::uint32_t source_gte_unr_divide(std::uint16_t h,std::uint16_t sz,
                                  bool& divide_overflow) noexcept;

// Admission excludes internal 44-bit MAC overflow. In the admitted domain,
// data/FLAG outputs include FIFO, IR, MAC, saturation and hardware UNR results.
// On any unavailable premise or excluded arithmetic, all state is preserved.
SourceGteProjectionResult source_gte_rtps_0180001(SourceGteRtpsState& state) noexcept;

// Ordinary GTE CPU reads, including signed/unsigned narrowing and SXYP mirror.
// Color-conversion registers28/29 are outside this slice and remain unavailable.
std::optional<std::uint32_t> source_gte_read_data(
    const SourceGteRtpsState& state,std::size_t index) noexcept;

struct SourcePlayerLeaseInputs {
    std::optional<std::array<std::uint32_t,8>> scratch_camera_1f800008;
    std::optional<std::uint32_t> mask_80098798;
    std::array<SourceHitProxyOwnerPoint*,24> owners_800be478{};
};
struct SourcePlayerLeaseResult {
    SourcePlayerProxyStatus status = SourcePlayerProxyStatus::NeedsMatrixTranslation;
    std::uint32_t consumed_mask = 0;
    std::size_t projected = 0;
    std::array<std::uint32_t,24> screen_xy{};
    std::array<std::uint16_t,24> depth{};
};

// Original80035508: load scratch camera into controls0..7, project high-mask
// leases in source index order, clear owner's BC/C0. The original screen/depth
// stores are local scratch outputs, not target selection or damage writes.
// The native adapter stages all24 leases and holds the entire mutation if a
// required owner/control/arithmetic premise is unavailable.
SourcePlayerLeaseResult source_player_consume_leases_80035508(
    SourceGteRtpsState& state,const SourcePlayerLeaseInputs& input) noexcept;

} // namespace l3
