#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace l3 {
using SourceRngDigest = std::array<uint8_t, 32>;
constexpr size_t kSourceRngProfileBytes = 256;
// The proved effect-4 burst allocates at most24 children with five calls each.
// Class03 still permits only its sixteen distinct source slots per update.
constexpr size_t kSourceRngUpdateMaximumCalls = 120;
constexpr uint32_t kSourceClass03RngCallerRa = 0x801022f8;
// ST00 subtype 0x1F callback 0x801062EC: jal 0x8007FE50 at 0x80106370.
constexpr uint32_t kSourceClass1FRngCallerRa = 0x80106378;
// ST00 subtype 0x19 callback 0x80104864: jal 0x8007FE50 at 0x801048E8.
constexpr uint32_t kSourceClass19RngCallerRa = 0x801048f0;

enum class SourceRngCaptureScope : uint32_t { WholeBoundedCapture = 1, CompletedCapturePrefix = 2 };
enum SourceRngWarning : uint32_t {
    SourceRngUnmodeledConsumers = 1, SourceRngSelectedReferenceNotRetail = 2,
    SourceRngCaptureIncomplete = 4, SourceRngObserverFailed = 8,
    SourceRngPendingCallExcluded = 16, SourceRngPartialLineExcluded = 32
};
// Only the selected, hash-guarded OpenBIOS recurrence is SOURCE_BACKED.
// The profile's observed game-entry seed is neither a retail default nor proof
// of native callback/competing-consumer chronology. Sidecars stay owner-local.
struct SourceRngProfile {
    SourceRngDigest bios_sha256{}, exe_sha256{}, overlay_sha256{};
    SourceRngDigest trace_sha256{}, validated_prefix_sha256{};
    SourceRngCaptureScope scope = SourceRngCaptureScope::CompletedCapturePrefix;
    uint32_t warnings = 0;
    uint32_t origin_seed32 = 0, origin_rand_ordinal = 0, origin_vsync = 0;
    uint32_t origin_game_entry_count = 0;
    uint32_t actual_rand_returns = 0, actual_reseed_returns = 0;
    uint32_t trace_rows = 0, validated_prefix_rows = 0, excluded_rows = 0;
    uint32_t captured_bytes = 0, validated_prefix_bytes = 0;
    uint32_t final_rand_ordinal = 0, observed_consumer_count = 0;
};
enum class SourceRngMode : uint8_t { Disabled, CandidateOpenBios };
enum class SourceRngStatus : uint8_t {
    Ready, Disabled, NeedsObservedProfile, InvalidProfile, InvalidContext,
    DuplicateCallback, UpdateFull, OrdinalExhausted, StaleTransaction
};
enum class SourceRngUpdatePhase : uint8_t { Class03Callbacks = 1, Effect2Callback = 2, SceneEventCallback = 3, PropLiftCallback = 4, Effect34Callback = 5, RewardPickupCallback = 6, E0ObjectCallback = 7 };
struct SourceRngUpdateContext {
    uint64_t native_frame = 0, native_update_ordinal = 0;
    SourceRngUpdatePhase phase = SourceRngUpdatePhase::Class03Callbacks;
};
struct SourceRngClass03Context {
    uint32_t caller_ra = 0, work = 0;
    uint8_t slot = 255, state08 = 0, action09 = 0, phase0a = 0;
};
// One original subtype-2 callback's three calls, in their source order.
// A separate transaction is required for every parent; no private seed exists.
struct SourceRngEffect2Context {
    uint32_t work = 0;
    uint64_t source_generation = 0;
    uint8_t slot = 255, call_index = 0;
};
struct SourceRngSceneEventContext {
    uint64_t source_generation = 0;
    uint8_t call_index = 0, call_count = 0;
};
struct SourceRngPropLiftContext {
    uint32_t work=0;
    uint64_t source_generation=0;
    uint8_t slot=255,call_index=0;
};
struct SourceRngEffect34Context {
    uint32_t work=0,callback=0;
    uint64_t source_generation=0;
    uint8_t slot=255,call_index=0,call_count=0;
};
// Enemy-reward pickups: 0x80046B34 expansion (work 0 for its first draw) and
// the subtype-0x87 states. caller_ra must be one of the seven original rand
// call sites (0x80046BAC/BF0/C30/DD8/E18, 0x80046480, 0x800468EC).
// ST00 class E0 object callbacks (pool 0x800BF118, stride 0x50). Admitted:
// E0/05 0x80113FEC (jal 0x8007FE50 at 0x80114018).
constexpr uint32_t kSourceE005RngCallerRa = 0x80114020;
// E0/08 target state 1 (0x801161D4): blink reload draw when +9 reaches 0.
constexpr uint32_t kSourceE008BlinkRngCallerRa = 0x80116244;
// ST00 E0 falling-block/slide states and their debris spawners (opening
// scene): rand() return addresses 0x801133E4 .. 0x80113E54.
constexpr std::array<uint32_t,10> kSourceE0BlockRngCallerRas{{0x801133e4u,0x8011367cu,0x80113ad8u,
    0x80113cc8u,0x80113cf4u,0x80113d0cu,0x80113d2cu,0x80113d94u,0x80113dc0u,0x80113e54u}};
