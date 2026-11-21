// Shop message ops on the message windows (docs/research_01516/SHOP_MENU.md), authored data:
// 0xBA (zenny through the number printer 0x8005BF10), 0xB9 (open on the last choice) and 0xBB
// (open on a setting byte) from a stage-style bank mapped next to the excerpt.
#include "legends3ds/mml_message_window.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <tuple>

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
    put32(options.bytes, 4 * 20, 0x800C1B68);
    put32(options.bytes, 4 * 14, 0x80098B6C);                    // setting 14 -> gp+0x1308 (test wiring)                    // setting 20 -> a byte the windows map
    ExeExcerpt::Range prices; prices.addr = kShopPrices8008fd3c; prices.bytes.assign(0x128, 0); put16(prices.bytes, 2 * 5, 100);
    ExeExcerpt::Range option_msg; option_msg.addr = kOptionMessage8008b69c; option_msg.bytes = {0x95};
    e.ranges = {widths, decor, number, digits, colors, table, options, option_msg, prices};
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
    {   // 0x90 calls a block on both passes; 0x91 returns.
        MessageWindows v(authored());
        std::vector<uint8_t> bk(0x80, 0);
        const std::vector<uint8_t> c0 = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x10, 0x00, 0x10, 0x00, 0x08, 0x01, 0x93, 0x02, 0x04,
            0x90, 0x01, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
        const std::vector<uint8_t> c1 = {0xBA, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x91, 0x00, 0x00};
        put16(bk, 0, 4); put16(bk, 2, uint16_t(4 + c0.size()));
        std::memcpy(bk.data() + 4, c0.data(), c0.size()); std::memcpy(bk.data() + 4 + c0.size(), c1.data(), c1.size());
        v.map_range(0x80157000u, bk); v.zenny_800c1b2c = 50;
        assert(v.open_8005382c(0, 0x80157000u, 0));
        for (int i = 0; i < 4; ++i) (void)v.tick_80053b40(0, 0);
        const uint32_t at2 = 0x80157004u + uint32_t(c0.size()) + 1u;
        const uint8_t want2[7] = {0xED, 0xED, 0xED, 0xED, 6, 1, 1};   // 50 -> "    50" + fixed 0
        for (uint32_t k = 0; k < 7; ++k) assert(v.byte(at2 + k) == want2[k]);
        assert(v.slot(0).exec_depth == 0 && (v.slot(0).flags & 0x80000000u));
    }
    {   // 0xBC picks the cursor row's item and checks the money; 0xBD buys or sells it.
        std::set<uint16_t> flags;
        auto run = [&](uint32_t money, bool sell) {
            MessageWindows v(authored());
            v.story_flag = [&](uint16_t f) { return flags.count(f) != 0; };
            v.set_story_flag = [&](uint16_t f, bool on) { if (on) flags.insert(f); else flags.erase(f); };
            if (sell) flags.insert(0x2EE); else flags.erase(0x2EE);
            v.list_800be2f8[0] = 5; v.zenny_800c1b2c = money;
            std::vector<uint8_t> bk(0x40, 0);
            const std::vector<uint8_t> blk = {0xA1, 0x8A, 0x00, 0x00, 0xBC, 0x00, 0xBB, 0x0E, 0x00, 0x01, 0x01, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
            const std::vector<uint8_t> buy = {0xBD, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
            put16(bk, 0, 4); put16(bk, 2, uint16_t(4 + blk.size()));
            std::memcpy(bk.data() + 4, blk.data(), blk.size()); std::memcpy(bk.data() + 4 + blk.size(), buy.data(), buy.size());
            v.map_range(0x80158000u, bk);
            assert(v.open_8005382c(0, 0x80158000u, 0));
            MessageWindowFrame last; int16_t bought = -1, sold = -1;
            for (int i = 0; i < 4; ++i) { last = v.tick_80053b40(0, 0); if (last.shop_bought >= 0) bought = last.shop_bought; if (last.shop_sold >= 0) sold = last.shop_sold; }
            return std::tuple<uint32_t, uint8_t, int16_t, int16_t, uint8_t>(v.zenny_800c1b2c, v.shop_short_gp1308, bought, sold, v.shop_item_gp10cc);
        };
        auto [m1, short1, b1, s1, item1] = run(50, false);    // short: 0xBB (setting 14 = gp+0x1308 == 0) does not open the buy block
        assert(item1 == 5 && short1 == 1 && m1 == 50 && b1 == -1 && s1 == -1 && !flags.count(0x505));
        auto [m2, short2, b2, s2, item2] = run(500, false);   // buys: 400 left, item 5 owned
        assert(short2 == 0 && m2 == 400 && b2 == 5 && flags.count(0x505));
        auto [m3, short3, b3, s3, item3] = run(10, true);     // sells for a quarter: +25, no longer owned
        assert(short3 == 0 && m3 == 35 && s3 == 5 && !flags.count(0x505));
        (void)item2; (void)item3; (void)s2; (void)b3;
    }
    {   // 0xB8: the item list (kind 1, flags from 0x500) and the window by row count.
        auto ex = authored();
        for (auto& r : ex.ranges) if (r.addr == kMsgClutTable80097f30) {
            put16(r.bytes, 0x34, 0x500); put16(r.bytes, 0x36, 0x480);   // 0x80097F64: parts 0x500, items 0x480
            r.bytes[0x38] = 8; r.bytes[0x39] = 8;                        // 0x80097F68: eight rows each
        }
        MessageWindows v(std::move(ex));
        v.story_flag = [](uint16_t) { return false; };
        v.shop_kind_gp0fec = 1; v.life_max_800b5260 = 0xA0; v.canteen_800c1b8d = 0;
        // Items 0x480..0x487: owned rows 0, 2, 3 (0x480 byte bits); row 2 is "in stock" (0x400 byte) so it hides.
        v.flags_800be378[0x480 >> 3] = 0x80 | 0x20 | 0x10;   // rows 0, 2, 3 owned
        v.flags_800be378[0x400 >> 3] = 0x20;                  // row 2's second flag
        std::vector<uint8_t> bk(0x80, 0);
        const std::vector<uint8_t> blk = {0xA1, 0xB8, 0x01, 0x05, 0x0B, 0x0C, 0x0D, 0x0E, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
        put16(bk, 0, 0x20);   // 16 blocks; block k > 0 = HALT at 0x20 + 2k
        for (int k = 1; k < 16; ++k) { put16(bk, size_t(2 * k), uint16_t(0x20 + 0x30 + 2 * k)); bk[size_t(0x50 + 2 * k)] = 0x95; }
        std::memcpy(bk.data() + 0x20, blk.data(), blk.size());
        v.map_range(0x80159000u, bk);
        assert(v.open_8005382c(0, 0x80159000u, 0));
        (void)v.tick_80053b40(0, 0);
        // Rows whose (second flag | ~owned) bit is clear go straight in: 0 and 3. Row 2 (second flag)
        // and the unowned rows fall to the item checks: 3 would need max life != 0xA0, 5 a canteen.
        assert(v.list_count_gp4420 == 2 && v.list_800be2f8[0] == 0 && v.list_800be2f8[1] == 3 && v.list_800be2f8[2] == 255);
        assert(v.slot(0).index == 0x0D);   // two rows -> the byte at +5 (0x0D)
    }
    {   // 0xB7 rows: the name index per row, the row's call target, Down at the last row scrolls.
        MessageWindows v(authored());
        v.story_flag = [](uint16_t) { return false; };
        for (uint8_t i = 0; i < 7; ++i) v.list_800be2f8[i] = uint8_t(10 + i);
        for (uint32_t i = 7; i < 128; ++i) v.list_800be2f8[i] = 255;
        v.list_count_gp4420 = 7; v.list_scroll_gp4408 = 0; v.shop_kind_gp0fec = 1; v.row_block_gp0f94 = 1;
        std::vector<uint8_t> bk(0x60, 0);
        const std::vector<uint8_t> blk = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x10, 0x00, 0x10, 0x00, 0x08, 0x02, 0x93, 0x02, 0x04,
            0xB7, 0x00, 0x01, 0xED, 0x90, 0x00, 0x86, 0xB7, 0x00, 0x01, 0xED, 0x90, 0x00,
            0x97, 0x40, 0xFF, 0xFF, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
        const std::vector<uint8_t> row = {0x91, 0x00, 0x00};
        put16(bk, 0, 4); put16(bk, 2, uint16_t(4 + blk.size()));
        std::memcpy(bk.data() + 4, blk.data(), blk.size()); std::memcpy(bk.data() + 4 + blk.size(), row.data(), row.size());
        v.map_range(0x8015A000u, bk);
        assert(v.open_8005382c(0, 0x8015A000u, 0));
        for (int i = 0; i < 8; ++i) (void)v.tick_80053b40(0, 0);
        assert(v.byte(0x8015A004u + 14 + 5) == 1 && v.byte(0x8015A004u + 21 + 5) == 1);   // call targets patched
        (void)v.tick_80053b40(0x40, 0x40);                                                 // Down: row 0 -> 1
        for (int i = 0; i < 2; ++i) (void)v.tick_80053b40(0, 0);
        assert(v.slot(0).cursor == 1 && v.list_scroll_gp4408 == 0);
        (void)v.tick_80053b40(0x40, 0x40);                                                 // Down at the last row: scroll
        assert(v.slot(0).cursor == 1 && v.list_scroll_gp4408 == 1);
        (void)v.tick_80053b40(0, 0);
        assert(v.name_index_gp4044 == uint8_t(10 + 1 + 1 + 0x20));                         // last drawn row: scroll 1 + row 1, items +0x20
    }
    {   // 0xC2 / 0xC3 / 0xC4: the canteen refill.
        auto run = [&](uint8_t cap, uint8_t fill, uint32_t money, int16_t w70) {
            MessageWindows v(authored());
            v.canteen_800c1b8d = cap; v.canteen_fill_800c1b8c = fill; v.zenny_800c1b2c = money; v.word_800c1b70 = w70;
            std::vector<uint8_t> bk(0x60, 0);
            // block 0: C2 full->1 / else continue; C3 pay->2, short->3, free->4. Blocks 1, 3 halt; 2 = C4 00 halt; 4 = C4 01 halt.
            const std::vector<uint8_t> b0 = {0xC2, 0x01, 0xFF, 0xC3, 0x02, 0x03, 0x04, 0x95, 0x00, 0x00};
            put16(bk, 0, 10); size_t at = 10; std::memcpy(bk.data() + at, b0.data(), b0.size()); at += b0.size();
            const std::vector<std::vector<uint8_t>> rest = {{0x95, 0, 0}, {0xC4, 0x00, 0x95, 0, 0}, {0x95, 0, 0}, {0xC4, 0x01, 0x95, 0, 0}};
            for (size_t k = 0; k < rest.size(); ++k) { put16(bk, 2 * (k + 1), uint16_t(at)); std::memcpy(bk.data() + at, rest[k].data(), rest[k].size()); at += rest[k].size(); }
            v.map_range(0x8015B000u, bk);
            assert(v.open_8005382c(0, 0x8015B000u, 0));
            for (int i = 0; i < 3; ++i) (void)v.tick_80053b40(0, 0);
            return std::tuple<uint16_t, uint32_t, uint8_t>(v.slot(0).index, v.zenny_800c1b2c, v.canteen_fill_800c1b8c);
        };
        auto [i1, m1, f1] = run(5, 5, 100, 0); assert(i1 == 1 && m1 == 100 && f1 == 5);   // already full
        auto [i2, m2, f2] = run(5, 2, 100, 0); assert(i2 == 2 && m2 == 70 && f2 == 5);    // pays 30
        auto [i3, m3, f3] = run(5, 2, 10, 0);  assert(i3 == 3 && m3 == 10 && f3 == 2);    // not enough money
        auto [i4, m4, f4] = run(5, 2, 10, 0x80); assert(i4 == 4 && m4 == 0 && f4 == 5);   // the free refill takes what is left
    }
    {   // 0xC5 draws the refill cost with pad kind 1; 0xC0 the row price (items table at 0x8008FD7C).
        MessageWindows v(authored());
        v.canteen_800c1b8d = 5; v.canteen_fill_800c1b8c = 2;
        std::vector<uint8_t> bk(0x40, 0);
        const std::vector<uint8_t> b0 = {0xA1, 0x8A, 0x00, 0x00, 0x8C, 0x10, 0x00, 0x10, 0x00, 0x08, 0x01, 0x93, 0x02, 0x04,
            0xC5, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x8A, 0x01, 0x00, 0x95, 0x00, 0x00};
        put16(bk, 0, 2); std::memcpy(bk.data() + 2, b0.data(), b0.size());
        v.map_range(0x8015C000u, bk);
        assert(v.open_8005382c(0, 0x8015C000u, 0));
        for (int i = 0; i < 3; ++i) (void)v.tick_80053b40(0, 0);
        const uint8_t want[7] = {0x00, 0x00, 0x00, 0x00, 4, 1, 1};   // 30 -> pad kind 1 (authored 0) x4, '3','0', fixed '0'
        for (uint32_t k = 0; k < 7; ++k) assert(v.byte(0x8015C002u + 15u + k) == want[k]);
    }
    std::cout << "shop_message_ops: ok\n";
    return 0;
}
