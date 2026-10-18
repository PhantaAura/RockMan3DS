#pragma once
#include "legends3ds/ebd_directory.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/actor_sequence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

// Class-20 EBD draw path, researched against USA ROCK_NEO.EXE. This format
// uses 20-byte mesh descriptors and 12-byte packets, unlike player P-type PBD.
// No resource extent is inferred from another directory pointer.
constexpr size_t kActorDrawDescriptorSlots = 24;
constexpr size_t kActorPackedPoseBytes = 128; // helper count 32 is u32 words.
constexpr size_t kActorPoseChannels = 75; // 25 padded XYZ vectors: root + 24 rotations.
constexpr size_t kActorRotationVectors = 24;
constexpr size_t kActorClass20PartLimit = 13;

enum class ActorMeshTier : uint8_t { NEAR = 0, MIDDLE = 1, FAR = 2 };

struct ActorDrawBinding {
    uint8_t mesh_index = 0;
    int8_t parent = -1; // slot 0 is the root; byte 1 there is a part count.
    uint8_t rotation_translation_index = 0;
    int8_t draw_control = 0; // negative disables the corresponding draw slot.
    uint8_t source_part_byte3 = 0; // retained; not assigned gameplay semantics.
    uint32_t material_word = 0; // high u16 CLUT, low u16 tpage before runtime state.
    uint32_t source_part_offset = 0;
};

struct ActorResourceFootprint {
    size_t descriptor_bytes = 0;
    size_t part_bytes = 0;
    size_t vertex_bytes = 0;
    size_t vertex_consumer_bytes = 0; // renderer reads vertices in groups of 3.
    size_t packet_bytes = 0;
    size_t decoded_bytes = 0; // estimated owned element storage, excluding allocator overhead.
};

struct ActorMeshResource {
    PbdModel model;
    std::vector<ActorDrawBinding> bindings; // same order as model.parts/draw slots.
    ActorResourceFootprint footprint;
    ActorMeshTier tier = ActorMeshTier::NEAR;
    uint32_t source_row = 0;
    uint32_t source_descriptor_offset = 0;
};

// Returns decoded neutral mesh elements suitable for the existing Pbd renderer.
// Requires an explicit row whose class-20 lookup/draw path the caller has proved.
// Throws on legacy/no payload, null required pointers, malformed ranges, invalid
// indices, unsupported part counts/hierarchy or the caller's decoded memory cap.
// RGB is neutral PS1 modulation here; retail runtime shading remains separate.
ActorMeshResource decode_ebd_actor_mesh(const EbdDirectory& ebd, size_t row,
                                      ActorMeshTier tier = ActorMeshTier::NEAR,
                                      size_t decoded_memory_cap = 256 * 1024);

struct ActorPoseResource {
    uint8_t request = 0;
    uint8_t record_index = 0;
    uint8_t frame_index = 0;
    uint8_t countdown = 0;
    int8_t next_control = 0;
    std::array<uint8_t, 8> sequencing_record{};
    std::array<int16_t, 3> root_translation{}; // signed first 3 packed channels.
    std::array<std::array<uint16_t, 3>, kActorRotationVectors> rotations{};
    std::vector<std::array<int16_t, 3>> base_translation; // per draw slot.
    uint32_t sequence_source_offset = 0;
    uint32_t pose_source_offset = 0;
    uint32_t translations_source_offset = 0;
};

// The caller provides request/record indices backed by actor state evidence.
// This does not decide progression flags, animation requests or gameplay states.
// The exact selected 8-byte record, frame pointer and 128-byte consumer window
// are bounded independently. No unknown action/frame counts are fabricated.
ActorPoseResource decode_ebd_actor_pose(const EbdDirectory& ebd, size_t row,
                                      const ActorMeshResource& mesh,
                                      uint8_t request, uint8_t record_index);

// Explicit class40 path: shared8002FA38 resource copy,80033A28 packed pose,
//80033598 matrix producer and80028F90 draw consumer. Class40 has24 descriptor
// slots and matrix bank+240; class20 retains its separate13-part limit.
ActorMeshResource decode_ebd_scene_actor40_mesh(const EbdDirectory& ebd, size_t row,
                                      ActorMeshTier tier = ActorMeshTier::NEAR,
                                      size_t decoded_memory_cap = 256 * 1024);
// Source state handlers may change draw-slot mesh indices while preserving
// hierarchy/count/rotation/control lanes. Validate those immutable lanes and
// decode the explicitly supplied indices from the same resident resource row.
ActorMeshResource decode_ebd_scene_actor40_mesh_variant(const EbdDirectory&,size_t row,
    const std::array<uint8_t,96>& descriptor_copy,ActorMeshTier tier,
    size_t decoded_memory_cap);
ActorPoseResource decode_ebd_scene_actor40_pose(const EbdDirectory& ebd, size_t row,
                                      const ActorMeshResource& mesh,
                                      uint8_t request, uint8_t record_index);

struct ActorPoseSequenceCache {
    std::vector<ActorPoseResource> poses; // record index equals vector index.
    std::vector<ActorSequenceRecord> records;
    size_t decoded_bytes = 0; // owned cache elements, excluding allocator overhead.
};
// Load-time only. Follows the source control edge from record0 until a backward
// edge closes the reachable set. Only a contiguous reachable set is supported;
// forward skips and unbounded chains reject, rather than fabricate records.
// Pose windows are checked independently by the existing decoder. No request
// semantics or callback cadence are selected here.
ActorPoseSequenceCache decode_ebd_actor_pose_sequence(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    uint8_t request, size_t decoded_memory_cap);
// Same bounded, contiguous sequence decode for class40 scene actors. The
// caller supplies a source-backed request; this does not assign its meaning.
ActorPoseSequenceCache decode_ebd_scene_actor40_pose_sequence(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    uint8_t request, size_t decoded_memory_cap);

std::array<uint16_t, kActorPoseChannels> unpack_actor_pose_channels(
    const uint8_t* bytes, size_t size);

} // namespace l3
