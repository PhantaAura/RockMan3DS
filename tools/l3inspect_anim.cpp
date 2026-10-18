#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3inspect_anim FILE.l3anim\n";
            return 2;
        }
        const auto b = l3::read_file(argv[1]);
        const auto h = l3::inspect_l3animation(b, true);
        std::cout << "L3AN v" << h.version
                  << " slots=" << h.stats.action_slots
                  << " populated=" << h.stats.populated_actions
                  << " frame_tables=" << h.stats.frame_table_actions
                  << " special_m=" << h.stats.special_m_actions
                  << " s_records=" << h.stats.s_records
                  << " frames=" << h.stats.frames
                  << " channels/frame=" << h.angle_channels_per_frame
                  << " bytes=" << h.file_bytes << " crc=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3inspect_anim: " << e.what() << "\n";
        return 1;
    }
}
