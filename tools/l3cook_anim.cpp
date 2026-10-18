#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static uint32_t parse_u32(const char* s) {
    size_t used = 0;
    const unsigned long v = std::stoul(s, &used, 0);
    if (used != std::string(s).size() || v > 0xFFFFFFFFul) throw std::runtime_error("invalid 32-bit load base");
    return static_cast<uint32_t>(v);
}

int main(int argc, char** argv) {
    try {
        if (argc != 6) {
            std::cerr << "usage: l3cook_anim S.PBD S_LOAD_BASE M.PBD M_LOAD_BASE OUT.l3anim\n";
            return 2;
        }
        const auto sbytes = l3::read_file(argv[1]);
        const auto mbytes = l3::read_file(argv[3]);
        const uint32_t sbase = parse_u32(argv[2]);
        const uint32_t mbase = parse_u32(argv[4]);
        const auto anim = l3::parse_player_pbd_animation(sbytes, sbase, mbytes, mbase);
        l3::L3AnimationStats stats;
        const auto cooked = l3::cook_l3animation(anim, &stats);
        std::ofstream f(argv[5], std::ios::binary);
        f.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));
        if (!f) throw std::runtime_error("failed writing output");
        std::cout << "slots=" << stats.action_slots
                  << " populated=" << stats.populated_actions
                  << " frame_tables=" << stats.frame_table_actions
                  << " special_m=" << stats.special_m_actions
                  << " s_records=" << stats.s_records
                  << " frames=" << stats.frames
                  << " channels/frame=" << l3::kPbdPackedAngleChannels
                  << " bytes=" << cooked.size() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3cook_anim: " << e.what() << "\n";
        return 1;
    }
}
