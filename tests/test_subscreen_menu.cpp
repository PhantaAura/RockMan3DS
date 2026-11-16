// In-game sub-screen (0x8005EC34) on an authored excerpt: tables at their
// original addresses, message bytes written for this test. Checks the menu
// rows (0x96) and help (0xD6), Options (choice 4 -> page 6 -> back to the
// menu), Close / Start -> exit, and the status bar sprites (0x80061E0C).
#include "legends3ds/subscreen_menu.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace l3;

namespace {
void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int k = 0; k < 4; ++k) b[o + size_t(k)] = uint8_t(v >> (8 * k)); }

ExeExcerpt authored() {
    ExeExcerpt e;
    ExeExcerpt::Range widths; widths.addr = kMsgGlyphWidths8008ae7c; widths.bytes.assign(132, 8);
    ExeExcerpt::Range decor; decor.addr = kMsgDecorParam8008af18; decor.bytes = {0, 0xFF, 1, 0, 1, 0xFF, 0xFF, 0xFF, 1};
    ExeExcerpt::Range colors; colors.addr = kMsgClutTable80097f30; colors.bytes.assign(0x60, 0x40);
    ExeExcerpt::Range table; table.addr = kMsgTable8008cacc; table.bytes.assign(4 * 0xE4, 0);
    ExeExcerpt::Range options; options.addr = kMsgOptionTable8008d0d4; options.bytes.assign(4 * 8, 0);
    put32(options.bytes, 4 * 2, 0x800C1B64); put32(options.bytes, 4 * 7, 0x8009879C);
    ExeExcerpt::Range frame; frame.addr = 0x8008E09C; frame.bytes.assign(0x84, 0);
    // One frame record (mode 0) at (16,32) 56x16, then the terminator.
    const uint8_t rec[16] = {0, 0, 0, 0, 16, 0, 32, 0, 0, 0xA0, 0x75, 0x3E, 56, 0, 16, 0};
    std::memcpy(frame.bytes.data(), rec, 16); frame.bytes[16] = 0xFF; frame.bytes[17] = 0xFF;
    ExeExcerpt::Range msg; msg.addr = 0x8008B600; msg.bytes.assign(0x800, 0);
    const auto place = [&](uint32_t id, uint32_t at, const std::vector<uint8_t>& m) {
        std::memcpy(msg.bytes.data() + (at - msg.addr), m.data(), m.size()); put32(table.bytes, 4 * id, at);
    };
    // Menu 0x32: six 0x96 rows (up/down nav), help 0xD6 2 0xDD, choice 0x97 0x80.
    std::vector<uint8_t> menu = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x58, 0x00, 0x50, 0x00, 0x0C, 0x06, 0x93, 0x02, 0x04};
    const uint8_t nav[6] = {0x51, 0x02, 0x13, 0x24, 0x35, 0x40};
    for (int r = 0; r < 6; ++r) { for (uint8_t b : {uint8_t(0x96), uint8_t(0x00), nav[r], uint8_t(0xED), uint8_t(0x15 + r), uint8_t(0x86)}) menu.push_back(b); }
    for (uint8_t b : {0xD6, 0x02, 0xDD, 0x97, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8A, 0x01, 0x00, 0x95}) menu.push_back(b);
    place(0x32, 0x8008B700, menu);
    // Options 0x26: one 0xAD row (Turn) and Close.
    place(0x26, kOptionMessage8008b69c, {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x1C, 0x00, 0x3E, 0x00, 0x05, 0x03, 0x93, 0x02, 0x04,
        0xAD, 0x00, 0x11, 0x02, 0x02, 0xE3, 0xE3, 0xED, 0x15, 0xAF, 0x12,
        0xAD, 0x11, 0x00, 0x07, 0x02, 0xE3, 0xE3, 0xED, 0x16, 0x97, 0x80, 0xFF, 0xFF, 0x8A, 0x01, 0x00, 0x95});
    const std::vector<uint8_t> help = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0xBC, 0x00, 0x17, 0x02, 0x15, 0x8A, 0x01, 0x00, 0x95};
    for (uint32_t id = 0xDD; id <= 0xE3; ++id) place(id, 0x8008BB00 + 0x20 * (id - 0xDD), help);
    e.ranges = {widths, decor, colors, table, options, frame, msg};
    return e;
}
bool sound(const SubScreenFrame& f, uint8_t id) { for (const auto& s : f.sounds) if (s[0] == id) return true; return false; }
size_t gauge_sprites(const SubScreenFrame& f, uint8_t v) {
    size_t n = 0;
    for (const auto& p : f.packets)
        if (p.length == 9 && (p.words[4] >> 16) == 0x000D && (p.words[2] >> 16) == 0x3E75 && uint8_t(p.words[2] >> 8) == v) ++n;
    return n;
}
} // namespace

