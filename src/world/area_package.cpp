#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/material_cache.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeader = 20, kRow = 16, kMaxSections = 64, kScratch = 8192;
// Conservative allowance per separately allocated vector. libc allocator overhead
// varies; this is an admission estimate, never a measurement of process RAM.
constexpr uint64_t kAllocationAllowance = 64;
using File = std::unique_ptr<FILE, int(*)(FILE*)>;
[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }
void require(bool condition, const char* message) { if (!condition) fail(message); }
uint64_t add(uint64_t a, uint64_t b) {
    require(b <= std::numeric_limits<uint64_t>::max() - a, "area package allocation estimate overflow");
    return a + b;
}
uint64_t array_bytes(uint64_t count, size_t stride) {
    require(!stride || count <= std::numeric_limits<uint64_t>::max() / stride, "area package array estimate overflow");
    return count * stride;
}
uint64_t vector_bytes(uint64_t count, size_t stride) {
    return add(array_bytes(count, stride), count ? kAllocationAllowance : 0);
}
void read_at(FILE* file, uint32_t offset, uint8_t* data, size_t bytes) {
    const long seek_offset = static_cast<long>(offset);
    require(seek_offset >= 0 && static_cast<uint64_t>(seek_offset) == offset, "area package seek exceeds platform range");
    require(std::fseek(file, seek_offset, SEEK_SET) == 0, "area package seek failed");
    require(!bytes || std::fread(data, 1, bytes, file) == bytes, "area package short read");
}
uint32_t crc_step(uint32_t c, const uint8_t* data, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) {
        c ^= data[i];
        for (unsigned b = 0; b < 8; ++b) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return c;
}
uint32_t file_crc(FILE* file, const AreaPackageSection& section) {
    std::array<uint8_t, kScratch> scratch{};
    uint32_t c = 0xffffffffu, offset = section.offset, remaining = section.size;
    while (remaining) {
        const size_t count = std::min<size_t>(remaining, scratch.size());
        read_at(file, offset, scratch.data(), count);
        c = crc_step(c, scratch.data(), count);
        offset += static_cast<uint32_t>(count);
        remaining -= static_cast<uint32_t>(count);
    }
    return ~c;
}
std::vector<uint8_t> read_section(FILE* file, const AreaPackageSection& section) {
    std::vector<uint8_t> data(section.size);
    read_at(file, section.offset, data.data(), data.size());
    // Detect a file changed between envelope verification and decoding.
    require(crc32(data.data(), data.size()) == section.crc32, "area package section changed while loading");
    return data;
}
void canonical(uint32_t begin, uint32_t count, uint32_t stride, uint32_t end, const char* message) {
    require(uint64_t(begin) + uint64_t(count) * stride == end, message);
}
uint64_t map_preflight(const std::vector<uint8_t>& data, const AreaPackageLimits& limits) {
    const auto h = inspect_l3map(data, true);
    const auto& s = h.stats;
    require(h.flags == 0 && h.off_areas == 84, "L3MP unsupported flags or area offset");
    canonical(h.off_areas, s.area_count, 24, h.off_cells, "L3MP noncanonical area range");
    canonical(h.off_cells, s.cell_count, 2, h.off_idx, "L3MP noncanonical cell range");
    canonical(h.off_idx, s.idx_count, 12, h.off_descriptors, "L3MP noncanonical IDX range");
    canonical(h.off_descriptors, s.descriptor_count, 44, h.off_contact_lists, "L3MP noncanonical descriptor range");
    canonical(h.off_contact_lists, s.contact_list_count, 8, h.off_contact_groups, "L3MP noncanonical list range");
    canonical(h.off_contact_groups, s.contact_group_count, 16, h.off_contact_primitives, "L3MP noncanonical group range");
    canonical(h.off_contact_primitives, s.contact_primitive_count, 8, h.file_bytes, "L3MP noncanonical primitive range");
    require(s.area_count != 0 && s.area_count <= limits.max_map_areas && s.descriptor_count <= 65535 && s.contact_list_count <= 65535,
            "L3MP absent areas or unaddressable table");
    uint64_t cell_cursor = 0, group_cursor = 0, primitive_cursor = 0;
    for (uint32_t i = 0; i < s.area_count; ++i) {
        const auto* p = data.data() + h.off_areas + size_t(i) * 24;
        const uint32_t first = rd_u32(p + 8), count = rd_u32(p + 12);
        require(first == cell_cursor && uint64_t(rd_u16(p + 4)) * rd_u16(p + 6) == count,
                "L3MP noncanonical area cell coverage");
        cell_cursor += count;
        require(cell_cursor <= s.cell_count, "L3MP area cell range overflow");
    }
    require(cell_cursor == s.cell_count, "L3MP incomplete cell coverage");
    uint32_t positive = 0, negative = 0;
    for (uint32_t i = 0; i < s.cell_count; ++i) {
        const int16_t cell = rd_s16(data.data() + h.off_cells + size_t(i) * 2);
        if (cell > 0) {
            ++positive;
            require(static_cast<uint16_t>(cell) < s.idx_count, "L3MP positive cell IDX missing");
        } else if (cell < 0) ++negative;
    }
    require(positive == s.positive_cell_count && negative == s.negative_cell_count, "L3MP cell statistics mismatch");
    for (uint32_t i = 0; i < s.contact_list_count; ++i) {
        const auto* p = data.data() + h.off_contact_lists + size_t(i) * 8;
        require(rd_u32(p) == group_cursor, "L3MP noncanonical group coverage");
        group_cursor += rd_u32(p + 4);
        require(group_cursor <= s.contact_group_count, "L3MP group range overflow");
    }
    require(group_cursor == s.contact_group_count, "L3MP incomplete group coverage");
    for (uint32_t i = 0; i < s.contact_group_count; ++i) {
        const auto* p = data.data() + h.off_contact_groups + size_t(i) * 16;
        require(rd_u32(p + 8) == primitive_cursor, "L3MP noncanonical contact primitive coverage");
        primitive_cursor += rd_u32(p + 12);
        require(primitive_cursor <= s.contact_primitive_count, "L3MP contact primitive range overflow");
    }
    require(primitive_cursor == s.contact_primitive_count, "L3MP incomplete contact primitive coverage");
    uint64_t estimate = sizeof(RuntimeMap);
    estimate = add(estimate, vector_bytes(s.area_count, sizeof(RuntimeMapArea)));
    estimate = add(estimate, vector_bytes(s.cell_count, sizeof(int16_t)));
    estimate = add(estimate, vector_bytes(s.idx_count, sizeof(RuntimeMapIdx)));
    estimate = add(estimate, vector_bytes(s.descriptor_count, sizeof(RuntimeMapDescriptor)));
    estimate = add(estimate, vector_bytes(s.contact_list_count, sizeof(RuntimeContactList)));
    estimate = add(estimate, vector_bytes(s.contact_group_count, sizeof(RuntimeContactGroup)));
    return add(estimate, vector_bytes(s.contact_primitive_count, sizeof(RuntimeContactPrimitive)));
}
struct RenderEstimate { uint64_t resident = 0, temporary = 0; };
RenderEstimate render_preflight(const std::vector<uint8_t>& data, size_t map_descriptors) {
    const auto h = inspect_l3render(data, true);
    const auto& s = h.stats;
    require(s.descriptor_count == map_descriptors, "area package map/render descriptor mismatch");
    require((h.flags & 7u) == 7u && !(h.flags & ~31u) && h.off_descriptors == 80,
            "L3RG unsupported flags or descriptor offset");
    if (h.flags & 16u) {
        require(h.special_negative_descriptor_count == 32, "L3RG special table count invalid");
        canonical(h.off_descriptors, s.descriptor_count, 12, h.off_special_negative_descriptors,
                  "L3RG noncanonical descriptors");
        canonical(h.off_special_negative_descriptors, 32, 2, h.off_records,
                  "L3RG noncanonical special descriptor table");
    } else {
        require(h.off_special_negative_descriptors == 0 && h.special_negative_descriptor_count == 0,
                "L3RG unbound special descriptor fields nonzero");
        canonical(h.off_descriptors, s.descriptor_count, 12, h.off_records,
                  "L3RG noncanonical descriptors");
    }
    canonical(h.off_records, s.record_count, 24, h.off_materials, "L3RG noncanonical records");
    canonical(h.off_materials, s.material_words, 4, h.off_stream_refs, "L3RG noncanonical materials");
    canonical(h.off_stream_refs, s.stream_references, 4, h.off_streams, "L3RG noncanonical references");
    canonical(h.off_streams, s.stream_count, 20, h.off_primitives, "L3RG noncanonical streams");
    canonical(h.off_primitives, s.primitive_count, 24, h.file_bytes, "L3RG noncanonical primitives");
    require(s.record_count <= 65535, "L3RG unaddressable records");
    uint64_t mat_cursor = 0, ref_cursor = 0, prim_cursor = 0, nested_allocations = 0;
    for (uint32_t i = 0; i < s.record_count; ++i) {
        const auto* p = data.data() + h.off_records + size_t(i) * 24;
        const uint16_t mats = rd_u16(p + 4), refs = rd_u16(p + 16);
        require(rd_u32(p) == mat_cursor && rd_u32(p + 12) == ref_cursor, "L3RG overlapping/gapped record ranges");
        mat_cursor += mats; ref_cursor += refs;
        require(mat_cursor <= s.material_words && ref_cursor <= s.stream_references, "L3RG record range overflow");
        nested_allocations += (mats != 0) + (refs != 0);
        for (uint16_t j = 0; j < refs; ++j) {
            const uint32_t index = rd_u32(data.data() + h.off_stream_refs + (rd_u32(p + 12) + size_t(j)) * 4);
            require(index < s.stream_count, "L3RG stream index invalid");
            const auto* stream = data.data() + h.off_streams + size_t(index) * 20;
            require(stream[2] < mats, "L3RG stream material slot invalid");
        }
    }
    require(mat_cursor == s.material_words && ref_cursor == s.stream_references, "L3RG incomplete record coverage");
    for (uint32_t i = 0; i < s.stream_count; ++i) {
        const auto* p = data.data() + h.off_streams + size_t(i) * 20;
        require(p[1] <= 15 && p[1] == (p[0] >> 4) && p[2] == (p[0] & 15), "L3RG inconsistent stream code/shift/material");
        require(rd_u32(p + 4) == prim_cursor, "L3RG overlapping/gapped primitive ranges");
        const uint32_t count = rd_u32(p + 8);
        prim_cursor += count;
        require(prim_cursor <= s.primitive_count, "L3RG primitive range overflow");
        nested_allocations += count != 0;
    }
    require(prim_cursor == s.primitive_count, "L3RG incomplete primitive coverage");
    RenderEstimate result;
    result.resident = sizeof(St00RenderSource);
    result.resident = add(result.resident, vector_bytes(s.descriptor_count, sizeof(SourceRenderDescriptor)));
    result.resident = add(result.resident, vector_bytes(s.record_count, sizeof(SourceRenderRecord)));
    result.resident = add(result.resident, vector_bytes(s.stream_count, sizeof(SourceRenderStream)));
    result.resident = add(result.resident, array_bytes(s.material_words, sizeof(uint32_t)));
    result.resident = add(result.resident, array_bytes(s.stream_references, sizeof(uint32_t)));
    result.resident = add(result.resident, array_bytes(s.primitive_count, sizeof(SourceRenderPrimitive)));
    result.resident = add(result.resident, array_bytes(nested_allocations, kAllocationAllowance));
    // Existing reader temporary arrays: records/streams metadata plus flat arrays.
    // 32/24 safely exceed its present metadata struct sizes on host and ARM.
    result.temporary = vector_bytes(s.record_count, 32);
    result.temporary = add(result.temporary, vector_bytes(s.stream_count, 24));
    result.temporary = add(result.temporary, vector_bytes(s.material_words, sizeof(uint32_t)));
    result.temporary = add(result.temporary, vector_bytes(s.stream_references, sizeof(uint32_t)));
    result.temporary = add(result.temporary, vector_bytes(s.primitive_count, sizeof(SourceRenderPrimitive)));
    return result;
}
void admitted(uint64_t decoded, uint64_t peak, const AreaPackageLimits& limits) {
    require(decoded <= limits.max_decoded_bytes, "area package decoded budget exceeded");
    require(peak <= limits.max_load_peak_bytes, "area package load peak budget exceeded");
}
constexpr uint32_t kRuntimeHeader = 40, kRuntimeRow = 20;
std::vector<uint8_t> slice(const std::vector<uint8_t>& bytes, uint32_t offset, uint32_t size) {
    require(offset <= bytes.size() && size <= bytes.size() - offset, "runtime binding slice outside section");
    return {bytes.begin() + offset, bytes.begin() + offset + size};
}
struct RuntimeEstimate {
    uint64_t resident = 0, temporary = 0, source_payload = 0;
};
RuntimeEstimate runtime_preflight(const std::vector<uint8_t>& bytes, size_t map_areas,
                                  bool require_source_identity) {
    require(bytes.size() >= kRuntimeHeader && rd_u32(bytes.data()) == 0x4252334cu &&
            rd_u16(bytes.data() + 4) == 1 && rd_u16(bytes.data() + 6) == kRuntimeHeader,
            "unsupported runtime binding header");
    require(rd_u32(bytes.data() + 8) == static_cast<uint32_t>(AreaRuntimeBindingBackend::St00FlowV1) &&
            rd_u32(bytes.data() + 12) == 0, "unsupported runtime binding backend or flags");
    const uint32_t count = rd_u32(bytes.data() + 16), rows = rd_u32(bytes.data() + 20);
    const uint32_t flow_offset = rd_u32(bytes.data() + 24), flow_size = rd_u32(bytes.data() + 28);
    require(count && count <= map_areas && rows == kRuntimeHeader &&
            uint64_t(rows) + uint64_t(count) * kRuntimeRow == flow_offset &&
            flow_offset <= bytes.size() && flow_size <= bytes.size() - flow_offset &&
            rd_u32(bytes.data() + 32) == bytes.size(), "runtime binding count or range invalid");
    require(crc32(bytes.data() + kRuntimeHeader, bytes.size() - kRuntimeHeader) == rd_u32(bytes.data() + 36),
            "runtime binding CRC mismatch");
    const auto flow_bytes = slice(bytes, flow_offset, flow_size);
    const auto flow = inspect_l3st00_flow(flow_bytes, true);
    require(flow.flags == 0x1fu && flow.off_areas == 40, "runtime L3SF flags or area offset invalid");
    canonical(flow.off_areas, flow.area_count, 48, flow.off_objects, "runtime L3SF noncanonical area table");
    require(uint64_t(flow.off_setup_blobs) + uint64_t(flow.area_count) * kSt00AreaSetupBlobBytes == flow_bytes.size(),
            "runtime L3SF noncanonical setup blobs");
    uint64_t flow_temporary = sizeof(St00FlowSource);
    uint32_t object_cursor = 0;
    for (uint32_t area = 0; area < flow.area_count; ++area) {
        const auto* row = flow_bytes.data() + flow.off_areas + size_t(area) * 48;
        const uint32_t objects = rd_u32(row + 8), aux = rd_u32(row + 40);
        require(rd_u32(row) == area && rd_u32(row + 4) == object_cursor && rd_u32(row + 44) == 0 &&
                uint64_t(object_cursor) + objects <= flow.object_count, "runtime L3SF area coverage invalid");
        object_cursor += objects;
        flow_temporary = add(flow_temporary, vector_bytes(objects, sizeof(StageObjectRecord)));
        flow_temporary = add(flow_temporary, vector_bytes(aux, sizeof(St00AuxRecord)));
    }
    require(object_cursor == flow.object_count, "runtime L3SF incomplete object coverage");
    RuntimeEstimate estimate;
    estimate.resident = vector_bytes(count, sizeof(TutorialRuntimeArea));
    uint64_t cursor = uint64_t(flow_offset) + flow_size, max_decode_temporary = 0;
    uint32_t prior_local = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const auto* row = bytes.data() + rows + size_t(i) * kRuntimeRow;
        const uint32_t local = rd_u32(row), source = rd_u32(row + 4);
        const uint32_t offset = rd_u32(row + 8), size = rd_u32(row + 12);
        require(local < map_areas && source < kSt00TutorialAreaCount && (!i || local > prior_local),
                "runtime binding area index invalid or duplicate");
        require(!require_source_identity || local == source,
                "ST00 spawn catalog requires identity runtime area selectors");
        prior_local = local;
        require(offset == cursor && cursor <= bytes.size() && size <= bytes.size() - cursor,
                "runtime binding EBD range invalid");
        cursor += size;
        auto ebd_bytes = slice(bytes, offset, size);
        require(crc32(ebd_bytes.data(), ebd_bytes.size()) == rd_u32(row + 16), "runtime binding EBD slice CRC mismatch");
        const auto ebd = inspect_l3ebd_directory(ebd_bytes, true);
        require(ebd.version == 2 && ebd.source_payload_bytes >= 4, "runtime binding requires complete L3EB v2");
        const auto* area = flow_bytes.data() + flow.off_areas + size_t(source) * 48;
        const uint32_t first = rd_u32(area + 4), objects = rd_u32(area + 8), aux = rd_u32(area + 40);
        estimate.resident = add(estimate.resident, vector_bytes(objects, sizeof(StageObjectRecord)));
        estimate.resident = add(estimate.resident, vector_bytes(aux, sizeof(St00AuxRecord)));
        estimate.resident = add(estimate.resident, vector_bytes(ebd.entry_count, sizeof(EbdDirectoryEntry)));
        // SharedBytePayload retains a vector and shared-control allocation as well
        // as every complete EBD byte, even before any actor mesh is decoded.
        estimate.resident = add(estimate.resident, vector_bytes(ebd.source_payload_bytes, 1));
        estimate.resident = add(estimate.resident, sizeof(SharedBytePayload::Storage) + kAllocationAllowance);
        estimate.source_payload = add(estimate.source_payload, ebd.source_payload_bytes);
        estimate.resident = add(estimate.resident, vector_bytes(objects, sizeof(StageRuntimeInstance)));
        std::array<size_t, 7> pool_used{};
        for (uint32_t j = 0; j < objects; ++j) {
            const auto* object = flow_bytes.data() + flow.off_objects + size_t(first + j) * kStageObjectRecordBytes;
            const auto family = classify_stage_object_family(object[1]);
            const auto spec = stage_object_pool_spec(family);
            if (spec) {
                require(++pool_used[static_cast<size_t>(family)] <= spec->capacity,
                        "runtime stage object source exceeds original pool capacity");
                estimate.resident = add(estimate.resident, vector_bytes(spec->original_stride_bytes, 1));
            }
            // find_ebd_entries grows vectors geometrically. Twice all EBD rows is
            // a conservative capacity bound for either candidate vector, avoiding
            // quadratic preflight work on malicious object/directory combinations.
            const uint64_t candidates = vector_bytes(uint64_t(ebd.entry_count) * 2, sizeof(size_t));
            estimate.resident = add(estimate.resident, add(candidates, candidates));
        }
        uint64_t temporary = add(vector_bytes(objects, sizeof(StageRuntimeObject)),
                                 vector_bytes(objects, sizeof(StageRuntimeAllocation)));
        // A candidate-vector growth keeps its prior allocation live until the
        // replacement succeeds. Include that transient allocation and stack
        // bootstrap objects as well as the final resident capacity bounds.
        temporary = add(temporary, vector_bytes(ebd.entry_count, sizeof(size_t)));
        temporary = add(temporary, sizeof(TutorialRuntimeArea) + sizeof(StageRuntimeInstance));
        temporary = add(temporary, vector_bytes(size, 1));
        max_decode_temporary = std::max(max_decode_temporary, temporary);
    }
    require(cursor == bytes.size(), "runtime binding incomplete EBD coverage");
    estimate.temporary = add(add(flow_temporary, vector_bytes(flow_size, 1)), max_decode_temporary);
    return estimate;
}
std::vector<TutorialRuntimeArea> load_runtime_bindings(const std::vector<uint8_t>& bytes) {
    const uint32_t count = rd_u32(bytes.data() + 16), rows = rd_u32(bytes.data() + 20);
    const auto flow = slice(bytes, rd_u32(bytes.data() + 24), rd_u32(bytes.data() + 28));
    std::vector<TutorialRuntimeArea> out;
    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const auto* row = bytes.data() + rows + size_t(i) * kRuntimeRow;
        const auto ebd = slice(bytes, rd_u32(row + 8), rd_u32(row + 12));
        auto area = load_tutorial_runtime_area(flow, ebd, rd_u32(row + 4), false);
        area.area_index = rd_u32(row);
        out.push_back(std::move(area));
    }
    return out;
}
uint64_t spawn_preflight(const std::vector<uint8_t>& bytes, size_t map_areas,
                         const std::vector<TutorialRuntimeArea>& bindings) {
    const auto h=inspect_l3st00_spawn(bytes,true);
    require(map_areas==h.area_count,"ST00 spawn catalog requires five local map areas");
    uint64_t resident=sizeof(St00SpawnCatalog);
    std::array<uint64_t,kSt00SpawnAreaCount> area_bytes{};
    for(uint32_t area=0;area<h.area_count;++area) {
        const auto* row=bytes.data()+h.off_areas+size_t(area)*8;
        const uint32_t first=rd_u32(row),count=rd_u32(row+4);
        area_bytes[area]=vector_bytes(count,sizeof(St00SpawnRegion));
        for(uint32_t region=first;region<first+count;++region) {
            const auto* region_row=bytes.data()+h.off_regions+size_t(region)*16;
            area_bytes[area]=add(area_bytes[area],vector_bytes(rd_u32(region_row+12),sizeof(St00ConditionalSpawn)));
        }
        resident=add(resident,area_bytes[area]);
    }
    // The native session consumes the per-area binding while the package also
    // retains the optional catalog. Account for each explicit copy before load.
    for(const auto& binding:bindings) {
        require(binding.area_index<h.area_count,"ST00 spawn binding area outside catalog");
        resident=add(resident,add(sizeof(St00SpawnArea),area_bytes[binding.area_index]));
    }
    return resident;
}
} // namespace

