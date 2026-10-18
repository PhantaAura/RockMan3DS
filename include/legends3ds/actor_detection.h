#pragma once
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

// Source work block+34C passed to800364B0. Offset names deliberately do not
// invent ownership/meaning for global player state or detection profile data.
struct ActorDetectionWork {
    std::array<int16_t, 3> origin_34c{};
    uint8_t profile_352 = 0;
    uint8_t phase_353 = 0;
    std::array<int16_t, 3> vector_354{};
    uint8_t countdown_35a = 0;
    int8_t offset_35b = -6;
};
struct ActorDetectionProfile {
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0;
};

// Phase1's normalization/interpolation and two original segment queries are
// separate owners. Values here must be produced by those proved helpers; the
// reducer never manufactures a direction, line-of-sight result or GTE output.
struct ActorDetectionPhase1 {
    std::array<int16_t, 3> normalized_8007a694{};
    std::array<int16_t, 3> interpolated_80033f04{};
    int32_t segment0_80049c54 = 0;
    int32_t segment1_80049c54 = 0;
};
struct ActorDetectionInputs {
    uint32_t distance_work80 = 0;
    std::array<int16_t, 3> actor_raw{};
    std::array<int16_t, 3> player_raw{};
    uint16_t heading56 = 0;
    std::optional<ActorDetectionProfile> profile;
    const ReferenceTables* reference = nullptr;
    std::optional<ActorDetectionPhase1> phase1;
    std::optional<int16_t> global_800bbd96;
    std::optional<uint8_t> scratch_1f800075;
};
enum class ActorDetectionStatus {
    SourceBackedPartial,
    NeedsProfile,
    NeedsReference,
    NeedsPhase1Helpers,
    NeedsPlayerGlobal,
    NeedsScratchMode,
};
struct ActorDetectionResult {
    ActorDetectionStatus status = ActorDetectionStatus::NeedsProfile;
    bool detected = false;
    bool notify_global_800bbd84 = false; // Original8005D4C0 writes byte1.
    bool sound156_intent = false; // Original call only on phase1 admission/mode2.
};

//8005C258 clears this subsystem;8005C2B8 decrements positive signed16
// countdowns once per owner update. Setter/global mode ownership stays explicit.
struct ActorDetectionGlobals {
    int16_t halfword_800bbd94 = 0;
    int16_t halfword_800bbd96 = 0;
    uint8_t byte_800bbd84 = 0;
};
ActorDetectionGlobals initialize_actor_detection_globals_8005c258() noexcept;
void tick_actor_detection_globals_8005c2b8(ActorDetectionGlobals& globals) noexcept;
void set_actor_detection_countdown_8005ebb4(ActorDetectionGlobals& globals,
                                           uint16_t bits) noexcept;
void notify_actor_detection_8005d4c0(ActorDetectionGlobals& globals) noexcept;

// Exact sign of80030628's MAC3 local Z (mask10) for a pure original heading
// matrix. Signed16 coordinate differences wrap before Q12 multiplication.
// Uses supplied owner trig table; no floating-point trigonometry or assets.
std::optional<bool> actor_detection_behind_80030628(
    const std::array<int16_t, 3>& actor_raw,
    const std::array<int16_t, 3>& player_raw, uint16_t heading56,
    const ReferenceTables* reference) noexcept;

// Strong hold: missing original-owned inputs leave work byte-for-byte unchanged.
// This is the source detector's bounded scalar state machine, not a complete
// enemy callback, a modern vision approximation, or measured retail parity.
ActorDetectionResult step_actor_detection_800364b0(
    ActorDetectionWork& work, const ActorDetectionInputs& input) noexcept;

} // namespace l3
