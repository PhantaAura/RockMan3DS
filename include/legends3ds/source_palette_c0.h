#pragma once
// ST00 family-C0 palette objects (subtypes 3 and 4) and the colour buffer
// they write: 0x800AD168, 64 rows of 256 RGB555 words. Rows 0..31 are the
// source palette bank the stage loader fills (0x8001BE10/0x8001BF44); rows
// 32..63 are the mirror uploaded every frame by 0x800178FC as VRAM
// (0,240,256,16) and (0,496,256,16). Original addresses are identity tokens.
#include "legends3ds/source_event_phase5.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
constexpr uint32_t kSourcePaletteBuffer800ad168 = 0x800AD168u;
constexpr size_t kSourcePaletteRows = 64, kSourcePaletteRowWords = 256;
constexpr size_t kSourcePaletteTimelineCapacity = 16;
// Exact cooked sizes (header 16 + payload), for read admission limits.
constexpr size_t kSourcePaletteC0ProfileBytes = 16 + 32 + 10 * (8 + 4 * kSourcePaletteTimelineCapacity) + 16 + 36 + 12 +
    2 * (1 + 6 * 32) + 2 * 24 * (1 + 4 * 32);
constexpr size_t kSourcePaletteBankBytes = 16 + 4 + 2 * 8192 + 8192;

struct SourcePaletteBuffer {
    std::array<uint16_t,kSourcePaletteRows * kSourcePaletteRowWords> words{};
    std::array<uint8_t,kSourcePaletteRows * kSourcePaletteRowWords> known{};
    uint64_t changed_rows = 0;   // bit r: row r written since the last take
};

struct SourcePaletteTimeline {
    uint32_t base_token = 0;     // original address of entry 0
    uint8_t count = 0;           // entries up to and including the first control < 0
    std::array<std::array<uint8_t,4>,kSourcePaletteTimelineCapacity> entries{};
};
// Cooked from the ST00 overlay (L3C0): subtype 3 timelines 0x8011CF28[v] and
// destination/mode pairs 0x8011CF38[v]; subtype 4 timelines 0x8011D10C[v] and
// {destination, count, mode} records 0x8011D178[v].
struct SourcePaletteC0Profile {
    bool loaded = false;
    std::array<SourcePaletteTimeline,4> sub3{};
    std::array<std::array<uint16_t,2>,4> sub3_destination{};
    std::array<SourcePaletteTimeline,6> sub4{};
    std::array<std::array<uint16_t,3>,6> sub4_record{};
    std::array<std::array<uint8_t,2>,6> e0_uv_frames{};   // 0x8011CA94 (E0 variant 4)
    std::vector<uint8_t> face_block;   // opening face cues/scripts (source_scene_face.h)
};
bool load_source_palette_c0_profile(const std::vector<uint8_t>& bytes, SourcePaletteC0Profile& out);

// Initial buffer from the cooked 32-row source bank (L3PB): rows 0..31 take
// the bank, mirror rows 32..63 take the bank rows the loader dirtied (the
// first 0x8001786C staging); other mirror words stay unknown.
bool load_source_palette_bank(const std::vector<uint8_t>& bytes, SourcePaletteBuffer& out);

// 0x8002ED6C family-C0 branch: 0x800316BC allocation (first free of 16 in
// the used word, +2..+0x13 cleared, +6 = 3), then +0 = rec[0], +2 = rec[2..3],
// +4 = rec[4..5], +0xC = rec[8..11], +0x14 = rec[12..15], +0x18 = rec[16..19].
// Returns the slot, or nothing when the pool is full or its owners unknown.
std::optional<size_t> spawn_source_c0_record_8002ed6c(SourceEventPhase5Pool<16,36>& pool,
    const std::array<uint8_t,20>& record) noexcept;

struct SourcePaletteC0Pass {
    uint8_t updated = 0, released = 0, held = 0;
};
// 0x80017478 sweep over the 16 C0 slots: (+0 & +6 & 1) dispatches subtype 3
// (0x80119398: states 0x801193D4/0x80119470/0x80119518) and subtype 4
// (0x80119800: 0x8011983C/0x801198D8). Release clears +0 (0x80031988 and
// 0x800319F4; the C0 pending-release bit and the stray family-80 mask write of
// 0x80031988 are not modelled). Other subtypes are held untouched.
SourcePaletteC0Pass update_source_palette_c0_80017478(SourceEventPhase5Pool<16,36>& pool,
    SourcePaletteBuffer& buffer, const SourcePaletteC0Profile& profile) noexcept;
} // namespace l3
