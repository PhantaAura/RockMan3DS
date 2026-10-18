#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "legends3ds/player_runtime.h"
#include "legends3ds/source_event_consumer.h"
#include "legends3ds/source_event_phase2.h"
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_event_phase6.h"
#include "legends3ds/source_fade.h"

namespace l3 {
constexpr size_t kSourceEventWorkBytes = 0xa8;
constexpr size_t kSourceEventFlagBytes = 256;
constexpr size_t kSourceEventPollMaximumRecords = 256; // bounded loader policy

// The original record uses a flag ID, an event ID and one unconsumed byte.
// Cooked records contain no pointers or executable instructions.
struct SourceEventPollRecord {
    uint16_t flag = 0;
    uint8_t event = 0, source_byte03 = 0;
};
struct SourceEventPollProfile {
    int8_t source_stage = 0;
    uint32_t decoder = 0; //1 = hash-guarded ST00poll80100598
    std::vector<SourceEventPollRecord> records;
};
// L3EPv1:64-byte header,4-byte records, source digest and payload CRC. The
// supported decoder currently names the original ST00 source explicitly.
SourceEventPollProfile load_source_event_poll_profile(const std::vector<uint8_t>& bytes);

struct SourceEventFlagSnapshot {
    // Semantic ID bit convention:1<<(id&7). Original bytes use80>>(id&7).
    std::array<uint8_t,kSourceEventFlagBytes> values{};
    std::array<uint8_t,kSourceEventFlagBytes> known{};
};
SourceEventFlagSnapshot source_event_flags_from_original(
    const std::array<uint8_t,kSourceEventFlagBytes>& source) noexcept;

struct SourceEventWork {
    // Unknown fields retain original offset labels. A zero-initialized native
    // object is NOT evidence that original NewGame globals were zero.
    std::array<uint8_t,kSourceEventWorkBytes> raw{};
    // One per-byte validity marker. Fresh scene reset proves only +00..03;
    // retained bytes become known through original stores or a full capture.
    std::array<uint8_t,kSourceEventWorkBytes> known{};
    uint8_t active00() const noexcept { return raw[0]; }
    uint8_t event01() const noexcept { return raw[1]; }
    uint8_t phase04() const noexcept { return raw[4]; }
};
SourceEventWork source_event_work_from_original(
    const std::array<uint8_t,kSourceEventWorkBytes>& source) noexcept;
// Exact fresh branch8001D8C0, including80067E18 bank-resident defaults and
//80067E4C flag1216. This is an explicit new-game operation, not a load fallback.
SourceEventFlagSnapshot initialize_source_story_flags_8001d8c0() noexcept;
// Exact8001D950 store; the retained +04..A7 bytes and validity are preserved.
void reset_source_event_work_8001d928(SourceEventWork& work) noexcept;
struct SourceEventAreaPrefixResult {
    bool committed = false;
    std::optional<uint32_t> store_8009888c;
};
// Only sourceST00 area0/1/2 branch80100460..801004F0. Original geometry,
// actor/camera/audio initialization outside this prefix is not acknowledged.
// Player mirror is explicitly supplied with known fields by its owning system.
SourceEventAreaPrefixResult apply_source_st00_area_event_prefix(
    int8_t source_stage,int8_t source_area,
    SourceEventFlagSnapshot& flags,L3PlayerCompatWork& player) noexcept;
struct SourceEventInputs {
    // Views outlive this synchronous step. No inferred all-clear flag bank.
    const SourceEventFlagSnapshot* flags = nullptr;
    std::optional<int8_t> stage_800c1b60;
    std::optional<int8_t> transition_status_800c356c;
    std::optional<int16_t> player_hp_800b521e;
    // The inherited01415research address800A8A72 was incorrect. Original
    // event1 actually reads80098A72, proved with the untouched instruction.
    std::optional<int16_t> busy_80098a72;
    L3PlayerCompatWork* player_work = nullptr;
    SourceEventPlayerKnown* player_known = nullptr;
    SourceEventDriverContext* driver = nullptr;
    SourceEventDriverIntentQueue* driver_intents = nullptr;
    std::optional<uint8_t> byte_800c1b93;
    SourceEventPhase2Context* phase2_context = nullptr;
    const SourceEventPhase2Profile* phase2_profile = nullptr;
    SourceEventCommandQueue* command_queue = nullptr;
    SourceEventPhase2IntentQueue* phase2_intents = nullptr;
    // Exact8001D878 raw byte. Its broader command meaning is unrenamed;
    // a caller must own/capture it rather than infer scene completion.
    std::optional<uint8_t> byte_80098ab8;
    SourceFadeContext* fade_context = nullptr;
    const SourceFadeStartupProfile* fade_profile = nullptr;
    SourceFadeSchedulerIntentQueue* fade_intents = nullptr;
    SourceEventPhase5Context* phase5_context = nullptr;
    const SourceEventPhase5Profile* phase5_profile = nullptr;
    const SourceEventPhase5ResourceBinding* phase5_resources = nullptr;
    SourceEventPhase6Context* phase6_context = nullptr;
    const SourceEventPhase6Profile* phase6_profile = nullptr;
    uint64_t scene_overlay_serial = 0;
    const SourceEventHandoffInputs* handoff = nullptr;
    const SourceEventRandomEffectReceipts* random_callback_receipts = nullptr;
    const SourceEventRandomEffectReceiptOwner* random_callback_owner = nullptr;
    const SourceEventCandidateRng* candidate_rng = nullptr;
};

enum class SourceEventStepStatus : uint8_t {
    SourceBackedPartial,
    NeedsInitialWork,
    NeedsTransitionStatus,
    NeedsStage,
    NeedsPollProfile,
    UnsupportedStageProfile,
    NeedsInitialStoryFlags,
    NeedsPlayerHealth,
    NeedsBusyHalfword,
    UnknownHandler,
    UnknownPhase,
    NeedsPlayerConsumer80042238,
    NeedsSourceConsumer
};
struct SourceEventConsumerRequest {
    uint32_t source_function = 0;
    uint8_t event = 0, phase = 0;
};
struct SourceEventStepResult {
    SourceEventStepStatus status = SourceEventStepStatus::NeedsInitialWork;
    bool poll_attempted = false, initializer_attempted = false, initialization_prepared = false;
    bool transition_pending = false, source_busy = false;
    bool handler_selected = false, committed = false;
    std::optional<uint16_t> selected_flag;
    std::optional<uint8_t> selected_event;
    std::optional<SourceEventConsumerRequest> request;
    std::optional<SourceEventPlayerConsumerStatus> player_consumer_status;
    std::optional<SourceEventPhase2Status> phase2_consumer_status;
    std::optional<SourceFadeStartupStatus> fade_consumer_status;
    std::optional<SourceEventPhase5Status> phase5_consumer_status;
    std::optional<SourceEventPhase6Status> phase6_consumer_status;
    uint32_t script_callback = 0;
    uint8_t script_actor_construction_count = 0,script_resource_binding_count = 0;
    bool pending_fade_scheduler_intent = false;
    uint8_t driver_receipt_count = 0;
    uint8_t sound_receipt_count = 0,command_receipt_count = 0;
    uint8_t random_callback_consumed_count = 0;
    std::optional<SourceEventRandomEffectStatus> random_callback_status;
};

// Original8001D990/ST00poll80100598/init8001DDE4 and event1phase0.
// A newly initialized event dispatches in this SAME original update. Phase4
// countdown branches with a nonzero decremented word are also source-backed.
// Missing globals or an excluded consumer preserve the complete work snapshot.
// Phase1 consumes the source player/logical driver only when explicit owners
// are supplied. Both native driver receipts are required for its phase advance.
// Phase2 consumes explicit sound/command owners and owner-local scalar lookup.
// Later command execution/message/transition operations stay explicit holds.
// Phase4 expiry publishes phase5 only after the exact fade startup no-op or
// constructor plus explicitly pending scheduler intent is admitted.
// Phase5 constructs bounded source-owned scripted records and advances to6;
// rendering, the callback table and command interpreters remain separate owners.
// Phase6 may finish the exact first callback once, retaining a continuation at
//8001EC0C. This does not acknowledge a full scene tick or its camera/timeline.
// No flags are set/cleared and no player/camera ownership is manufactured.
SourceEventStepResult step_source_event_pipeline(
    std::optional<SourceEventWork>& work,
    const SourceEventPollProfile* profile,const SourceEventInputs& inputs) noexcept;
} // namespace l3
