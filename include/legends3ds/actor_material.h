#pragma once
#include "legends3ds/actor_resource.h"
#include "legends3ds/material_cache.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
constexpr size_t kActorMaterialEntryLimit = 64;
constexpr uint64_t kActorMaterialDecodedLimit = 1024u * 1024u;
// Original CLUT/TPAGE bindings; no texture-name or palette substitution.
uint32_t actor_material_word(const PbdTri& face);
uint32_t actor_material_word(const PbdQuad& face);
std::vector<uint32_t> collect_actor_material_words(const ActorMeshResource& resource);
// Host-only load-time collection of the resource profiles already admitted by
// ActorAreaRuntime. Unsupported actors are not assigned guessed material data.
std::vector<uint32_t> collect_admitted_actor_material_words(const EbdDirectory& ebd);
// ST00 E0/05 boxes and E0/08 targets/doors drawn by the native runtime: the
// raw single-part material word of every (E0,5)/(E0,8) row, all three tiers.
std::vector<uint32_t> collect_admitted_e0_object_material_words(const EbdDirectory& ebd);
MaterialCache load_actor_material_cache(const std::vector<uint8_t>& bytes,
                                       uint64_t decoded_limit = kActorMaterialDecodedLimit);
// Same byte UV/pixel-center and bottom-origin conversion as the player renderer.
// Does not clamp V128 into a 128-row upload or invent unwritten VRAM pixels.
std::array<float,2> actor_render_uv(uint8_t u, uint8_t v);
} // namespace l3
