// Message windows (0x8005382C / 0x80053B40) on an authored excerpt: the
// tables sit at their original addresses, the message bytes are written for
// this test (no game text). Checks the 0xAD row navigation, the 0xAE value
// brightness, Left/Right/Cross toggles and their sounds, the 0x97 choice
// (Close / Triangle cancel) and the setting bytes.
#include "legends3ds/mml_message_window.h"
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
    ExeExcerpt::Range colors; colors.addr = kMsgClutTable80097f30; colors.bytes.assign(0x60, 0);
    for (int i = 0; i < 16; ++i) { colors.bytes[size_t(2 * i)] = uint8_t(0x3C + i); colors.bytes[size_t(2 * i + 1)] = 0x7F; }
    for (size_t i = 0; i < 16; ++i) colors.bytes[0x48 + i] = uint8_t(i & 2 ? 0x80 : 0xF0);   // border greys at 0x80097F78
    ExeExcerpt::Range table; table.addr = kMsgTable8008cacc; table.bytes.assign(4 * 16, 0);
    ExeExcerpt::Range options; options.addr = kMsgOptionTable8008d0d4; options.bytes.assign(4 * 8, 0);
    put32(options.bytes, 4 * 2, 0x800C1B64); put32(options.bytes, 4 * 3, 0x800C1B65); put32(options.bytes, 4 * 7, 0x8009879C);
    ExeExcerpt::Range msg; msg.addr = 0x8008B600; msg.bytes.assign(0x400, 0);
    const std::vector<uint8_t> menu = {
        0x99, 0x01, 0x05, 0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x1C, 0x00, 0x3E, 0x00, 0x05, 0x03, 0x93, 0x02, 0x04, 0xAF, 0x03,
        0xAD, 0x00, 0x11, 0x02, 0x02, 0x06, 0x07, 0xED, 0x15, 0x16, 0xDB, 0x6A, 0x00, 0x41, 0x00,
        0xAE, 0x02, 0x00, 0x17, 0xB0, 0x80, 0xAE, 0x02, 0x01, 0x18, 0xB0, 0x80, 0xAF, 0x12,
        0xAD, 0x11, 0x00, 0x07, 0x02, 0x08, 0x08, 0xED, 0x19,
        0x97, 0x80, 0xFF, 0xFF, 0x8A, 0x01, 0x00, 0x95};
    std::memcpy(msg.bytes.data() + (kOptionMessage8008b69c - msg.addr), menu.data(), menu.size());
    const std::vector<uint8_t> help = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x1C, 0x00, 0xBC, 0x00, 0x05, 0x01, 0x93, 0x02, 0x04, 0x15, 0x8A, 0x01, 0x00, 0x95};
    for (uint32_t id = 5; id <= 8; ++id) {
        const uint32_t at = 0x8008B800 + 0x20 * id;
        std::memcpy(msg.bytes.data() + (at - msg.addr), help.data(), help.size());
        msg.bytes[at - msg.addr + 14] = uint8_t(0x14 + id);   // one distinct glyph per help message
        put32(table.bytes, 4 * id, at);
    }
    e.ranges = {widths, decor, colors, table, options, msg};
    return e;
}

size_t glyphs_with_u(const MessageWindowFrame& f, uint8_t u, uint8_t v, uint8_t* brightness = nullptr) {
    size_t n = 0;
    for (const auto& p : f.packets)
        if (p.length == 9 && (p.words[0] >> 24) == 0x2C && uint8_t(p.words[2]) == u && uint8_t(p.words[2] >> 8) == v) {
            ++n; if (brightness) *brightness = uint8_t(p.words[0]);
        }
    return n;
}
bool has_sound(const MessageWindowFrame& f, uint8_t id) {
    for (const auto& s : f.sounds) if (s[0] == id) return true;
    return false;
}
} // namespace

