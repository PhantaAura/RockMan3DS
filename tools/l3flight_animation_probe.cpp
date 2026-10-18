#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 3) {
            std::cerr << "usage: l3flight_animation_probe FILE.l3anim [REQUIRED_REQUEST]\n";
            return 2;
        }
        const auto animation = l3::load_l3animation(l3::read_file(argv[1]), true);
        int required = -1;
        if (argc == 3) required = static_cast<int>(std::strtoul(argv[2], nullptr, 0));
        bool required_ok = required < 0;
        unsigned mapped = 0, records = 0, zero_ticks = 0, branches = 0, repeats = 0;
        for (unsigned request = 0; request < 256u; ++request) {
            const auto* s = l3::find_pbd_animation_action(animation, static_cast<uint16_t>(request));
            const auto* m = l3::pbd_main_body_pose_action_for_request(animation, static_cast<uint8_t>(request));
            if (!s || !s->has_s_records || s->s_records.empty() || !m) continue;
            ++mapped;
            bool ok = true;
            for (size_t i = 0; i < s->s_records.size(); ++i) {
                const auto& r = s->s_records[i];
                const uint8_t pose = static_cast<uint8_t>(r.word0 & 0xFFu);
                const uint8_t ticks = static_cast<uint8_t>(r.word0 >> 8);
                const int8_t ctl = static_cast<int8_t>(r.word1 >> 8);
                if (pose >= m->angle_frames.size()) ok = false;
                if (ticks == 0) ++zero_ticks;
                if (ctl == -1) ++repeats;
                else if (ctl < 0) {
                    ++branches;
                    if ((static_cast<uint8_t>(ctl) & 0x7Fu) >= s->s_records.size()) ok = false;
                }
                ++records;
            }
            std::cout << "request=0x" << std::hex << std::setw(2) << std::setfill('0') << request
                      << std::dec << " S_records=" << s->s_records.size()
                      << " M_action=" << m->action_index << " M_frames=" << m->angle_frames.size()
                      << " structural=" << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) return 1;
            if (required == static_cast<int>(request)) required_ok = true;
        }
        std::cout << "mapped_requests=" << mapped << " S_records=" << records
                  << " zero_tick_records=" << zero_ticks << " branches=" << branches
                  << " repeats=" << repeats << " crc=PASS\n";
        if (!required_ok) {
            std::cerr << "required request has no proven S->M mapping\n";
            return 1;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3flight_animation_probe: " << e.what() << "\n";
        return 1;
    }
}
