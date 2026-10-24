#pragma once
#include "legends3ds/class60_06.h"
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/reference_tables.h"

namespace l3 {
struct SourceEventPhase5Context;
class SourceRngService;
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
    NeedsField,NeedsPoolBitmap,NeedsReleaseBitmap,NeedsReferences
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
// 0x8004920C at a point (x, y, z): the native map query owner supplies it.
using SourceEffectFloorQuery=std::optional<int32_t>(*)(const void* user,
    int16_t x,int16_t y,int16_t z) noexcept;
struct SourceEffect128Inputs {
    // Explicit receipts (differential fixtures). When absent, subtype 0x80
    // state 0 asks floor_query and resolves 0x8002F9C4 in ebd itself.
    std::optional<int32_t> floor_8004920c;
    // Exact8002F9C4 result for class80/subtype80/source stack selector.
    // A nonzero pointer alone does not establish this external consumer.
    std::optional<SourceEffect128Resource> resource;
    SourceEffectFloorQuery floor_query=nullptr;
    const void* floor_user=nullptr;
    // Stage EBD loaded at 0x8016C000 (the 0x8002FA38 row list).
    const EbdDirectory* ebd=nullptr;
    // Packed RotMatrixY table (0x80092EC4) for the subtype 0x80 state 1 matrix.
    const ReferenceTables* references=nullptr;
    SourceAudioQueue* audio=nullptr;
    // Subtype 0x82 (Buster impact, 0x80044AB0): state 0 calls rand() at RA
    // 0x80044B50 on the shared stream; step timers 0x8008A240 (cooked L3BP/2).
    SourceRngService* rng=nullptr;
    uint64_t rng_frame=0,source_generation=0;
    std::optional<std::array<uint8_t,8>> impact82_timers_8008a240;
};
struct SourceEffectTick {
    SourceEffectStatus status=SourceEffectStatus::Inactive;
    bool committed=false,released=false;
    uint8_t audio_queued=0,audio_dropped=0;
    // Subtype 0x80 state 1 registers a 0x800341BC hit proxy (radius +0x3E/2)
    // every frame. Not ported: reported here so the omission stays visible.
    bool proxy_800341bc_skipped=false;
};
struct SourceEffectBatch {
    uint16_t committed=0,held=0,released=0,proxies_skipped=0;
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
// 128 (0x80043F34, states 0x8008A208): init 0x80043F70 for every parameter
// except bit 0x20 (helper children) and 0xFE; grow/fade 0x8004441C; release.
// State 0 needs a floor receipt or query (probe path only), the 0x8002F9C4
// row (receipt or EBD) and the audio queue; state 1 needs the trig table.
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
// 0x8002F9C4 -> 0x8002FA38 for a class-0x80 work: the first EBD row with
// class 0x80, the given subtype and variant. The token is the source pointer
// (load base + resource offset) stored at +0x14.
std::optional<uint32_t> source_effect_resource_8002fa38(const EbdDirectory&,
    uint8_t subtype,uint8_t variant) noexcept;
// Renderer kind 1 (0x8002A160) view of a subtype-0x80 work: +0x14 descriptor
// token, +0x18 matrix (Q12), +0x38 position, +0x40 colour word. Visible only
// while +0 & +6 & 2 and +0 & 0x1C == 4.
struct SourceEffectModelView {
    uint32_t descriptor_14=0;
    std::array<int16_t,9> matrix_18{};
    std::array<int16_t,3> position_38{};
    uint32_t color_40=0;
};
std::optional<SourceEffectModelView> source_effect_model_view(const SourceEffectWork&) noexcept;
// Caller-side stores of the tutorial producers after a 0x80031618
// allocation: header bytes 0/2/3 and up to four halfwords at lane_offset.
bool write_source_effect_header(SourceEffectPool&,uint32_t identity,uint8_t work00,
    uint8_t subtype02,uint8_t parameter03,size_t lane_offset,
    const std::array<int16_t,4>& lanes,uint8_t lane_count) noexcept;
} // namespace l3
