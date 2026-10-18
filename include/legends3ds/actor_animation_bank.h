#pragma once
#include "legends3ds/actor_resource.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

// Request IDs are byte indices supplied by proved initializer/action call sites.
// This is a domain bound, not an inferred source animation-table length.
constexpr size_t kActorAnimationRequestLimit = 256;

struct ActorAnimationRequestCache {
    uint8_t request = 0;
    ActorPoseSequenceCache sequence;
};
struct ActorAnimationBank {
    uint32_t source_row = 0;
    std::vector<ActorAnimationRequestCache> requests;
    // Owned bank metadata, request slots, sequence/pose and translation elements.
    // Excludes allocator overhead, live pose copies, source bytes and GPU storage.
    size_t decoded_bytes = 0;
};

// Load-time operation only. Each explicit requested ID decodes its own original
// sequence and pose pointer tables, never a substitute request0. Duplicate IDs,
// malformed source windows, unclosed sequences and the decoded-element cap
// reject the entire result. Callers share the immutable bank for a resource row.
// Source request semantics and allowed class/action policy remain caller-owned.
ActorAnimationBank decode_ebd_actor_animation_bank(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    const uint8_t* requests, size_t request_count, size_t decoded_memory_cap);
// Class40 uses the same sequencer with its distinct24-part resource decoder.
// The class20 live-pose helpers below retain their13-part contract.
ActorAnimationBank decode_ebd_scene_actor40_animation_bank(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    const uint8_t* requests, size_t request_count, size_t decoded_memory_cap);

const ActorAnimationRequestCache* find_actor_animation_request(
    const ActorAnimationBank& bank, uint8_t request) noexcept;

enum class ActorAnimationStatus : uint8_t {
    SourceBackedPartial,
    MissingRequest,
    InvalidCache,
    PartCountMismatch
};
struct ActorAnimationResult {
    ActorAnimationStatus status = ActorAnimationStatus::InvalidCache;
    bool request_changed = false;
    bool record_changed = false;
};

struct ActorAnimationPreview {
    ActorAnimationResult result;
    const ActorPoseResource* pose=nullptr; // resident immutable bank owner
    ActorSequenceCursor cursor;
};
ActorAnimationPreview actor_animation_preview(const ActorAnimationBank& bank,
    uint8_t request,const ActorPoseResource& live,
    const ActorSequenceCursor& cursor) noexcept;

// Preflight/reset/step only use resident caches and equal-size live translation
// storage. All failures preserve both pose and cursor. No decoder, vector resize
// or allocation runs here. Missing requests are explicit, including IDs absent
// from the prepared bank even if another request's pointer is valid.
ActorAnimationStatus actor_animation_request_ready(
    const ActorAnimationBank& bank, uint8_t request,
    const ActorPoseResource& live_pose) noexcept;
ActorAnimationResult actor_animation_reset(
    const ActorAnimationBank& bank, uint8_t request,
    ActorPoseResource& live_pose, ActorSequenceCursor& cursor) noexcept;
ActorAnimationResult actor_animation_step(
    const ActorAnimationBank& bank, ActorPoseResource& live_pose,
    ActorSequenceCursor& cursor) noexcept;

} // namespace l3