void LoadedAreaPackage::unload() noexcept {
    LoadedAreaPackage empty;
    std::swap(map, empty.map);
    std::swap(render, empty.render);
    directory.swap(empty.directory);
    negative_materials.swap(empty.negative_materials);
    runtime_areas.swap(empty.runtime_areas);
    spawns.swap(empty.spawns);
    diagnostics = {};
}

uint64_t preflight_area_material_cache(const std::vector<uint8_t>& bytes,
                                       uint64_t max_decoded_bytes) {
    const auto h = inspect_l3material_cache(bytes, true);
    require(h.flags == 3u, "area material cache unsupported flags");
    uint64_t resident = add(sizeof(MaterialCache), vector_bytes(h.material_count, sizeof(MaterialCacheEntry)));
    require(resident <= max_decoded_bytes, "area material cache decoded budget exceeded");
    uint64_t cursor = h.off_payload;
    uint32_t resolved = 0, previous_word = 0;
    constexpr uint32_t page_bytes = uint32_t(kL3MaterialTextureWidth) * kL3MaterialTextureHeight;
    for (uint32_t i = 0; i < h.material_count; ++i) {
        const auto* row = bytes.data() + h.off_records + size_t(i) * h.record_bytes;
        const uint32_t word = rd_u32(row), flags = rd_u32(row + 4);
        const auto page = decode_ps1_tpage(static_cast<uint16_t>(word));
        require((page.bpp == 4 || page.bpp == 8) && row[8] == page.bpp &&
                (!i || previous_word < word) && flags <= 1u &&
                rd_u32(row + 32) == 0 && rd_u32(row + 36) == 0,
                "area material cache record invalid");
        previous_word = word;
        const uint16_t palettes = rd_u16(row + 10);
        const uint32_t index_offset = rd_u32(row + 16), indices = rd_u32(row + 20);
        const uint32_t palette_offset = rd_u32(row + 24), palette_bytes = rd_u32(row + 28);
        if (!flags) {
            require(!palettes && !index_offset && !indices && !palette_offset && !palette_bytes,
                    "area unresolved material has payload");
            continue;
        }
        require(palettes == (page.bpp == 4 ? 16u : 256u) && indices == page_bytes &&
                palette_bytes == uint32_t(palettes) * 2u && rd_u16(row + 12) != 0 &&
                rd_u16(row + 12) <= kL3MaterialTextureWidth && rd_u16(row + 14) != 0 &&
                rd_u16(row + 14) <= kL3MaterialTextureHeight,
                "area resolved material dimensions invalid");
        require(index_offset == cursor && cursor + indices == palette_offset &&
                cursor + indices + palette_bytes <= bytes.size(),
                "area material payload overlaps or exceeds cache");
        for (uint32_t j = 0; j < indices; ++j)
            require(bytes[size_t(index_offset) + j] < palettes, "area material palette index invalid");
        cursor += indices + palette_bytes;
        resident = add(resident, add(vector_bytes(indices, 1), vector_bytes(palettes, sizeof(uint16_t))));
        require(resident <= max_decoded_bytes, "area material cache decoded budget exceeded");
        ++resolved;
    }
    require(cursor == bytes.size() && resolved == h.resolved_count,
            "area material cache incomplete payload or count mismatch");
    return resident;
}

