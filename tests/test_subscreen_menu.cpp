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
    // Item names: table 0x8008CCA4 + 4 * id (inside the message table range).
    put32(table.bytes, (0x8008CCA4 - 0x8008CACC) - 4, 0x8008BC00);         // id -1: empty name
    put32(table.bytes, (0x8008CCA4 - 0x8008CACC) + 4, 0x8008BC04);         // id 1
    msg.bytes[0x8008BC00 - msg.addr] = 0x91;
    for (uint8_t b : {uint8_t(0x1A), uint8_t(0x1B), uint8_t(0x91)}) msg.bytes[0x8008BC04 - msg.addr + (b == 0x1A ? 0 : b == 0x1B ? 1 : 2)] = b;
    // Bank 0x801F2000 (STFF_FE.MSG layout: u16 offsets): 20 title -> 21 list.
    ExeExcerpt::Range bank; bank.addr = 0x801F2000; bank.bytes.assign(0x400, 0);
    const auto bank_msg = [&](unsigned id, uint16_t at, const std::vector<uint8_t>& m) {
        bank.bytes[2 * id] = uint8_t(at); bank.bytes[2 * id + 1] = uint8_t(at >> 8);
        std::memcpy(bank.bytes.data() + at, m.data(), m.size());
    };
    bank_msg(20, 0x200, {0x99, 0x02, 0x15, 0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0x20, 0x00, 0x17, 0x01, 0x93, 0x02, 0x04, 0x1C, 0x8A, 0x01, 0x00, 0x95});
    // 21: max scroll 0x2C, two rows (down/up between them), help base 0xA4, choice 0xDC.
    bank_msg(21, 0x240, {0xAC, 0x1A, 0x2C, 0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0x38, 0x00, 0x17, 0x0A, 0x93, 0x02, 0x04, 0xDF,
                         0xDD, 0x00, 0x01, 0x00, 0x00, 0x4F, 0xE0, 0x86, 0xDD, 0x00, 0x01, 0x01, 0x01, 0x4F, 0xE0, 0x86,
                         0xDE, 0x03, 0xA4, 0x8A, 0x01, 0x00, 0xDC});
    for (unsigned id = 0xA3; id <= 0xA6; ++id)
        bank_msg(id, uint16_t(0x300 + 0x20 * (id - 0xA3)), {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0xBC, 0x00, 0x17, 0x02, 0x15, 0x8A, 0x01, 0x00, 0x95});
    options.bytes.resize(4 * 30, 0);
    put32(options.bytes, 4 * 26, 0x800989D4);
    // Layout table 0x8008ECDC: 6 = the frame list, 26/27 = arrows (one blink sprite each).
    ExeExcerpt::Range layouts; layouts.addr = 0x8008ECDC; layouts.bytes.assign(8 * 28, 0);
    put32(layouts.bytes, 8 * 6 + 4, 0x8008E09C);
    for (int l : {6, 26, 27}) layouts.bytes[size_t(8 * l + 2)] = 13;     // E1 page (+2)
    put32(layouts.bytes, 8 * 26 + 4, 0x8008E0C0); put32(layouts.bytes, 8 * 27 + 4, 0x8008E0E0);
    const uint8_t up[16] = {0, 1, 0, 0, 152, 0, 51, 0, 0x40, 0x78, 0x77, 0x3E, 16, 0, 8, 0};
    const uint8_t dn[16] = {0, 1, 0, 0, 152, 0, 173, 0, 0x50, 0x78, 0x77, 0x3E, 16, 0, 8, 0};
    std::memcpy(frame.bytes.data() + 0x24, up, 16); frame.bytes[0x34] = 0xFF; frame.bytes[0x35] = 0xFF;
    std::memcpy(frame.bytes.data() + 0x44, dn, 16); frame.bytes[0x54] = 0xFF; frame.bytes[0x55] = 0xFF;
    // Special Items: bank 25 (title -> 26), 26 = two 0xD7 cells + 0xD9; layout 2
    // (handler 1): cell 0 mode 0 on flag 0x527, cell 1 mode 2 on flag 0x530.
    bank_msg(25, 0x380, {0x99, 0x02, 0x1A, 0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0x20, 0x00, 0x17, 0x01, 0x93, 0x02, 0x04, 0x1C, 0x8A, 0x01, 0x00, 0x95});
    bank_msg(26, 0x3A0, {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x16, 0x00, 0x38, 0x00, 0x17, 0x0A, 0x93, 0x02, 0x04,
                         0xD7, 0x00, 0x00, 0x01, 0x01, 0x4F, 0xD7, 0x01, 0x01, 0x00, 0x00, 0x4F, 0x8A, 0x01, 0x00, 0xD9});
    layouts.bytes[8 * 2] = 1;                                   // handler 1
    put32(layouts.bytes, 8 * 2 + 4, 0x8008E100);
    const uint8_t icon0[16] = {0, 0, 0x27, 0x05, 0, 0, 0, 0, 0x60, 0x20, 0x48, 0x3C, 32, 0, 32, 0};
    const uint8_t icon1[16] = {1, 2, 0x30, 0x05, 0, 0, 0, 0, 0x40, 0x40, 0x49, 0x3C, 32, 0, 32, 0};
    std::memcpy(frame.bytes.data() + 0x64, icon0, 16); std::memcpy(frame.bytes.data() + 0x74, icon1, 16);
    frame.bytes.resize(0x86, 0); frame.bytes[0x84] = 0xFF; frame.bytes[0x85] = 0xFF;
    // Buster Parts: bank 29 = Equip/Sort/Close (0x96 rows, choice 0x97 C0 -> stay/stay/close).
    bank_msg(29, 0x200 - 0x60, {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x82, 0x00, 0x66, 0x00, 0x05, 0x03, 0x93, 0x02, 0x04,
                                0x96, 0x00, 0x21, 0xED, 0x15, 0x86, 0x96, 0x11, 0x02, 0xED, 0x16, 0x86, 0x96, 0x22, 0x10, 0xED, 0x17,
                                0x97, 0xC0, 0xFF, 0xFF, 0xFF, 0x8A, 0x01, 0x00, 0x95});
    layouts.bytes[8 * 7 + 2] = 12; put32(layouts.bytes, 8 * 7 + 4, 0x8008E09C + 0x10);   // terminator only
    e.ranges = {widths, decor, colors, table, options, frame, msg, bank, layouts};
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
    {   // Normal Items (choice 0 -> page 2): empty list, then one owned item.
        SubScreenMenu n(authored());
        SubScreenValues iv = v;
        n.enter();
        for (int i = 0; i < 7; ++i) f = n.update(0, 0, iv);
        f = n.update(0, 0x4000, iv);                         // Cross on Normal Items
        f = n.update(0, 0, iv);
        assert(n.page() == 2);
        for (int i = 0; i < 9; ++i) f = n.update(0, 0, iv);
        assert(n.options_substate() == 2 && f.unported_opcodes == 0);
        assert(n.windows().list_max_scroll_gp4464 == 0x2C && n.windows().list_800be2f8[0] == 255);
        f = n.update(0, 0x4000, iv);                         // pick the empty slot
        f = n.update(0, 0, iv);
        assert(sound(f, 131) && n.options_substate() == 2);
        f = n.update(0, 0x1000, iv);                         // Triangle leaves
        assert(sound(f, 131) && n.options_substate() == 7);
        f = n.update(0, 0, iv);
        assert(n.page() == 0);
        // Owned item 1 (bit 0x40 of byte 0): it is list entry 0 and Cross posts code 65.
        SubScreenMenu o(authored());
        iv.owned_items_800be420[0] = 0x40;
        o.enter();
        for (int i = 0; i < 7; ++i) f = o.update(0, 0, iv);
        f = o.update(0, 0x4000, iv); f = o.update(0, 0, iv);
        for (int i = 0; i < 9; ++i) f = o.update(0, 0, iv);
        assert(o.windows().list_800be2f8[0] == 1 && o.windows().list_count_gp4420 == 1);
        size_t name_glyphs = 0;
        for (const auto& p : f.packets) if (p.length == 9 && (p.words[0] >> 24) == 0x2C && (p.words[4] >> 16) == kMsgFontTpage) {
            const uint8_t u = uint8_t(p.words[2]), vv = uint8_t(p.words[2] >> 8);
            name_glyphs += (u == 0x1A % 21 * 12 && vv == 0x1A / 21 * 12) || (u == 0x1B % 21 * 12 && vv == 0x1B / 21 * 12);
        }
        assert(name_glyphs == 2);                            // the item name came through 0xE0 / 0x91
        f = o.update(0, 0x4000, iv);
        f = o.update(0, 0, iv);
        assert(sound(f, 130) && o.options_substate() == 3);
    }
    {   // Special Items (choice 1 -> page 3): icons follow the story flags.
        const auto icons = [](const SubScreenFrame& fr, uint16_t clut) {
            size_t n = 0;
            for (const auto& p : fr.packets) if (p.length == 9 && (p.words[4] >> 16) == 0x000C && (p.words[2] >> 16) == clut) ++n;
            return n;
        };
        for (const bool owned : {false, true}) {
            SubScreenMenu sp(authored());
            SubScreenValues sv = v;
            sv.story_flag = [owned](uint16_t fl) { return owned && fl == 0x527; };
            sp.enter();
            for (int i = 0; i < 7; ++i) f = sp.update(0, 0, sv);
            f = sp.update(0, 0x40, sv); f = sp.update(0, 0x4000, sv); f = sp.update(0, 0, sv);
            assert(sp.page() == 3);
            for (int i = 0; i < 4; ++i) f = sp.update(0, 0, sv);
            assert(f.unported_opcodes == 0);
            // Cell 0: the icon (CLUT 0x3C48 from its record) when owned, else the empty box (also 0x3C48).
            assert(icons(f, 0x3C48) == 1 && icons(f, 0x3C49) == 0);   // cell 1 (mode 2, flag 0x530 clear) hidden
            size_t icon_u = 0;
            for (const auto& p : f.packets) if (p.length == 9 && (p.words[4] >> 16) == 0x000C) icon_u = uint8_t(p.words[2]);
            assert(icon_u == (owned ? 0x60u : 0x80u));
            f = sp.update(0, 0x1000, sv);                             // Triangle leaves to the menu
            assert(sound(f, 131) && sp.page() == 0);
        }
    }
    {   // Buster Parts (choice 2 -> page 4): 8 gauges of two quads (CLUT 0x3E83), Close leaves.
        SubScreenMenu bp(authored());
        SubScreenValues bv = v;
        bp.enter();
        for (int i = 0; i < 7; ++i) f = bp.update(0, 0, bv);
        f = bp.update(0, 0x40, bv); f = bp.update(0, 0x40, bv); f = bp.update(0, 0x4000, bv); f = bp.update(0, 0, bv);
        assert(bp.page() == 4);
        for (int i = 0; i < 8; ++i) f = bp.update(0, 0, bv);
        size_t gauge = 0;
        for (const auto& p : f.packets) gauge += p.length == 9 && (p.words[4] >> 16) == 0x000C && (p.words[2] >> 16) == 0x3E83;
        assert(f.unported_opcodes == 0 && gauge == 16);
        f = bp.update(0, 0x1000, bv);                 // Triangle: the menu cancels to Close
        for (int i = 0; i < 3; ++i) f = bp.update(0, 0, bv);
        assert(bp.page() == 0 || bp.page() == 1);
    }
    std::cout << "subscreen_menu: ok\n";
    return 0;
}
