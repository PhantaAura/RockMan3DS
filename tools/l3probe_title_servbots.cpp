// Owner-local probe for the title Servbot parade: decodes the DEMO_DAT.EBD
// class-20 row and runs 0x80015428 + 0x800153EC over the cooked scripts,
// printing one line per active slot and frame for comparison with an oracle
// RAM trace (class-20 pool 0x8009F108, stride 0x3C0).
#include "legends3ds/actor_resource.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/frontend_servbots.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read(const char* path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error(std::string("cannot open ") + path);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    if (argc != 5 && argc != 6) {
        std::cerr << "usage: l3probe_title_servbots title.l3ebd servbots.l3ts reference.l3ref FRAMES [WAVE]\n";
        return 2;
    }
    try {
        const auto ebd = l3::load_l3ebd_directory(read(argv[1]));
        const auto rows = l3::find_ebd_entries(ebd, 0x20, 0);
        if (rows.size() != 1) throw std::runtime_error("expected one class 0x20/0 row");
        const auto mesh = l3::decode_ebd_actor_mesh(ebd, rows[0]);
        std::printf("# mesh row=%zu parts=%zu decoded=%zu\n", rows[0], mesh.model.parts.size(), mesh.footprint.decoded_bytes);
        std::set<uint32_t> words;
        for (const auto& b : mesh.bindings) words.insert(b.material_word);
        for (const auto w : words) std::printf("# material 0x%08X\n", w);

        const auto scripts = l3::load_title_servbot_scripts(read(argv[2]));
        const auto refs = l3::load_l3reference(read(argv[3]));
        const unsigned frames = unsigned(std::stoul(argv[4]));
        l3::TitleServbotParade parade;
        if (argc == 6) parade.wave_08 = int16_t(std::stoi(argv[5]));   // a later wave's spawn
        std::set<uint8_t> requests;
        std::printf("frame,slot,state,wave,x,y,z,heading,anim,sounds\n");
        for (unsigned frame = 0; frame < frames; ++frame) {
            // 0x800137D4 order: the spawner, then the class callbacks (0x80031AA4).
            l3::TitleServbotFrame out;
            // The title: state 1 sets up and spawns, then state 2. A WAVE
            // argument simulates the menu (state 4) from that wave's spawn.
            const uint8_t state = argc == 6 ? 4 : frame == 0 ? 1 : 2;
            l3::title_servbot_spawn_80015428(parade, state, 0, out);
            l3::title_servbot_update_800153ec(parade, scripts, refs, out);
            for (size_t i = 0; i < parade.slots.size(); ++i) {
                const auto& s = parade.slots[i];
                if (!s.active) continue;
                if (s.anim_ac != 0xFF) requests.insert(s.anim_ac);
                std::printf("%u,%zu,%u,%u,%d,%d,%d,%u,%u,%zu\n", frame, i, unsigned(s.state), unsigned(s.wave),
                            int(s.raw[0]), int(s.raw[1]), int(s.raw[2]), unsigned(s.heading & 0xFFF),
                            unsigned(s.anim_ac), out.sounds.size());
            }
        }
        std::printf("# requests");
        for (const auto r : requests) std::printf(" %u", unsigned(r));
        std::printf("\n");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "l3probe_title_servbots: " << error.what() << '\n';
        return 1;
    }
}
