// Runs the cooked OPTION message (cook_option_menu.py) through the message
// windows and prints each window's glyphs as text with their positions.
#include "legends3ds/mml_message.h"
#include "legends3ds/mml_message_window.h"
#include "legends3ds/st00_source.h"
#include <cstdio>
#include <map>

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: l3probe_option_menu option.l3ex [pad...]\n"); return 2; }
    l3::MessageWindows w(l3::load_exe_excerpt(l3::read_file(argv[1])));
    w.open_8005382c(0, l3::kOptionMessage8008b69c, 255);
    for (int i = 2; i <= argc; ++i) {
        const uint16_t pad = i < argc ? uint16_t(std::strtoul(argv[i], nullptr, 0)) : 0;
        const auto f = w.tick_80053b40(0, pad);
        std::printf("tick pad=0x%04x unported=%u sounds:", pad, f.unported_opcodes);
        for (const auto& s : f.sounds) std::printf(" %u", s[0]);
        std::printf(" result=0x%08x turn=%u look=%u lock=%u sound=%u map=%u\n", w.flags_80098a58,
            w.settings().turn_800c1b64, w.settings().free_look_800c1b65, w.settings().lock_on_800c1b66,
            w.settings().sound_800c1b68, w.settings().map_800c1b69);
        if (i < argc) continue;
        std::map<std::pair<int, int>, std::string> lines;
        for (const auto& p : f.packets) {
            const uint8_t code = uint8_t(p.words[0] >> 24);
            if (code == 0x2C) {
                const int x = int16_t(p.words[1]), y = int16_t(p.words[1] >> 16);
                const unsigned u = uint8_t(p.words[2]), v = uint8_t(p.words[2] >> 8), g = v / 12 * 21 + u / 12;
                char c = l3::mml_glyph_ascii(uint8_t(g)); if (g == 0x60) c = '>'; if (!c) c = '#';
                const bool dim = uint8_t(p.words[0]) < 128;
                lines[{y, x}] = dim ? std::string(1, char(c | 0x20)) == std::string(1, c) ? std::string("_") + c : std::string("_") + c : std::string(1, c);
            } else if (code == 0x38 || code == 0x2A) {
                std::printf("  prim %02x xy0 (%d,%d) xy3 (%d,%d) rgb %06x\n", code, int16_t(p.words[1]), int16_t(p.words[1] >> 16),
                    code == 0x38 ? int16_t(p.words[7]) : int16_t(p.words[4]), code == 0x38 ? int16_t(p.words[7] >> 16) : int16_t(p.words[4] >> 16), p.words[0] & 0xFFFFFF);
            }
        }
        int last_y = -999;
        for (const auto& [k, s] : lines) {
            if (k.first != last_y) { std::printf("\n y=%3d x=%3d: ", k.first, k.second); last_y = k.first; }
            std::printf("%s", s.c_str());
        }
        std::printf("\n");
    }
    return 0;
}
