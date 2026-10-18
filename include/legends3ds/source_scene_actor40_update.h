#pragma once
#include "legends3ds/source_scene_actor40_init.h"
#include "legends3ds/source_scene_actor40_tail.h"

namespace l3 {
struct SourceEventWork;
enum class SourceSceneActor40LaterStatus : uint8_t {
    SourceBackedPrefix,NeedsWorkFields,NeedsResourceOwnership,UnsupportedActor,
    UnsupportedState,NeedsPose,NeedsReference,NeedsScratchShift,UnsupportedScope,
    NeedsExternalActor,NeedsReleaseOwner
};
struct SourceSceneActor40AudioCue {
    uint16_t id=0;
    uint32_t raw_a1=0,raw_a2=100;
};
// All receipts and write counters are valid only when committed is true.
struct SourceSceneActor40LaterResult {
    SourceSceneActor40LaterStatus status=SourceSceneActor40LaterStatus::NeedsWorkFields;
    bool committed=false,request0_reset=false,sequence_advanced=false;
    bool shadow_pending=false,timed_cues_pending=false;
    bool final_player_mirrors_applied=false;
    std::optional<SourceSceneActor40TailStatus> tail_status;
    bool request_reset=false;
    uint8_t request=0,state=0,substate=0,external_stores=0;
    // 8001A008 receipt only. The native audio owner consumes this independently.
    std::optional<SourceSceneActor40AudioCue> audio_cue;
    // Final mirroring requires the independent shadow/cue non-aliasing proof.
    std::optional<SourceSceneActor40PlayerMirrorReceipt> pre_presentation_mirrors;
};
bool project_source_scene_actor40_fixed_position_80030150(
    SourceEventPhase5Bytes<1444>& work) noexcept;
// Original8010D7A4 local state handlers0..12, then80030150 and80032488.
// Explicit scene owners admit only original global pointers to occupied source
// pools. All state/sequence/external writes preflight and commit together.
// Stops before8002C8CC shadow and8010C820 timed cues; no source cadence claim.
SourceSceneActor40LaterResult update_source_scene_actor40_prefix_8010d824(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,std::optional<uint8_t>,
    const SourceEventWork* event_work=nullptr,SourceEventPhase5Context* scene=nullptr) noexcept;
// Compatibility entry; enforces state0 before entering the shared path.
SourceSceneActor40LaterResult update_source_scene_actor40_state0_prefix_8010d824(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,std::optional<uint8_t>) noexcept;
} // namespace l3
