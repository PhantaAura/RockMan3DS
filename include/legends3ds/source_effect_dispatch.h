#pragma once
#include "legends3ds/source_effect2.h"
#include "legends3ds/source_effect34.h"
#include <memory>
#include <vector>

namespace l3 {
constexpr size_t kSourceEffectDispatchPackageBytes=384;
constexpr uint32_t kSourceEffectDispatchTable=0x8011a604u;
constexpr uint32_t kSourceEffectDispatchScratchOwner=0x1f800064u;
// L3E2v1 retains neutral scalar words only. Loading verifies the exact supported
// source identities, canonical framing/CRC, and SHA-256 of the original table
// and control payload. Source releases contain its digest, not those words.
class SourceEffectDispatchProfile {
public:
    bool admitted() const noexcept { return admitted_; }
    std::optional<uint32_t> callback(uint8_t subtype) const noexcept;
    const SourceEffect2ColorReceipt& colors() const noexcept { return colors_; }
    const SourceEffect34ControlReceipt& controls() const noexcept { return controls_; }
    const std::array<uint8_t,32>& ebd_digest() const noexcept { return ebd_digest_; }
private:
    bool admitted_=false;
    std::array<uint8_t,32> ebd_digest_{};
    std::array<uint32_t,16> callbacks_{};
    SourceEffect2ColorReceipt colors_{};
    SourceEffect34ControlReceipt controls_{};
    friend SourceEffectDispatchProfile load_source_effect_dispatch_profile(const std::vector<uint8_t>&);
};
SourceEffectDispatchProfile load_source_effect_dispatch_profile(const std::vector<uint8_t>&);

class SourceEffectDispatchLease {
public:
    // Explicit source overlay-load admission corresponding to80100354..360.
    // Default construction supplies no scratch-table owner. The current scene,
    // P5 binding and immutable EBD payload must remain the exact bound owners.
    bool bind_overlay_load_80100354(const SourceEffectDispatchProfile&,
        const SourceEventPhase5Profile&,const SourceEventPhase5ResourceBinding&,
        const EbdDirectory&,const void* scene_owner,uint64_t scene_generation) noexcept;
    bool current(const SourceEffectDispatchProfile&,
        const SourceEventPhase5Profile&,const SourceEventPhase5ResourceBinding&,
        const EbdDirectory&,const void* scene_owner,uint64_t scene_generation) const noexcept;
    void invalidate() noexcept;
    uint64_t generation() const noexcept { return generation_; }
private:
    const SourceEffectDispatchProfile* profile_=nullptr;
    const SourceEventPhase5Profile* phase5_=nullptr;
    const SourceEventPhase5ResourceBinding* binding_=nullptr;
    const void* scene_owner_=nullptr;
    const void* payload_identity_=nullptr;
    uint64_t generation_=0;
    std::weak_ptr<const SharedBytePayload::Storage> payload_;
};

// Fixed function pointers avoid std::function/heap storage in the original
// 64-record sweep. Preparation borrows external receipts; completion receives
// the fully published callback or a held result. Only that external owner may
// commit/abort its reservation. This module never owns or advances an RNG.
struct SourceEffectDispatchInputProvider {
    void* user=nullptr;
    void (*prepare2)(void*,const SourceEventPhase5Context&,uint32_t,SourceEffect2Inputs&) noexcept=nullptr;
    void (*complete2)(void*,const SourceEventPhase5Context&,uint32_t,const SourceEffect2Result&) noexcept=nullptr;
    void (*prepare34)(void*,const SourceEventPhase5Context&,uint32_t,uint32_t,SourceEffect34Inputs&) noexcept=nullptr;
    void (*complete34)(void*,const SourceEventPhase5Context&,uint32_t,uint32_t,const SourceEffect34Result&) noexcept=nullptr;
    // Native opt-in candidate path. Explicit prepare2 callbacks take priority;
    // recorded-reference mode must leave this null.
    SourceRngService* effect2_candidate_rng=nullptr;
    SourceRngUpdateContext candidate_update{0,0,SourceRngUpdatePhase::Effect2Callback};
    SourceRngService* effect34_candidate_rng=nullptr;
    SourceRngUpdateContext candidate34_update{0,0,SourceRngUpdatePhase::Effect34Callback};
};
enum class SourceEffectDispatchStatus : uint8_t { Ready,Partial,NeedsLease };
enum class SourceEffectDispatchHoldKind : uint8_t {
    GateField,SubtypeField,UnsupportedCallback,Effect2,Effect34
};
struct SourceEffectDispatchHold {
    uint8_t slot=255,subtype=255,consumer_status=0;
    SourceEffectDispatchHoldKind kind=SourceEffectDispatchHoldKind::GateField;
    uint32_t callback=0;
    std::optional<uint8_t> missing_offset;
};
struct SourceEffectDispatchBatch {
    SourceEffectDispatchStatus status=SourceEffectDispatchStatus::NeedsLease;
    uint8_t visited=0,callbacks=0,committed=0,held=0,released=0;
    uint16_t children=0,rng_consumed=0;
    std::array<SourceEffectDispatchHold,64> holds{};
};
// Original raw-slot order/gate800172F4. Known callbacks2/3/4 are individually
// atomic. An unsupported/held slot remains visible; successful earlier callbacks
// are never rolled back. A newly allocated later child is visited this sweep.
// Continuing other ready callbacks after an unknown slot is diagnostic native
// scheduling, not proof that the entire original world phase completed.
SourceEffectDispatchBatch tick_source_effect_dispatch_800172f4(
    SourceEventPhase5Context&,const SourceEffectDispatchProfile&,
    const SourceEffectDispatchLease&,const SourceEventPhase5Profile&,
    const SourceEventPhase5ResourceBinding&,const EbdDirectory&,
    const void* scene_owner,uint64_t scene_generation,
    const SourceEffectDispatchInputProvider& = {}) noexcept;
} // namespace l3