int main() {
    SubScreenMenu m(authored());
    SubScreenValues v; v.life_800b521e = 40; v.life_max_800b5260 = 80; v.zenny_800c1b2c = 1234; v.play_time_800c1b1c = 1800 * 61;
    m.enter();
    auto f = m.update(0, 0, v);
    assert(m.page() == 1 && f.unported_opcodes == 0);
    // Status bar: 5 life cells (80 HP), 2 full + 1 half... (40 HP -> 2 full, 1 half? 40 = 16+16+8 -> full, full, half, empty, empty).
    assert(gauge_sprites(f, 0xB8) == 5 + 0);
    size_t full = 0, half = 0, empty = 0;
    for (const auto& p : f.packets) if (p.length == 9 && uint8_t(p.words[2] >> 8) == 0xB8 && (p.words[2] >> 16) == 0x3E75) {
        const uint8_t u = uint8_t(p.words[2]); full += u == 0x70; half += u == 0x78; empty += u == 0x80;
    }
    assert(full == 2 && half == 1 && empty == 2);
    // Digits (v 0xB0): zenny 1234 -> 4 (leading zeros blank), hours 00 -> 2, minutes 01 -> 2.
    assert(gauge_sprites(f, 0xB0) == 4 + 2 + 2);
    // Frame record drawn (v 0xA0) on entry 30, the cells on top (status bar added first).
    assert(gauge_sprites(f, 0xA0) == 1);
    // Menu: Down x4 to Options (sounds 129), Cross -> page 6.
    for (int i = 0; i < 5; ++i) f = m.update(0, 0, v);
    for (int i = 0; i < 4; ++i) { f = m.update(0, 0x40, v); assert(sound(f, 129)); }
    f = m.update(0, 0x4000, v);
    assert(sound(f, 130));
    f = m.update(0, 0, v);
    assert(m.page() == 6);
    f = m.update(0, 0, v); f = m.update(0, 0, v);        // options sub 0, 1 (open 0x26)
    assert(m.options_substate() == 2 && gauge_sprites(f, 0xA0) == 0);   // layout cleared on the Options page
    for (int i = 0; i < 6; ++i) f = m.update(0, 0, v);
    f = m.update(0, 0x20, v);                             // Right on Turn
    assert(sound(f, 130) && m.windows().settings().turn_800c1b64 == 1);
    f = m.update(0, 0x40, v); f = m.update(0, 0, v);
    f = m.update(0, 0x4000, v);                           // Cross on Close
    f = m.update(0, 0, v);
    assert(f.settings_applied && m.options_substate() == 3);
    f = m.update(0, 0, v);
    assert(m.page() == 0);
    f = m.update(0, 0, v);
    assert(m.page() == 1);
    // Start in the menu: exit (page 8), and the windows are cleared.
    for (int i = 0; i < 6; ++i) f = m.update(0, 0, v);
    f = m.update(0, 0x0008, v);
    f = m.update(0, 0, v);
    assert(m.page() == 8 && f.exited);
    std::cout << "subscreen_menu: ok\n";
    return 0;
}
