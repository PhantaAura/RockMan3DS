#pragma once
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_effects.h"
#include "legends3ds/source_rng_service.h"

namespace l3 {
constexpr uint32_t source_effect2_callback=0x80116e68u;
constexpr uint32_t source_effect2_color_table=0x8011cd74u;
// Value receipts are external BIOS A0/2F results, never a generated seed.
// Each receipt binds the consumer's actual parent, newly allocated child,
// parent clock and original return address before this transaction commits.
struct SourceEffect2RngReceipt {
    uint32_t callback_identity=0,parent_identity=0,child_identity=0;
    uint32_t return_address=0,v0=0;
    uint64_t global_ordinal=0,source_generation=0;
    std::array<uint16_t,3> parent_xyz1c{};
    uint8_t parent_clock11=0,parent_parameter03=0,parent_mode0d=0,parent_tag0e=0;
};
// The caller must admit these eight original scalar words from its retained
// owner package. This object neither embeds a palette nor grants provenance.
struct SourceEffect2ColorReceipt {
    uint32_t source_table_identity=0;
    std::array<uint32_t,8> words{};
    bool admitted=false;
};
struct SourceEffect2Inputs {
    uint32_t callback_identity=source_effect2_callback;
    const SourceEffect2RngReceipt* rng=nullptr;
    size_t rng_count=0;
    uint64_t source_generation=0;
    const SourceEffect2ColorReceipt* colors=nullptr;
};
enum class SourceEffect2Status : uint8_t {
    SourceBackedPartial,Inactive,InvalidIdentity,UnknownCallback,UnknownDispatch,
    NeedsField,NeedsPoolBitmap,NeedsReleaseBitmap,InvalidPool,
    SourceAllocationExhausted,NeedsRngReceipts,RngContextMismatch,
    NeedsColorReceipt,ColorContextMismatch
};
struct SourceEffect2Result {
    SourceEffect2Status status=SourceEffect2Status::SourceBackedPartial;
    bool committed=false,child_allocated=false,released=false;
    uint8_t rng_consumed=0,rng_required=0;
    uint32_t child_identity=0;
    std::optional<uint8_t> missing_slot,missing_offset;
    std::array<uint32_t,3> rng_return_addresses{{0x80116f60u,0x80116f98u,0x80116fe4u}};
};
// Complete proved scalar subtype2 callback, including original allocator,
// initialization, child fade and deferred release. No hot-loop allocation.
// All needed known lanes, masks and external receipts preflight before any
// publication. Exhaustion holds because original blindly dereferences NULL;
// native code does not emulate that unsafe write. This single callback does
// not supply the ascending800172F4 sweep, original cadence, RNG owner or draw.
SourceEffect2Result tick_source_effect2_80116e68(SourceEventPhase5Context&,
    uint32_t source_identity,const SourceEffect2Inputs& = {}) noexcept;
// Explicit candidate scheduling only: borrow the already admitted shared
// OpenBIOS service, stage the exact triple and commit it only with the effect.
// Disabled/malformed owners retain the original hold. This is not retail RNG
// chronology and never relaxes recorded-reference context checks.
SourceEffect2Result tick_source_effect2_candidate_rng(SourceEventPhase5Context&,
    uint32_t source_identity,const SourceEffect2Inputs&,SourceRngService&,
    const SourceRngUpdateContext&) noexcept;
} // namespace l3
