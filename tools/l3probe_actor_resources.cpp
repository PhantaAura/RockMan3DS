#include "legends3ds/actor_runtime.h"
#include "legends3ds/tutorial_runtime.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

static std::vector<uint8_t> read(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open owner-local cooked input");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: l3probe_actor_resources flow.l3flow area.l3ebd area_index [map.l3map]\n";
        return 2;
    }
    try {
        const auto area = l3::load_tutorial_runtime_area(read(argv[1]), read(argv[2]), std::stoul(argv[3]));
        const auto map = argc == 5 ? std::optional<l3::RuntimeMap>(l3::load_l3map(read(argv[4]))) : std::nullopt;
        auto runtime = l3::spawn_imported_actors(area.stage, area.ebd);
        std::cout << "{\"schema\":\"native-actor-resource-probe/1\",\"area\":" << area.area_index
                  << ",\"source_records\":" << area.stage.instances.size()
                  << ",\"actor_instances\":" << runtime.actors.size()
                  << ",\"unique_resources\":" << runtime.unique_resource_count
                  << ",\"decoded_element_bytes\":" << runtime.decoded_resource_bytes << ",\"actors\":[";
        bool first = true;
        for (auto& actor : runtime.actors) {
            if (!first) std::cout << ',';
            first = false;
            size_t vertices = 0, triangles = 0, quads = 0;
            for (const auto& part : actor.resource->model.parts) {
                vertices += part.vertices.size(); triangles += part.triangles.size(); quads += part.quads.size();
            }
            std::cout << "{\"source_index\":" << actor.source_index << ",\"row\":" << actor.row
                      << ",\"initializer\":\"0x80106974\",\"parts\":" << actor.resource->model.parts.size()
                      << ",\"vertices\":" << vertices << ",\"triangles\":" << triangles << ",\"quads\":" << quads
                      << ",\"source_heading\":" << actor.heading << ",\"raw_scale\":["
                      << actor.raw_scale[0] << ',' << actor.raw_scale[1] << ',' << actor.raw_scale[2]
                      << "],\"neutral_shade_rgb\":[128,128,128],\"material_words\":[";
            for (size_t i = 0; i < actor.resource->bindings.size(); ++i) {
                if (i) std::cout << ',';
                std::cout << actor.resource->bindings[i].material_word;
            }
            std::cout << ']';
            if (map) {
                const auto floor = l3::sample_imported_actor_floor(actor, *map, area.area_index);
                std::cout << ",\"floor_sample\":{\"status\":\"" << l3::runtime_contact_status_name(floor.status)
                          << "\",\"status_code\":" << unsigned(floor.status)
                          << ",\"candidate_cells\":" << floor.candidate_cells
                          << ",\"unsupported_primitives\":" << floor.unsupported_primitives << ",\"raw_y\":";
                if (floor.floor_y) {
                    std::cout << *floor.floor_y;
                    l3::bind_imported_actor_floor(actor, *floor.floor_y);
                } else std::cout << "null";
                std::cout << '}';
            }
            std::cout << ",\"pose_cases\":[";
            bool pose_first = true;
            for (const auto choice : {std::pair<uint8_t, uint8_t>{0, 0}, {2, 16}}) {
                if (!pose_first) std::cout << ',';
                pose_first = false;
                const auto p = l3::decode_ebd_actor_pose(area.ebd, actor.row, *actor.resource, choice.first, choice.second);
                l3::bind_imported_actor_pose(actor, area.ebd, choice.first, choice.second);
                const auto transforms = l3::build_imported_actor_pose_matrices(actor);
                std::array<float, 3> minimum{{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}};
                std::array<float, 3> maximum{{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}};
                for (size_t part = 0; part < actor.resource->model.parts.size(); ++part) {
                    if (!transforms.draw_enabled[part]) continue;
                    for (const auto& vertex : actor.resource->model.parts[part].vertices) {
                        const auto world = l3::player_pose_transform_point(transforms.world[part],
                            {float(vertex.x), float(vertex.y), float(vertex.z)});
                        const std::array<float, 3> point{{world.x, world.y, world.z}};
                        for (size_t axis = 0; axis < 3; ++axis) {
                            minimum[axis] = std::min(minimum[axis], point[axis]);
                            maximum[axis] = std::max(maximum[axis], point[axis]);
                        }
                    }
                }
                std::cout << "{\"request\":" << unsigned(p.request) << ",\"record\":" << unsigned(p.record_index)
                          << ",\"frame_index\":" << unsigned(p.frame_index) << ",\"countdown\":" << unsigned(p.countdown)
                          << ",\"next_control\":" << int(p.next_control)
                          << ",\"sequence_source_offset\":" << p.sequence_source_offset
                          << ",\"pose_source_offset\":" << p.pose_source_offset
                          << ",\"world_bounds_min\":[" << minimum[0] << ',' << minimum[1] << ',' << minimum[2]
                          << "],\"world_bounds_max\":[" << maximum[0] << ',' << maximum[1] << ',' << maximum[2]
                          << "],\"packed_channels\":" << l3::kActorPoseChannels
                          << ",\"retail_state_selection_verified\":false}";
            }
            std::cout << "]}";
        }
        std::cout << "],\"diagnostics\":[";
        first = true;
        for (const auto& d : runtime.diagnostics) {
            if (!first) std::cout << ',';
            first = false;
            // Reasons are controlled decoder/profile strings without owner strings.
            std::cout << "{\"source_index\":" << d.source_index << ",\"state\":" << int(d.state)
                      << ",\"reason\":\"" << d.reason << "\"}";
        }
        l3::unload_imported_actors(runtime);
        std::cout << "],\"unload_actor_count\":" << runtime.actors.size()
                  << ",\"unload_decoded_bytes\":" << runtime.decoded_resource_bytes
                  << ",\"limits\":[\"static source poses; progression flag selection unresolved\","
                     "\"no enemy identity, AI, animation request timing, texture residency or render parity claim\"]}\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "actor probe failed: " << e.what() << '\n';
        return 1;
    }
}
