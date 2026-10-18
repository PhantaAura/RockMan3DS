#pragma once
#include "legends3ds/source_scene_actor40.h"
#include "legends3ds/reference_tables.h"
#include <optional>

namespace l3 {
enum class SourceSceneActor40PoseStatus : uint8_t {
    SourceBackedPartial,NeedsReference,NeedsScratchShift,UnsupportedScratchShift,
    NeedsWorkFields,UnsupportedActor,NeedsPose,NeedsPerPartScale,
    NeedsResourceOwnership,InvalidHierarchy
};
struct SourceSceneActor40IntegerPart {
    // Exact first20 bytes at work+240+part*20hex. Word+10hex preserves the
    // full32-bit final ScaleMatrix store, including its upper halfword.
    std::array<uint8_t,20> matrix{};
    std::array<int16_t,3> position{}; // source work+180+part*8, shifted domain
    bool transform_written=false,draw_enabled=false;
};
struct SourceSceneActor40IntegerPose {
    SourceSceneActor40PoseStatus status=SourceSceneActor40PoseStatus::NeedsPose;
    uint8_t part_count=0,scratch_shift=0;
    std::array<uint8_t,20> scaled_heading_160{};
    std::array<SourceSceneActor40IntegerPart,kActorDrawDescriptorSlots> parts{};
};
// Exact ordinary-scale class40 branch8003346C ->80033598. RotMatrix CPU
// low32 wrapping, MVMVA(sf1,mx0,v0,cv3) IR saturation/MAC outputs, ScaleMatrix
// ASR12, signed16 point stores and repeated-root branches are retained.
// Shift0/1/2 require an explicit source owner; native frame count is irrelevant.
// This pure result does not mutate work, global state, scratch or GTE flags.
SourceSceneActor40IntegerPose produce_source_scene_actor40_integer_pose_8003346c(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const ReferenceTables*,std::optional<uint8_t> scratch_shift_1f800074) noexcept;
// Same source integer transform with the current predecoded sequence record.
SourceSceneActor40IntegerPose produce_source_scene_actor40_integer_pose_8003346c(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const ReferenceTables*,std::optional<uint8_t> scratch_shift_1f800074,
    const ActorPoseResource&) noexcept;
// Resolve the source work+A8 request and +98 sequence pointer into the
// area-local cache. A stale pointer cannot silently select record zero.
const ActorPoseResource* select_source_scene_actor40_pose(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&) noexcept;
struct SourceSceneActor40PoseCommit {
    SourceSceneActor40PoseStatus status=SourceSceneActor40PoseStatus::NeedsPose;
    bool committed=false;
    uint8_t part_count=0;
};
// Complete request0/record0 work-owned stores of800322A8 for either the fresh
// initializer (work08=0) or the proved state09=0 later reset (work08=1), including
// sequence tokens/word164 and integer banks. The opaque temporary matrix-copy
// padding at work174..17F is invalidated; no original stack bytes are invented.
// Source work08 and overlay/player wrapper globals remain untouched.
// Preflights immutable EBD identity/directory and all required known lanes.
SourceSceneActor40PoseCommit commit_source_scene_actor40_initial_pose_800322a8(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,
    std::optional<uint8_t> scratch_shift_1f800074) noexcept;
// Original800322A8 for an explicitly prepared request/record. All original
// source identity and matrix guards are retained; failure preserves all bytes.
SourceSceneActor40PoseCommit reset_source_scene_actor40_pose_800322a8(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,std::optional<uint8_t>,
    uint8_t request,uint16_t record_index=0) noexcept;
struct SourceSceneActor40SequenceTick {
    SourceSceneActor40PoseStatus status=SourceSceneActor40PoseStatus::NeedsPose;
    bool committed=false,record_changed=false;
    uint16_t record_index=0;
    uint8_t part_count=0;
};
// Source80032488 record advancement plus8003346C integer banks for the
// already-initialized class40 actor. Other later callback/script writes and
// retail scheduling remain independent gates. Never allocates per frame.
SourceSceneActor40SequenceTick advance_source_scene_actor40_pose_80032488(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,
    std::optional<uint8_t> scratch_shift_1f800074) noexcept;
// Convert already produced source integer banks to raw terrain units. Further
// vertex projection, material shading/depth/fog and draw order are separate.
SourceSceneActor40Matrices present_source_scene_actor40_integer_pose(
    const SourceSceneActor40IntegerPose&) noexcept;
} // namespace l3
