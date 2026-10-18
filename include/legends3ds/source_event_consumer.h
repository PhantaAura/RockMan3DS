#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "legends3ds/player_runtime.h"

namespace l3 {
struct SourceEventWork;
using SourceEventPlayerKnown = std::array<uint8_t,L3_PLAYER_COMPAT_WORK_BYTES>;
// Exact always-taken zero stores in player initializer8003BE6C, before
// control-profile and fresh/continued branches. All other fields remain owned
// by their native/source initialization; this does not acknowledge the whole
// original player initializer or its later pose/render helpers.
void initialize_source_event_player_fields_8003bee8(L3PlayerCompatWork& player,
    SourceEventPlayerKnown& known) noexcept;
// Explicit fresh branch: original NewGame stores80013C7C byte800C1B92=0;
// then player initializer8003BE6C writes00/06=3 and clears02..05/08..0F.
// Only those original header stores are admitted, before ST00 area prefix.
void initialize_source_event_fresh_player_header_8003be6c(L3PlayerCompatWork& player,
    SourceEventPlayerKnown& known) noexcept;
constexpr size_t kSourceEventVoiceCount=24, kSourceEventVoiceBytes=54;
struct SourceEventVoiceWork {
    std::array<uint8_t,kSourceEventVoiceBytes> raw{},known{};
};
struct SourceEventDriverContext {
    std::optional<uint32_t> lock_800bc050;
    std::optional<uint16_t> voice_800c0bfa;
    std::array<SourceEventVoiceWork,kSourceEventVoiceCount> voices{};
    std::array<std::optional<uint16_t>,2> pending_800c5680{},active_8009ae40{};
};
// These receipts describe execution of the source logical driver call. They
// contain no samples and do not establish native SPU playback or timing.
struct SourceEventDriverIntent {
    uint32_t source_function=0x80071220;
    int16_t voice=0;
    int32_t source_return=0;
    std::array<uint16_t,2> pending{},active{};
};
struct SourceEventDriverIntentQueue {
    std::array<SourceEventDriverIntent,24> records{};
    size_t count=0;
    bool initialized=false;
};
// Only the exact lock store8006B580 and mask reset800729D0..EF are applied.
// Retained voice fields/currentvoice remain unknown. The calling native owner
// explicitly admits this logical driver initialization; hardware is excluded.
void initialize_source_event_driver_fields(SourceEventDriverContext& driver,
    SourceEventDriverIntentQueue& queue) noexcept;
// The hash-guarded original executable LOAD image contains this halfword=0.
// This is an explicit cold-boot premise, never a scene-reset/NewGame fallback.
int16_t initialize_source_event_busy_from_executable_boot() noexcept;

enum class SourceEventPlayerConsumerStatus : uint8_t {
    Consumed,NotReady,NeedsEventWork,WrongEventPhase,NeedsPlayerField,
    NeedsSourceByte800c1b93,NeedsDriverField,NeedsIntentQueue,
    MalformedIntentQueue,IntentQueueFull
};
struct SourceEventPlayerConsumerResult {
    SourceEventPlayerConsumerStatus status=SourceEventPlayerConsumerStatus::NeedsEventWork;
    bool committed=false,player_ready=false;
    uint8_t receipt_count=0;
    std::optional<uint16_t> missing_player_offset;
};
// Original player predicate/mutations, calls22/23 through8001B2F0→80071220→
//800742C8, then original ST00 player00&FC and phase increment. Complete
// transaction holds on missing premises or queue capacity. Both source calls
// must receive native logical driver receipts before phase1 can become2.
SourceEventPlayerConsumerResult consume_source_event1_phase1_80042238(
    SourceEventWork& work,L3PlayerCompatWork& player,SourceEventPlayerKnown& known,
    SourceEventDriverContext& driver,SourceEventDriverIntentQueue& queue,
    std::optional<uint8_t> byte_800c1b93={}) noexcept;
} // namespace l3
