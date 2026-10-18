#pragma once
#include "legends3ds/actor_resource.h"
#include "legends3ds/actor_animation_bank.h"
#include "legends3ds/player_pose.h"
#include "legends3ds/source_event_phase5.h"
#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace l3 {
// Owner-local L3A4 holds only the first selected script through its first wait.
// The loader binds exact EXE/script hashes; bytes never ship in source releases.
struct SourceSceneActor40Bootstrap {
    bool admitted=false;
    uint32_t script_token=0;
    std::vector<uint16_t> words;
};
SourceSceneActor40Bootstrap load_source_scene_actor40_bootstrap(const std::vector<uint8_t>& bytes);

struct SourceSceneActor40MeshVariant {
    ActorMeshResource mesh;
    std::array<uint8_t,96> descriptor{};
};
struct SourceSceneActor40Resources {
    ActorMeshResource mesh;
    ActorPoseResource pose;
    // Prepared once at area load. The original actor advances these eight-byte
    // records after its first pose; no owner data is decoded in the frame loop.
    ActorPoseSequenceCache pose_sequence;
    ActorAnimationBank additional_requests;
    std::optional<SourceSceneActor40MeshVariant> mesh_variant;
    uint32_t descriptor_token=0,translations_token=0,frames_token=0,sequence_token=0;
    std::array<uint8_t,96> descriptor_copy{};
    std::weak_ptr<const SharedBytePayload::Storage> payload;
    const void* payload_identity=nullptr;
    uint32_t load_base=0;
    std::array<EbdDirectoryEntry,14> directory_entries{};
};
// Load-time only. Requires the existing immutable area0 resource witness and
// first null-variant class40/subtype5 row; this is a separate source actor owner.
SourceSceneActor40Resources decode_source_scene_actor40_resources(
    const SourceSceneActor40Bootstrap&,const SourceEventPhase5Profile&,
    const SourceEventPhase5ResourceBinding&,const EbdDirectory&,
    size_t decoded_memory_cap=256*1024);

// Actual phase5 record uses work04=1 ->ST00 wrapper8010D824 ->8010C930.
// Its first request is the original immediate0, independently of global scripts.
SourceSceneActor40Resources decode_source_scene_actor40_phase5_resources(
    const SourceEventPhase5Profile&,const SourceEventPhase5ResourceBinding&,
    const EbdDirectory&,size_t decoded_memory_cap=256*1024);
// Exact request IDs are supplied by proved source state handlers. This
// load-time transaction counts existing geometry/cache storage against cap;
// failure preserves the resident first-pose resource and prior request bank.
void prepare_source_scene_actor40_request_bank(SourceSceneActor40Resources&,
    const EbdDirectory&,const uint8_t* requests,size_t request_count,
    size_t decoded_memory_cap);
const ActorPoseSequenceCache* find_source_scene_actor40_pose_sequence(
    const SourceSceneActor40Resources&,uint8_t request) noexcept;
size_t source_scene_actor40_decoded_bytes(const SourceSceneActor40Resources&) noexcept;
void prepare_source_scene_actor40_mesh_variant(SourceSceneActor40Resources&,
    const EbdDirectory&,const std::array<uint8_t,96>&,size_t decoded_memory_cap);
const ActorMeshResource* select_source_scene_actor40_mesh(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&) noexcept;

enum class SourceSceneActor40Status : uint8_t {
    SourceBackedPrefix,NeedsBootstrap,NeedsResourceOwnership,NeedsWorkFields,
    UnsupportedActor,UnsupportedScript,NeedsPose,NeedsPerPartScale
};
struct SourceSceneActor40PrefixResult {
    SourceSceneActor40Status status=SourceSceneActor40Status::NeedsBootstrap;
    bool committed=false;
    uint8_t request=0;
    uint16_t script_index=0,wait_remaining=0;
};
// Exact scalar/resource/script prefix80014AC0 ->80015198. Ends before movement,
// sounds, request transition and integer/GTE pose consumers. No elapsed-time
// continuation or full initializer acknowledgement is fabricated.
SourceSceneActor40PrefixResult initialize_source_scene_actor40_prefix_80014ac0(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Bootstrap&,
    const SourceSceneActor40Resources&,const EbdDirectory&) noexcept;
// Exact actual-phase5 fresh initializer prefix8010C930 ->800322A8. Preserves
// constructor position, sets original heading2213/Q12 scale512/request0. The
// following integer pose producer and wrapper/global writes are not committed.
SourceSceneActor40PrefixResult initialize_source_scene_actor40_prefix_8010c930(
    SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&,
    const EbdDirectory&) noexcept;
// Separate pure80014BCC script prefix for differential authored cases. Stops at
// the first source wait or state2 store; unknown/bounded-out chains roll back.
SourceSceneActor40PrefixResult consume_source_scene_actor40_script_80014bcc(
    SourceEventPhase5Bytes<1444>&,const uint16_t* words,size_t count) noexcept;

struct SourceSceneActor40Matrices {
    SourceSceneActor40Status status=SourceSceneActor40Status::NeedsPose;
    size_t part_count=0;
    std::array<PlayerPoseMatrix,kActorDrawDescriptorSlots> world{};
    std::array<bool,kActorDrawDescriptorSlots> draw_enabled{};
};
// Float presentation of common80033598 hierarchy in raw terrain units. Uses
// original18-part geometry/pose; integer/GTE rounding, DPCS shading/depth/fog,
// draw order and full callback chronology remain independent parity gates.
SourceSceneActor40Matrices build_source_scene_actor40_pose_matrices(
    const SourceEventPhase5Bytes<1444>&,const SourceSceneActor40Resources&) noexcept;
} // namespace l3
