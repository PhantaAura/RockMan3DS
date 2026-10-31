#pragma once
#include "legends3ds/pbd_model.h"
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_prop_attach.h"
#include "legends3ds/source_prop_lift.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
// E0/subtype3 uses one mesh descriptor, not the class20 hierarchy or pose bank.
// These tiers are selected by the source camera-space depth at8002A38C.
enum class SceneE0MeshTier : uint8_t { Near, Middle, Far };
SceneE0MeshTier scene_e0_tier_for_source_depth(int32_t depth) noexcept;
struct SceneE0MeshResource {
    PbdPart part;
    uint32_t descriptor_offset=0,part_offset=0,material_word=0;
    uint8_t source_part_byte3=0;
    size_t decoded_bytes=0;
};
// Load-time decoder for the proved8002A160→8002A6E8 single-part consumer.
// Every pointer/window/index is independently bounded; no adjacent-pointer
// extent or class20 animation semantics are inferred. Original material is raw.
SceneE0MeshResource decode_scene_e0_mesh(const EbdDirectory& directory,size_t row,
    SceneE0MeshTier tier,size_t decoded_memory_cap=128*1024);
class SceneE0Resources {
public:
    bool resident_in(const EbdDirectory& directory) const noexcept;
    const SceneE0MeshResource* find(uint32_t resource_token,uint8_t variant,
        SceneE0MeshTier tier) const noexcept;
    size_t resource_count() const noexcept { return meshes_.size(); }
    size_t decoded_bytes() const noexcept { return decoded_bytes_; }
private:
    struct Binding { uint32_t token=0;uint8_t variant=0;std::array<uint8_t,3> mesh{}; };
    std::array<Binding,15> bindings_{};
    std::vector<SceneE0MeshResource> meshes_;
    SourceEventPhase5ResourceBinding ownership_;
    size_t decoded_bytes_=0;
    friend SceneE0Resources load_scene_e0_resources(const EbdDirectory&,
        const SourceEventPhase5Profile&,const SourceEventPhase5ResourceBinding&,size_t);
};
// Deduplicates identical descriptor/part windows across the selected15 records
// and3LOD tiers. It borrows immutable owner residency; it does not keep an area
// resident after unload. The complete element budget includes cache capacity.
SceneE0Resources load_scene_e0_resources(const EbdDirectory& directory,
    const SourceEventPhase5Profile& profile,const SourceEventPhase5ResourceBinding& ownership,
    size_t decoded_memory_cap=128*1024);
enum class SceneE0Status : uint8_t {
    Initialized,NotScheduled,NeedsFields,NeedsResourceOwnership,UnsupportedSubtype,
    UnsupportedVariant,NeedsVariantBehavior,UnsupportedDrawMode,Ready,
    Updated,NoOp,NeedsReferences,NeedsScratchOwner,NeedsLinkedOwner,
    NeedsCameraOwner,NeedsResourceMutation,NeedsRngOrChildOwner,UnsupportedBehaviorState,
    NeedsPropCurves,NeedsPropFrame,NeedsForeignReleaseOwner
};
struct SceneE0Result {
    SceneE0Status status=SceneE0Status::NeedsFields;
    bool committed=false,foreign_release_committed=false;
    std::optional<uint16_t> missing_offset;
    uint32_t source_function=0;
};
// Exact first80112BDC callback, admitted only for the currently proved variants
// 0..7. No later behavior/pose update is fabricated. Caller must establish the
// original80017528 update-sweep chronology, separate from constructor execution.
SceneE0Result initialize_scene_e0_first_update_80112bdc(SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory) noexcept;
struct SceneE0UpdateInputs {
    // Reuses the owner's existing packed80092EC4 L3RF table. The camera's
    // quarter-wave lookup has a different owner and must not be substituted.
    const ReferenceTables* references=nullptr;
    const SourceEventWork* event_work=nullptr;
    const SourceEventPhase5Pool<32,80>* pool=nullptr;
    std::optional<uint16_t> scratch_half_1f800002;
    // Current source context only; the wrapper verifies the exact E0 slot
    // address before a leaf can publish linked record/child stores.
    SourceEventPhase5Context* scene_context=nullptr;
    std::optional<uint32_t> prop_identity;
    SourcePropForeignReleaseOwner* prop_release_owner=nullptr;
    SourcePropLiftInputs lift;
    const SourcePropLiftCandidateRng* lift_candidate=nullptr;
    // E0-object rand() channel for the block/slide states, begun by the caller
    // (SourceRngUpdatePhase::E0ObjectCallback) and committed only on commit.
    SourceRngUpdateTransaction* e0_rng=nullptr;

};
// Bounded80112B9C/80113EA4 consumer for the selected subtype3 variants0..7.
// Exact supported branches commit atomically; missing camera/RNG/effect/UV
// owners hold before all stores. This does not establish main-loop chronology.
SceneE0Result update_scene_e0_80112b9c(SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory,
    const SceneE0UpdateInputs& inputs) noexcept;
struct SceneE0DrawView {
    const SceneE0MeshResource* mesh=nullptr; // borrow ends on cache destruction
    std::array<int16_t,9> rotation{}; // row-major signed12 matrix atwork+18
    std::array<int16_t,3> position{}; // source integer world position atwork+38
    uint32_t modulation_word=0; // low RGB24; bit25 is source semitransparency
    uint8_t flags=0;
};
struct SceneE0DrawResult {
    SceneE0Status status=SceneE0Status::NeedsFields;
    std::optional<uint16_t> missing_offset;
    SceneE0DrawView view;
};
// Exact eligibility/field adapter for80029B94→8002A160. This does not implement
// GTE clipping, depth shade, source blend order, or later variant callbacks.
// Dynamic part selection(flag20) and other draw dispatches remain explicit holds.
SceneE0DrawResult scene_e0_draw_view(const SourceEventPhase5Bytes<80>& work,
    const SceneE0Resources& resources,const EbdDirectory& directory,SceneE0MeshTier tier) noexcept;
} // namespace l3
