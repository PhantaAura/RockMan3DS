#include "legends3ds/runtime_session.h"
#include "legends3ds/reference_tables.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path.string());
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static l3::RuntimeVec3i first_cell_center(const l3::RuntimeMap& map, size_t area) {
    if (area >= map.areas.size()) throw std::runtime_error("map area missing");
    const auto& a = map.areas[area];
    for (uint32_t local = 0; local < a.cell_count; ++local) {
        const uint32_t ci = a.cell_first + local;
        if (ci >= map.cells.size()) break;
        if (map.cells[ci] == 0) continue;
        const uint32_t lx = local % a.width;
        const uint32_t lz = local / a.width;
        const int32_t gx = int32_t(a.start_x) + int32_t(lx);
        const int32_t gz = int32_t(a.start_z) + int32_t(lz);
        return {(gx << 9) - 0x7F00 + 0x100, 0, (gz << 9) - 0x7F00 + 0x100};
    }
    return {a.world_origin_x + 0x100, 0, a.world_origin_z + 0x100};
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3runtime_session_probe <legends3ds/tutorial runtime dir>\n";
            return 2;
        }
        const fs::path root = argv[1];
        const auto map = l3::load_l3map(read_file(root / "st00.l3map"), true);
        const auto flow_bytes = read_file(root / "st00.l3flow");
        const auto refs = l3::load_l3reference(read_file(root / "reference.l3ref"), true);
        std::vector<l3::MmlMessageFile> messages;
        messages.push_back(l3::load_l3message(read_file(root / "messages/st00_01.l3msg"), true));
        messages.push_back(l3::load_l3message(read_file(root / "messages/st00_02.l3msg"), true));

        std::vector<l3::TutorialRuntimeArea> areas;
        for (size_t i = 0; i < 5; ++i) {
            char name[64]; std::snprintf(name, sizeof(name), "ebd/area_%02zu.l3ebd", i);
            areas.push_back(l3::load_tutorial_runtime_area(flow_bytes, read_file(root / name), i, true));
        }
        size_t ebd_retained_bytes=0, ebd_available_pointers=0;
        for(const auto& area:areas) {
            ebd_retained_bytes+=area.ebd.source_payload.size();
            for(size_t row=0;row<area.ebd.entries.size();++row)
                for(size_t slot=0;slot<3;++slot)
                    if(l3::ebd_resource_span(area.ebd,row,slot,1).status==
                       l3::EbdResourceSpanStatus::AVAILABLE)++ebd_available_pointers;
        }
        L3QolSettings q{}; l3_qol_settings_defaults(&q);
        l3::RuntimeSession session(map, areas[0], messages, q);
        session.set_reference_tables(refs);
        for (size_t i = 0; i < areas.size(); ++i) session.register_area(i, areas[i]);

        size_t mapped = 0, moved = 0, ui_commands = 0;
        for (size_t i = 0; i < areas.size(); ++i) {
            // A positive map cell is not necessarily walkable space. Area 1
            // has a source-backed player handoff from overlay 0x80101274;
            // use it for the movement assertion and keep the synthetic cell
            // centers as load/map-coverage probes only.
            const auto spawn = i == 1 ? l3::RuntimeVec3i{2822, -1, -4833}
                                      : first_cell_center(map, i);
            if (!session.transition_to_area(i, spawn, 0)) throw std::runtime_error("area transition failed");
            auto before = session.player_position();
            l3::RuntimeInputFrame in{}; in.held_psx_mask = 0x0010; // retail profile-0 forward
            // The retail acceleration path can spend its first update changing
            // state without advancing the fixed-point position.
            for (unsigned frame = 0; frame < 8; ++frame) session.tick(in);
            const auto after = session.player_position();
            if (after.x != before.x || after.y != before.y || after.z != before.z) ++moved;
            if (session.probe_player_map().in_area) ++mapped;
            ui_commands += session.bottom_screen_layout().commands.size();
            session.set_story_flag(static_cast<uint16_t>(i), true);
        }
        const auto save = session.make_save_state(0x300);
        for (size_t i = 0; i < areas.size(); ++i) {
            if (!l3::runtime_story_flag_get(save, static_cast<uint16_t>(i))) throw std::runtime_error("story flag persistence failed");
        }
        session.start_dialogue({0,0});
        for (unsigned i=0;i<128 && session.dialogue().active;++i) {
            l3::RuntimeInputFrame in{};
            if (session.dialogue().waiting_for_advance) in.advance_dialogue=true;
            if (session.dialogue().waiting_for_script) session.resume_dialogue_script_wait();
            session.tick(in);
        }
        const auto audio = session.take_audio_requests();
        std::cout << "areas=" << areas.size()
                  << " ebd_retained_bytes=" << ebd_retained_bytes
                  << " ebd_available_pointers=" << ebd_available_pointers
                  << " mapped=" << mapped
                  << " moved=" << moved
                  << " ui_commands=" << ui_commands
                  << " dialogue_audio_requests=" << audio.size()
                  << " save_area=" << save.area_id
                  << " save_bytes=" << l3::cook_l3save(save).size() << "\n";
        if (mapped != areas.size() || moved == 0) return 1;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
