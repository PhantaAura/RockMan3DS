#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "legends3ds/source_event_consumer.h"

namespace l3 {
struct SourceEventWork;
constexpr size_t kSourceEventSoundDescriptorCount=4;
constexpr size_t kSourceEventSequenceBytes=176;
constexpr size_t kSourceEventCommandCapacity=16;
// Scalar request lookup supplied by a hash-guarded owner-local package. These
// source words retain their original labels; no message/cutscene meaning is
// assigned. The native executable contains no retail lookup table payload.
struct SourceEventPhase2Profile {
    uint32_t request_8001d58c=0,id=0;
    std::array<uint32_t,2> pair_words{};
    bool initialized=false;
};
struct SourceEventSoundDescriptor {
    std::array<uint8_t,8> raw{},known{};
};
struct SourceEventSequenceOwner {
    std::optional<int16_t> id;
    std::optional<uint8_t> program;
    // Explicit owner of *(800C4CF0+id*4)+program*176. A captured/keyed view
    // supplies identity; the native runtime never dereferences PS-X pointers.
    std::array<uint8_t,kSourceEventSequenceBytes> raw{},known{};
};
struct SourceEventPhase2Context {
    std::array<SourceEventSoundDescriptor,kSourceEventSoundDescriptorCount> descriptors{};
    std::array<SourceEventSequenceOwner,kSourceEventSoundDescriptorCount> sequences{};
    std::optional<uint8_t> voice_count_800bc820;
    std::optional<int16_t> halfword_800b5168;
};
struct SourceEventCommandRecord {
    std::array<uint8_t,16> raw{},known{};
};
struct SourceEventCommandQueue {
    std::array<SourceEventCommandRecord,kSourceEventCommandCapacity> records{};
    size_t count=0,current_index=0;
    bool initialized=false;
    std::optional<uint32_t> request_800989b8,word_800989bc,word_800989c0;
    std::optional<uint32_t> counter_80098828,word_80098968,word_800989c8,phase_800989c4;
    std::optional<uint8_t> byte_800988d0,byte_80098964;
};
struct SourceEventPhase2SoundIntent {
    uint32_t source_function=0;
    int16_t id=0;
    uint8_t program=0,stop_receipt_count=0;
};
struct SourceEventPhase2IntentQueue {
    std::array<SourceEventPhase2SoundIntent,4> records{};
    size_t count=0;
    bool initialized=false;
};
// Explicit one-time cold LOAD admission from a hash-guarded owner package.
// This must not be called for scene reset or continued/load-game sessions.
void initialize_source_event_phase2_from_executable_boot(SourceEventPhase2Context& context,
    SourceEventCommandQueue& queue,const std::array<uint8_t,32>& descriptor_load,
    const std::array<uint8_t,256>& queue_load) noexcept;
// Exact logical slices only. Descriptor reset writes +02=FF; command reset
// writes the sixteen type words only and proves current/write indices zero.
// Other record bytes and their validity survive both source resets.
void initialize_source_event_phase2_descriptors_80019f0c(SourceEventPhase2Context& context) noexcept;
void initialize_source_event_command_queue_8001b44c(SourceEventCommandQueue& queue) noexcept;

enum class SourceEventPhase2Status : uint8_t {
    Consumed,NeedsEventWork,WrongEventPhase,NeedsDescriptor,NeedsSequenceOwner,
    NeedsSequenceField,NeedsVoiceCount,UnsupportedVoiceCount,NeedsDriverField,
    NeedsIntentQueue,MalformedIntentQueue,IntentQueueFull,NeedsProfile,
    UnsupportedRequest,NeedsCommandQueue,MalformedCommandQueue,CommandQueueFull,
    NeedsCurrentCommandField
};
struct SourceEventPhase2Result {
    SourceEventPhase2Status status=SourceEventPhase2Status::NeedsEventWork;
    bool committed=false;
    uint8_t sound_receipt_count=0,driver_receipt_count=0,command_receipt_count=0;
    std::optional<uint16_t> missing_offset;
};
// Original ST00 event1phase2:8001B33C→80070114/8007013C→8006FF90,
// then8001D58C(20000003) and phase increment. Missing owners/capacity preserve
// event, sound state, voices, receipts, command records and scalar globals.
// Logical receipts do not prove SPU samples, CD callbacks or playback timing.
SourceEventPhase2Result consume_source_event1_phase2_8001b33c(SourceEventWork& work,
    SourceEventPhase2Context& context,const SourceEventPhase2Profile& profile,
    SourceEventCommandQueue& commands,SourceEventPhase2IntentQueue* sound_intents=nullptr,
    SourceEventDriverContext* driver=nullptr,SourceEventDriverIntentQueue* driver_intents=nullptr) noexcept;
// The original current-record type4 dispatch is8001B89C; its CD/control
// callbacks are not acknowledged. A bounded request carries the actual packet
// and function for the owning consumer and never creates raw completionbit20.
struct SourceEventCommandRequest {
    uint32_t source_function=0;
    std::array<uint32_t,4> words{};
};
std::optional<SourceEventCommandRequest> source_event_command_front_request(
    const SourceEventCommandQueue& queue) noexcept;
} // namespace l3
