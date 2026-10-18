#pragma once
#include "st00_source.h"
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {
constexpr uint16_t kL3RenderVersion = 3;

struct SourceRenderPrimitive {
    uint32_t prefix = 0;
    std::array<uint8_t,8> uv{};
    // Legacy L3RG storage gather; all twelve compressed bytes are retained.
    // Use positive_terrain.h to recover the executable-backed XYZ interpretation.
    // The inherited flattened expander remains available for the 0143 fallback.
    std::array<int8_t,4> local_x{};
    std::array<int8_t,4> local_z{};
    std::array<int8_t,4> ignored_raw{};
};
struct SourceRenderStream {
    uint32_t source_uv_word = 0;
    uint32_t source_geom_word = 0;
    uint8_t code = 0;
    uint8_t base_shift = 0;
    uint8_t material_slot = 0;
    std::vector<SourceRenderPrimitive> primitives;
};
struct SourceRenderRecord {
    uint32_t source_ptr = 0;
    uint8_t flags_high2 = 0;
    uint8_t byte2 = 0;
    uint8_t byte3 = 0;
    std::vector<uint32_t> materials; // proven packed PS1 (CLUT<<16)|TPAGE words; kept raw too.
    std::vector<uint32_t> stream_indices;
};
struct SourceRenderDescriptor {
    std::array<uint16_t,6> record_indices{{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}};
};
struct St00RenderSource {
    std::vector<SourceRenderDescriptor> descriptors;
    std::vector<SourceRenderRecord> records;
    std::vector<SourceRenderStream> streams;
    // Neutral identity of the descriptor selected by negative cells with bit7
    // clear. The supplied STG starts with that descriptor, which is absent from
    // HED/IDX references. FFFF means this binding was not cooked or not needed.
    uint16_t default_negative_descriptor = 0xFFFFu;
    // Neutral HD2-selected identities. FFFF keeps missing/null/unresolved rows
    // explicit; the packed selector is (negative_cell >> 2) & 31.
    std::array<uint16_t, 32> special_negative_descriptors{{
        0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
        0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
        0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
        0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}};
};

struct ExpandedRenderPrimitive {
    uint32_t prefix = 0;
    uint32_t material_word = 0;
    // Unknown dynamic bindings retain opaque geometry without borrowing a cache.
    bool texture_binding_known = true;
    std::array<uint8_t,8> uv{};
    std::array<int32_t,4> world_x{};
    std::array<int32_t,4> world_y{};
    std::array<int32_t,4> world_z{};
};


enum class St00RenderBank : uint8_t { Primary = 0, Secondary = 1 };
enum class St00Lod : uint8_t { Near = 0, Mid = 1, Far = 2 };

struct St00LodSelection {
    St00Lod lod = St00Lod::Near;
    bool near_replay_path = false;
    uint8_t descriptor_slot = 0;
};

// Exact primary-bank depth thresholds traced at ROCK_NEO.EXE 0x8002196C.
// depth < 0x0401 is still near LOD but is queued for the alternate near/replay renderer.
St00LodSelection select_st00_primary_lod(uint32_t transformed_depth);
uint8_t st00_descriptor_slot(St00RenderBank bank, St00Lod lod);

struct St00CellOrigin {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

struct L3RenderStats {
    uint32_t descriptor_count=0, record_count=0, stream_count=0, stream_references=0, material_words=0, primitive_count=0;
};
struct L3RenderHeader {
    uint16_t version=0, header_bytes=0; uint32_t flags=0; L3RenderStats stats{};
    uint32_t off_descriptors=0, off_records=0, off_materials=0, off_stream_refs=0, off_streams=0, off_primitives=0;
    uint32_t file_bytes=0, payload_crc32=0;
    uint16_t default_negative_descriptor = 0xFFFFu;
    uint32_t off_special_negative_descriptors = 0;
    uint32_t special_negative_descriptor_count = 0;
};

St00RenderSource parse_st00_render(const St00Source& source, const std::vector<uint8_t>& stg);

// Legacy grid-centered origin helper retained for compatibility. Positive IDX
// rendering uses the authored XYZ through area_placement.h instead. For a positive cell the Y comes
// from IDX[cell].y. Negative/special cells use -((cell & 0x1F00) >> 1), matching 0x80021834/0x800221E0.
St00CellOrigin st00_cell_origin(uint8_t grid_x, uint8_t grid_z, int16_t packed_cell, const St00Source& source);

// CPU reference expansion for one descriptor slot (0..5). Slots 0..2 are the proven primary near/mid/far LODs;
// slots 3..5 are a preserved secondary bank whose switching semantics remain unknown. The engine can feed the resulting
// X/Y/Z + UV + material directly to a backend after material/texture resolution. packed_cell low two bits
// are the exact quarter-turn controls; global_scale_shift is the runtime scale byte added to stream.base_shift.
std::vector<ExpandedRenderPrimitive> expand_st00_descriptor_side(const St00RenderSource& source,
                                                                 size_t descriptor_index, size_t side,
                                                                 const St00CellOrigin& origin,
                                                                 uint16_t packed_cell,
                                                                 uint8_t global_scale_shift = 0);

std::vector<ExpandedRenderPrimitive> expand_st00_descriptor_lod(const St00RenderSource& source,
                                                                size_t descriptor_index, St00RenderBank bank, St00Lod lod,
                                                                const St00CellOrigin& origin,
                                                                uint16_t packed_cell,
                                                                uint8_t global_scale_shift = 0);

std::vector<uint8_t> cook_l3render(const St00RenderSource& source, L3RenderStats* stats=nullptr);
L3RenderHeader inspect_l3render(const std::vector<uint8_t>& bytes,bool verify_crc=true);
St00RenderSource load_l3render(const std::vector<uint8_t>& bytes,bool verify_crc=true);
}
