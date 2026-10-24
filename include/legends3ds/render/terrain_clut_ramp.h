#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// 0x80025900 (positive mode 2) depth ramp. After the depth cue it ORs
// ramp[(otz >> (9 + LOD)) & 31] << 16 into the face's word 3, moving the CLUT
// that many 16-entry palettes to the right. The ramp is the area record's
// bytes +0..31 (0x8011AC54 + area * 40, copied to 0x1F800100). The ramp
// palettes in VRAM are darker copies of the base palette; they follow no
// single integer rule (about 58% of entries match (9 - k) / 9).
//
// otz, from the four SZ values (bit 31/27 of the primitive prefix):
//   bit31 clear: (max + (sum >> 2)) >> 1, or max - 16 with bit27;
//   bit31 set:   max, or SZ0 with bit27.
// A bit31-clear face with max SZ < 0xC1 is not drawn (near cull). Faces with
// prefix bit 28 skip both the depth cue and the ramp write; they reuse the
// previous face's ramp in the original (CANDIDATE: 0 here).
struct TerrainFaceDepth { int32_t otz = 0; bool near_culled = false; };
TerrainFaceDepth terrain_face_depth_80025900(const std::array<uint16_t,4>& sz, uint32_t prefix) noexcept;
uint8_t terrain_clut_ramp_step(int32_t otz, uint32_t prefix,
    const std::array<uint8_t,40>& area_record, uint8_t lod_1f800074) noexcept;

// Per-material RGB scales for ramp steps 0..7 (q12, 4096 = unchanged),
// least-squares fitted from the real variant palettes at cook time.
// CANDIDATE: a scale per channel approximates the authored palettes (mean
// error about 0.67 of a 5-bit step on Areas 1-2).
struct TerrainClutRampEntry {
    uint32_t material_word = 0;
    std::array<std::array<uint16_t,3>,8> scale_q12{};
};
// `variants[k]` are the 15-bit palettes of word + (k << 16); an empty or
// size-mismatched variant keeps scale 4096 for that step.
TerrainClutRampEntry fit_terrain_clut_ramp(uint32_t material_word, const std::vector<uint16_t>& base,
    const std::array<std::vector<uint16_t>,8>& variants);
// L3CR1: "L3CR001\0", version, count, payload CRC32, then 52-byte entries.
std::vector<uint8_t> cook_l3clut_ramp(const std::vector<TerrainClutRampEntry>& entries);
std::vector<TerrainClutRampEntry> load_l3clut_ramp(const uint8_t* bytes, size_t size);
const TerrainClutRampEntry* find_terrain_clut_ramp(const std::vector<TerrainClutRampEntry>& entries,
    uint32_t material_word) noexcept;
} // namespace l3
