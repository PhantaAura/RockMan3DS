#pragma once
#include "legends3ds/class60_06.h"
#include "legends3ds/source_audio_queue.h"

namespace l3 {
struct SourceEventPhase5Context;
constexpr size_t source_effect_capacity=64,source_effect_slot_bytes=72;
constexpr uint32_t source_effect_base=0x800a3b40u;
using SourceEffectWork=std::array<uint8_t,source_effect_slot_bytes>;
struct SourceEffectPool {
    std::array<SourceEffectWork,source_effect_capacity> slots{};
    std::array<uint32_t,2> allocated_80098838{},pending_release_800988c8{};
};
enum class SourceEffectStatus : uint8_t {
    SourceBackedPartial,Inactive,InvalidIdentity,InvalidPool,UnknownSubtype,
    UnknownState,UnsupportedParameter,NeedsResourceConsumer,NeedsFloorConsumer,
    StaleResource,NeedsSoundConsumer,NeedsSource128Consumers,
    NeedsField,NeedsPoolBitmap,NeedsReleaseBitmap
};
struct SourceEffectAllocation {
    SourceEffectStatus status=SourceEffectStatus::SourceBackedPartial;
    // Zero is returned only when both original allocation masks are full.
    uint32_t source_identity=0;
};
struct SourceEffect128Resource {
    uint8_t selector_8002f9c4=0;
    uint32_t source_descriptor_14=0;
};
struct SourceEffect128Inputs {
    std::optional<int32_t> floor_8004920c;
    // Exact8002F9C4 result for class80/subtype80/source stack selector.
    // A nonzero pointer alone does not establish this external consumer.
    std::optional<SourceEffect128Resource> resource;
    SourceAudioQueue* audio=nullptr;
};
struct SourceEffectTick {
    SourceEffectStatus status=SourceEffectStatus::Inactive;
    bool committed=false,released=false;
    uint8_t audio_queued=0,audio_dropped=0;
};
struct SourceEffectBatch {
    uint16_t committed=0,held=0,released=0;
    SourceEffectStatus first_hold=SourceEffectStatus::Inactive;
};
std::optional<size_t> source_effect_slot(uint32_t source_identity) noexcept;
// Boot8002E924..8002E97C: zero all72 bytes, install class byte80|index,
// clear both allocation and deferred release masks.
void initialize_source_effect_pool_8002e924(SourceEffectPool&) noexcept;
// Complete80031618 allocator. Reuse clears only offsets2..19, then sets6=3;
// class/flags and all primitive bytes20..71 survive until their producers write.
SourceEffectAllocation allocate_source_effect_80031618(SourceEffectPool&) noexcept;
bool mark_source_effect_release_80031988(SourceEffectPool&,uint32_t) noexcept;
void consume_source_effect_releases_80031b84(SourceEffectPool&) noexcept;
// Exact80110164/8010FDD4 initializer intents, after a real pool allocation.
SourceEffectStatus initialize_child_effect(SourceEffectPool&,
    const Class60_06EffectIntent&) noexcept;
// Original common callback dispatch800173B4: active bit/work6, subtype128/131.
//131's complete five scalar states need no geometry or resource boundary.
//128 admits child parameters2/66 initialization only with keyed floor/resource
// and real audio queue receipts; later geometry/proxy/rotation remains held.
SourceEffectTick tick_source_effect(SourceEffectPool&,uint32_t,
    const SourceEffect128Inputs& = {}) noexcept;
SourceEffectBatch tick_source_effect_batch(SourceEffectPool&,
    const SourceEffect128Inputs& = {}) noexcept;
struct SourceEffectPhase5Batch {
    SourceEffectStatus status=SourceEffectStatus::SourceBackedPartial;
    bool committed=false;
    uint8_t updated=0,released=0;
    std::optional<uint8_t> missing_slot,missing_offset;
};
// Narrow original800173B4 sweep over phase5's known lanes. It visits all64
// records, tests00&06&1, skips subtypes<128, and admits only131 callbacks.
// Preflight of the entire sweep precedes publication. Unknown or unported
// active callbacks hold atomically; no allocation bitmap is read until release.
// Source caller80015E2C belongs to80015DB4 after E0/before80038894. The caller
// must establish that phase boundary; this API supplies no host-frame cadence.
SourceEffectPhase5Batch tick_source_phase5_effects_800173b4(SourceEventPhase5Context&) noexcept;
// Family80 slice of80031AA4 (stores80031B84..80031BB0). Its cooperative caller
// is800155F4, after the selected phase handler returns. Both mask banks must
// be known; this separate atomic operation never edits record bytes.
SourceEffectPhase5Batch consume_source_phase5_effect_releases_80031aa4(SourceEventPhase5Context&) noexcept;
//80029B10 selects primitive+14 only for renderer kind0. Source projection,
// fog and material resolution remain the caller's admitted consumers.
std::optional<Class60_06Primitive> source_effect_primitive(const SourceEffectWork&) noexcept;
} // namespace l3
