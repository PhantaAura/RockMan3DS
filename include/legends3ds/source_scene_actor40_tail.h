#pragma once
#include "legends3ds/source_scene_actor40_init.h"

namespace l3 {
enum class SourceSceneActor40TailStatus : uint8_t {
    SourceBackedPartial,NeedsWorkFields,UnsupportedActor,StaleStateReceipt,
    NeedsResourceOwnership,UnsafeMaterialDestination
};
struct SourceSceneActor40TailMirrors {
    SourceSceneActor40TailStatus status=SourceSceneActor40TailStatus::NeedsWorkFields;
    bool admitted=false,presentation_pending=false;
    bool shadow_pending=false,material_cues_pending=false;
    std::optional<SourceSceneActor40PlayerMirrorReceipt> player_mirrors;
};
// Original later wrapper tail:8002C8CC is a shadow-queue producer;8010C820
// and its cue helpers write overlay clocks and pointed model material words.
// Under the resident, non-aliasing descriptor guard they preserve actor raw
// position and all pose/proxy banks. This admits the exact final LHU/SH mirror
// while keeping unexecuted shadow/material presentation explicitly pending.
// The supplied state receipt must match the current actor position and tag.
// No work, resource, clock, queue or player mutation and no heap allocation.
SourceSceneActor40TailMirrors admit_source_scene_actor40_final_mirrors_8010d824(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&,const SourceSceneActor40PlayerMirrorReceipt&) noexcept;
} // namespace l3
