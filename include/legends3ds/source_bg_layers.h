#pragma once
// The EXE's background layers: 11 records of 56 bytes at 0x800BBA90,
// initialised by 0x80022FD4 from a stage descriptor table and updated/drawn by
// 0x80023144 (tiled 32x32 sprites, OT entry 28 + record+0x2F). Original
// addresses are identity tokens. The camera-driven scroll (record+1 == 0) is
// not ported: such layers hold and are reported.
#include "legends3ds/frontend_title.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
constexpr size_t kSourceBgLayers = 11, kSourceBgMaxCells = 512;
constexpr size_t kSourceBgProfileBytes = 16 + 32 + kSourceBgLayers * (4 + 22 + 4 * kSourceBgMaxCells + 2);

// One cooked descriptor (L3BG, from the stage's 0x80022FD4 table).
struct SourceBgDescriptor {
    bool present = false;
    std::array<uint16_t,11> init{};    // p+0..p+20, in 0x80022FD4 read order
    std::vector<uint32_t> tiles;       // width*height words from p+32
};
using SourceBgProfile = std::array<SourceBgDescriptor,kSourceBgLayers>;
// L3BG v2, or v3 with the area's 40-byte record after the descriptors.
bool load_source_bg_profile(const std::vector<uint8_t>& bytes, SourceBgProfile& out);
// v3 only: the area record (0x800C0DC0 copy: ramp table, far colour +32..34,
// fog distance +0x24 for 0x8007A1CC).
std::optional<std::array<uint8_t,40>> source_bg_area_record(const std::vector<uint8_t>& bytes);

// The fields of one 56-byte record the routines read and write.
struct SourceBgLayer {
    bool present = false;
    uint8_t flags = 0;        // +0: bit 0 update scroll, bit 1 draw
    uint8_t mode = 0;         // +1: != 0 velocity scroll (wrapped); 0 camera scroll
    int32_t x16 = 0, y16 = 0; // +4, +8 (16.16)
    int16_t x = 0, y = 0;     // +0xC, +0xE: the drawn scroll
    int16_t x28 = 0, y30 = 0; // +0x1C, +0x1E
    int16_t vx = 0, vy = 0;   // +0x20, +0x22 (1/256 px per update)
    int16_t width = 0, height = 0;   // +0x24, +0x26 (tiles)
    uint16_t tile_add = 0;    // +0x28 (added << 14 to each tile word)
    uint16_t tpage = 0;       // +0x2A
    uint32_t colour = 0;      // +0x2C word (low 24 bits used)
    uint8_t ot_entry = 0;     // +0x2F (high byte of +0x2E)
    uint8_t semi = 0, abr = 0;   // +0x30, +0x31
    std::vector<uint32_t> tiles;
};
using SourceBgLayers = std::array<SourceBgLayer,kSourceBgLayers>;

// 0x80022FD4 for every present descriptor; absent layers stay inactive.
void init_source_bg_layers_80022fd4(const SourceBgProfile& profile, SourceBgLayers& layers) noexcept;

struct SourceBgUpdate { uint8_t updated = 0, camera_held = 0; };
// 0x80023144 scroll part (+ 0x8002366C wrap and integer copy), one update.
SourceBgUpdate update_source_bg_layers_80023144(SourceBgLayers& layers) noexcept;

// 0x80023144 draw part: per drawn layer, one 32x32 tile per cell over the
// 320x240 screen from -(x mod 32), -(y mod 32), map indices wrapping. Each
// SPRT (code 0x64 | semi << 1, tpage from the preceding E1 word) is returned
// as a POLY_FT4 over the same 32x32 rectangle with texels u..u+31, v..v+31.
// Packets come in the original's draw order (layers share OT entries and
// AddPrim prepends, so layer 10 draws first);
// bucket = 28 + ot_entry. widescreen_margin extends the columns by that many
// pixels on each side (port presentation only; 0 = the original).
std::vector<FrontendPacket> draw_source_bg_layers_80023144(const SourceBgLayers& layers, int widescreen_margin = 0);
} // namespace l3
