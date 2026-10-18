#include "legends3ds/area_package.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/positive_terrain.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

// Aggregate-only diagnostic. It emits no original coordinates, vertices,
// textures, material words or story labels from owner-supplied package data.
int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3area_placement_probe cooked.mmlpkg\n";
            return 2;
        }
        l3::AreaPackageLimits limits;
        auto package = l3::load_area_package(argv[1], limits);
        std::cout << "{\"status\":\"SOURCE_BACKED_PARTIAL\",\"areas\":[";
        for (size_t ai = 0; ai < package.map.areas.size(); ++ai) {
            auto placement = l3::collect_area_positive_placement(package.map, ai);
            size_t primary = 0, secondary = 0;
            for (const auto& instance : placement.instances) {
                if (instance.bank == l3::St00RenderBank::Primary) ++primary;
                else ++secondary;
            }
            const auto legacy = l3::count_area_render_primitives(package.map, package.render, ai, UINT64_MAX);
            const auto placed = l3::count_area_placed_render_primitives(package.render, placement,
                                                                       l3::St00Lod::Mid, UINT64_MAX);
            uint64_t expanded_count = 0;
            int32_t min_y = std::numeric_limits<int32_t>::max();
            int32_t max_y = std::numeric_limits<int32_t>::min();
            for (const auto& instance : placement.instances) {
                if (instance.descriptor_index == 0xFFFFu) continue;
                const auto expanded = l3::expand_positive_terrain_descriptor_lod(package.render,
                    instance.descriptor_index, instance.bank, l3::St00Lod::Mid, instance.origin, 0);
                const auto record_index = package.render.descriptors[instance.descriptor_index]
                    .record_indices[l3::st00_descriptor_slot(instance.bank, l3::St00Lod::Mid)];
                size_t primitive_index = 0;
                if (record_index != 0xFFFFu) {
                    const auto& record = package.render.records[record_index];
                    for (const auto stream_index : record.stream_indices) {
                        const auto& stream = package.render.streams[stream_index];
                        for (const auto& primitive : stream.primitives) {
                            if (primitive_index >= expanded.size())
                                throw std::runtime_error("positive expansion shorter than source streams");
                            const auto& p = expanded[primitive_index++];
                            if (p.uv != primitive.uv || p.prefix != primitive.prefix ||
                                p.material_word != record.materials[stream.material_slot])
                                throw std::runtime_error("positive expansion lost UV/material association");
                            // The retained-byte inverse is independent of the
                            // helper's coordinate gather. Positive retail XYZ
                            // uses signed X/Z and unsigned Y subtraction.
                            const std::array<int8_t, 12> raw{{
                                primitive.local_x[0], primitive.local_z[0], primitive.ignored_raw[0],
                                primitive.local_x[1], primitive.ignored_raw[1], primitive.local_x[2],
                                primitive.local_z[1], primitive.ignored_raw[2], primitive.local_z[2],
                                primitive.ignored_raw[3], primitive.local_x[3], primitive.local_z[3]}};
                            const int64_t scale = int64_t{1} << stream.base_shift;
                            for (size_t vertex = 0; vertex < 4; ++vertex) {
                                if (p.world_x[vertex] != instance.origin.x + raw[vertex * 3] * scale ||
                                    p.world_y[vertex] != instance.origin.y -
                                        static_cast<uint8_t>(raw[vertex * 3 + 1]) * scale ||
                                    p.world_z[vertex] != instance.origin.z + raw[vertex * 3 + 2] * scale)
                                    throw std::runtime_error("positive expansion differs from source XYZ/origin");
                                min_y = std::min(min_y, p.world_y[vertex]);
                                max_y = std::max(max_y, p.world_y[vertex]);
                            }
                        }
                    }
                }
                if (primitive_index != expanded.size())
                    throw std::runtime_error("positive expansion longer than source streams");
                expanded_count += expanded.size();
            }
            if (expanded_count != placed)
                throw std::runtime_error("positive expansion differs from admitted primitive count");
            if (ai) std::cout << ',';
            std::cout << "{\"area\":" << ai
                      << ",\"positive_occurrences\":" << placement.positive_occurrences
                      << ",\"unique_positive_instances\":" << placement.instances.size()
                      << ",\"duplicate_occurrences\":" << placement.duplicate_occurrences
                      << ",\"negative_cells_pending\":" << placement.negative_cells
                      << ",\"zero_cells\":" << placement.zero_cells
                      << ",\"bitmap_bytes\":" << placement.bitmap_bytes
                      << ",\"primary_instances\":" << primary
                      << ",\"secondary_instances\":" << secondary
                      << ",\"legacy_mid_primitive_count\":" << legacy
                      << ",\"placed_mid_primitive_count\":" << placed
                      << ",\"expanded_geometry_count\":" << expanded_count
                      << ",\"source_xyz_uv_material_checked\":true"
                      << ",\"vertical_span\":" << (expanded_count ? int64_t(max_y) - min_y : 0)
                      << ",\"placed_uv_vbo_byte_estimate\":" << placed * 6u * 20u << '}';
        }
        std::cout << "]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
