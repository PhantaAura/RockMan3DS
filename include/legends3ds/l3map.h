#pragma once
#include "st00_source.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace l3 {
constexpr uint16_t kL3MapVersion = 1;
struct CookStats {
    uint32_t area_count = 0;
    uint32_t cell_count = 0;
    uint32_t positive_cell_count = 0;
    uint32_t negative_cell_count = 0;
    uint32_t idx_count = 0;
    uint32_t descriptor_count = 0;
    uint32_t contact_list_count = 0;
    uint32_t contact_group_count = 0;
    uint32_t contact_primitive_count = 0;
};
struct L3MapHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    CookStats stats{};
    uint32_t off_areas = 0;
    uint32_t off_cells = 0;
    uint32_t off_idx = 0;
    uint32_t off_descriptors = 0;
    uint32_t off_contact_lists = 0;
    uint32_t off_contact_groups = 0;
    uint32_t off_contact_primitives = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

// Fully pointer-free runtime view of L3MP.  This mirrors the cooker sections
// instead of recreating PS1 pointers, so the same structure can be used by the
// host validation harness and the eventual 3DS backend.
struct RuntimeMapArea {
    int16_t start_x = 0;
    int16_t start_z = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint32_t cell_first = 0;
    uint32_t cell_count = 0;
    int32_t world_origin_x = 0;
    int32_t world_origin_z = 0;
};

struct RuntimeMapIdx {
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    int16_t raw6 = 0;
    uint16_t descriptor_index = 0xFFFFu;
};

struct RuntimeMapDescriptor {
    uint32_t word0 = 0;
    uint16_t alt_contact_index = 0xFFFFu;
    uint16_t default_contact_index = 0xFFFFu;
    std::array<int16_t, 6> raw_words3_5{};
    std::array<uint32_t, 6> render_offsets{};
};

struct RuntimeContactPrimitive {
    int16_t y0 = 0;
    int16_t y1 = 0;
    uint16_t code = 0;
    uint16_t flags = 0;
};

struct RuntimeContactGroup {
    int16_t x0 = 0;
    int16_t x1 = 0;
    int16_t z0 = 0;
    int16_t z1 = 0;
    uint32_t primitive_first = 0;
    uint32_t primitive_count = 0;
};

struct RuntimeContactList {
    uint32_t group_first = 0;
    uint32_t group_count = 0;
};

struct RuntimeMap {
    std::vector<RuntimeMapArea> areas;
    std::vector<int16_t> cells;
    std::vector<RuntimeMapIdx> idx;
    std::vector<RuntimeMapDescriptor> descriptors;
    std::vector<RuntimeContactList> contact_lists;
    std::vector<RuntimeContactGroup> contact_groups;
    std::vector<RuntimeContactPrimitive> contact_primitives;
};

struct RuntimeCellRef {
    size_t area_index = 0;
    uint16_t grid_x = 0;
    uint16_t grid_z = 0;
    uint32_t cell_index = 0;
    int16_t packed_cell = 0;
    int32_t world_origin_x = 0;
    int32_t world_origin_z = 0;
};

enum class RuntimeContactDispatchFamily : uint8_t {
    Family0 = 0,
    Family1 = 1,
    Family2 = 2,
    Family3 = 3,
    Family4 = 4,
    Family5 = 5,
    Family6 = 6,
    Family7 = 7,
};

struct RuntimeContactPrimitiveRef {
    uint32_t primitive_index = 0;
    RuntimeContactPrimitive primitive{};
    std::optional<RuntimeContactDispatchFamily> dispatch_family;
};

struct RuntimeContactGroupRef {
    uint16_t contact_list_index = 0xFFFFu;
    uint32_t group_index = 0;
    int32_t world_x0 = 0;
    int32_t world_x1 = 0;
    int32_t world_z0 = 0;
    int32_t world_z1 = 0;
    uint32_t primitive_first = 0;
    uint32_t primitive_count = 0;
    std::vector<RuntimeContactPrimitiveRef> primitives;
};