struct SourceRngE0Context {
    uint32_t caller_ra = 0, work = 0;
    uint8_t slot = 255;
};
struct SourceRngRewardContext {
    uint32_t caller_ra=0,work=0;
    uint64_t source_generation=0;
    uint8_t slot=255;
};
struct SourceRngCallRecord {
    SourceRngUpdateContext update{};
    SourceRngClass03Context actor{};
    uint64_t native_global_ordinal = 0;
    uint32_t seed_before32 = 0, seed_after32 = 0;
    uint16_t sample32a = 0;
    uint64_t source_generation = 0;
};
struct SourceRngStagedSample {
    SourceRngStatus status = SourceRngStatus::NeedsObservedProfile;
    uint16_t sample32a = 0;
    uint64_t native_global_ordinal = 0;
};

class SourceRngService {
public:
    bool admitted() const noexcept { return admitted_; }
    uint32_t seed32() const noexcept { return seed_; }
    uint64_t native_global_ordinal() const noexcept { return ordinal_; }
    uint64_t generation() const noexcept { return generation_; }
    const SourceRngProfile& profile() const noexcept { return profile_; }
    size_t last_call_count() const noexcept { return last_count_; }
    const std::array<SourceRngCallRecord, kSourceRngUpdateMaximumCalls>& last_calls() const noexcept { return last_calls_; }
private:
    SourceRngProfile profile_{};
    uint32_t seed_ = 0;
    uint64_t ordinal_ = 0, generation_ = 0;
    bool admitted_ = false;
    std::array<SourceRngCallRecord, kSourceRngUpdateMaximumCalls> last_calls_{};
    size_t last_count_ = 0;
    friend struct SourceRngAccess;
};
// One shared service across all actors/areas. A transaction may cover one
// callback or an entirely staged fixed update. Roll it back only with the same
// actor-state scope: never undo earlier committed RNG after publishing actors.
// Per-callback diagnostic hold rollback is CANDIDATE native scheduling.
class SourceRngUpdateTransaction {
public:
    bool active() const noexcept { return active_; }
    size_t call_count() const noexcept { return count_; }
    const std::array<SourceRngCallRecord, kSourceRngUpdateMaximumCalls>& calls() const noexcept { return calls_; }
private:
    const SourceRngService* owner_ = nullptr;
    uint64_t generation_ = 0, ordinal_ = 0;
    uint32_t seed_ = 0, used_slots_ = 0;
    bool active_ = false;
    SourceRngUpdateContext context_{};
    std::array<SourceRngCallRecord, kSourceRngUpdateMaximumCalls> calls_{};
    size_t count_ = 0;
    uint8_t expected_calls_ = 0;
    friend struct SourceRngAccess;
};

// Decodes one canonical 256-byte L3RG profile without allocating or throwing.
SourceRngStatus load_l3source_rng_profile(const uint8_t*, size_t, SourceRngProfile&) noexcept;
SourceRngStatus install_source_rng_service(SourceRngService&, const SourceRngProfile&, SourceRngMode) noexcept;
SourceRngStatus begin_source_rng_update(const SourceRngService&, SourceRngUpdateTransaction&, const SourceRngUpdateContext&) noexcept;
// Must be called once for EVERY admitted original class03 callback, including
// bodies that do not use sample32A. Original pool slot/work identity is required;
// an object-record source_index is not a source pool slot.
SourceRngStagedSample stage_source_rng_class03(SourceRngUpdateTransaction&, const SourceRngClass03Context&) noexcept;
SourceRngStagedSample stage_source_rng_effect2(SourceRngUpdateTransaction&, const SourceRngEffect2Context&) noexcept;
SourceRngStagedSample stage_source_rng_scene_event(SourceRngUpdateTransaction&, const SourceRngSceneEventContext&) noexcept;
SourceRngStagedSample stage_source_rng_prop_lift(SourceRngUpdateTransaction&, const SourceRngPropLiftContext&) noexcept;
SourceRngStagedSample stage_source_rng_effect34(SourceRngUpdateTransaction&, const SourceRngEffect34Context&) noexcept;
SourceRngStagedSample stage_source_rng_reward(SourceRngUpdateTransaction&, const SourceRngRewardContext&) noexcept;
SourceRngStagedSample stage_source_rng_e0(SourceRngUpdateTransaction&, const SourceRngE0Context&) noexcept;
SourceRngStatus commit_source_rng_update(SourceRngService&, SourceRngUpdateTransaction&) noexcept;
void rollback_source_rng_update(SourceRngUpdateTransaction&) noexcept;
} // namespace l3
