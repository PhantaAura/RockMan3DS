#include "legends3ds/area_package.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/render/negative_terrain.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <tuple>
#include <new>

namespace {
using Sections = std::vector<std::pair<uint32_t, std::vector<uint8_t>>>;
std::vector<uint8_t> envelope(const Sections& sections) {
    std::vector<uint8_t> out;
    l3::wr_u32(out, 0x324c4d4d); l3::wr_u16(out, 2); l3::wr_u16(out, uint16_t(sections.size()));
    l3::wr_u32(out, 0); l3::wr_u32(out, 0x12345678); l3::wr_u32(out, 0);
    uint32_t offset = uint32_t(20 + 16 * sections.size());
    for (const auto& section : sections) {
        l3::wr_u32(out, section.first); l3::wr_u32(out, offset);
        l3::wr_u32(out, uint32_t(section.second.size()));
        l3::wr_u32(out, l3::crc32(section.second.data(), section.second.size()));
        offset += uint32_t(section.second.size());
    }
    l3::patch_u32(out, 8, offset);
    l3::patch_u32(out, 16, l3::crc32(out.data() + 20, out.size() - 20));
    for (const auto& section : sections) out.insert(out.end(), section.second.begin(), section.second.end());
    return out;
}
Sections fixture(uint32_t identity, bool two_streams = false, bool negative = false, bool two_areas = false,
                 size_t map_area_count = 0) {
    l3::St00Source source;
    l3::SourceArea area;
    area.start_x = 63; area.start_z = 63; area.width = 2; area.height = 1;
    area.cells = negative ? std::vector<int16_t>{int16_t(0x8104), int16_t(0x83ff)}
                          : std::vector<int16_t>{1, 0};
    const size_t areas=map_area_count?map_area_count:(two_areas?2u:1u);
    for(size_t i=0;i<areas;++i)source.areas.push_back(area);
    source.idx.resize(2);
    source.idx[1].y = int16_t(identity);
    source.idx[1].descriptor_ptr = l3::kSt00StgBase + 4;
    l3::SourceDescriptor descriptor;
    descriptor.source_ptr = l3::kSt00StgBase + 4;
    descriptor.default_contact_ptr = l3::kSt00MdtBase + 16;
    source.descriptors.push_back(descriptor);
    l3::SourceContactPrimitive contact;
    contact.y0 = int16_t(identity); contact.y1 = int16_t(identity); contact.code = 0x10;
    l3::SourceContactGroup group; group.x1 = 512; group.z1 = 512; group.primitives.push_back(contact);
    l3::SourceContactList list; list.source_ptr = l3::kSt00MdtBase + 16; list.groups.push_back(group);
    source.contact_lists.push_back(list);
    l3::St00RenderSource render;
    render.descriptors.resize(1);
    render.descriptors[0].record_indices[1] = 0;
    l3::SourceRenderRecord record;
    record.materials = {identity}; record.stream_indices = {0};
    l3::SourceRenderPrimitive primitive;
    primitive.prefix = identity;
    primitive.local_x = {0, 1, 0, 1}; primitive.local_z = {0, 0, 1, 1};
    l3::SourceRenderStream stream; stream.primitives.push_back(primitive);
    render.streams.push_back(stream);
    if (two_streams) { render.streams.push_back(stream); record.stream_indices.push_back(1); }
    render.records.push_back(record);
    if (negative) {
        render.default_negative_descriptor = 0;
        render.special_negative_descriptors[31] = 0;
    }
    Sections sections{{1, {'{', '}'}}, {2, l3::cook_st00_map(source)},
                      {3, l3::cook_l3render(render)}, {0x42, {1, 2, 3, 4}}};
    if (negative) {
        l3::NegativeMaterialTables tables(1);
        tables[0][1] = {0x0010001du, 12, true};
        sections.push_back({l3::kAreaNegativeMaterialsSection, l3::cook_l3negative_materials(tables)});
    }
    return sections;
}
void write(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw std::runtime_error("test fixture write failed");
}
void rejected(const std::filesystem::path& path, const l3::AreaPackageLimits& limits) {
    bool failed = false;
    try { (void)l3::load_area_package(path.string(), limits); }
    catch (const std::runtime_error&) { failed = true; }
    assert(failed);
}
void render_crc(std::vector<uint8_t>& bytes) {
    l3::patch_u32(bytes, 64, l3::crc32(bytes.data() + 80, bytes.size() - 80));
}
void negative_material_crc(std::vector<uint8_t>& bytes) {
    l3::patch_u32(bytes, 12, l3::crc32(bytes.data() + 16, bytes.size() - 16));
}
using RuntimeBinding = std::tuple<uint32_t, uint32_t, std::vector<uint8_t>>;
std::vector<uint8_t> runtime_section(const std::vector<uint8_t>& flow,
                                     const std::vector<RuntimeBinding>& bindings) {
    std::vector<uint8_t> out{'L', '3', 'R', 'B'};
    l3::wr_u16(out, 1); l3::wr_u16(out, 40);
    l3::wr_u32(out, 1); l3::wr_u32(out, 0); l3::wr_u32(out, uint32_t(bindings.size()));
    l3::wr_u32(out, 40); l3::wr_u32(out, uint32_t(40 + bindings.size() * 20));
    l3::wr_u32(out, uint32_t(flow.size())); l3::wr_u32(out, 0); l3::wr_u32(out, 0);
    uint32_t offset = uint32_t(40 + bindings.size() * 20 + flow.size());
    for (const auto& binding : bindings) {
        const auto& ebd = std::get<2>(binding);
        l3::wr_u32(out, std::get<0>(binding)); l3::wr_u32(out, std::get<1>(binding));
        l3::wr_u32(out, offset); l3::wr_u32(out, uint32_t(ebd.size()));
        l3::wr_u32(out, l3::crc32(ebd.data(), ebd.size()));
        offset += uint32_t(ebd.size());
    }
    out.insert(out.end(), flow.begin(), flow.end());
    for (const auto& binding : bindings) out.insert(out.end(), std::get<2>(binding).begin(), std::get<2>(binding).end());
    l3::patch_u32(out, 32, uint32_t(out.size()));
    l3::patch_u32(out, 36, l3::crc32(out.data() + 40, out.size() - 40));
    return out;
}
void exercise_runtime_bindings(const std::filesystem::path& path,
                               const std::filesystem::path& bad_path,
                               const l3::AreaPackageLimits& limits) {
    l3::St00FlowSource source;
    std::vector<uint8_t> object(20, 0);
    object[0] = 7; object[1] = 0x20; object[2] = 2; object[3] = 3;
    object[12] = 20; object[14] = 30; object[16] = 40;
    object.push_back(0xff);
    source.areas[3].objects = l3::parse_stage_object_list(object);
    source.areas[3].aux.records.resize(1);
    source.areas[3].setup_blob[0] = 99;
    const auto flow = l3::cook_l3st00_flow(source);
    const uint32_t base = 0x80400000;
    std::vector<uint8_t> raw;
    l3::wr_u32(raw, 1); raw.insert(raw.end(), {0x20, 2, 3, 0});
    l3::wr_u32(raw, base + 20); l3::wr_u32(raw, 0); l3::wr_u32(raw, 0);
    raw.insert(raw.end(), {12, 34, 56, 78});
    const auto ebd = l3::cook_l3ebd_directory(l3::parse_ebd_directory(raw, base));
    auto original = fixture(55, false, false, true);
    original.push_back({l3::kAreaRuntimeBindingsSection, runtime_section(flow, {{1, 3, ebd}})});
    write(path, envelope(original));
    auto package = l3::load_area_package(path.string(), limits);
    assert(package.runtime_areas.size() == 1 && package.runtime_areas[0].area_index == 1);
    const auto& area = package.runtime_areas[0];
    assert(area.flow.objects.records.size() == 1 && area.flow.setup_blob[0] == 99);
    assert(area.stage.instances.size() == 1 && area.stage.instances[0].object.source_position[0] == 20);
    assert(area.stage.instances[0].work && area.stage.instances[0].work->bytes.size() == 0x3c0);
    assert(area.stage.instances[0].ebd.state == l3::StageEbdBindingState::UniqueClassSubtype);
    const auto resource = l3::ebd_resource_span(area.ebd, 0, 0, 4);
    assert(resource.status == l3::EbdResourceSpanStatus::AVAILABLE && resource.data[2] == 56);
    assert(area.ebd.source_payload == raw);
    assert(package.directory.back().decoded && package.diagnostics.runtime_source_payload_bytes == raw.size());
    assert(package.diagnostics.runtime_binding_estimated_bytes > raw.size());
    auto bounded = limits;
    bounded.max_decoded_bytes = package.diagnostics.decoded_estimated_bytes;
    bounded.max_load_peak_bytes = package.diagnostics.load_peak_estimated_bytes;
    assert(l3::load_area_package(path.string(), bounded).runtime_areas.size() == 1);
    --bounded.max_decoded_bytes; rejected(path, bounded);
    bounded = limits; bounded.max_load_peak_bytes = package.diagnostics.load_peak_estimated_bytes - 1;
    rejected(path, bounded);
    auto malformed = [&](const Sections& sections) { write(bad_path, envelope(sections)); rejected(bad_path, limits); };
    auto changed = original; changed.push_back(original.back()); malformed(changed);
    changed = original;
    changed.back().second = runtime_section(flow, {{1, 3, ebd}, {1, 4, ebd}}); malformed(changed);
    changed.back().second = runtime_section(flow, {{1, 3, ebd}, {0, 4, ebd}}); malformed(changed);
    changed.back().second = runtime_section(flow, {{0, 3, ebd}, {1, 4, ebd}});
    write(path, envelope(changed));
    auto complete = l3::load_area_package(path.string(), limits);
    assert(complete.runtime_areas.size() == 2 && complete.runtime_areas[0].area_index == 0 &&
           complete.runtime_areas[1].area_index == 1 && complete.runtime_areas[1].stage.instances.empty());
    assert(complete.diagnostics.runtime_source_payload_bytes == raw.size() * 2);
    // Every malformed case keeps the MML2 CRCs valid. Recompute L3RB's inner
    // checksum except for its own CRC test to exercise bounds/backend checks.
    for (const auto& field : std::vector<std::pair<size_t, uint32_t>>{
             {0, 0}, {4, 0x00280002}, {8, 2}, {12, 1}, {16, 0}, {20, 41}, {24, 59},
             {28, UINT32_MAX}, {32, 0}, {36, 1}, {40, 2}, {44, 5}, {48, 0}, {52, UINT32_MAX}, {56, 1}}) {
        changed = original;
        l3::patch_u32(changed.back().second, field.first, field.second);
        if (field.first != 36) l3::patch_u32(changed.back().second, 36,
            l3::crc32(changed.back().second.data() + 40, changed.back().second.size() - 40));
        malformed(changed);
    }
    auto bad_flow = flow; bad_flow[36] ^= 1;
    changed = original; changed.back().second = runtime_section(bad_flow, {{1, 3, ebd}}); malformed(changed);
    bad_flow = flow; l3::patch_u32(bad_flow, 8, 15);
    changed.back().second = runtime_section(bad_flow, {{1, 3, ebd}}); malformed(changed);
    auto bad_ebd = ebd; bad_ebd[28] ^= 1;
    changed.back().second = runtime_section(flow, {{1, 3, bad_ebd}}); malformed(changed);
    const auto legacy = l3::cook_l3ebd_directory(l3::EbdDirectory{});
    changed.back().second = runtime_section(flow, {{1, 3, legacy}}); malformed(changed);
    bad_ebd = ebd; bad_ebd[44] = 2;
    l3::patch_u32(bad_ebd, 28, l3::crc32(bad_ebd.data() + 40, bad_ebd.size() - 40));
    changed.back().second = runtime_section(flow, {{1, 3, bad_ebd}}); malformed(changed);
    changed = original; changed.back().second.push_back(0);
    l3::patch_u32(changed.back().second, 32, uint32_t(changed.back().second.size()));
    l3::patch_u32(changed.back().second, 36, l3::crc32(changed.back().second.data() + 40, changed.back().second.size() - 40));
    malformed(changed);
    const auto lifetime = area.ebd.source_payload.lifetime();
    bool failed = false;
    try { package = l3::load_area_package(bad_path.string(), limits); }
    catch (const std::runtime_error&) { failed = true; }
    assert(failed && !lifetime.expired() && package.runtime_areas[0].stage.instances[0].object.id == 7);
    package.unload();
    assert(lifetime.expired() && package.runtime_areas.empty() && package.runtime_areas.capacity() == 0);
    assert(package.diagnostics.runtime_binding_estimated_bytes == 0 && package.diagnostics.runtime_source_payload_bytes == 0);
    write(path, envelope(fixture(55)));
    auto legacy_package = l3::load_area_package(path.string(), limits);
    assert(legacy_package.runtime_areas.empty() && legacy_package.diagnostics.runtime_binding_estimated_bytes == 0);
}
void exercise_negative_package(const std::filesystem::path& path,
                               const std::filesystem::path& bad_path,
                               const l3::AreaPackageLimits& limits) {
    const auto original = fixture(44, false, true);
    write(path, envelope(original));
    auto package = l3::load_area_package(path.string(), limits);
    assert(package.render.default_negative_descriptor == 0 &&
           package.render.special_negative_descriptors[31] == 0);
    assert(package.negative_materials.size() == 1 && package.negative_materials[0][1].known);
    assert(package.directory[4].kind == l3::kAreaNegativeMaterialsSection && package.directory[4].decoded);
    assert(package.diagnostics.skipped_sections == 2 && package.diagnostics.unknown_sections == 1);
    assert(package.diagnostics.negative_material_estimated_bytes >= sizeof(l3::NegativeMaterialTable));
    const auto placement = l3::collect_area_negative_placement(package.map, package.render, 0);
    assert(placement.instances.size() == 2 && !placement.default_cells_unbound && !placement.special_cells_unbound);
    assert(l3::count_area_negative_mid_primitives(package.render, placement, 2) == 2);
    const auto* binding = l3::find_negative_material_binding(package.negative_materials, 0,
                                                          placement.instances[0].packed_cell);
    const auto geometry = l3::expand_negative_terrain_mid(package.render, placement.instances[0], binding);
    assert(geometry.size() == 1 && geometry[0].texture_binding_known && geometry[0].material_word == 0x0010001du);
    assert(!l3::find_negative_material_binding(package.negative_materials, 0, placement.instances[1].packed_cell));
    const auto special = l3::expand_negative_terrain_mid(package.render, placement.instances[1], binding);
    assert(special.size() == 1 && !special[0].texture_binding_known && special[0].material_word == 0);

    // Decoded and peak admission include the optional material allocation.
    auto bounded = limits;
    bounded.max_decoded_bytes = package.diagnostics.decoded_estimated_bytes;
    bounded.max_load_peak_bytes = package.diagnostics.load_peak_estimated_bytes;
    assert(l3::load_area_package(path.string(), bounded).negative_materials.size() == 1);
    --bounded.max_decoded_bytes; rejected(path, bounded);
    bounded = limits; bounded.max_load_peak_bytes = package.diagnostics.load_peak_estimated_bytes - 1;
    rejected(path, bounded);

    auto malformed = [&](const Sections& sections) {
        write(bad_path, envelope(sections)); rejected(bad_path, limits);
    };
    // Outer CRCs and (where applicable) inner CRCs remain valid, so these
    // specifically exercise preflight and semantic malformed-data rejection.
    for (const auto& field : std::vector<std::pair<size_t, uint32_t>>{
             {8, 30}, {8, 63}, {68, 1}, {72, 94}, {76, 31}, {8, 15}}) {
        auto changed = original;
        l3::patch_u32(changed[2].second, field.first, field.second);
        malformed(changed);
    }
    auto changed = original;
    const auto table = l3::rd_u32(changed[2].second.data() + 72);
    changed[2].second[table + 31 * 2] = 1;
    changed[2].second[table + 31 * 2 + 1] = 0;
    render_crc(changed[2].second); malformed(changed);
    changed = original; changed.push_back(original[4]); malformed(changed);
    changed = original; changed[4].second[6] = 2; malformed(changed);
    changed = original;
    changed[4].second = l3::cook_l3negative_materials(l3::NegativeMaterialTables(2));
    malformed(changed);
    changed = original; changed[4].second[12] ^= 1; malformed(changed);
    for (const auto& field : std::vector<std::pair<size_t, uint8_t>>{
             {0, 0}, {4, 3}, {8, 0}, {16 + 12 + 4, 64}, {16 + 12 + 5, 4},
             {16 + 12 + 6, 1}, {16 + 12, 0}, {16, 1}, {16 + 4, 1}}) {
        changed = original; changed[4].second[field.first] = field.second;
        negative_material_crc(changed[4].second); malformed(changed);
    }
    const auto* retained_materials = package.negative_materials.data();
    bool failed = false;
    try { package = l3::load_area_package(bad_path.string(), limits); }
    catch (const std::runtime_error&) { failed = true; }
    assert(failed && package.negative_materials.data() == retained_materials &&
           package.negative_materials[0][1].material_word == 0x0010001du);

    // The three optional-binding combinations preserve legacy v3 flags7.
    for (unsigned mode = 0; mode < 3; ++mode) {
        changed = original;
        auto render = l3::load_l3render(changed[2].second);
        if (mode != 1) render.default_negative_descriptor = 0xFFFFu;
        if (mode != 2) render.special_negative_descriptors.fill(0xFFFFu);
        changed[2].second = l3::cook_l3render(render);
        changed.pop_back();
        write(path, envelope(changed));
        auto compatible = l3::load_area_package(path.string(), limits);
        const uint32_t expected_flags[] = {7, 15, 23};
        assert(l3::rd_u32(changed[2].second.data() + 8) == expected_flags[mode]);
        assert(compatible.negative_materials.empty() && compatible.diagnostics.negative_material_estimated_bytes == 0);
        const auto available = l3::collect_area_negative_placement(compatible.map, compatible.render, 0);
        assert(available.instances.size() == (mode == 0 ? 0u : 1u));
        assert(available.default_cells_unbound == (mode == 1 ? 0u : 1u));
        assert(available.special_cells_unbound == (mode == 2 ? 0u : 1u));
        if (!available.instances.empty()) {
            const auto fallback = l3::expand_negative_terrain_mid(compatible.render, available.instances[0], nullptr);
            assert(fallback.size() == 1 && !fallback[0].texture_binding_known);
        }
    }
    package.unload();
    assert(package.negative_materials.empty() && package.negative_materials.capacity() == 0);
    assert(package.render.default_negative_descriptor == 0xFFFFu);
    for (uint16_t descriptor : package.render.special_negative_descriptors) assert(descriptor == 0xFFFFu);
    assert(package.diagnostics.decoded_estimated_bytes == 0 &&
           package.diagnostics.load_peak_estimated_bytes == 0 &&
           package.diagnostics.negative_material_estimated_bytes == 0);
    write(path, envelope(original));
    package = l3::load_area_package(path.string(), limits);
    assert(package.negative_materials.size() == 1 && package.render.special_negative_descriptors[31] == 0);
}
void exercise_runtime_views(const l3::LoadedAreaPackage& package, uint32_t identity) {
    const auto cell = l3::runtime_map_find_cell(package.map, 0, -256, -256);
    assert(cell && cell->packed_cell == 1);
    const auto* idx = l3::runtime_map_cell_idx(package.map, *cell);
    assert(idx && idx->y == int16_t(identity));
    const auto groups = l3::runtime_map_cell_contact_groups(package.map, *cell);
    assert(groups.size() == 1 && groups[0].primitives.size() == 1);
    assert(groups[0].primitives[0].primitive.y0 == int16_t(identity));
    const l3::St00CellOrigin origin{cell->world_origin_x, idx->y, cell->world_origin_z};
    const auto geometry = l3::expand_st00_descriptor_lod(package.render, idx->descriptor_index,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, uint16_t(cell->packed_cell));
    assert(geometry.size() == 1 && geometry[0].material_word == identity);
    assert(geometry[0].prefix == identity && geometry[0].world_y[0] == int32_t(identity));
}
void exercise_runtime_session(const std::filesystem::path& first_path,
                              const std::filesystem::path& second_path,
                              const l3::AreaPackageLimits& limits) {
    auto first = l3::load_area_package(first_path.string(), limits);
    auto second = l3::load_area_package(second_path.string(), limits);
    L3QolSettings settings{};
    l3_qol_settings_defaults(&settings);
    l3::RuntimeSession session(std::move(first.map), {}, {}, settings);
    session.reset_player({-256, 11, -256}, 0, 100, 100);
    auto probe = session.probe_player_map();
    assert(probe.in_area && probe.idx && probe.idx->y == 11);
    assert(probe.contact_groups.size() == 1);
    auto geometry = l3::expand_st00_descriptor_lod(first.render, probe.idx->descriptor_index,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, {-256, probe.idx->y, -256}, 1);
    assert(geometry.size() == 1 && geometry[0].material_word == 11);
    session.register_location(1, std::move(second.map), 0, {});
    assert(session.transition_to_location(1, {-256, 22, -256}, 0));
    probe = session.probe_player_map();
    assert(probe.in_area && probe.idx && probe.idx->y == 22);
    assert(probe.contact_groups.size() == 1);
    geometry = l3::expand_st00_descriptor_lod(second.render, probe.idx->descriptor_index,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, {-256, probe.idx->y, -256}, 1);
    assert(geometry.size() == 1 && geometry[0].material_word == 22);
    session.tick({});
    assert(session.frame_counter() == 1);
    // No roles, enemies, event behavior, or retail physics parity are invented.
    assert(session.targets().empty() && session.projectiles().empty());
}
void exercise_expansion_admission(const l3::LoadedAreaPackage& package) {
    assert(l3::count_area_render_primitives(package.map, package.render, 0, 1) == 1);
    auto map = package.map;
    auto render = package.render;
    map.cells[1] = 1; // Both cells reuse the same descriptor/stream payload.
    assert(l3::count_area_render_primitives(map, render, 0, 2) == 2);
    auto reject = [&](uint64_t cap, size_t area = 0) {
        bool failed = false;
        try { (void)l3::count_area_render_primitives(map, render, area, cap); }
        catch (const std::runtime_error&) { failed = true; }
        assert(failed);
    };
    reject(1);
    // Record reuse amplifies again without adding any stored primitive payload.
    render.records[0].stream_indices.push_back(0);
    assert(l3::count_area_render_primitives(map, render, 0, 4) == 4);
    reject(3);
    map.cells.assign(1000, 1); map.areas[0].width = 1000; map.areas[0].cell_count = 1000;
    reject(32); // Early admission failure before 2,000 instances are expanded.
    render.records[0].stream_indices[1] = 100;
    reject(UINT64_MAX); // Bad references remain errors even when the budget is large.
    render = package.render;
    map.cells.assign(2, 0); map.areas[0].width = 2; map.areas[0].cell_count = 2;
    assert(l3::count_area_render_primitives(map, render, 0, 0) == 0);
    map.cells.clear(); map.areas[0].width = 0; map.areas[0].cell_count = 0;
    assert(l3::count_area_render_primitives(map, render, 0, 0) == 0);
    reject(UINT64_MAX, 1);
    map.areas[0].cell_first = UINT32_MAX; map.areas[0].cell_count = 2;
    reject(UINT64_MAX);
}
void exercise_material_admission() {
    l3::MaterialCache cache;
    l3::MaterialCacheEntry entry;
    entry.material_word=0;entry.bpp=4;entry.resolved=true;
    entry.palette_entries=16;entry.source_width=256;entry.source_height=256;
    entry.indices.resize(256u*256u);entry.palette_ps1.resize(16);
    cache.entries.push_back(entry);entry.material_word=1;cache.entries.push_back(entry);
    const auto valid=l3::cook_l3material_cache(cache);
    const auto resident=l3::preflight_area_material_cache(valid,UINT64_MAX);
    assert(resident>2u*256u*256u);
    assert(l3::preflight_area_material_cache(valid,resident)==resident);
    auto rejected_cache=[&](const std::vector<uint8_t>& bytes,uint64_t cap=UINT64_MAX) {
        bool failed=false;
        try {(void)l3::preflight_area_material_cache(bytes,cap);}
        catch (const std::runtime_error&) {failed=true;}
        assert(failed);
    };
    rejected_cache(valid,resident-1);
    auto changed=valid;
    // The loose decoder accepts repeated slice ownership, amplifying a small
    // file into duplicated CPU pages and GPU textures. Native preflight rejects
    // that alias before any material cache is decoded or active GPU freed.
    const size_t first=48,second=48+l3::kL3MaterialRecordBytes;
    l3::patch_u32(changed,second+16,l3::rd_u32(changed.data()+first+16));
    l3::patch_u32(changed,second+24,l3::rd_u32(changed.data()+first+24));
    l3::patch_u32(changed,44,l3::crc32(changed.data()+48,changed.size()-48));
    assert(l3::load_l3material_cache(changed).entries.size()==2);
    rejected_cache(changed);
    changed=valid;changed[l3::rd_u32(changed.data()+first+16)]=16;
    l3::patch_u32(changed,44,l3::crc32(changed.data()+48,changed.size()-48));
    rejected_cache(changed);
    changed=valid;l3::patch_u32(changed,8,7);rejected_cache(changed);
    changed=valid;changed.back()^=1;rejected_cache(changed);
}
void exercise_transition_rollback(const std::filesystem::path& first_path,
                                  const std::filesystem::path& second_path,
                                  const std::filesystem::path& bad_path,
                                  const l3::AreaPackageLimits& limits) {
    auto active=l3::load_area_package(first_path.string(),limits);
    const auto* old_cells=active.map.cells.data();
    int visible_resources=11;
    size_t commits=0,restores=0;
    const auto rejected_transition=[&](const std::filesystem::path& path,
                                       const l3::AreaPackageLimits& cap,bool after_release) {
        l3::LoadedAreaPackage candidate;
        bool failed=false;
        try {
            l3::transition_area_resources([&] {
                candidate=l3::load_area_package(path.string(),cap);
                if (after_release) {
                    visible_resources=22; // A partially built next GPU view.
                    throw std::bad_alloc();
                }
            },[&] {
                ++commits;active=std::move(candidate);
            },[&] {
                ++restores;candidate.unload();visible_resources=11;
            });
        } catch (const std::exception&) {failed=true;}
        assert(failed && commits==0 && visible_resources==11);
        assert(active.map.cells.data()==old_cells);
        exercise_runtime_views(active,11); // Includes old collision and material.
        assert(candidate.runtime_areas.empty() && candidate.map.cells.capacity()==0);
    };
    rejected_transition(first_path.parent_path()/"missing.mmlpkg",limits,false);
    auto corrupt=envelope(fixture(22));corrupt.back()^=1;write(bad_path,corrupt);
    rejected_transition(bad_path,limits,false);
    auto bounded=limits;bounded.max_decoded_bytes=1;
    rejected_transition(second_path,bounded,false);
    rejected_transition(second_path,limits,true);
    assert(restores==4);
    bool restoration_failed=false;
    try {
        l3::transition_area_resources([] {throw std::bad_alloc();},[] {},[] {
            throw std::runtime_error("outgoing data unavailable during restoration");
        });
    } catch (const std::runtime_error& error) {
        restoration_failed=std::string(error.what())=="outgoing data unavailable during restoration";
    }
    assert(restoration_failed); // A failed restoration must never claim success.
    l3::LoadedAreaPackage candidate;
    l3::transition_area_resources([&] {
        candidate=l3::load_area_package(second_path.string(),limits);visible_resources=22;
    },[&] {++commits;active=std::move(candidate);},[&] {++restores;});
    assert(commits==1 && restores==4 && visible_resources==22);
    exercise_runtime_views(active,22);
}
void exercise_transition_event_cleanup(const std::filesystem::path& path,
                                       const l3::AreaPackageLimits& limits) {
    write(path,envelope(fixture(33,false,false,true)));
    auto package=l3::load_area_package(path.string(),limits);
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    l3::MmlMessageFile message;message.blocks.resize(1);
    message.blocks[0].tokens.push_back({l3::MmlMessageTokenKind::Control,0x8F,{1,0,0,0}});
    l3::RuntimeSession session(std::move(package.map),{},{message},settings);
    session.register_area(1,{});
    assert(session.registered_area(1) && !session.registered_area(2));
    session.reset_player({-256,33,-256},0,100,100);
    session.register_target({7,{-256,33,-256-100},1,2,true,true});
    session.start_dialogue({0,0});session.tick({});
    assert(session.dialogue().active && session.dialogue().audio_requests.size()==1);
    session.begin_cutscene(9);
    const auto position=session.player_position();
    assert(!session.transition_to_area(99,{0,0,0},123));
    int visible_resources=33;
    bool commit_rejected=false;
    try {
        l3::transition_area_resources([&] {visible_resources=66;},[&] {
            if (!session.transition_to_area(99,{0,0,0},123))
                throw std::runtime_error("runtime registration disappeared before commit");
        },[&] {visible_resources=33;});
    } catch (const std::runtime_error&) {commit_rejected=true;}
    assert(commit_rejected && visible_resources==33 && session.map_area_index()==0);
    assert(session.player_position().x==position.x && session.cutscene_active());
    assert(session.targets().size()==1 && session.dialogue().audio_requests.size()==1);
    bool bad_registration=false;
    try {session.register_area(99,{});}catch (const std::out_of_range&) {bad_registration=true;}
    assert(bad_registration && !session.has_registered_area(99));
    assert(session.transition_to_area(1,{-256,33,-256},0));
    assert(session.targets().empty() && session.projectiles().empty() && !session.cutscene_active());
    assert(!session.dialogue().active && session.dialogue().visible_glyphs.empty());
    assert(session.take_audio_requests().empty() && session.take_script_requests().empty());
    assert(session.take_aux_triggers().empty() && !session.locked_target_id());
    assert(session.map_area_index()==1 && session.probe_player_map().in_area);
}
void exercise_spawn_package(const std::filesystem::path& path,
                            const std::filesystem::path& bad_path,
                            const l3::AreaPackageLimits& limits) {
    l3::St00SpawnCatalog catalog;
    l3::St00SpawnRegion region;
    region.id=11;region.qualifier=3;region.min_x=-1;region.min_z=-2;region.width=4;region.depth=5;
    l3::St00ConditionalSpawn record;
    record.object.raw[0]=7;record.object.raw[1]=0x20;record.predicate=17;
    region.records.push_back(record);catalog.areas[2].regions.push_back(region);
    const auto spawn=l3::cook_l3st00_spawn(catalog);
    auto sections=fixture(66,false,false,false,5);
    sections.push_back({l3::kAreaSpawnsSection,spawn});
    write(path,envelope(sections));
    auto package=l3::load_area_package(path.string(),limits);
    assert(package.spawns && package.spawns->areas[2].regions[0].records[0].predicate==17);
    assert(package.spawns->areas[2].regions[0].records[0].object.id==7);
    assert(package.directory.back().decoded && package.diagnostics.spawn_estimated_bytes>sizeof(catalog));
    auto bounded=limits;
    bounded.max_decoded_bytes=package.diagnostics.decoded_estimated_bytes;
    bounded.max_load_peak_bytes=package.diagnostics.load_peak_estimated_bytes;
    assert(l3::load_area_package(path.string(),bounded).spawns);
    --bounded.max_decoded_bytes;rejected(path,bounded);
    bounded=limits;bounded.max_load_peak_bytes=package.diagnostics.load_peak_estimated_bytes-1;
    rejected(path,bounded);
    const auto* retained=package.spawns->areas[2].regions.data();
    const auto malformed=[&](const Sections& changed) {
        write(bad_path,envelope(changed));
        bool failed=false;
        try {package=l3::load_area_package(bad_path.string(),limits);}
        catch(const std::runtime_error&) {failed=true;}
        assert(failed && package.spawns && package.spawns->areas[2].regions.data()==retained);
    };
    auto changed=sections;changed.push_back(sections.back());malformed(changed);
    for(const auto& field:std::vector<std::pair<size_t,uint32_t>>{{8,4},{12,65},{16,257},{20,41},{32,0},
            {40,1},{80,128},{80,128u<<16},{88,1},{96+20,2048}}) {
        changed=sections;l3::patch_u32(changed.back().second,field.first,field.second);
        l3::patch_u32(changed.back().second,36,
            l3::crc32(changed.back().second.data()+40,changed.back().second.size()-40));
        malformed(changed);
    }
    changed=sections;changed.back().second[96]=0x80;
    l3::patch_u32(changed.back().second,36,l3::crc32(changed.back().second.data()+40,changed.back().second.size()-40));
    malformed(changed);
    changed=fixture(66);changed.push_back(sections.back());malformed(changed);
    l3::St00FlowSource flow;
    const auto ebd=l3::cook_l3ebd_directory(l3::parse_ebd_directory(std::vector<uint8_t>(4),0x80400000));
    changed=sections;changed.push_back({l3::kAreaRuntimeBindingsSection,
        runtime_section(l3::cook_l3st00_flow(flow),{{2,3,ebd}})});malformed(changed);
    changed.back().second=runtime_section(l3::cook_l3st00_flow(flow),{{2,2,ebd}});
    write(path,envelope(changed));
    const auto bound=l3::load_area_package(path.string(),limits);
    assert(bound.runtime_areas[0].area_index==2 && bound.runtime_areas[0].conditional_spawns &&
           bound.runtime_areas[0].conditional_spawns->regions[0].records[0].predicate==17);
    package.unload();
    assert(!package.spawns && package.diagnostics.spawn_estimated_bytes==0);
    write(path,envelope(fixture(66)));
    package=l3::load_area_package(path.string(),limits);
    assert(!package.spawns && package.diagnostics.spawn_estimated_bytes==0);
}
struct TemporaryDirectory {
    std::filesystem::path path;
    TemporaryDirectory() : path(std::filesystem::temp_directory_path() /
        ("legends3ds-area-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        if (!std::filesystem::create_directory(path)) throw std::runtime_error("test directory creation failed");
    }
    ~TemporaryDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
}

int main() {
    TemporaryDirectory temp;
    const auto first_path = temp.path / "first.mmlpkg", second_path = temp.path / "second.mmlpkg";
    const auto bad_path = temp.path / "bad.mmlpkg";
    const auto first = fixture(11), second = fixture(22);
    write(first_path, envelope(first)); write(second_path, envelope(second));
    const l3::AreaPackageLimits limits;
    exercise_negative_package(temp.path / "negative.mmlpkg", bad_path, limits);
    exercise_runtime_bindings(temp.path / "bindings.mmlpkg", bad_path, limits);
    exercise_runtime_session(first_path, second_path, limits);
    exercise_material_admission();
    exercise_transition_rollback(first_path,second_path,bad_path,limits);
    exercise_transition_event_cleanup(temp.path/"events.mmlpkg",limits);
    exercise_spawn_package(temp.path/"spawns.mmlpkg",bad_path,limits);
    auto active = l3::load_area_package(first_path.string(), limits);
    exercise_runtime_views(active, 11);
    exercise_expansion_admission(active);
    assert(active.map.idx[1].y == 11 && active.render.streams[0].primitives[0].prefix == 11);
    assert(active.map.contact_primitives[0].y0 == 11);
    assert(active.directory.size() == 4 && active.directory[1].decoded && active.directory[2].decoded);
    assert(!active.directory[3].decoded && active.directory[3].kind == 0x42 && active.directory[3].size == 4);
    assert(active.diagnostics.skipped_sections == 2 && active.diagnostics.unknown_sections == 1);
    assert(active.diagnostics.source_profile_crc == 0x12345678);
    assert(active.diagnostics.decoded_estimated_bytes > 0 &&
           active.diagnostics.load_peak_estimated_bytes > active.diagnostics.decoded_estimated_bytes);
    const auto* retained_cells = active.map.cells.data();
    auto corrupt = envelope(second); corrupt.back() ^= 1; write(bad_path, corrupt);
    bool failed = false;
    try { active = l3::load_area_package(bad_path.string(), limits); }
    catch (const std::runtime_error&) { failed = true; }
    assert(failed && active.map.cells.data() == retained_cells && active.map.idx[1].y == 11);
    // Each admission policy can independently reject a valid package.
    auto bounded = limits; bounded.max_file_bytes = envelope(first).size() - 1; rejected(first_path, bounded);
    bounded = limits; bounded.max_section_bytes = first[1].second.size() - 1; rejected(first_path, bounded);
    bounded = limits; bounded.max_decoded_bytes = active.diagnostics.decoded_estimated_bytes - 1; rejected(first_path, bounded);
    bounded = limits; bounded.max_load_peak_bytes = active.diagnostics.load_peak_estimated_bytes - 1; rejected(first_path, bounded);
    bounded = limits; bounded.max_sections = 3; rejected(first_path, bounded);
    bounded = limits; bounded.max_map_areas = 0; rejected(first_path, bounded);
    // Structural errors retain valid envelope/payload CRCs, exercising decoder preflight.
    auto modified = first;
    modified.erase(modified.begin() + 2); write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = first; modified.push_back(first[1]); write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = first; l3::patch_u32(modified[1].second, 48, 0); write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = first;
    const auto off_cells = l3::rd_u32(modified[1].second.data() + 52);
    modified[1].second[off_cells] = 3;
    l3::patch_u32(modified[1].second, 80, l3::crc32(modified[1].second.data() + 84, modified[1].second.size() - 84));
    write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = first;
    l3::St00RenderSource extra_descriptor;
    extra_descriptor.descriptors.resize(2);
    modified[2].second = l3::cook_l3render(extra_descriptor);
    write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = fixture(33, true);
    const auto off_streams = l3::rd_u32(modified[2].second.data() + 52);
    l3::patch_u32(modified[2].second, off_streams + 20 + 4, 0); render_crc(modified[2].second);
    write(bad_path, envelope(modified)); rejected(bad_path, limits);
    modified = first;
    const auto stream_offset = l3::rd_u32(modified[2].second.data() + 52);
    modified[2].second[stream_offset] = 8; modified[2].second[stream_offset + 2] = 8;
    render_crc(modified[2].second); write(bad_path, envelope(modified)); rejected(bad_path, limits);
    // Outer overlap is rejected before streaming or payload allocation.
    corrupt = envelope(first);
    l3::patch_u32(corrupt, 20 + 3 * 16 + 4, l3::rd_u32(corrupt.data() + 20 + 16 + 4));
    l3::patch_u32(corrupt, 16, l3::crc32(corrupt.data() + 20, 4 * 16));
    write(bad_path, corrupt); rejected(bad_path, limits);
    corrupt.resize(19); write(bad_path, corrupt); rejected(bad_path, limits);
    active = l3::load_area_package(second_path.string(), limits);
    exercise_runtime_views(active, 22);
    assert(active.map.idx[1].y == 22 && active.render.records[0].materials[0] == 22);
    active.unload();
    assert(active.map.areas.empty() && active.render.streams.empty() && active.directory.empty());
    assert(active.map.areas.capacity() == 0 && active.render.streams.capacity() == 0 && active.directory.capacity() == 0);
    assert(active.diagnostics.file_bytes == 0);
    active = l3::load_area_package(first_path.string(), limits);
    assert(active.map.idx[1].y == 11);
    std::cout << "PASS: two-package geometry/contact loading, streaming validation, admission, failure preservation, unload/reload\n";
}
