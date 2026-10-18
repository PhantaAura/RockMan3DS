#include "legends3ds/area_package.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/negative_terrain.h"
#include "legends3ds/render/positive_terrain.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
// Match the current native package/terrain admission policy. Host sizeof-based
// estimates are conservative diagnostics, not measured Old3DS memory usage.
constexpr uint64_t kFileBytes = 8u * 1024u * 1024u;
constexpr uint64_t kSectionBytes = 4u * 1024u * 1024u;
constexpr uint64_t kDecodedBytes = 4u * 1024u * 1024u;
constexpr uint64_t kPeakBytes = 8u * 1024u * 1024u;
constexpr uint64_t kPrimitiveCap = (4u * 1024u * 1024u) / (5u * sizeof(float)) / 6u;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct Counts {
    uint64_t areas = 0, empty_areas = 0, positive_cells = 0, positive_instances = 0;
    uint64_t aliases = 0, positive_unbound = 0, positive_primitives = 0;
    uint64_t default_cells = 0, special_cells = 0, default_bound = 0, special_bound = 0;
    uint64_t default_unbound = 0, special_unbound = 0, negative_primitives = 0;
    uint64_t default_known_primitives = 0, default_unknown_primitives = 0;
    uint64_t special_unknown_primitives = 0, material_areas = 0;
    uint64_t decoded_estimate = 0, peak_estimate = 0, unloaded = 0;
};
Counts audit(const char* path) {
    l3::AreaPackageLimits limits;
    limits.max_file_bytes = kFileBytes; limits.max_section_bytes = kSectionBytes;
    limits.max_decoded_bytes = kDecodedBytes; limits.max_load_peak_bytes = kPeakBytes;
    auto package = l3::load_area_package(path, limits);
    Counts out;
    out.areas = package.map.areas.size(); out.material_areas = package.negative_materials.size();
    out.decoded_estimate = package.diagnostics.decoded_estimated_bytes;
    out.peak_estimate = package.diagnostics.load_peak_estimated_bytes;
    for (size_t ai = 0; ai < package.map.areas.size(); ++ai) {
        const auto positive = l3::collect_area_positive_placement(package.map, ai);
        const auto negative = l3::collect_area_negative_placement(package.map, package.render, ai);
        const uint64_t positive_count = l3::count_area_placed_render_primitives(
            package.render, positive, l3::St00Lod::Mid, kPrimitiveCap);
        const uint64_t negative_count = l3::count_area_negative_mid_primitives(
            package.render, negative, kPrimitiveCap - positive_count);
        out.empty_areas += positive_count + negative_count == 0;
        out.positive_cells += positive.positive_occurrences;
        out.positive_instances += positive.instances.size(); out.aliases += positive.duplicate_occurrences;
        out.default_unbound += negative.default_cells_unbound;
        out.special_unbound += negative.special_cells_unbound;
        const auto& area = package.map.areas[ai];
        uint64_t default_cells = 0, special_cells = 0;
        for (uint32_t local = 0; local < area.cell_count; ++local) {
            const int16_t cell = package.map.cells[size_t(area.cell_first) + local];
            if (cell < 0) {
                if (static_cast<uint16_t>(cell) & 0x80u) ++special_cells;
                else ++default_cells;
            }
        }
        out.default_cells += default_cells; out.special_cells += special_cells;
        uint64_t positive_expanded = 0, negative_expanded = 0, default_bound = 0, special_bound = 0;
        for (const auto& instance : positive.instances) {
            if (instance.descriptor_index == 0xFFFFu) { ++out.positive_unbound; continue; }
            const auto expanded = l3::expand_positive_terrain_descriptor_lod(package.render,
                instance.descriptor_index, instance.bank, l3::St00Lod::Mid, instance.origin, 0);
            const auto record_index = package.render.descriptors[instance.descriptor_index]
                .record_indices[l3::st00_descriptor_slot(instance.bank, l3::St00Lod::Mid)];
            size_t pi = 0;
            if (record_index != 0xFFFFu) {
                const auto& record = package.render.records[record_index];
                for (uint32_t si : record.stream_indices) {
                    const auto& stream = package.render.streams[si];
                    for (const auto& source : stream.primitives) {
                        require(pi < expanded.size(), "positive expansion count shortened");
                        const auto& primitive = expanded[pi++];
                        require(primitive.texture_binding_known && primitive.prefix == source.prefix &&
                                primitive.uv == source.uv &&
                                primitive.material_word == record.materials[stream.material_slot],
                                "positive source material/UV mismatch");
                    }
                }
            }
            require(pi == expanded.size(), "positive expansion count amplified");
            positive_expanded += expanded.size();
        }
        for (const auto& instance : negative.instances) {
            const bool special = (instance.packed_cell & 0x80u) != 0;
            const uint16_t expected_descriptor = special
                ? package.render.special_negative_descriptors[(instance.packed_cell >> 2) & 31u]
                : package.render.default_negative_descriptor;
            require(instance.descriptor_index == expected_descriptor, "negative descriptor selector mismatch");
            if (special) ++special_bound; else ++default_bound;
            const auto* binding = l3::find_negative_material_binding(package.negative_materials, ai,
                                                                  instance.packed_cell);
            require(!special || !binding, "special branch borrowed the default material table");
            const auto expanded = l3::expand_negative_terrain_mid(package.render, instance, binding);
            const auto record_index = package.render.descriptors[instance.descriptor_index]
                .record_indices[l3::st00_descriptor_slot(l3::St00RenderBank::Primary, l3::St00Lod::Mid)];
            size_t pi = 0;
            if (record_index != 0xFFFFu) {
                const auto& record = package.render.records[record_index];
                for (uint32_t si : record.stream_indices) for (const auto& source : package.render.streams[si].primitives) {
                    require(pi < expanded.size(), "negative expansion count shortened");
                    const auto& primitive = expanded[pi++];
                    const bool known = binding && binding->known;
                    require(primitive.texture_binding_known == known && primitive.prefix == source.prefix,
                            "negative source material state mismatch");
                    require(primitive.material_word == (known ? binding->material_word : 0) &&
                            primitive.uv == (known ? l3::negative_material_quad_uv(*binding) : source.uv),
                            "negative source material/UV mismatch");
                    for (int32_t y : primitive.world_y)
                        require(y == -int32_t((instance.packed_cell & 0x1F00u) >> 1),
                                "negative encoded flat height mismatch");
                    if (special) ++out.special_unknown_primitives;
                    else if (known) ++out.default_known_primitives;
                    else ++out.default_unknown_primitives;
                }
            }
            require(pi == expanded.size(), "negative expansion count amplified");
            negative_expanded += expanded.size();
        }
        require(default_bound + negative.default_cells_unbound == default_cells &&
                special_bound + negative.special_cells_unbound == special_cells,
                "negative bound/unbound cell coverage mismatch");
        require(positive_expanded == positive_count && negative_expanded == negative_count,
                "placement count/expansion mismatch");
        out.default_bound += default_bound; out.special_bound += special_bound;
        out.positive_primitives += positive_expanded; out.negative_primitives += negative_expanded;
    }
    package.unload();
    require(package.map.areas.capacity() == 0 && package.render.streams.capacity() == 0 &&
            package.directory.capacity() == 0 && package.negative_materials.capacity() == 0 &&
            package.render.default_negative_descriptor == 0xFFFFu &&
            package.diagnostics.decoded_estimated_bytes == 0 && package.diagnostics.load_peak_estimated_bytes == 0,
            "package unload retained owned data");
    for (uint16_t descriptor : package.render.special_negative_descriptors)
        require(descriptor == 0xFFFFu, "package unload retained special identity");
    out.unloaded = 1;
    return out;
}
void print(const Counts& c) {
    std::cout << "\"areas\":" << c.areas << ",\"empty_areas\":" << c.empty_areas
        << ",\"positive_cells\":" << c.positive_cells << ",\"positive_instances\":" << c.positive_instances
        << ",\"positive_aliases\":" << c.aliases << ",\"positive_unbound\":" << c.positive_unbound
        << ",\"positive_primitives\":" << c.positive_primitives
        << ",\"default_negative_cells\":" << c.default_cells << ",\"special_negative_cells\":" << c.special_cells
        << ",\"default_bound\":" << c.default_bound << ",\"special_bound\":" << c.special_bound
        << ",\"default_unbound\":" << c.default_unbound << ",\"special_unbound\":" << c.special_unbound
        << ",\"negative_primitives\":" << c.negative_primitives
        << ",\"default_material_known_primitives\":" << c.default_known_primitives
        << ",\"default_material_unknown_primitives\":" << c.default_unknown_primitives
        << ",\"special_material_unknown_primitives\":" << c.special_unknown_primitives
        << ",\"default_binding_unavailable\":" << (c.default_unbound ? "true" : "false")
        << ",\"special_binding_unavailable\":" << (c.special_unbound ? "true" : "false")
        << ",\"special_geometry_bound_material_unknown\":" << (c.special_bound ? "true" : "false")
        << ",\"material_table_areas\":" << c.material_areas
        << ",\"decoded_estimated_bytes\":" << c.decoded_estimate
        << ",\"load_peak_estimated_bytes\":" << c.peak_estimate << ",\"unloaded_packages\":" << c.unloaded;
}
void accumulate(Counts& total, const Counts& c) {
    total.areas += c.areas; total.empty_areas += c.empty_areas;
    total.positive_cells += c.positive_cells; total.positive_instances += c.positive_instances;
    total.aliases += c.aliases; total.positive_unbound += c.positive_unbound;
    total.positive_primitives += c.positive_primitives; total.default_cells += c.default_cells;
    total.special_cells += c.special_cells; total.default_bound += c.default_bound;
    total.special_bound += c.special_bound; total.default_unbound += c.default_unbound;
    total.special_unbound += c.special_unbound; total.negative_primitives += c.negative_primitives;
    total.default_known_primitives += c.default_known_primitives;
    total.default_unknown_primitives += c.default_unknown_primitives;
    total.special_unknown_primitives += c.special_unknown_primitives;
    total.material_areas += c.material_areas; total.unloaded += c.unloaded;
    total.decoded_estimate = std::max(total.decoded_estimate, c.decoded_estimate);
    total.peak_estimate = std::max(total.peak_estimate, c.peak_estimate);
}
std::string escaped(const char* message) {
    std::string out;
    for (const char* p = message; *p; ++p) {
        if (*p == '"' || *p == '\\') out += '\\';
        if (static_cast<unsigned char>(*p) >= 32) out += *p;
    }
    return out;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    Counts total;
    unsigned failed = 0;
    for (int i = 1; i < argc; ++i) {
        std::cout << "{\"kind\":\"package\",\"package_index\":" << i - 1 << ',';
        try {
            const auto counts = audit(argv[i]); print(counts); accumulate(total, counts);
            std::cout << ",\"passed\":true}\n";
        } catch (const std::exception& error) {
            ++failed;
            std::cout << "\"passed\":false,\"error\":\"" << escaped(error.what()) << "\"}\n";
        }
    }
    std::cout << "{\"kind\":\"summary\",\"packages\":" << argc - 1 << ",\"failed_packages\":" << failed << ',';
    print(total);
    std::cout << ",\"max_file_bytes\":" << kFileBytes << ",\"max_section_bytes\":" << kSectionBytes
              << ",\"max_decoded_bytes\":" << kDecodedBytes << ",\"max_load_peak_bytes\":" << kPeakBytes
              << ",\"per_area_primitive_cap\":" << kPrimitiveCap
              << ",\"status\":\"SOURCE_BACKED_PARTIAL\"}\n";
    return failed ? 1 : 0;
}
