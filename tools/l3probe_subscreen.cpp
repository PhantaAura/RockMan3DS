// Runs the in-game sub-screen (SubScreenMenu) on cooked owner data and
// writes the last frame as a PPM: l3probe_subscreen option.l3ex font.l3mc
// subscreen.l3mc out.ppm [pad...] (one pad word per update, 0x... PS1 bits).
#include "legends3ds/message_window_raster.h"
#include "legends3ds/st00_source.h"
#include "legends3ds/subscreen_menu.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 5) { std::fprintf(stderr, "usage: l3probe_subscreen option.l3ex font.l3mc subscreen.l3mc out.ppm [pad...]\n"); return 2; }
    l3::SubScreenMenu menu(l3::load_exe_excerpt(l3::read_file(argv[1])));
    l3::MessageFont pages;
    pages.add(l3::load_l3material_cache(l3::read_file(argv[2]), true));
    pages.add(l3::load_l3material_cache(l3::read_file(argv[3]), true));
    if (const char* key = std::getenv("SUBKEY_L3MC")) pages.add(l3::load_l3material_cache(l3::read_file(key), true));
    if (const char* wpn = std::getenv("SUBWPN_L3MC")) pages.add(l3::load_l3material_cache(l3::read_file(wpn), true));
    l3::SubScreenValues v; v.life_800b521e = 72; v.life_max_800b5260 = 80; v.zenny_800c1b2c = 0; v.play_time_800c1b1c = 30 * 120;
    v.story_flag = [](uint16_t f) { return f == 0x527; };   // the crystal (oracle, tutorial)
    menu.enter();
    l3::SubScreenFrame f;
    // The sub-screen never clears the frame: transparent tile rows keep the
    // previous frame (the buffer persists, as on the PS1).
    // Two persistent buffers, alternated per update like the PS1's double
    // buffer (each holds the frame from two updates earlier).
    std::vector<uint8_t> buffers[2] = {std::vector<uint8_t>(320u * 240u * 3u, 0), std::vector<uint8_t>(320u * 240u * 3u, 0)};
    int which = 0;
    for (int i = 5; i <= argc + 8 + (std::getenv("EXTRA") ? std::atoi(std::getenv("EXTRA")) : 0); ++i) {
        const uint16_t pad = i < argc ? uint16_t(std::strtoul(argv[i], nullptr, 0)) : 0;
        f = menu.update(0, pad, v);
        std::printf("pad=0x%04x page=%u sub=%u sounds=%zu exited=%u unported=%u\n", pad, menu.page(), menu.options_substate(),
                    f.sounds.size(), unsigned(f.exited), f.unported_opcodes);
        which ^= 1;
        (void)l3::rasterize_message_windows(f.packets, pages, buffers[which].data());
    }
    auto& rgb = buffers[which];
    const size_t drawn = f.packets.size();
    if (std::FILE* out = std::fopen(argv[4], "wb")) { std::fprintf(out, "P6 320 240 255\n"); std::fwrite(rgb.data(), 1, rgb.size(), out); std::fclose(out); }
    std::printf("raster: %zu primitives\n", drawn);
    return 0;
}
