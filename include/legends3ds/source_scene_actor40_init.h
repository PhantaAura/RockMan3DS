#pragma once
#include "legends3ds/source_scene_actor40_pose.h"
#include "legends3ds/source_event_consumer.h"

namespace l3 {
// Original overlay addresses are identity tokens, never native pointers.
// These two source writes replace prior/unknown values after first pose returns.
struct SourceSceneActor40InitializerGlobals {
    std::optional<uint32_t> word_80122470,word_80122474;
};
enum class SourceSceneActor40InitializerStatus : uint8_t {
    SourceBackedPartial,NeedsWorkFields,NeedsFreshInitializer,
    NeedsResourceOwnership,NeedsPrefix,NeedsPose,NeedsReference,
    NeedsScratchShift,UnsupportedScope
};
struct SourceSceneActor40PlayerMirrorReceipt {
    // LHU/SH raw bits, from actor20/22/24. No fixed16.16/yaw/state inference.
    std::array<uint16_t,3> previous_2c{},primary_14{};
    uint8_t source_tag=0;
    bool admitted=false;
};
struct SourceSceneActor40InitializerResult {
    SourceSceneActor40InitializerStatus status=SourceSceneActor40InitializerStatus::NeedsFreshInitializer;
    SourceSceneActor40Status prefix_status=SourceSceneActor40Status::NeedsWorkFields;
    SourceSceneActor40PoseStatus pose_status=SourceSceneActor40PoseStatus::NeedsPose;
    bool committed=false;
    uint8_t pose_parts=0;
    std::optional<SourceSceneActor40PlayerMirrorReceipt> player_mirrors;
};
// Complete fresh actual8010C930 initializer, including800322A8/8003346C,
// original80122470/74 stores and work08=1. Unknown temporary-stack lanes remain
// invalid as in the proved pose consumer. Later callbacks hold explicitly.
// All work/global writes are published only after the entire admission succeeds.
SourceSceneActor40InitializerResult initialize_source_scene_actor40_8010c930(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,std::optional<uint8_t> source_shift,
    SourceSceneActor40InitializerGlobals&) noexcept;
// Fresh8010D824 branch only: capture previous player halfwords before the
// initializer, run it completely, then capture primary halfwords after it.
// Player mutation is a separate exact receipt application; no native physics
// accumulator, heading or player state is manufactured from these halfwords.
SourceSceneActor40InitializerResult initialize_source_scene_actor40_wrapper_8010d824(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const ReferenceTables*,std::optional<uint8_t> source_shift,
    SourceSceneActor40InitializerGlobals&) noexcept;
// Independent prefix evidence when the complete callback cannot run. Returns
// no data unless all six actor-position bytes have explicit source ownership.
std::optional<std::array<uint16_t,3>> capture_source_scene_actor40_previous_8010d824(
    const SourceEventPhase5Bytes<1444>&) noexcept;
// Exact original first-player dispatch admission at8003C07C..8003C098.
// Input-control flags at+BD do not gate this call. Unknown00/06 holds.
std::optional<bool> source_player_update_gate_8003c07c(
    const L3PlayerCompatWork&,const SourceEventPlayerKnown&) noexcept;
// Writes only player14..19 and2C..31 and their12 known-mask bytes. All other
// player bytes/knowledge, including fixed16.16, heading and states are retained.
bool apply_source_scene_actor40_player_mirrors_8010d824(
    const SourceSceneActor40PlayerMirrorReceipt&,L3PlayerCompatWork&,
    SourceEventPlayerKnown&) noexcept;
} // namespace l3
