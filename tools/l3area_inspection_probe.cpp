#include "legends3ds/area_package.h"
#include "legends3ds/render/area_inspection.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/negative_terrain.h"
#include "legends3ds/render/positive_terrain.h"
#include <iostream>
#include <stdexcept>

// Aggregate-only view diagnostic: no original XYZ arrays, textures, story names,
// object payloads or executable bytes are emitted.
int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3area_inspection_probe cooked.mmlpkg\n";
            return 2;
        }
        const auto package = l3::load_area_package(argv[1], {});
        std::cout << "{\"status\":\"CANDIDATE\",\"retail_entry\":\"UNKNOWN\","
                     "\"retail_camera\":\"UNKNOWN\",\"geometry_source\":\"ADMITTED_POSITIVE_AND_BOUND_NEGATIVE\","
                     "\"negative_floor_contact\":\"UNKNOWN\",\"areas\":[";
        for (size_t ai = 0; ai < package.map.areas.size(); ++ai) {
            const auto placement = l3::collect_area_positive_placement(package.map, ai);
            constexpr uint64_t max_primitives = 4u * 1024u * 1024u / 120u;
            const auto positive_count = l3::count_area_placed_render_primitives(
                package.render, placement, l3::St00Lod::Mid, 4u * 1024u * 1024u / 120u);
            const auto negative = l3::collect_area_negative_placement(package.map, package.render, ai);
            const auto negative_count = l3::count_area_negative_mid_primitives(package.render,
                negative, max_primitives - positive_count);
            const auto primitive_count = positive_count + negative_count;
            l3::AreaInspectionGeometry geometry(static_cast<size_t>(primitive_count));
            for (const auto& instance : placement.instances) {
                if (instance.descriptor_index == 0xFFFFu) continue;
                for (const auto& primitive : l3::expand_positive_terrain_descriptor_lod(
                         package.render, instance.descriptor_index, instance.bank,
                         l3::St00Lod::Mid, instance.origin, 0))
                    geometry.add(primitive);
            }
            for (const auto& instance : negative.instances) {
                const auto* binding = l3::find_negative_material_binding(package.negative_materials,
                    ai, instance.packed_cell);
                for (const auto& primitive : l3::expand_negative_terrain_mid(package.render,
                         instance, binding))
                    geometry.add(primitive);
            }
            l3::St00CellOrigin legacy_entry{};
            bool legacy_entry_found = false, legacy_grid_idx_mismatch = false;
            const auto& area = package.map.areas[ai];
            for (uint32_t local = 0; local < area.cell_count; ++local) {
                const int16_t packed = package.map.cells[size_t(area.cell_first) + local];
                if (packed <= 0) continue;
                const auto& idx = package.map.idx[static_cast<uint16_t>(packed)];
                legacy_entry = {(int32_t(area.start_x) + int32_t(local % area.width)) * 0x200 - 0x7F00,
                    idx.y, (int32_t(area.start_z) + int32_t(local / area.width)) * 0x200 - 0x7F00};
                legacy_entry_found = true;
                legacy_grid_idx_mismatch = legacy_entry.x != idx.x || legacy_entry.z != idx.z;
                break;
            }
            const auto entry = l3::choose_area_inspection_entry(package.map, ai, geometry);
            if (entry.primitive_count && (!entry.valid || !entry.primitives_with_visible_vertex))
                throw std::runtime_error("admitted geometry has no visible diagnostic entry vertex");
            if (ai) std::cout << ',';
            std::cout << "{\"area\":" << ai << ",\"admitted_primitives\":" << primitive_count
                      << ",\"positive_primitives\":" << positive_count
                      << ",\"negative_primitives\":" << negative_count
                      << ",\"default_negative_cells_unbound\":" << negative.default_cells_unbound
                      << ",\"special_negative_cells_unbound\":" << negative.special_cells_unbound
                      << ",\"default_entry_found\":" << (legacy_entry_found ? "true" : "false")
                      << ",\"default_grid_idx_mismatch\":" << (legacy_grid_idx_mismatch ? "true" : "false")
                      << ",\"default_camera_primitives_with_visible_vertex\":"
                      << (legacy_entry_found ? geometry.primitives_with_visible_vertex(legacy_entry, 0) : 0)
                      << ",\"inspection_entry_valid\":" << (entry.valid ? "true" : "false")
                      << ",\"inspection_candidate_source\":\""
                      << l3::area_inspection_candidate_name(entry.candidate_source)
                      << "\",\"inspection_camera_primitives_with_visible_vertex\":"
                      << entry.primitives_with_visible_vertex
                      << ",\"inspection_geometry_storage_bytes\":" << geometry.storage_bytes()
                      << ",\"bounds_horizontal_span_x\":"
                      << (entry.bounds.valid ? int64_t(entry.bounds.max[0]) - entry.bounds.min[0] : 0)
                      << ",\"bounds_horizontal_span_z\":"
                      << (entry.bounds.valid ? int64_t(entry.bounds.max[2]) - entry.bounds.min[2] : 0)
                      << ",\"ground_contact_applied\":false}";
        }
        std::cout << "]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
