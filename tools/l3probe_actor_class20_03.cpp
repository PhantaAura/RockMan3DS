#include "legends3ds/actor_runtime.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace {
std::vector<uint8_t> read(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open existing owner-local EBD");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
int number(const char* value, int lo, int hi) {
    size_t used = 0;
    const int n = std::stoi(value, &used, 0);
    if (value[used] || n < lo || n > hi) throw std::runtime_error("probe scalar out of range");
    return n;
}
}
int main(int argc, char** argv) {
    if (argc != 9 && argc != 11) {
        std::cerr << "usage: l3probe_actor_class20_03 area.l3ebd x y z heading parameter3 raw_stats_selector context_high_byte [st00.l3map area_index]\n";
        return 2;
    }
    try {
        const auto ebd = l3::load_l3ebd_directory(read(argv[1]));
        l3::StageObjectRecord record;
        record.id = 1; record.object_class = 0x20;
        record.raw[0] = 1; record.raw[1] = 0x20; record.raw[2] = 3;
        record.x = static_cast<int16_t>(number(argv[2], -32768, 32767));
        record.y = static_cast<int16_t>(number(argv[3], -32768, 32767));
        record.z = static_cast<int16_t>(number(argv[4], -32768, 32767));
        const int heading = number(argv[5], 0, 4095);
        if (heading % 16) throw std::runtime_error("source heading must be a multiple of16");
        record.raw[7] = static_cast<uint8_t>(heading / 16);
        record.raw[3] = static_cast<uint8_t>(number(argv[6], 0, 255));
        const auto selector = static_cast<uint8_t>(number(argv[7], 0, 255));
        record.raw[11] = static_cast<uint8_t>(number(argv[8], 0, 255));
        for (size_t i = 0; i < record.words_02_to_0a.size(); ++i)
            record.words_02_to_0a[i] = l3::rd_u16(record.raw.data() + 2 + i * 2);
        l3::StageObjectList list; list.records.push_back(record);
        const auto stage = l3::build_fresh_stage_runtime_area(list, ebd);
        auto runtime = l3::spawn_imported_actors(stage, ebd);
        if (runtime.actors.size() != 1 || !runtime.actors[0].pose)
            throw std::runtime_error(runtime.diagnostics.empty() ? "actor failed to bind" : runtime.diagnostics[0].reason);
        const auto& actor = runtime.actors[0];
        std::optional<l3::ImportedActorFloorSample> floor;
        if (argc == 11) {
            const auto map = l3::load_l3map(read(argv[9]));
            floor = l3::sample_imported_actor_floor(actor, map, static_cast<size_t>(number(argv[10], 0, 255)));
        }
        const auto transforms = l3::build_imported_actor_pose_matrices(actor);
        size_t vertices = 0, triangles = 0, quads = 0, enabled = 0;
        std::array<float, 3> lo{{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}};
        std::array<float, 3> hi{{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}};
        for (size_t i = 0; i < actor.resource->model.parts.size(); ++i) {
            const auto& part = actor.resource->model.parts[i];
            vertices += part.vertices.size(); triangles += part.triangles.size(); quads += part.quads.size();
            if (!transforms.draw_enabled[i]) continue;
            ++enabled;
            for (const auto& v : part.vertices) {
                const auto p = l3::player_pose_transform_point(transforms.world[i], {float(v.x), float(v.y), float(v.z)});
                const std::array<float, 3> xyz{{p.x, p.y, p.z}};
                for (size_t axis = 0; axis < 3; ++axis) { lo[axis] = std::min(lo[axis], xyz[axis]); hi[axis] = std::max(hi[axis], xyz[axis]); }
            }
        }
        const auto hp = l3::class20_03_initial_hp(selector, record.raw[11]);
        std::cout << "{\"schema\":\"native-class20-03-resource-probe/1\",\"status\":\"SOURCE_BACKED_PARTIAL\","
                  << "\"placement_diagnostic\":true,\"natural_spawn_executed\":false,\"initializer\":\"0x80102410\",\"row\":" << actor.row
                  << ",\"parts\":" << transforms.part_count << ",\"draw_enabled_parts\":" << enabled
                  << ",\"vertices\":" << vertices << ",\"triangles\":" << triangles << ",\"quads\":" << quads
                  << ",\"decoded_element_bytes\":" << runtime.decoded_resource_bytes
                  << ",\"pose_request\":" << unsigned(actor.pose->request) << ",\"pose_record\":" << unsigned(actor.pose->record_index)
                  << ",\"pose_frame\":" << unsigned(actor.pose->frame_index) << ",\"pose_source_offset\":" << actor.pose->pose_source_offset
                  << ",\"pose_selected_by_retail_initializer\":" << (actor.pose_selected_by_retail_state ? "true" : "false")
                  << ",\"shade_verified\":false,\"floor_bounds_status\":\"SOURCE_BACKED_PARTIAL\","
                  << "\"floor_bounds\":[-60,60,-60,60,-128,0],\"floor_query_executed\":" << (floor ? "true" : "false")
                  << ",\"floor_status\":\"" << (floor ? l3::runtime_contact_status_name(floor->status) : "NOT_SAMPLED")
                  << "\",\"floor_y\":";
        if (floor && floor->floor_y) std::cout << *floor->floor_y; else std::cout << "null";
        if (floor) std::cout << ",\"floor_status_code\":" << unsigned(floor->status)
                            << ",\"floor_candidate_cells\":" << floor->candidate_cells
                            << ",\"floor_unsupported_primitives\":" << floor->unsupported_primitives;
        std::cout << ",\"initial_hp\":";
        if (hp) std::cout << *hp; else std::cout << "null";
        std::cout << ",\"world_bounds_min\":[" << lo[0] << ',' << lo[1] << ',' << lo[2]
                  << "],\"world_bounds_max\":[" << hi[0] << ',' << hi[1] << ',' << hi[2] << "]";
        l3::unload_imported_actors(runtime);
        std::cout << ",\"actors_after_unload\":" << runtime.actors.size()
                  << ",\"decoded_bytes_after_unload\":" << runtime.decoded_resource_bytes << "}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