uint64_t count_area_render_primitives(const RuntimeMap& map, const St00RenderSource& render,
                                     size_t area_index, uint64_t max_primitives) {
    require(area_index < map.areas.size(), "terrain area outside map");
    const auto& area = map.areas[area_index];
    require(uint64_t(area.cell_first) + area.cell_count <= map.cells.size(), "terrain cells outside map");
    require(uint64_t(area.width) * area.height == area.cell_count, "terrain area dimensions mismatch");
    const auto slot = st00_descriptor_slot(St00RenderBank::Primary, St00Lod::Mid);
    uint64_t count = 0;
    for (uint32_t local = 0; local < area.cell_count; ++local) {
        const auto packed = map.cells[size_t(area.cell_first) + local];
        if (packed <= 0) continue;
        const size_t index = static_cast<uint16_t>(packed);
        require(index < map.idx.size(), "terrain cell IDX missing");
        const auto descriptor = map.idx[index].descriptor_index;
        if (descriptor == 0xffffu) continue;
        require(descriptor < map.descriptors.size() && descriptor < render.descriptors.size(), "terrain descriptor missing");
        const auto record_index = render.descriptors[descriptor].record_indices[slot];
        if (record_index == 0xffffu) continue;
        require(record_index < render.records.size(), "terrain record missing");
        const auto& record = render.records[record_index];
        for (const auto stream_index : record.stream_indices) {
            require(stream_index < render.streams.size(), "terrain stream missing");
            const auto& stream = render.streams[stream_index];
            require(stream.material_slot < record.materials.size(), "terrain stream material missing");
            require(stream.base_shift <= 15, "terrain stream shift exceeds expansion range");
            const uint64_t instances = stream.primitives.size();
            // Subtraction both admits against the explicit cap and prevents sum overflow.
            require(instances <= max_primitives - count, "terrain expanded primitive budget exceeded");
            count += instances;
        }
    }
    return count;
}

