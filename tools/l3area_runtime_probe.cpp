#include "legends3ds/area_package.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/actor_runtime.h"
#include <iostream>
#include <stdexcept>
#include <utility>

// Exercises the same package readers, session map binding and render expansion
// used by the native frontend. Emits counts only, never retail payloads.
int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3area_runtime_probe cooked.mmlpkg\n";
            return 2;
        }
        l3::AreaPackageLimits limits;
        limits.max_file_bytes = 8u * 1024u * 1024u;
        limits.max_section_bytes = 4u * 1024u * 1024u;
        limits.max_decoded_bytes = 4u * 1024u * 1024u;
        limits.max_load_peak_bytes = 8u * 1024u * 1024u;
        auto package = l3::load_area_package(argv[1], limits);
        const auto diagnostics = package.diagnostics;
        auto render = std::move(package.render);
        auto bound_areas=std::move(package.runtime_areas);
        L3QolSettings settings{};
        l3_qol_settings_defaults(&settings);
        l3::RuntimeSession session(std::move(package.map), {}, {}, settings);
        package.unload();
        const auto& map = session.active_map();
        for (size_t i=0;i<map.areas.size();++i) {
            l3::TutorialRuntimeArea area{};area.area_index=i;
            for (auto& bound:bound_areas) if (bound.area_index==i) {area=std::move(bound);break;}
            session.register_area(i,std::move(area));
        }
        std::vector<l3::TutorialRuntimeArea>().swap(bound_areas);
        std::cout << "{\"status\":\"PASS\",\"package_bytes\":" << diagnostics.file_bytes
                  << ",\"decoded_estimate\":" << diagnostics.decoded_estimated_bytes
                  << ",\"load_peak_estimate\":" << diagnostics.load_peak_estimated_bytes
                  << ",\"map_initialization_writes\":" << diagnostics.map_initialization_writes
                  << ",\"map_initialization_temporary_bytes\":" << diagnostics.map_initialization_temporary_bytes
                  << ",\"spawn_decoded_estimate\":" << diagnostics.spawn_estimated_bytes
                  << ",\"skipped_sections\":" << diagnostics.skipped_sections << ",\"areas\":[";
        for (size_t ai = 0; ai < map.areas.size(); ++ai) {
            const auto& area = map.areas[ai];
            size_t positive = 0, primitives = 0, contacts = 0;
            bool map_probe = false;
            for (uint32_t local = 0; local < area.cell_count; ++local) {
                const int16_t packed = map.cells[area.cell_first + local];
                if (packed <= 0) continue;
                const auto& index = map.idx[static_cast<uint16_t>(packed)];
                const int32_t gx = int32_t(area.start_x) + int32_t(local % area.width);
                const int32_t gz = int32_t(area.start_z) + int32_t(local / area.width);
                const l3::St00CellOrigin origin{(gx * 0x200) - 0x7F00, index.y, (gz * 0x200) - 0x7F00};
                ++positive;
                if (!map_probe) {
                    if (!session.transition_to_area(ai, {origin.x, origin.y, origin.z}, 0))
                        throw std::runtime_error("loaded area failed session activation");
                    const auto probe = session.probe_player_map();
                    if (!probe.in_area || !probe.cell || probe.cell->cell_index != area.cell_first + local)
                        throw std::runtime_error("loaded map failed session contact query");
                    contacts = probe.contact_groups.size();
                    map_probe = true;
                }
                if (index.descriptor_index == 0xFFFFu) continue;
                primitives += l3::expand_st00_descriptor_lod(render, index.descriptor_index,
                    l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin,
                    static_cast<uint16_t>(packed), 0).size();
            }
            if(!session.transition_to_area(ai,{area.world_origin_x+256,0,area.world_origin_z+256},0))
                throw std::runtime_error("package runtime binding activation failed");
            auto actors=l3::spawn_imported_actors(session.active_area().stage,session.active_area().ebd);
            const size_t actor_count=actors.actors.size(),actor_bytes=actors.decoded_resource_bytes;
            l3::unload_imported_actors(actors);
            if(!actors.actors.empty()||actors.decoded_resource_bytes)throw std::runtime_error("actor unload failed");
            if (ai) std::cout << ',';
            std::cout << "{\"index\":" << ai << ",\"cells\":" << area.cell_count
                      << ",\"positive_cells\":" << positive << ",\"render_primitives\":" << primitives
                      << ",\"map_probe\":" << (map_probe ? "true" : "false")
                      << ",\"probe_contact_groups\":" << contacts
                      << ",\"bound_object_records\":" << session.active_area().stage.instances.size()
                      << ",\"conditional_spawn_regions\":" << (session.active_area().conditional_spawns?
                          session.active_area().conditional_spawns->regions.size():0)
                      << ",\"retained_ebd_bytes\":" << session.active_area().ebd.source_payload.size()
                      << ",\"supported_actor_instances\":" << actor_count
                      << ",\"actor_decoded_bytes\":" << actor_bytes
                      << ",\"actor_unload_verified\":true" << '}';
        }
        std::cout << "]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
