#pragma once
#include "legends3ds/source_rng_service.h"
#include "legends3ds/source_event_phase5.h"

namespace l3 {
constexpr uint32_t kSourceEventRandomEffectCallback=0x80100d6c;
constexpr size_t kSourceEventRandomEffectMaximumCalls=6;
enum class SourceEventRandomEffectStatus : uint8_t {
    Ready,Consumed,NeedsEventWork,WrongEventPhase,NeedsActorBinding,
    NeedsActorField,NeedsPoolBitmap,NeedsPoolHeader,NeedsRandomOwner,
    WrongReceiptCount,WrongReceiptContext,InvalidReceipt,StaleReceipt
};
struct SourceEventRandomEffectPlan {
    SourceEventRandomEffectStatus status=SourceEventRandomEffectStatus::NeedsEventWork;
    uint32_t frame=0,source_actor=0;
    uint8_t allocation_count=0,rng_calls=0;
    std::array<uint8_t,2> slots{{255,255}};
    std::array<uint32_t,6> caller_ras{};
    std::array<uint16_t,3> source_position{};
    std::array<std::optional<uint32_t>,2> bitmap_before{},bitmap_after{};
    bool clears_control_80122424=false;
    std::optional<uint16_t> missing_offset;
};
// Original signed frame<31 includes negative raw words. Two source allocator
// attempts yield0/1/2 records and exactly0/3/6 random calls, respectively.
// A full pool is a source null-allocation result, not a capacity failure.
SourceEventRandomEffectPlan plan_source_event_random_effect_80100d6c(
    const SourceEventWork&,const SourceEventPhase5Context&) noexcept;
struct SourceEventRandomEffectReceipt {
    uint32_t caller_ra=0,return_v0=0;
    uint64_t global_ordinal=0;
};
struct SourceEventRandomEffectReceipts {
    const void* owner=nullptr;
    uint64_t generation=0;
    bool completed=false;
    uint32_t callback=0,source_work=0,event_frame=0,source_actor=0;
    uint8_t count=0;
    std::array<SourceEventRandomEffectReceipt,6> calls{};
};
struct SourceEventRandomEffectReceiptOwner {
    const void* owner=nullptr;
    uint64_t generation=0;
};
struct SourceEventRandomEffectResult {
    SourceEventRandomEffectStatus status=SourceEventRandomEffectStatus::NeedsEventWork;
    bool committed=false;
    uint8_t committed_call_count=0,allocated_count=0;
    std::array<uint8_t,2> allocated_slots{{255,255}};
    // Explicit source scalar-store receipt. The enclosing prefix publishes it
    // only with its complete transaction; this module owns no hidden global.
    std::optional<uint8_t> control_80122424;
    std::optional<uint16_t> missing_offset;
};
// Pure bounded scalar callback consumer: no RNG generator, seed, receipt cursor,
// allocation, draw/resource inference or main-loop chronology. It validates
// completed explicit original-result receipts and the current external owner
// generation, then atomically publishes at most two72-byte family80 records.
// Caller establishes active resource residency and commits the RNG transaction
// only with committed_call_count and the enclosing callback's scalar stores.
SourceEventRandomEffectResult consume_source_event_random_effect_80100d6c(
    const SourceEventWork&,SourceEventPhase5Context&,
    const SourceEventRandomEffectReceipts* receipts=nullptr,
    const SourceEventRandomEffectReceiptOwner* current_owner=nullptr) noexcept;
struct SourceEventCandidateRng {
    SourceRngService* service=nullptr;
    uint64_t source_generation=0;
    SourceRngUpdateContext update{0,0,SourceRngUpdatePhase::SceneEventCallback};
};
// Explicit native candidate adapter. The original planner decides call count
// and allocation; the same shared observed-profile service stages all calls.
// No missing recorded receipt is replaced, no initial counter is supplied,
// and this adapter establishes neither competing-consumer nor retail parity.
SourceEventRandomEffectResult consume_source_event_random_effect_candidate(
    const SourceEventWork&,SourceEventPhase5Context&,const SourceEventCandidateRng&) noexcept;
} // namespace l3