LoadedAreaPackage load_area_package(const std::string& path, const AreaPackageLimits& limits) {
    require(limits.max_sections <= kMaxSections, "area package section policy exceeds format limit");
    File file(std::fopen(path.c_str(), "rb"), std::fclose);
    require(bool(file), "area package open failed");
    require(std::fseek(file.get(), 0, SEEK_END) == 0, "area package length seek failed");
    const long file_length = std::ftell(file.get());
    require(file_length >= static_cast<long>(kHeader), "area package truncated header");
    const uint64_t length = static_cast<uint64_t>(file_length);
    require(length <= UINT32_MAX && length <= limits.max_file_bytes, "area package file budget exceeded");
    std::array<uint8_t, kHeader + kRow * kMaxSections> table{};
    read_at(file.get(), 0, table.data(), kHeader);
    const uint32_t count = rd_u16(table.data() + 6);
    require(rd_u32(table.data()) == 0x324c4d4d && rd_u16(table.data() + 4) == 2 &&
            rd_u32(table.data() + 8) == length && count <= limits.max_sections, "invalid area package envelope");
    const uint32_t table_end = static_cast<uint32_t>(kHeader + count * kRow);
    require(table_end <= length, "area package truncated directory");
    read_at(file.get(), kHeader, table.data() + kHeader, count * kRow);
    require(crc32(table.data() + kHeader, count * kRow) == rd_u32(table.data() + 16), "area package directory CRC mismatch");
    LoadedAreaPackage result;
    result.diagnostics.file_bytes = length;
    result.diagnostics.source_profile_crc = rd_u32(table.data() + 12);
    result.directory.reserve(count);
    size_t map_section = count, render_section = count, negative_material_section = count, runtime_section = count;
    size_t spawn_section=count, map_initialization_section=count;
    for (uint32_t i = 0; i < count; ++i) {
        const auto* p = table.data() + kHeader + size_t(i) * kRow;
        AreaPackageSection section{rd_u32(p), rd_u32(p + 4), rd_u32(p + 8), rd_u32(p + 12), false};
        require(section.offset >= table_end && section.offset <= length && section.size <= length - section.offset,
                "area package section range invalid");
        require(section.size <= limits.max_section_bytes, "area package section budget exceeded");
        for (const auto& other : result.directory) {
            require(section.kind != other.kind, "area package duplicate kind");
            require(!section.size || !other.size || uint64_t(section.offset) >= uint64_t(other.offset) + other.size ||
                    uint64_t(other.offset) >= uint64_t(section.offset) + section.size, "area package sections overlap");
        }
        if (section.kind == 2) map_section = i;
        else if (section.kind == 3) render_section = i;
        else if (section.kind == kAreaNegativeMaterialsSection) negative_material_section = i;
        else if (section.kind == kAreaRuntimeBindingsSection) runtime_section = i;
        else if (section.kind == kAreaSpawnsSection) spawn_section = i;
        else if (section.kind == kAreaMapInitializationSection) map_initialization_section = i;
        else {
            ++result.diagnostics.skipped_sections;
            if (section.kind != 1 && section.kind != 4) ++result.diagnostics.unknown_sections;
        }
        result.directory.push_back(section);
    }
    require(map_section != count && render_section != count, "area package requires map and render sections");
    const uint64_t base = add(sizeof(LoadedAreaPackage), vector_bytes(count, sizeof(AreaPackageSection)));
    const uint64_t working = table.size() + kScratch;
    admitted(base, add(base, working), limits);
    for (const auto& section : result.directory)
        require(file_crc(file.get(), section) == section.crc32, "area package payload CRC mismatch");
    {
        admitted(base, add(add(base, working), vector_bytes(result.directory[map_section].size, 1)), limits);
        auto bytes = read_section(file.get(), result.directory[map_section]);
        result.diagnostics.map_estimated_bytes = map_preflight(bytes, limits);
        const uint64_t decoded = add(base, result.diagnostics.map_estimated_bytes);
        const uint64_t peak = add(add(decoded, working), vector_bytes(bytes.size(), 1));
        admitted(decoded, peak, limits);
        // Preflight verified the inner CRC on this immutable local buffer.
        result.map = load_l3map(bytes, false);
        result.directory[map_section].decoded = true;
        result.diagnostics.load_peak_estimated_bytes = peak;
    }
    {
        const uint64_t map_resident = add(base, result.diagnostics.map_estimated_bytes);
        admitted(map_resident, add(add(map_resident, working), vector_bytes(result.directory[render_section].size, 1)), limits);
        auto bytes = read_section(file.get(), result.directory[render_section]);
        const auto estimate = render_preflight(bytes, result.map.descriptors.size());
        result.diagnostics.render_estimated_bytes = estimate.resident;
        result.diagnostics.decoded_estimated_bytes = add(add(base, result.diagnostics.map_estimated_bytes), estimate.resident);
        const uint64_t peak = add(add(add(result.diagnostics.decoded_estimated_bytes, working), estimate.temporary), vector_bytes(bytes.size(), 1));
        admitted(result.diagnostics.decoded_estimated_bytes, peak, limits);
        result.render = load_l3render(bytes, false);
        result.directory[render_section].decoded = true;
        result.diagnostics.load_peak_estimated_bytes = std::max(peak, result.diagnostics.load_peak_estimated_bytes);
    }
    require(result.map.descriptors.size() == result.render.descriptors.size(), "area package map/render descriptor mismatch");
    if (map_initialization_section != count) {
        const auto& section = result.directory[map_initialization_section];
        require(section.size <= kMapInitializationMaxFileBytes,
                "map initialization exceeds finite identity cap");
        const uint64_t input_peak = add(add(result.diagnostics.decoded_estimated_bytes, working),
                                        vector_bytes(section.size, 1));
        admitted(result.diagnostics.decoded_estimated_bytes, input_peak, limits);
        const auto bytes = read_section(file.get(), section);
        const auto header = inspect_map_initialization(bytes, true);
        const uint64_t temporary = add(vector_bytes(bytes.size(), 1), header.decoded_bytes);
        const uint64_t peak = add(add(result.diagnostics.decoded_estimated_bytes, working), temporary);
        admitted(result.diagnostics.decoded_estimated_bytes, peak, limits);
        const auto initialization = load_map_initialization(bytes, false);
        const auto applied = apply_map_initialization(result.map, result.render, initialization,
            result.directory[map_section].crc32, result.directory[render_section].crc32);
        result.directory[map_initialization_section].decoded = true;
        result.diagnostics.map_initialization_temporary_bytes = temporary;
        result.diagnostics.map_initialization_writes = applied.write_count;
        result.diagnostics.map_initialization_profile = static_cast<uint32_t>(header.profile);
        result.diagnostics.load_peak_estimated_bytes = std::max(peak, result.diagnostics.load_peak_estimated_bytes);
        // A later section failure destroys this candidate; the active map remains untouched.
    }
    if (negative_material_section != count) {
        const auto& section = result.directory[negative_material_section];
        const uint64_t expected_bytes = 16u + uint64_t(result.map.areas.size()) * 32u * 8u;
        require(section.size == expected_bytes, "negative material section area count/size mismatch");
        const uint64_t resident = vector_bytes(result.map.areas.size(), sizeof(NegativeMaterialTable));
        const uint64_t decoded = add(result.diagnostics.decoded_estimated_bytes, resident);
        const uint64_t peak = add(add(decoded, working), vector_bytes(section.size, 1));
        admitted(decoded, peak, limits);
        auto bytes = read_section(file.get(), section);
        // Reject a forged header count before allocating decoded tables. The
        // section size is already bounded by the admitted map's area count.
        require(bytes.size() >= 16 && rd_u32(bytes.data()) == 0x4d4e334cu &&
                rd_u16(bytes.data() + 4) == 1 && rd_u16(bytes.data() + 6) == result.map.areas.size() &&
                rd_u32(bytes.data() + 8) == bytes.size(),
                "negative material header or area count mismatch");
        result.negative_materials = load_l3negative_materials(bytes, true);
        require(result.negative_materials.size() == result.map.areas.size(), "negative material area count mismatch");
        result.directory[negative_material_section].decoded = true;
        result.diagnostics.negative_material_estimated_bytes = resident;
        result.diagnostics.decoded_estimated_bytes = decoded;
        result.diagnostics.load_peak_estimated_bytes = std::max(peak, result.diagnostics.load_peak_estimated_bytes);
    }
    if (runtime_section != count) {
        const auto& section = result.directory[runtime_section];
        // The preflight uses bounded flow/EBD slice copies, never decoded stage
        // allocations. Two section lengths cover those simultaneously live slices.
        const uint64_t preflight_peak = add(add(result.diagnostics.decoded_estimated_bytes, working),
                                            array_bytes(3, vector_bytes(section.size, 1)));
        admitted(result.diagnostics.decoded_estimated_bytes, preflight_peak, limits);
        auto bytes = read_section(file.get(), section);
        const auto estimate = runtime_preflight(bytes,result.map.areas.size(),spawn_section!=count);
        const uint64_t decoded = add(result.diagnostics.decoded_estimated_bytes, estimate.resident);
        const uint64_t peak = std::max(preflight_peak,
            add(add(add(decoded, working), vector_bytes(bytes.size(), 1)), estimate.temporary));
        admitted(decoded, peak, limits);
        result.runtime_areas = load_runtime_bindings(bytes);
        result.directory[runtime_section].decoded = true;
        result.diagnostics.runtime_binding_estimated_bytes = estimate.resident;
        result.diagnostics.runtime_source_payload_bytes = estimate.source_payload;
        result.diagnostics.decoded_estimated_bytes = decoded;
        result.diagnostics.load_peak_estimated_bytes = std::max(peak, result.diagnostics.load_peak_estimated_bytes);
    }
    if(spawn_section!=count) {
        const auto& section=result.directory[spawn_section];
        constexpr uint64_t max_spawn_bytes=40u+5u*8u+uint64_t(kSt00SpawnMaxRegions)*16u+
                                            uint64_t(kSt00SpawnMaxRecords)*24u;
        require(section.size<=max_spawn_bytes,"ST00 spawn section exceeds finite record caps");
        admitted(result.diagnostics.decoded_estimated_bytes,
            add(add(result.diagnostics.decoded_estimated_bytes,working),vector_bytes(section.size,1)),limits);
        const auto bytes=read_section(file.get(),section);
        const uint64_t resident=spawn_preflight(bytes,result.map.areas.size(),result.runtime_areas);
        const uint64_t decoded=add(result.diagnostics.decoded_estimated_bytes,resident);
        const uint64_t temporary=sizeof(St00SpawnRegion)+sizeof(St00ConditionalSpawn);
        const uint64_t peak=add(add(add(decoded,working),vector_bytes(section.size,1)),temporary);
        admitted(decoded,peak,limits);
        result.spawns=load_l3st00_spawn(bytes,false);
        for(auto& area:result.runtime_areas)
            area.conditional_spawns=result.spawns->areas[area.area_index];
        result.directory[spawn_section].decoded=true;
        result.diagnostics.spawn_estimated_bytes=resident;
        result.diagnostics.decoded_estimated_bytes=decoded;
        result.diagnostics.load_peak_estimated_bytes=std::max(peak,result.diagnostics.load_peak_estimated_bytes);
    }
    return result;
}
} // namespace l3
