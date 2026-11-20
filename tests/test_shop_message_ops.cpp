// Shop message ops on the message windows (docs/research_01516/SHOP_MENU.md), authored data:
// 0xBA (zenny through the number printer 0x8005BF10), 0xB9 (open on the last choice) and 0xBB
// (open on a setting byte) from a stage-style bank mapped next to the excerpt.
#include "legends3ds/mml_message_window.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace l3;

namespace {
void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int k = 0; k < 4; ++k) b[o + size_t(k)] = uint8_t(v >> (8 * k)); }
void put16(std::vector<uint8_t>& b, size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); }
ExeExcerpt authored() {
    ExeExcerpt e;
    ExeExcerpt::Range widths; widths.addr = kMsgGlyphWidths8008ae7c; widths.bytes.assign(132, 8);
    ExeExcerpt::Range decor; decor.addr = kMsgDecorParam8008af18; decor.bytes = {0, 0xFF, 1, 0, 1, 0xFF, 0xFF, 0xFF, 1, 0, 0, 0};
    ExeExcerpt::Range number; number.addr = kMsgNumberDivisors8008af24; number.bytes.assign(0x24, 0);
    const uint32_t div[6] = {100000, 10000, 1000, 100, 10, 1};
    for (int k = 0; k < 6; ++k) put32(number.bytes, size_t(4 * k), div[k]);
    number.bytes[0x18] = 0xED;                                   // pad glyph for kind 0
    ExeExcerpt::Range digits; digits.addr = kMsgDigitGlyphs8008d028; digits.bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ExeExcerpt::Range colors; colors.addr = kMsgClutTable80097f30; colors.bytes.assign(0x60, 0);
    ExeExcerpt::Range table; table.addr = kMsgTable8008cacc; table.bytes.assign(4 * 16, 0);
    ExeExcerpt::Range options; options.addr = kMsgOptionTable8008d0d4; options.bytes.assign(4 * 32, 0);
    put32(options.bytes, 4 * 20, 0x800C1B68);                    // setting 20 -> a byte the windows map
    ExeExcerpt::Range option_msg; option_msg.addr = kOptionMessage8008b69c; option_msg.bytes = {0x95};
    e.ranges = {widths, decor, number, digits, colors, table, options, option_msg};
    return e;
}
} // namespace

int main() {
    MessageWindows w(authored());
    // A two-block bank at 0x80156000: block 0 shows the money and opens block 1 in slot 1
    // when the last choice byte is 0; block 1 opens block 1 again in slot 2 when setting 20 is 1.
    std::vector<uint8_t> bank(0x100, 0);
    const std::vector<uint8_t> b0 = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x10, 0x00, 0x10, 0x00, 0x08, 0x01, 0x93, 0x02, 0x04,
        0xBA, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0xB9, 0x00, 0x01, 0x01, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
    const std::vector<uint8_t> b1 = {0xA1, 0x8A, 0x00, 0x00, 0xBB, 0x14, 0x01, 0x02, 0x01, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
    put16(bank, 0, 4); put16(bank, 2, uint16_t(4 + b0.size()));
    std::memcpy(bank.data() + 4, b0.data(), b0.size());
    std::memcpy(bank.data() + 4 + b0.size(), b1.data(), b1.size());
    w.map_range(0x80156000u, bank);
    w.zenny_800c1b2c = 1234;
    w.flags_80098a58 = 0;   // last choice 0
    assert(w.open_8005382c(0, 0x80156000u, 0));
    for (int i = 0; i < 4; ++i) (void)w.tick_80053b40(0, 0);
    // 1234 -> "  1234" (two pads) then the fixed 0: glyphs ED ED 2 3 4 5 1.
    const uint32_t at = 0x80156004u + 15u;
    const uint8_t want[7] = {0xED, 0xED, 2, 3, 4, 5, 1};
    for (uint32_t k = 0; k < 7; ++k) assert(w.byte(at + k) == want[k]);
    assert(w.slot(0).number_40 == 1234);
    assert(w.slot(1).flags & 0x80000000u);              // 0xB9 opened block 1 in slot 1
    assert(!(w.slot(2).flags & 0x80000000u));           // setting 20 is 0: 0xBB left slot 2 shut
    std::cout << "shop_message_ops: ok\n";
    return 0;
}