int main() {
    {   // The excerpt container round-trips and rejects corruption.
        const auto e = authored();
        auto cooked = cook_exe_excerpt(e);
        const auto back = load_exe_excerpt(cooked);
        assert(back.ranges.size() == e.ranges.size() && *back.at(kOptionMessage8008b69c) == 0x99);
        assert(back.u32(kMsgOptionTable8008d0d4 + 8) == 0x800C1B64);
        cooked[20] ^= 1;
        bool threw = false; try { (void)load_exe_excerpt(cooked); } catch (const std::exception&) { threw = true; }
        assert(threw);
    }
    MessageWindows w(authored());
    w.clear_80053a30();
    assert(w.open_8005382c(0, kOptionMessage8008b69c, 255));
    assert((w.flags_80098a58 & 0x88000000u) == 0x88000000u);
    auto f = w.tick_80053b40(0, 0);
    // Frame 1: the whole page runs at speed 0 up to the 0x97 choice.
    assert(f.unported_opcodes == 0 && f.sounds.empty());
    const auto& s0 = w.slot(0);
    assert(w.byte(s0.read) == 0x97 && s0.rows == 2 && s0.cursor == 0 && s0.timer == 4);
    assert(s0.x == 28 && s0.y == 62 && s0.width == 5 && s0.height == 3 && s0.box[2] == 4 + 5 * 12 && s0.box[5] == 4 + 3 * 12);
    assert(w.slot(1).flags & 0x80000000u);                       // 0x99 01 05 opened slot 1
    assert((w.slot(2).flags & 0x80000000u) && w.slot(2).page == 0x8008B8C0);   // row 0 help, value 0 -> id 6
    assert(w.byte(kOptionMessage8008b69c + 19 + 7) == 0x60);     // cursor glyph on row 0
    assert(w.byte(kOptionMessage8008b69c + 48 + 7) == 0xED);
    uint8_t lit = 0, dim = 0;
    assert(glyphs_with_u(f, 0x17 % 21 * 12, 0x17 / 21 * 12, &lit) == 1 && lit == 128);   // value 0 lit
    assert(glyphs_with_u(f, 0x18 % 21 * 12, 0x18 / 21 * 12, &dim) == 1 && dim == 64);    // value 1 dim
    assert(glyphs_with_u(f, 0x60 % 21 * 12, 0x60 / 21 * 12) == 1);                       // cursor drawn
    // Frame packets: the font E1 (OT 31) last; frame G4 edges + F4 at OT 3.
    assert(f.packets.back().bucket == 31 && f.packets.back().words[0] == (0xE1000000u | kMsgFontTpage));
    size_t g4 = 0, f4 = 0;
    for (const auto& p : f.packets) { g4 += (p.words[0] >> 24) == 0x38; f4 += (p.words[0] >> 24) == 0x2A; }
    assert(g4 == 3 * 4 && f4 == 3 * 2);   // three windows (menu, title, help)

    // The 0x97 timer (5 updates) gates only the choice: the 0xAD row draw
    // reads the pad every update. Cross during the timer toggles, no choice.
    f = w.tick_80053b40(0, 0x4000);
    assert(has_sound(f, 130) && w.settings().turn_800c1b64 == 1 && !(w.flags_80098a58 & 0x40000000u));
    f = w.tick_80053b40(0, 0x4000);
    assert(w.settings().turn_800c1b64 == 0);
    for (int i = 0; i < 3; ++i) { f = w.tick_80053b40(0, 0); assert(f.sounds.empty()); }
    // Right on row 0: value 1, sound 130; the help message follows the value.
    f = w.tick_80053b40(0, 0x20);
    assert(has_sound(f, 130) && w.settings().turn_800c1b64 == 1 && w.slot(2).page == 0x8008B8E0);
    f = w.tick_80053b40(0, 0x20);
    assert(!has_sound(f, 130));                                      // already 1: silent
    f = w.tick_80053b40(0, 0x80);
    assert(has_sound(f, 130) && w.settings().turn_800c1b64 == 0);   // Left -> 0
    f = w.tick_80053b40(0, 0x4000);
    assert(has_sound(f, 130) && w.settings().turn_800c1b64 == 1 && !(w.flags_80098a58 & 0x40000000u));   // Cross toggles
    // Down: cursor to Close, sound 129; held Down auto-repeats after 13 updates.
    f = w.tick_80053b40(0x40, 0x40);
    assert(has_sound(f, 129) && w.slot(0).cursor == 1);
    // Row 0 set 0x800000 (one cursor row per draw pass): row 1 shows the
    // cursor from the next update.
    assert(w.byte(kOptionMessage8008b69c + 48 + 7) == 0xED);
    f = w.tick_80053b40(0, 0);
    assert(w.byte(kOptionMessage8008b69c + 48 + 7) == 0x60 && w.byte(kOptionMessage8008b69c + 19 + 7) == 0xED);
    assert(w.slot(2).page == 0x8008B900);   // Close help, id 8
    // Cross on Close: sound 130, choice 1 posted to 0x80098A58, cursor byte cleared.
    f = w.tick_80053b40(0, 0x4000);
    assert(has_sound(f, 130) && (w.flags_80098a58 & 0x400000FFu) == 0x40000001u);
    assert(w.byte(kOptionMessage8008b69c + 48 + 7) == 0xED && w.byte(w.slot(0).read) == 0x95);
    f = w.tick_80053b40(0, 0);
    assert(!(w.flags_80098a58 & 0x40000000u));   // cleared by the next tick (mask 0x3043FFFF)
    assert(w.settings().turn_800c1b64 == 1);

    {   // Triangle cancels (type 0x80): sound 131 and the last row is chosen.
        MessageWindows t(authored());
        t.open_8005382c(0, kOptionMessage8008b69c, 255);
        for (int i = 0; i < 6; ++i) (void)t.tick_80053b40(0, 0);
        const auto c = t.tick_80053b40(0, 0x1000);
        assert(has_sound(c, 131) && (t.flags_80098a58 & 0x400000FFu) == 0x40000001u);
        // Start alone does nothing in the front end (phase 3), cancels in phase 4.
        MessageWindows u(authored());
        u.open_8005382c(0, kOptionMessage8008b69c, 255);
        for (int i = 0; i < 6; ++i) (void)u.tick_80053b40(0, 0);
        assert(u.tick_80053b40(0, 0x0008).sounds.empty());
        u.phase_800c1b10 = 4;
        assert(u.tick_80053b40(0, 0x0008).sounds.empty());   // 0xAD rows: only Triangle off the last row
        (void)u.tick_80053b40(0, 0x40);
        assert(has_sound(u.tick_80053b40(0, 0x0008), 131) && (u.flags_80098a58 & 0x40000000u));
    }
    {   // Held Up auto-repeat: 13 updates of hold, then a move every second update.
        MessageWindows r(authored());
        r.open_8005382c(0, kOptionMessage8008b69c, 255);
        for (int i = 0; i < 6; ++i) (void)r.tick_80053b40(0, 0);
        int moves = 0;
        for (int i = 0; i < 20; ++i) moves += has_sound(r.tick_80053b40(0x10, 0), 129);
        assert(moves == 4);   // moves at hold updates 14, 16, 18, 20
    }
    {   // Settings round-trip through the original addresses.
        OptionSettings s; s.free_look_800c1b65 = 1; s.map_800c1b69 = 1;
        w.set_settings(s);
        assert(w.byte(0x800C1B65) == 1 && w.byte(0x800C1B69) == 1 && w.settings().turn_800c1b64 == 0);
    }
    std::cout << "mml_message_window: ok\n";
    return 0;
}