RuntimeMap load_l3map(const std::vector<uint8_t>& bytes, bool verify_crc = true);

// Exact ST00 containing-cell convention from retail collision candidate gather:
// one cell is 0x200 world units, grid-zero bounds begin at -0x8000, and its
// renderer/contact origin is the center at -0x7F00. This routine only resolves
// the grid cell; it does not guess contact-surface semantics.
std::optional<RuntimeCellRef> runtime_map_find_cell(const RuntimeMap& map,
                                                    size_t area_index,
                                                    int32_t world_x,
                                                    int32_t world_z);

// Returns the IDX row referenced by a positive packed cell.  Negative/special
// cells intentionally return null because their semantics are encoded in the
// packed value rather than an IDX row.
const RuntimeMapIdx* runtime_map_cell_idx(const RuntimeMap& map,
                                          const RuntimeCellRef& cell);

const RuntimeMapDescriptor* runtime_map_cell_descriptor(const RuntimeMap& map,
                                                        const RuntimeCellRef& cell);

// USA ROCK_NEO.EXE 0x8005172C..0x80051754 chooses descriptor +4 (alternate)
// when IDX raw6 low 12 bits are nonzero, otherwise descriptor +8 (default).
// These helpers reproduce that proved list-selection and cell-local group transform
// without guessing the still-untranslated primitive response handlers.
std::optional<uint16_t> runtime_map_cell_contact_list_index(const RuntimeMap& map,
                                                             const RuntimeCellRef& cell);
const RuntimeContactList* runtime_map_cell_contact_list(const RuntimeMap& map,
                                                        const RuntimeCellRef& cell);
std::vector<RuntimeContactGroupRef> runtime_map_cell_contact_groups(const RuntimeMap& map,
                                                                    const RuntimeCellRef& cell);

// USA ROCK_NEO.EXE 0x8005185C..0x80051880 dispatches contact-group
// geometry through (primitive.code & 0x00F0) >> 4.  The observed retail jump
// table contains families 0..7.  This helper intentionally exposes only that
// proved dispatcher identity; it does not assign gameplay semantics to a family.
std::optional<RuntimeContactDispatchFamily> runtime_contact_dispatch_family(uint16_t code);

// Exact four-neighbor candidate gather performed by USA ROCK_NEO.EXE
// 0x8004B1E0. The original routine samples the current cell plus one X/Z
// neighbor selected from the query's low 9 bits, filters against the active
// area's bounds, and de-duplicates only positive packed-cell references.
// Zero cells are included only when include_zero is true; negative/special
// cells are retained exactly like retail and are not de-duplicated.
std::vector<RuntimeCellRef> runtime_map_collect_contact_candidates_8004b1e0(
    const RuntimeMap& map,
    size_t area_index,
    int16_t query_x,
    int16_t query_z,
    bool include_zero = false);

struct RuntimeContactFamily01Query {
    int16_t subtract_x0 = 0; // retail scratch +0x126
    int16_t subtract_x1 = 0; // retail scratch +0x124
    int16_t subtract_z0 = 0; // retail scratch +0x12A
    int16_t subtract_z1 = 0; // retail scratch +0x128
    int16_t query_x = 0;     // retail scratch +0x0C0
    int16_t query_z = 0;     // retail scratch +0x0C4
};

// Exact arithmetic/control-flow translation of USA ROCK_NEO.EXE 0x8004E304,
// shared by dispatcher families 0 and 1.  Return value is the original 0..4
// classifier code; no semantic name is assigned until the downstream retail
// response routine is recovered.
uint8_t runtime_contact_family01_classify_8004e304(const RuntimeContactGroupRef& group,
                                                    const RuntimeContactFamily01Query& query);

std::vector<uint8_t> cook_st00_map(const St00Source& src, CookStats* stats = nullptr);
L3MapHeader inspect_l3map(const std::vector<uint8_t>& bytes, bool verify_crc = true);
uint32_t crc32(const uint8_t* data, size_t size);
}
