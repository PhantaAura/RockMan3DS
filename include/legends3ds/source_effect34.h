#pragma once
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_rng_service.h"

namespace l3 {
constexpr uint32_t source_effect3_callback=0x801172fcu;
constexpr uint32_t source_effect4_callback=0x80117558u;
constexpr uint32_t source_effect3_sequence=0x8011cda8u;
constexpr uint32_t source_effect4_rows=0x8011cdccu;
constexpr size_t source_effect34_maximum_children=24;
constexpr size_t source_effect34_maximum_rng=120;
// Neutral scalar control rows retained by the checked overlay package. This
// object grants no asset provenance by itself and embeds no original table.
struct SourceEffect34ControlReceipt {
    uint32_t sequence3_identity=0,rows4_identity=0;
    std::array<uint16_t,11> sequence3{};
    std::array<std::array<uint8_t,4>,5> rows4{};
    bool admitted=false;
};
struct SourceEffect34RngReceipt {
    uint32_t callback_identity=0,parent_identity=0,child_identity=0;
    uint32_t return_address=0,v0=0;
    uint64_t global_ordinal=0,source_generation=0;
    std::array<uint16_t,3> parent_xyz1c{};
    uint8_t parent_clock09=0,parent_parameter03=0,parent_mode0d=0,parent_tag0f=0;
};
struct SourceEffect34Inputs {
    const SourceEffect34ControlReceipt* controls=nullptr;
    const SourceEffect34RngReceipt* rng=nullptr;
    size_t rng_count=0;
    uint64_t source_generation=0;
};
enum class SourceEffect34Status : uint8_t {
    SourceBackedPartial,Inactive,InvalidIdentity,UnknownCallback,UnknownDispatch,
    NeedsField,NeedsPoolBitmap,NeedsReleaseBitmap,InvalidPool,
    NeedsControls,ControlContextMismatch,NeedsRngReceipts,RngContextMismatch
};
struct SourceEffect34Result {
    SourceEffect34Status status=SourceEffect34Status::SourceBackedPartial;
    bool committed=false,released=false,source_null_allocation=false;
    uint8_t allocated_count=0,rng_required=0,rng_consumed=0;
    std::array<uint32_t,24> child_identities{};
    std::array<uint32_t,120> rng_return_addresses{};
    std::optional<uint8_t> missing_slot,missing_offset;
};
// Exact scalar callbacks3/4: bounded source allocation, initialization,
// child motion/control/lifetime and deferred release. No generator, provider
// commit, renderer, hot-loop heap or callback scheduling exists here. Original
// null-allocation branches are preserved. All required owners/lanes/receipts
// preflight before parent/children/masks are published together.
SourceEffect34Result tick_source_effect34(SourceEventPhase5Context&,
    uint32_t source_identity,uint32_t callback_identity,
    const SourceEffect34Inputs& = {}) noexcept;
SourceEffect34Result tick_source_effect34_candidate_rng(SourceEventPhase5Context&,
    uint32_t,uint32_t,const SourceEffect34Inputs&,SourceRngService&,const SourceRngUpdateContext&) noexcept;
} // namespace l3
