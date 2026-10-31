#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
template<size_t Size> struct SourceFadeBytes {
    std::array<uint8_t,Size> raw{};
    std::array<uint8_t,Size> known{};
};
struct SourceFadeContext {
    SourceFadeBytes<16> work_80098a68;
    std::array<SourceFadeBytes<16>,2> tiles_80098a28;
    std::array<SourceFadeBytes<12>,2> draw_modes_80098a10;
};
struct SourceFadeStartupProfile {
    uint32_t decoder = 0;
    std::optional<uint16_t> duration_80097a44;
    std::optional<uint16_t> step_80097a4c;
    // Only L3FDv2 admits the distinct source type10 duration. Never substitute
    // the type2 duration80097A44 when this owner is absent.
    std::optional<uint16_t> duration_80097a46=std::nullopt;
    // L3FDv3: the whole step table 0x80097A48..0x80097A4E (index type & 3).
    std::optional<std::array<uint16_t,4>> steps_80097a48=std::nullopt;
};
// L3FDv1: 64-byte header, four-byte owner-derived payload. Scalars are not
// embedded in this implementation. The loader validates identity and CRC.
SourceFadeStartupProfile load_source_fade_startup_profile(const std::vector<uint8_t>& bytes);
struct SourceFadeSchedulerIntent {
    uint32_t source_function = 0x80012e10u;
    uint32_t callback = 0x80017e94u;
    uint8_t slot = 2;
    // This is an unconsumed native intent, never a BIOS thread receipt.
};
constexpr size_t kSourceFadeSchedulerIntentCapacity = 8;
struct SourceFadeSchedulerIntentQueue {
    bool initialized = false;
    size_t count = 0;
    std::array<SourceFadeSchedulerIntent,kSourceFadeSchedulerIntentCapacity> entries{};
};
enum class SourceFadeStartupStatus : uint8_t {
    NeedsBusyHalfword, NeedsProfile, UnsupportedProfile, NeedsSchedulerIntentQueue,
    SchedulerIntentQueueFull, SourceBackedBusyNoop, PendingSchedulerIntent
};
struct SourceFadeStartupResult {
    SourceFadeStartupStatus status = SourceFadeStartupStatus::NeedsBusyHalfword;
    bool committed = false;
    bool pending_scheduler_intent = false;
};
// Exact type2 startup only. Retained/unknown packet lanes remain retained.
// busy != 0 is the original no-op. busy == 0 requires both owner scalars and
// storage for the still unconsumed80012E10(slot2,callback80017E94) intent.
// No fade callback, presentation, BIOS thread or completion is fabricated.
SourceFadeStartupResult consume_source_fade_startup_80017c30_type2(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents) noexcept;
// Every type 0x80017C30 sends to 0x80017C74 (mode 0): 0..3 and 8..11.
// Duration 0x80097A44[type >> 3], step 0x80097A48[type & 3]; L3FDv3 only
// for steps other than 0x80097A4C. Other types: UnsupportedProfile.
SourceFadeStartupResult consume_source_fade_startup_80017c30_mode0(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents,uint8_t type) noexcept;
SourceFadeStartupResult consume_source_fade_startup_80017c30_type10(
    SourceFadeContext& context,const SourceFadeStartupProfile* profile,
    SourceFadeSchedulerIntentQueue* intents) noexcept;

// An explicit native owner for original80017E94 ->80017ED0. Acknowledging the
// queue means this bounded consumer owns the callback; it does not mean that a
// PS1 BIOS thread was opened, scheduled, closed or returned a handle.
struct SourceFadeCallbackState {
    bool running = false;
    uint32_t committed_iterations = 0;
};
enum class SourceFadeCallbackStatus : uint8_t {
    NeedsSchedulerIntent, UnsupportedSchedulerIntent, AlreadyRunning,
    NotRunning, NeedsMode, UnsupportedMode, NeedsBusyHalfword,
    NeedsPhase, NeedsIntensity, NeedsStep, NeedsEndpointHoldByte,
    YieldedSourceIteration, TerminatedSourceCallback
};
struct SourceFadeCallbackResult {
    SourceFadeCallbackStatus status = SourceFadeCallbackStatus::NotRunning;
    bool committed = false;
    bool native_scheduler_acknowledged = false;
    bool yield_one_scheduler_pass = false;
    bool source_callback_terminated = false;
    uint32_t source_boundary = 0;
};
// The first iteration executes immediately, as in the original callback.
// endpoint_hold is the explicitly owned original byte1F800001, sampled only at
// a ramp endpoint. It is not a guessed fade duration or a global constant.
// Missing/unsupported inputs preserve context, state and queue atomically.
SourceFadeCallbackResult acknowledge_source_fade_scheduler_intent(
    SourceFadeContext& context,SourceFadeSchedulerIntentQueue& intents,
    SourceFadeCallbackState& state,std::optional<uint8_t> endpoint_hold) noexcept;
// Call once for each explicitly admitted native scheduler pass after a yield.
// This does not establish native-vs-retail frame cadence.
SourceFadeCallbackResult advance_source_fade_callback(
    SourceFadeContext& context,SourceFadeCallbackState& state,
    std::optional<uint8_t> endpoint_hold) noexcept;

enum class SourceFadePresentationStatus : uint8_t {
    NeedsActiveHalfword, NeedsIntensity, NeedsMode, UnsupportedMode,
    NeedsTileFields, NeedsDrawModeFields, NeedsBufferSelector,
    UnsupportedBufferSelector, NeedsOrderingTableTag, NeedsPacketTag,
    SourceInactive, SourceBackedPresentation
};
struct SourceFadePresentation {
    SourceFadePresentationStatus status = SourceFadePresentationStatus::NeedsActiveHalfword;
    bool visible = false;
    uint8_t grayscale = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    // Original E1000140 DR_MODE + semitransparent TILE62 selects destination
    // minus source color. No alpha approximation or GPU quantization claim.
    bool subtract_from_scene = false;
};
// Resolves exact owned type2 packet fields and scalar intensity for a native
// presentation backend. Raw tags/buffer pointers are deliberately not invented.
SourceFadePresentation describe_source_fade_presentation(const SourceFadeContext& context) noexcept;
struct SourceFadePacketInput {
    std::optional<uint8_t> buffer_1f800000;
    std::optional<uint32_t> ordering_table_tag_116;
};
struct SourceFadePacketResult {
    SourceFadePresentationStatus status = SourceFadePresentationStatus::NeedsActiveHalfword;
    bool committed = false;
    std::optional<uint32_t> ordering_table_tag_116;
};
// Exact original80017B24 selected double-buffer packet writes/link order.
// This helper is useful for byte differential evidence; native backends consume
// the description above rather than fabricating PS1 address ownership.
SourceFadePacketResult bind_source_fade_presentation_packets(
    SourceFadeContext& context,const SourceFadePacketInput& input) noexcept;
} // namespace l3
