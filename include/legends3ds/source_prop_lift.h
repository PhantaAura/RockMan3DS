#pragma once
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_rng_service.h"
#include <vector>
namespace l3 {
constexpr uint32_t kSourcePropLiftCallback=0x80113280u;
constexpr uint32_t kSourcePropLiftCurveTable=0x8011c9fcu;
constexpr size_t kSourcePropLiftProfileBytes=384;
struct SourcePropLiftCurveReceipt {
    uint32_t source_table_identity=0;
    std::array<uint32_t,3> curve_identities{};
    // Explicit indexed source windows, not extents inferred from neighbours.
    std::array<std::array<uint8_t,64>,3> windows{};
    bool admitted=false;
};
class SourcePropLiftProfile {
public:
    bool admitted() const noexcept { return curves_.admitted; }
    const SourcePropLiftCurveReceipt& curves() const noexcept { return curves_; }
    const std::array<uint8_t,32>& ebd_digest() const noexcept { return ebd_digest_; }
private:
    SourcePropLiftCurveReceipt curves_{};
    std::array<uint8_t,32> ebd_digest_{};
    friend SourcePropLiftProfile load_source_prop_lift_profile(const std::vector<uint8_t>&);
};
SourcePropLiftProfile load_source_prop_lift_profile(const std::vector<uint8_t>&);
// The caller decodes the actual current byte through the admitted immutable EBD
// sequence pointer. This receipt does not manufacture a frame from native time.
struct SourcePropLiftActorFrameReceipt {
    uint32_t source_actor=0,sequence_identity=0;
    uint64_t source_generation=0;
    uint8_t frame=0;
    bool admitted=false;
};
struct SourcePropLiftRngOwner {const void* owner=nullptr;uint64_t generation=0;};
struct SourcePropLiftRngReceipt {
    const void* owner=nullptr;
    uint64_t owner_generation=0,source_generation=0,global_ordinal=0;
    uint32_t callback_identity=0,prop_identity=0,actor_identity=0,sequence_identity=0;
    uint32_t return_address=0,v0=0,child_identity=0;
    std::array<uint16_t,3> prop_before_xyz{};
    uint16_t actor_part7_y=0;
    uint8_t actor_selector14=0,sequence_frame=0,state09=0,sub10=0;
};
struct SourcePropLiftInputs {
    const SourcePropLiftCurveReceipt* curves=nullptr;
    const SourcePropLiftActorFrameReceipt* frame=nullptr;
    const SourcePropLiftRngReceipt* rng=nullptr;
    size_t rng_count=0;
    const SourcePropLiftRngOwner* current_rng_owner=nullptr;
    uint64_t source_generation=0;
};
enum class SourcePropLiftStatus : uint8_t {
    SourceBackedPartial,Inactive,InvalidIdentity,UnsupportedScope,UnsupportedState,
    NeedsField,NeedsEventBinding,NeedsActorOwner,NeedsFrameReceipt,
    FrameContextMismatch,NeedsCurves,CurveContextMismatch,NeedsRngReceipts,
    RngContextMismatch,NeedsPoolBitmap,NeedsPoolHeader,InvalidPool
};
struct SourcePropLiftResult {
    SourcePropLiftStatus status=SourcePropLiftStatus::NeedsField;
    bool committed=false,child_allocated=false,source_null_allocation=false;
    uint8_t rng_required=0,rng_consumed=0;
    uint32_t child_identity=0,source_function=0;
    std::array<uint32_t,2> rng_return_addresses{{0x80113238u,0x80113e54u}};
    std::optional<uint8_t> missing_slot;
    std::optional<uint16_t> missing_offset;
};
// Only E0/subtype3/parameter2 initialized source states09=1/2. Source state1
// sub0 and state1 sub2..4/state2 sub0..2 need no RNG. Changing state1/sub1
// requires the complete original rand/effect transaction before any publication.
// Current enclosing scene/EBD/overlay ownership is the runtime caller's guard.
SourcePropLiftResult consume_source_prop_lift_80113280(SourceEventPhase5Context&,
    const SourceEventWork&,uint32_t prop_identity,const SourcePropLiftInputs& = {}) noexcept;
struct SourcePropLiftCandidateRng {
    SourceRngService* service=nullptr;
    SourceRngUpdateContext update{0,0,SourceRngUpdatePhase::PropLiftCallback};
};
SourcePropLiftResult consume_source_prop_lift_candidate(SourceEventPhase5Context&,
    const SourceEventWork&,uint32_t,const SourcePropLiftInputs&,const SourcePropLiftCandidateRng&) noexcept;
} // namespace l3
