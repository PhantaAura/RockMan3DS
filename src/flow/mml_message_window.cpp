#include "legends3ds/mml_message_window.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {

uint8_t* ExeExcerpt::at(uint32_t addr) {
    for (auto& r : ranges)
        if (addr >= r.addr && addr - r.addr < r.bytes.size()) return r.bytes.data() + (addr - r.addr);
    return nullptr;
}
const uint8_t* ExeExcerpt::at(uint32_t addr) const { return const_cast<ExeExcerpt*>(this)->at(addr); }
uint32_t ExeExcerpt::u32(uint32_t addr) const {
    uint32_t v = 0;
    for (unsigned k = 0; k < 4; ++k) {
        const uint8_t* p = at(addr + k);
        if (!p) throw std::runtime_error("EXE excerpt: unmapped address");
        v |= uint32_t(*p) << (8 * k);
    }
    return v;
}

ExeExcerpt load_exe_excerpt(const std::vector<uint8_t>& b) {
    if (b.size() < 16 || std::string(b.begin(), b.begin() + 8) != "L3EX0001") throw std::runtime_error("not an L3EX0001 excerpt");
    if (crc32(b.data(), b.size() - 4) != rd_u32(b.data() + b.size() - 4)) throw std::runtime_error("L3EX CRC mismatch");
    ExeExcerpt e;
    const uint32_t count = rd_u32(b.data() + 8);
    size_t p = 12;
    for (uint32_t i = 0; i < count; ++i) {
        if (p + 8 > b.size() - 4) throw std::runtime_error("L3EX truncated");
        ExeExcerpt::Range r; r.addr = rd_u32(b.data() + p); const uint32_t n = rd_u32(b.data() + p + 4); p += 8;
        if (n > b.size() - 4 - p) throw std::runtime_error("L3EX range truncated");
        r.bytes.assign(b.begin() + std::ptrdiff_t(p), b.begin() + std::ptrdiff_t(p + n)); p += n;
        e.ranges.push_back(std::move(r));
    }
    if (p != b.size() - 4) throw std::runtime_error("L3EX trailing bytes");
    return e;
}

std::vector<uint8_t> cook_exe_excerpt(const ExeExcerpt& e) {
    std::vector<uint8_t> o{'L', '3', 'E', 'X', '0', '0', '0', '1'};
    wr_u32(o, uint32_t(e.ranges.size()));
    for (const auto& r : e.ranges) { wr_u32(o, r.addr); wr_u32(o, uint32_t(r.bytes.size())); o.insert(o.end(), r.bytes.begin(), r.bytes.end()); }
    wr_u32(o, crc32(o.data(), o.size()));
    return o;
}

namespace {
constexpr uint32_t kSlotBit = 0x08000000u;
uint32_t xy(int x, int y) { return uint32_t(uint16_t(int16_t(x))) | (uint32_t(uint16_t(int16_t(y))) << 16); }
uint32_t grey(uint8_t g) { return uint32_t(g) * 0x010101u; }
} // namespace

MessageWindows::MessageWindows(ExeExcerpt exe) : exe_(std::move(exe)) {
    for (uint32_t a : {kMsgGlyphWidths8008ae7c, kMsgClutTable80097f30, kMsgTable8008cacc, kMsgOptionTable8008d0d4,
                       kMsgDecorParam8008af18, kMsgFrameColors80097f6c, kOptionMessage8008b69c})
        if (!exe_.at(a)) throw std::runtime_error("message windows: EXE excerpt lacks a required table");
    clear_80053a30();
}

uint8_t& MessageWindows::mem(uint32_t addr) {
    if (addr >= 0x800C1B60u && addr < 0x800C1B70u) return ram_800c1b60[addr - 0x800C1B60u];
    if (addr == 0x8009879Cu) return dummy_8009879c;
    if (addr == 0x800989D4u) return list_max_scroll_gp4464;          // gp+4464 (settings entry 26)
    if (addr == 0x8009899Cu) return list_scroll_gp4408;              // gp+4408 (settings entry 15)
    if (addr == 0x80098930u) return shop_item_gp10cc;                // gp+0x10CC
    if (addr == 0x80098850u) return shop_kind_gp0fec;                // gp+0xFEC
    if (addr == 0x800987F8u) return row_block_gp0f94;                // gp+0xF94
    if (addr == 0x80098A64u) return cancel_slot_gp1200;              // gp+0x1200
    if (addr == 0x80098A65u) return cancel_block_gp1201;             // gp+0x1201
    if (addr == 0x800988F4u) return shop_scroll0_gp1090;             // gp+0x1090
    if (addr == 0x80098B6Cu) return shop_short_gp1308;               // gp+0x1308
    if (addr >= 0x800BB6B8u && addr < 0x800BB6B8u + 5u * 196u) {   // slot +121..+123: remembered cursors
        const uint32_t o = (addr - 0x800BB6B8u) % 196u, n = (addr - 0x800BB6B8u) / 196u;
        if (o >= 121u && o <= 123u) return slots_[n].remembered[o - 121u];
    }
    if (addr >= 0x800BE2F8u && addr < 0x800BE378u) return list_800be2f8[addr - 0x800BE2F8u];
    if (addr >= 0x800BE420u && addr < 0x800BE428u) return owned_items_800be420[addr - 0x800BE420u];
    if (addr >= 0x800B5600u && addr < 0x800B5604u) return equipped_parts_800b5600[addr - 0x800B5600u];
    if (addr >= 0x800B5604u && addr < 0x800B5644u) return owned_parts_800b5604[addr - 0x800B5604u];
    if (uint8_t* p = exe_.at(addr)) return *p;
    throw std::runtime_error("message windows: read outside the EXE excerpt");
}
uint8_t MessageWindows::byte(uint32_t addr) const { return const_cast<MessageWindows*>(this)->mem(addr); }
uint8_t& MessageWindows::setting(uint8_t i) { return mem(exe_.u32(kMsgOptionTable8008d0d4 + 4u * i)); }

OptionSettings MessageWindows::settings() const {
    OptionSettings s;
    s.turn_800c1b64 = ram_800c1b60[4]; s.free_look_800c1b65 = ram_800c1b60[5]; s.lock_on_800c1b66 = ram_800c1b60[6];
    s.sound_800c1b68 = ram_800c1b60[8]; s.map_800c1b69 = ram_800c1b60[9];
    return s;
}
void MessageWindows::set_settings(const OptionSettings& s) {
    ram_800c1b60[4] = s.turn_800c1b64; ram_800c1b60[5] = s.free_look_800c1b65; ram_800c1b60[6] = s.lock_on_800c1b66;
    ram_800c1b60[8] = s.sound_800c1b68; ram_800c1b60[9] = s.map_800c1b69;
}

void MessageWindows::clear_80053a30() {
    // The voice-stop call 0x8001D494 (slot bit 0x40000) has no port here.
    for (auto& s : slots_) { s.flags = 0; s.index = 255; }
}

bool MessageWindows::open_8005382c(int n, uint32_t data, uint8_t index) {
    if (n < 0 || n > 4) return false;
    if (flags_80098a58 & 0x00400000u) return false;   // (0x800A8824 is 0 here)
    MessageSlot& s = slots_[size_t(n)];
    if (n != 4) { slots_[4].flags = 0; slots_[4].index = 255; }
    s.flags = 0x80000000u;
    s.speed = 1; s.color = 0; s.index = index; s.timer = 0;
    s.x = s.y = 0;
    if (index != 255) { s.index_base = data; s.page = s.read = data + u16(data + 2u * index); }
    else { s.page = s.read = data; s.index_base = 0; }
    s.ot_layer = 3; s.cursor = 0; s.rows = 0; s.row_draw = 0;
    s.decor_state = 0; s.width = 0; s.height = 0; s.lines = 0; s.brightness = 128;
    s.default_color = s.color; s.scroll = 0; s.repeat = 0; s.draw = s.read;
    flags_80098a58 |= 0x80000000u | (kSlotBit >> n);
    return true;
}

// Execute-table handlers. Returns the handler's v0 (nonzero: keep executing).
bool MessageWindows::exec(MessageSlot& s, uint8_t op, MessageWindowFrame& out, bool& unported) {
    const uint32_t r = s.read;
    switch (op) {
    case 0x86: ++s.lines; s.read += 1; return true;                         // 0x8005457C
    case 0x89: case 0xB0: s.read += 2; return true;                         // 0x800543C8
    case 0x8A: { const uint16_t v = u16(r + 1); s.speed = uint8_t(v); s.timer = int16_t(v & 0xFF); s.read += 3; return true; }   // 0x800546B0
    case 0x8C: s.x = int16_t(u16(r + 1)); s.y = int16_t(u16(r + 3)); s.width = mem(r + 5); s.height = mem(r + 6); s.read += 7; return true;   // 0x80054798
    case 0x93: {                                                             // 0x80054ADC
        if (s.flags & 0x02000000u) { s.flags &= ~0x02000000u; s.decor_state = 0; }
        s.decor = mem(r + 1);
        if (!decor_exec(s)) { unported = true; return false; }
        return false;
    }
    case 0x95: return false;                                                 // 0x80054BAC: halt
    case 0xEA: case 0xE8:
    case 0x97: {                                                             // 0x80054E54
        if (!(s.flags & 0x01000000u)) {
            s.flags = (s.flags | 0x01000000u) & 0xFFFF7FFFu;
            s.timer = 5;
            const uint8_t t = mem(r + 1);
            const int which = (t & 0x40) ? 0 : (t & 0x20) ? 1 : (t & 0x10) ? 2 : -1;
            if (t & 0x80) s.flags |= 0x8000u;
            s.cursor = which >= 0 ? int8_t(s.remembered[size_t(which)]) : int8_t(t & 63);
        }
        s.flags = (s.flags | 0x10000000u) & 0xFFBFFFFFu;
        flags_80098a58 |= 0x00040000u;
        if (s.timer > 0) { --s.timer; s.brightness = 128; return false; }
        const bool last = s.cursor == int8_t(s.rows - 1);
        if ((s.flags & 0x4000u) && !last && !(s.pressed & 0x1000u)) { s.brightness = 128; return false; }
        const bool cross = s.pressed & 0x4000u, tri = s.pressed & 0x1000u, start = s.pressed & 0x0008u;
        const bool cancel_tri = tri && (s.flags & 0x8000u), cancel_start = start && phase_800c1b10 == 4;
        if (cancel_tri || cancel_start) {
            sound(out, 131, 1); s.cursor = int8_t(s.rows - 1);
        } else if (cross) sound(out, 130, 1);
        else { s.brightness = 128; return false; }
        flags_80098a58 &= ~0xFFu;
        // 0x800550AC: 0xEA (Sort) rows 1..4 sort the owned parts by attack /
        // energy / range / rapid (0x80060F84 / 61064 / 61144 / 61224) and stay
        // on the menu (brightness 128, no pick).
        if (mem(r) == 0xEA && s.cursor >= 1 && s.cursor <= 4) {
            sort_parts_80060f84(s.cursor - 1);
            s.brightness = 128;
            return false;
        }
        // 0x80055150: 0xE8 picks the list entry 0x800BE2F8[cursor]; others the cursor.
        flags_80098a58 |= 0x40000000u | (mem(r) == 0xE8 ? list_800be2f8[size_t(uint8_t(s.cursor)) & 127] : uint8_t(s.cursor));
        if (s.cursor_byte) mem(s.cursor_byte) = 0xED;
        s.timer = 5;
        s.flags = (s.flags | 0x00400000u) & 0xEEFF97FFu;
        const uint8_t t = mem(r + 1), target = mem(r + 2u + uint32_t(uint8_t(s.cursor)));
        if (t & 0x40) s.remembered[0] = uint8_t(s.cursor);
        if (t & 0x20) s.remembered[1] = uint8_t(s.cursor);
        if (t & 0x10) s.remembered[2] = uint8_t(s.cursor);
        if (target == 255) { s.read += uint32_t(s.rows) + 2u; s.brightness = 128; return true; }
        s.index = target;
        s.page = s.read = s.index_base ? s.index_base + u16(s.index_base + 2u * target) : exe_.u32(kMsgTable8008cacc + 4u * target);
        s.color = 0; s.rows = 0; s.brightness = 128;
        return true;
    }
    case 0x99: {                                                             // 0x80055344
        const int8_t n = int8_t(mem(r + 1)); const uint8_t id = mem(r + 2);
        s.read += 3;
        if (s.index_base) open_8005382c(n, s.index_base, id);
        else open_8005382c(n, exe_.u32(kMsgTable8008cacc + 4u * id), 255);
        return true;
    }
    case 0xA0: case 0xAE: case 0xD6: s.read += 3; return true;               // 0x800543E0
    case 0xA1: s.flags |= 0x20000000u; s.read += 1; return true;             // 0x8005563C
    case 0xAD: s.repeat = 0; ++s.rows; s.flags |= 0x00404000u; s.read += 7; return true;   // 0x80055CC4
    case 0x96: s.repeat = 0; ++s.rows; s.flags |= 0x00400800u; s.read += 3; return true;   // 0x80054BB4
    case 0xAF: ++s.lines; s.read += 2; return true;                          // 0x80056128
    case 0xDB: s.read += 5; return true;                                     // 0x800543F8
    case 0xED: s.read += 1; return true;                                     // 0x800543B0
    case 0x90: {                                                             // 0x8005497C: call block arg (push read + 2)
        if (s.exec_depth >= s.exec_stack.size()) { unported = true; return false; }
        s.exec_stack[s.exec_depth++] = r + 2;
        const uint8_t id = mem(r + 1);
        s.read = s.index_base ? s.index_base + u16(s.index_base + 2u * id) : exe_.u32(kMsgTable8008cacc + 4u * id);
        return true;
    }
    case 0x91:                                                               // 0x80054A84: return
        if (!s.exec_depth) { unported = true; return false; }
        s.read = s.exec_stack[--s.exec_depth]; return true;
    case 0xAC: setting(mem(r + 1)) = mem(r + 2); s.read += 3; return true;   // 0x80055C80
    case 0xB9: {                                                             // 0x800570B0: open on the last choice
        const uint8_t want = mem(r + 1); const int8_t n = int8_t(mem(r + 2)); const uint8_t id = mem(r + 3);
        s.read += 4;
        if (uint8_t(flags_80098a58) == want) {
            if (s.index_base) open_8005382c(n, s.index_base, id);
            else open_8005382c(n, exe_.u32(kMsgTable8008cacc + 4u * id), 255);
        }
        return true;
    }
    case 0xBB: {                                                             // 0x80057184: open on a setting byte
        const uint8_t idx = mem(r + 1), want = mem(r + 2); const int8_t n = int8_t(mem(r + 3)); const uint8_t id = mem(r + 4);
        s.read += 5;
        if (setting(idx) == want) {
            if (s.index_base) open_8005382c(n, s.index_base, id);
            else open_8005382c(n, exe_.u32(kMsgTable8008cacc + 4u * id), 255);
        }
        return true;
    }
    case 0xBA: s.number_40 = zenny_800c1b2c; s.read += 1; return true;      // 0x80057124: +0x40 = zenny
    case 0xBC: {                                                             // 0x8005721C: the cursor row's item and its price
        shop_item_gp10cc = uint8_t(list_800be2f8[size_t((list_scroll_gp4408 + uint32_t(int8_t(s.cursor))) & 0x7Fu)] + mem(r + 1));
        const bool sell = story_flag && story_flag(0x2EE);
        const uint32_t price = u16(kShopPrices8008fd3c + 2u * shop_item_gp10cc);
        shop_short_gp1308 = (!sell && zenny_800c1b2c < price) ? 1 : 0;   // 0x8004327C(-price)
        s.read += 2; return true;
    }
    case 0xB8: {                                                             // 0x80056D10: build the shop list
        const uint8_t kind = shop_kind_gp0fec;
        const uint32_t start = u16(0x80097F64u + 2u * kind), n = mem(0x80097F68u + kind);
        list_count_gp4420 = 0; list_scroll_gp4408 = shop_scroll0_gp1090;
        // Prices that follow the player: life-up (0x23) by the max life, canteen (0x25) by its count.
        auto put_price = [&](uint8_t item, uint16_t v) { mem(kShopPrices8008fd3c + 2u * item) = uint8_t(v); mem(kShopPrices8008fd3c + 2u * item + 1) = uint8_t(v >> 8); };
        if (life_max_800b5260 < 0xA0) {
            int32_t d = 0xA0 - life_max_800b5260; d = d >= 0 ? (d + 15) >> 4 : d >> 4;
            put_price(0x23, u16(kShopPrices8008fd3c + 2u * uint32_t(0x85 - d)));
        } else put_price(0x23, u16(0x8008FE44u));
        put_price(0x25, canteen_800c1b8d < 0x12 ? u16(0x8008FE40u + 2u * canteen_800c1b8d) : u16(0x8008FE62u));
        const bool sell = story_flag && story_flag(0x2EE);
        auto fbyte = [&](int32_t bit) -> uint8_t { return (bit >= 0 && bit < 2048) ? flags_800be378[size_t(bit >> 3)] : 0; };
        for (uint32_t i = 0; i < n; ++i) {
            const uint8_t bit = uint8_t(0x80u >> (i & 7));
            const uint8_t owned = fbyte(int32_t(start + i)), stock = fbyte(int32_t(start + i) - 0x80);
            auto push = [&] { if (list_count_gp4420 < 128) list_800be2f8[list_count_gp4420++] = uint8_t(i); };
            if (!sell) {
                if (!((stock | uint8_t(~owned)) & bit)) { push(); continue; }
                if (kind != 1) continue;
                if (i == 3) { if (life_max_800b5260 != 0xA0) push(); }
                else if (i == 5) { if (canteen_800c1b8d && canteen_800c1b8d < 0x63) push(); }
            } else {
                if (!(stock & owned & bit)) continue;
                if (kind != 1) { push(); continue; }
                if (i < 0x20 || (i >= 0x24 && i < 0x38) || (i >= 0x4C && i < 0x51)) push();
            }
        }
        for (uint32_t i = list_count_gp4420; i < 128; ++i) list_800be2f8[i] = 255;
        // Block: 0 rows -> arg 1, 1..4 rows -> arg (7 - rows), 5+ -> continue after the command.
        uint8_t target = 255;
        if (list_count_gp4420 == 0) target = mem(r + 2);
        else if (list_count_gp4420 < 5) target = mem(r + 7 - list_count_gp4420);
        if (target == 255) { s.read += 7; return true; }
        s.index = target;
        s.read = s.page = s.index_base ? s.index_base + u16(s.index_base + 2u * target) : exe_.u32(kMsgTable8008cacc + 4u * target);
        s.color = 0; s.rows = 0;   // +0x3E, +0x72 (+0x3C also cleared: no port field)
        return true;
    }
    case 0xB7: {                                                             // 0x80056778: a list row
        mem(r + 5) = row_block_gp0f94;                                       // the row's 0x90 call target
        name_index_gp4044 = uint8_t(list_800be2f8[size_t((list_scroll_gp4408 + uint32_t(int8_t(s.rows))) & 0x7Fu)] +
                                    (shop_kind_gp0fec ? 0x20u : 0u));
        s.repeat = 0; ++s.rows; s.flags |= 0x00402000u; s.read += 3;
        return true;
    }
    case 0xC2: case 0xC3: {                                                  // 0x80057744 / 0x800577FC: canteen branches
        uint8_t target;
        if (op == 0xC2) target = canteen_fill_800c1b8c == canteen_800c1b8d ? mem(r + 1) : mem(r + 2);   // full / not full
        else {
            const uint32_t cost = uint32_t(canteen_800c1b8d - canteen_fill_800c1b8c) * 10u;
            if (zenny_800c1b2c >= cost) target = mem(r + 1);                                         // affordable
            else if (word_800c1b70 < 0x80 || zenny_800c1b2c < 1u) target = mem(r + 2);               // not enough money
            else target = mem(r + 3);                                                                // the free refill
        }
        if (target == 255) { s.read += op == 0xC2 ? 3u : 4u; return true; }
        s.index = target;
        s.read = s.page = s.index_base ? s.index_base + u16(s.index_base + 2u * target) : exe_.u32(kMsgTable8008cacc + 4u * target);
        s.color = 0; s.rows = 0;
        return true;
    }
    case 0xC4: {                                                             // 0x80057924: refill (arg 0: pay; else all money)
        const uint32_t cost = mem(r + 1) == 0 ? uint32_t(canteen_800c1b8d - canteen_fill_800c1b8c) * 10u : zenny_800c1b2c;
        zenny_800c1b2c = zenny_800c1b2c >= cost ? zenny_800c1b2c - cost : 0u;   // 0x80043294(-cost)
        canteen_fill_800c1b8c = canteen_800c1b8d;
        s.read += 2; return true;
    }
    case 0xBD: {                                                             // 0x800572C8: buy or sell the item
        const uint8_t item = shop_item_gp10cc;
        const uint32_t price = u16(kShopPrices8008fd3c + 2u * item);
        if (story_flag && story_flag(0x2EE)) {                               // sell: a quarter back, drop 0x500 | item
            zenny_800c1b2c = std::min<uint32_t>(zenny_800c1b2c + (price >> 2), 999999u);   // 0x80043294(+)
            if (set_story_flag) set_story_flag(uint16_t(0x500u | item), false);
            out.shop_sold = item;
        } else {                                                             // buy: pay, own 0x500 | item
            zenny_800c1b2c = zenny_800c1b2c >= price ? zenny_800c1b2c - price : 0u;   // 0x80043294(-)
            if (set_story_flag) set_story_flag(uint16_t(0x500u | item), true);
            out.shop_bought = item;
        }
        s.read += 1; return true;
    }
    case 0xB6: s.read += 3; return true;                                     // 0x800543E0 (drawn by 0x800563A8)
    case 0xC0: case 0xC5: s.read += 1; return true;                          // 0x800543B0 (drawn by 0x80057574 / 0x800579D8)
    case 0xDE: s.read += 3; return true;                                     // 0x800543E0
    case 0x98: s.flags &= ~0x08000000u; s.read += 1; return true;           // 0x8005531C
    case 0x8B: case 0x84: {                                                  // 0x80054700 pause / 0x80054450 close
        if (!(s.flags & 0x01000000u)) { s.flags |= 0x01000000u; s.timer = int16_t(u16(r + 1)); }
        s.flags |= 0x10000000u;
        if (s.timer > 0) { --s.timer; return false; }
        if (op == 0x8B) { s.flags &= ~0x01000000u; s.read += 3; return false; }
        s.flags |= 0x20000u;
        if (s.flags & 0x04000000u) {                                         // close the frame first
            s.flags |= 0x00100000u;
            (void)decor_exec(s);
            return false;
        }
        s.flags &= ~0x20000u; s.flags &= 0x7EFFFFFFu; s.read += 3;          // slot closed
        s.index = 255; s.flags &= ~0x40000u;
        return false;
    }
    case 0xA9:                                                               // 0x80055BB8: end
        if (s.flags & 0x04000000u) {
            s.flags |= 0x00100000u;
            (void)decor_exec(s);
            return false;
        }
        s.read += 1; return true;
    case 0x9F:                                                               // 0x80055688: wait for a button
        if (!(s.flags & 0x01000000u)) { s.flags |= 0x01000000u; s.timer = 10; }
        s.flags |= 0x10000000u;
        if (s.timer > 0) { --s.timer; return false; }
        if ((s.flags & 0x40000u) || (s.held & 0xF000u)) {                   // a face button held
            flags_80098a58 |= 0x80000u; s.flags &= ~0x01000000u; s.read += 1;
        }   // (the blinking "next" arrow 0x8001FE38 is not drawn)
        return false;
    case 0xC1:                                                               // 0x800576C4: call name 0x8008CBA4[gp+4044]
        if (s.exec_depth >= s.exec_stack.size()) { unported = true; return false; }
        s.exec_stack[s.exec_depth++] = r + 1;
        s.read = exe_.u32(0x8008CBA4u + 4u * name_index_gp4044);
        return true;
    case 0xDA: {                                                             // 0x80058C28: call a name if its flag is set
        if (s.exec_depth >= s.exec_stack.size()) { unported = true; return false; }
        s.exec_stack[s.exec_depth++] = r + 3;
        const uint16_t f = u16(r + 1);
        s.read = (story_flag && story_flag(f)) ? exe_.u32(0x8008BAA4u + 4u * f) : exe_.u32(0x8008CE58u);
        return true;
    }
    case 0xA7:                                                               // 0x80055B14: layout start / clear
        if (u16(r + 1)) out.layout_calls.push_back({int8_t(int16_t(u16(r + 3))), -1});
        else out.layout_calls.push_back({int8_t(u16(r + 3)), int16_t(u16(r + 5))});
        s.read += 9; return true;
    case 0xBF: {                                                             // 0x80057408: jump to message d2
        if (mem(r) == 0xD5) { unported = true; return false; }
        const uint8_t target = mem(r + 2);
        if (target == 255) { s.read += 3; return true; }
        s.index = target;
        s.page = s.read = s.index_base ? s.index_base + u16(s.index_base + 2u * target) : exe_.u32(kMsgTable8008cacc + 4u * target);
        s.color = 0; s.rows = 0;
        return true;
    }
    case 0xE1: {                                                             // 0x8005958C: call a part name
        if (s.exec_depth >= s.exec_stack.size()) { unported = true; return false; }
        s.exec_stack[s.exec_depth++] = r + 3;
        int8_t part;
        if (mem(r + 1)) part = int8_t(owned_parts_800b5604[std::min<size_t>(63, size_t(mem(r + 2)) + list_scroll_gp4408)]);
        else {
            mem(r - 1) = 0;
            if ((flags_80098a58 & 0x20000u) && mem(r + 2) == uint8_t(s.cursor)) mem(r - 1) = 3;
            part = int8_t(equipped_parts_800b5600[mem(r + 2) & 3]);
        }
        s.read = exe_.u32(uint32_t(0x8008CBA0 + 4 * int32_t(part)));
        return true;
    }
    case 0x9C: {                                                             // 0x80055438: branch on a story flag
        const bool set = story_flag && story_flag(u16(r + 1));
        const uint8_t target = mem(r + (set ? 3u : 4u));
        if (target == 255) { s.read += 5; return true; }
        if (!s.index_base) { unported = true; return false; }
        s.index = target;
        s.read = s.page = s.index_base + u16(s.index_base + 2u * target);
        return true;
    }
    case 0xD7:                                                               // 0x80058788: grid cell
        s.repeat = 0; ++s.rows; s.flags |= 0x00400800u; s.read += 5;
        slot_bits_gp4812 &= ~(0x20000u << cur_slot_);
        return true;
    case 0xD9: {                                                             // 0x80058AEC: grid choice
        if (!(s.flags & 0x01000000u)) { s.flags = (s.flags | 0x01000000u) & 0xFFFF7FFFu; s.timer = 5; }
        s.flags &= 0xFFBFFFFFu;
        if (flags_80098a58 & 0x08000000u) flags_80098a58 |= 0x20000u; else flags_80098a58 &= ~0x20000u;
        if (s.timer > 0) --s.timer;
        else if (s.pressed & 0x4000u) flags_80098a58 = (flags_80098a58 & ~0xFFu) | 0x40000000u | uint8_t(s.cursor);
        else if (s.pressed & 0x1000u) flags_80098a58 = (flags_80098a58 & ~0xFFu) | 0x20000000u | uint8_t(s.rows - 1);
        s.brightness = 128;
        return false;
    }
    case 0xDF:                                                               // 0x80058DB4
        list_800be2f8.fill(255); list_scroll_gp4408 = 0; s.read += 1; return true;
    case 0xDD:                                                               // 0x80058EA0
        s.repeat = 0; ++s.rows; s.flags |= 0x00402800u; s.read += 5;
        list_picked_gp4077 = 255; slot_bits_gp4812 &= ~(0x20000u << cur_slot_);
        return true;
    case 0xE0:                                                               // 0x800594CC: call the item name
        if (s.exec_depth >= s.exec_stack.size()) { unported = true; return false; }
        s.exec_stack[s.exec_depth++] = r + 1;
        s.read = name_ptr(int8_t(list_at(int(list_scroll_gp4408) + s.rows - 1)));
        return true;
    case 0xDC: {                                                             // 0x800592FC: list choice
        if (!(s.flags & 0x01000000u)) { s.flags = (s.flags | 0x01000000u) & 0xFFFF7FFFu; s.timer = 5; }
        s.flags &= 0xFFBFFFFFu;
        if (s.timer > 0) --s.timer;
        else {
            out.layout_calls.push_back({0, -1}); out.layout_calls.push_back({1, -1});   // 0x80063F28(0), (1)
            if (s.pressed & 0x4000u) {
                flags_80098a58 &= ~0xFFu;
                const uint8_t id = list_at(int(list_scroll_gp4408) + s.cursor);
                flags_80098a58 |= id != 255 ? 0x40000000u | uint8_t(id + 64) : 0x400000FFu;
            } else if (list_scroll_gp4408 != 0) {
                out.layout_calls.push_back({0, 26});                         // up arrow
                if (list_scroll_gp4408 < list_max_scroll_gp4464) out.layout_calls.push_back({1, 27});
            } else if (list_scroll_gp4408 < list_max_scroll_gp4464) out.layout_calls.push_back({0, 27});   // down arrow
        }
        const uint8_t id = list_at(int(list_scroll_gp4408) + s.cursor);    // 0x80059470
        slot_bits_gp4812 = id == 255 ? 255u : uint8_t(id + 64);
        s.brightness = 128;
        return false;
    }
    default: unported = true; return false;
    }
}

// Draw-table handlers (they advance s.draw).
void MessageWindows::draw(MessageSlot& s, uint8_t op, MessageWindowFrame& out, int) {
    const uint32_t d = s.draw;
    switch (op) {
    case 0x86: s.pen_x = s.x; s.color = s.default_color; s.pen_y = int16_t(s.pen_y + 12); s.draw += 1; return;   // 0x8005459C
    case 0x89: s.color = mem(d + 1); s.draw += 2; return;                    // 0x80054694
    case 0x8A: case 0x99: case 0x93: s.draw += 3; return;                    // 0x80054324
    case 0x8C: s.draw += 7; return;                                          // 0x80054374
    case 0x95: case 0xA1: s.draw += 1; return;                               // 0x800542FC
    case 0x97: s.draw += uint32_t(s.rows) + 2u; return;                      // 0x80055304
    case 0x96: {                                                             // 0x80054BEC
        // Selection row: args (left/right nav, up/down nav); cursor glyph
        // byte after the command. Rows draw at 80, the cursor row at 128.
        mem(d + 3) = 0xED;
        s.brightness = 0;
        if (s.cursor == s.row_draw && !(s.flags & 0x00C00000u)) {
            const int8_t old = s.cursor;
            const uint8_t lr = mem(d + 1), ud = mem(d + 2);
            const uint32_t pad = uint32_t(s.held) | (uint32_t(s.pressed) << 16);
            const auto repeat = [&](int8_t target) {
                if (s.repeat < 13) ++s.repeat; else { s.repeat = 12; s.cursor = target; }
            };
            if (!(s.pressed & 0x4000u)) {
                if (s.pressed & 0x10u) { s.repeat = 0; s.cursor = int8_t(ud >> 4); }
                else if ((pad & 0x100010u) == 0x10u) repeat(int8_t(ud >> 4));
                else if (s.pressed & 0x40u) { s.repeat = 0; s.cursor = int8_t(ud & 15); }
                else if ((pad & 0x400040u) == 0x40u) repeat(int8_t(ud & 15));
                else if (s.pressed & 0x80u) { s.repeat = 0; s.cursor = int8_t(lr >> 4); }
                else if ((pad & 0x800080u) == 0x80u) repeat(int8_t(lr >> 4));
                else if (s.pressed & 0x20u) { s.repeat = 0; s.cursor = int8_t(lr & 15); }
                else if ((pad & 0x200020u) == 0x20u) repeat(int8_t(lr & 15));
            }
            mem(d + 3) = 0x60;
            s.brightness = uint8_t(s.brightness + 48);
            s.cursor_byte = d + 3;
            s.flags |= 0x00800000u;
            if (old != s.cursor) sound(out, 129, 1);
        }
        ++s.row_draw; s.brightness = uint8_t(s.brightness + 80); s.draw += 3;
        return;
    }
    case 0xB7: {                                                             // 0x80056820: a list row (draw)
        mem(d + 3) = 0xED;
        s.brightness = 0;
        bool cancel = false;
        if (s.cursor == s.row_draw && !(s.flags & 0x00C00000u)) {
            const int8_t old = s.cursor;
            const uint8_t lr = mem(d + 1), ud = mem(d + 2);
            const uint32_t pad = uint32_t(s.held) | (uint32_t(s.pressed) << 16);
            const uint32_t count = list_count_gp4420;
            const auto scroll_up = [&] { if (count >= 6 && s.cursor == old && list_scroll_gp4408) { sound(out, 129, 1); --list_scroll_gp4408; } };
            const auto scroll_down = [&] { if (count >= 6 && s.cursor == old && list_scroll_gp4408 < count - 5) { sound(out, 129, 1); ++list_scroll_gp4408; } };
            const auto repeat = [&](int8_t target, bool vertical_up, bool vertical_down) {
                if (s.repeat < 13) { ++s.repeat; return; }
                s.repeat = 12; s.cursor = target;
                if (vertical_up) scroll_up(); else if (vertical_down) scroll_down();
            };
            if (!(s.pressed & 0x4000u)) {
                if (s.pressed & 0x1000u) { cancel = true; sound(out, 131, 1); s.held = 0; s.pressed = 0; }
                else if (s.pressed & 0x10u) { s.cursor = int8_t(ud >> 4); s.repeat = 0; scroll_up(); }
                else if ((pad & 0x100010u) == 0x10u) repeat(int8_t(ud >> 4), true, false);
                else if (s.pressed & 0x40u) { s.cursor = int8_t(ud & 15); s.repeat = 0; scroll_down(); }
                else if ((pad & 0x400040u) == 0x40u) repeat(int8_t(ud & 15), false, true);
                else if (s.pressed & 0x80u) { s.cursor = int8_t(lr >> 4); s.repeat = 0; }
                else if ((pad & 0x800080u) == 0x80u) repeat(int8_t(lr >> 4), false, false);
                else if (s.pressed & 0x20u) { s.cursor = int8_t(lr & 15); s.repeat = 0; }
                else if ((pad & 0x200020u) == 0x20u) repeat(int8_t(lr & 15), false, false);
            }
            s.cursor_byte = d + 3;
            s.flags |= 0x00800000u;
            if (old != s.cursor) sound(out, 129, 1);
            if (count >= 6) {   // 0x80063F28 / 0x80063FC0: the scroll arrows (slot choice CANDIDATE)
                out.layout_calls.push_back({0, -1}); out.layout_calls.push_back({1, -1});
                if (!cancel) {
                    const bool up = list_scroll_gp4408 > 0, down = list_scroll_gp4408 < count - 5;
                    if (up) out.layout_calls.push_back({0, 3});
                    if (down) out.layout_calls.push_back({int8_t(up ? 1 : 0), 4});
                }
            }
        }
        if (s.cursor == s.row_draw) { s.brightness = uint8_t(s.brightness + 0x30); mem(d + 3) = 0x60; }
        name_index_gp4044 = uint8_t(list_800be2f8[size_t((list_scroll_gp4408 + uint32_t(int8_t(s.row_draw))) & 0x7Fu)] +
                                    (shop_kind_gp0fec ? 0x20u : 0u));
        mem(d + 5) = row_block_gp0f94;
        ++s.row_draw; s.brightness = uint8_t(s.brightness + 0x50); s.draw += 3;
        if (cancel) {
            if (s.index_base) open_8005382c(int8_t(cancel_slot_gp1200), s.index_base, cancel_block_gp1201);
            else open_8005382c(int8_t(cancel_slot_gp1200), exe_.u32(kMsgTable8008cacc + 4u * cancel_block_gp1201), 255);
        }
        return;
    }
    case 0xC0: {                                                             // 0x80057574: the row's price (a quarter when selling)
        const uint8_t item = list_800be2f8[size_t((list_scroll_gp4408 + uint32_t(int8_t(s.row_draw)) - 1u) & 0x7Fu)];
        uint32_t price = u16((shop_kind_gp0fec ? 0x8008FD7Cu : kShopPrices8008fd3c) + 2u * item);
        if (story_flag && story_flag(0x2EE)) price >>= 2;
        print_number_8005bf10(0, price, d + 1u);
        s.draw += 1; return;
    }
    case 0xC5:                                                               // 0x800579D8: the refill cost
        print_number_8005bf10(1, uint32_t(canteen_800c1b8d - canteen_fill_800c1b8c) * 10u, d + 1u);
        s.draw += 1; return;
    case 0xB6: {                                                             // 0x800563A8: the cursor item's help window
        const uint8_t item = list_800be2f8[size_t((list_scroll_gp4408 + uint32_t(int8_t(s.cursor))) & 0x7Fu)];
        name_index_gp4044 = uint8_t(item + (shop_kind_gp0fec ? 0x20u : 0u));
        if (s.flags & 0x2000u) {
            const int8_t slot = int8_t(mem(d + 1)); const uint8_t id = uint8_t(mem(d + 2) + item);
            if (s.index_base) open_8005382c(slot, s.index_base, id);
            else open_8005382c(slot, exe_.u32(kMsgTable8008cacc + 4u * id), 255);
        }
        s.draw += 3; return;
    }
    case 0xD6: {                                                             // 0x800564C8
        // Help for the cursor row: message (arg2 + cursor) in slot arg1.
        const uint8_t slot = mem(d + 1), base = mem(d + 2);
        if (s.index_base) open_8005382c(slot, s.index_base, uint8_t(base + uint8_t(s.cursor)));
        else open_8005382c(slot, exe_.u32(kMsgTable8008cacc + 4u * uint32_t(uint8_t(base + s.cursor))), 255);
        s.draw += 3;
        return;
    }
    case 0xA0: s.pen_x = int16_t(s.pen_x + int16_t(u16(d + 1))); s.draw += 3; return;   // 0x800555F4
    case 0xAE: s.brightness = setting(mem(d + 1)) == mem(d + 2) ? 128 : 64; s.draw += 3; return;   // 0x800560D0
    case 0xAF: s.pen_x = s.x; s.color = s.default_color; s.pen_y = int16_t(s.pen_y + mem(d + 1)); s.draw += 2; return;   // 0x80056148
    case 0xB0: s.brightness = mem(d + 1); s.draw += 2; return;               // 0x80056180
    case 0xDB: s.pen_x = int16_t(u16(d + 1)); s.pen_y = int16_t(u16(d + 3)); s.draw += 5; return;   // 0x80058D64
    case 0xED: s.pen_x = int16_t(s.pen_x + 12); s.draw += 1; return;        // 0x8005A858
    case 0x90: {                                                             // 0x80054A04: the same on the draw pass
        if (s.draw_depth >= s.draw_stack.size()) { s.draw += 2; return; }
        s.draw_stack[s.draw_depth++] = d + 2;
        const uint8_t id = mem(d + 1);
        s.draw = s.index_base ? s.index_base + u16(s.index_base + 2u * id) : exe_.u32(kMsgTable8008cacc + 4u * id);
        return;
    }
    case 0x91: if (s.draw_depth) s.draw = s.draw_stack[--s.draw_depth]; else s.draw += 1; return;   // 0x80054AB4
    case 0xAC: s.draw += 3; return;
    case 0xBA:                                                               // 0x80057144: the number into the 7 bytes after
        s.number_40 = zenny_800c1b2c;
        print_number_8005bf10(0, s.number_40, s.draw + 1u);
        s.draw += 1; return;
    case 0xB9: case 0xC3: s.draw += 4; return;                               // 0x80054338
    case 0xBB: s.draw += 5; return;                                          // 0x8005434C
    case 0xBC: case 0xC4: s.draw += 2; return;                               // 0x80054310
    case 0xB8: s.draw += 7; return;                                          // 0x80054374
    case 0xBD: s.draw += 1; return;                                          // 0x800542FC
    case 0xC2: s.draw += 3; return;                                          // 0x80054324
    case 0xDC: case 0xD9: case 0x98: case 0xA9: s.draw += 1; return;
    case 0x8B: case 0x84: case 0xBF: s.draw += 3; return;
    case 0xA7: s.draw += 9; return;                                          // 0x80054388
    case 0x9F: s.draw += 1; return;
    case 0xC1:                                                               // 0x80057708
        if (s.draw_depth < s.draw_stack.size()) { s.draw_stack[s.draw_depth++] = d + 1; s.draw = exe_.u32(0x8008CBA4u + 4u * name_index_gp4044); }
        else s.draw += 1;
        return;
    case 0xDA:                                                               // 0x80058CC8
        if (s.draw_depth < s.draw_stack.size()) {
            s.draw_stack[s.draw_depth++] = d + 3;
            const uint16_t f = u16(d + 1);
            s.draw = (story_flag && story_flag(f)) ? exe_.u32(0x8008BAA4u + 4u * f) : exe_.u32(0x8008CE58u);
        } else s.draw += 3;
        return;
    case 0xE1: {                                                             // 0x80059660
        if (s.draw_depth >= s.draw_stack.size()) { s.draw += 3; return; }
        s.draw_stack[s.draw_depth++] = d + 3;
        int8_t part;
        if (mem(d + 1)) part = int8_t(owned_parts_800b5604[std::min<size_t>(63, size_t(mem(d + 2)) + list_scroll_gp4408)]);
        else {
            mem(d - 1) = 0;
            if ((flags_80098a58 & 0x20000u) && mem(d + 3) == uint8_t(s.cursor)) mem(d - 1) = 3;
            part = int8_t(equipped_parts_800b5600[mem(d + 2) & 3]);
        }
        s.draw = exe_.u32(uint32_t(0x8008CBA0 + 4 * int32_t(part)));
        return;
    }
    case 0x9C: s.draw += 5; return;                                         // 0x8005434C
    case 0xD7: {                                                             // 0x800587F8: grid cell
        mem(d + 5) = 0xED; s.brightness = 0;
        if (s.cursor == s.row_draw && !(s.flags & 0x00C00000u)) {
            const uint32_t bit = 0x20000u << cur_slot_;
            slot_bits_gp4812 &= ~bit;
            const int8_t old = s.cursor;
            if (!(slots_[0].flags & 0x80000000u) && !(s.pressed & 0x4000u)) {   // no input under a slot-0 window
                const uint32_t pad = uint32_t(s.held) | (uint32_t(s.pressed) << 16);
                const auto step = [&](uint8_t target, uint32_t press, uint32_t mask) {
                    if (s.pressed & press) { s.repeat = 0; s.cursor = int8_t(target); return true; }
                    if ((pad & mask) != press) return false;
                    if (s.repeat < 13) ++s.repeat; else { s.repeat = 12; s.cursor = int8_t(target); }
                    return true;
                };
                if (!step(mem(d + 1), 0x10u, 0x100010u) && !step(mem(d + 2), 0x40u, 0x400040u) &&
                    !step(mem(d + 3), 0x80u, 0x800080u)) (void)step(mem(d + 4), 0x20u, 0x200020u);
            }
            mem(d + 5) = (flags_80098a58 & 0x20000u) ? 0xED : 0x60;
            s.brightness = 48; s.cursor_byte = d + 5; s.flags |= 0x00800000u;
            if (old != s.cursor) { sound(out, 129, 1); slot_bits_gp4812 |= bit; }
        }
        ++s.row_draw; s.brightness = uint8_t(s.brightness + 80); s.draw += 5;
        return;
    }
    case 0xDF: {                                                             // 0x80058DEC: owned ids -> list
        list_count_gp4420 = 0;
        for (unsigned id = 0; id < 64; ++id)
            if (owned_items_800be420[id >> 3] & (0x80u >> (id & 7))) list_800be2f8[list_count_gp4420++] = uint8_t(id);
        for (uint32_t i = list_count_gp4420; i < 128; ++i) list_800be2f8[i] = 255;
        s.draw += 1; return;
    }
    case 0xDE: {                                                             // 0x80056558: help for the item
        const uint8_t slot = mem(d + 1), base = mem(d + 2);
        const int8_t id = int8_t(list_at(int(list_scroll_gp4408) + s.cursor));
        if (s.index_base) open_8005382c(slot, s.index_base, uint8_t(base + uint8_t(id)));
        else open_8005382c(slot, exe_.u32(kMsgTable8008cacc + uint32_t(4 * (int32_t(base) + id))), 255);
        s.draw += 3; return;
    }
    case 0xE0:                                                               // 0x80059530
        if (s.draw_depth < s.draw_stack.size()) {
            s.draw_stack[s.draw_depth++] = d + 1;
            s.draw = name_ptr(int8_t(list_at(int(list_scroll_gp4408) + s.row_draw - 1)));
        } else s.draw += 1;
        return;
    case 0xDD: {                                                             // 0x80058F18: list row
        const int here = int(s.row_draw) + list_scroll_gp4408;
        if (list_picked_gp4077 == here && (frame_gp4348 & 1)) { mem(d + 5) = 0x60; s.brightness = 16; }
        else { mem(d + 5) = 0xED; s.brightness = 0; }
        if (s.cursor == s.row_draw && !(s.flags & 0x00C00000u)) {
            const uint32_t bit = 0x20000u << cur_slot_;
            slot_bits_gp4812 &= ~bit;
            const int8_t old = s.cursor;
            const uint32_t pad = uint32_t(s.held) | (uint32_t(s.pressed) << 16);
            const auto step = [&](uint8_t target, uint32_t press, uint32_t mask, int dir) {
                bool moved_now = false;
                if (s.pressed & press) { s.repeat = 0; s.cursor = int8_t(target); moved_now = true; }
                else if ((pad & mask) == press) {
                    if (s.repeat < 13) { ++s.repeat; return true; }
                    s.repeat = 12; s.cursor = int8_t(target); moved_now = true;
                } else return false;
                if (moved_now && dir && s.cursor == old) {                    // at the edge: scroll the list
                    const bool can = dir < 0 ? list_scroll_gp4408 != 0 : list_scroll_gp4408 < list_max_scroll_gp4464;
                    if (can) { slot_bits_gp4812 |= bit; sound(out, 129, 1); list_scroll_gp4408 = uint8_t(list_scroll_gp4408 + 2 * dir); }
                }
                return true;
            };
            if (!(s.pressed & 0x4000u)) {
                if (!step(mem(d + 1), 0x10u, 0x100010u, -1))
                    if (!step(mem(d + 2), 0x40u, 0x400040u, +1))
                        if (!step(mem(d + 3), 0x80u, 0x800080u, 0))
                            (void)step(mem(d + 4), 0x20u, 0x200020u, 0);
            }
            mem(d + 5) = 0x60; s.brightness = 48; s.cursor_byte = d + 5; s.flags |= 0x00800000u;
            if (old != s.cursor) { sound(out, 129, 1); slot_bits_gp4812 |= bit; }
        }
        ++s.row_draw; s.brightness = uint8_t(s.brightness + 80); s.draw += 5;
        return;
    }
    case 0xAD: {                                                             // 0x80055CFC
        mem(d + 7) = 0xED;
        s.brightness = 0;
        if (s.cursor == s.row_draw && !(s.flags & 0x00C00000u)) {
            const int8_t old = s.cursor;
            const uint8_t nav = mem(d + 2);
            const uint32_t pad = uint32_t(s.held) | (uint32_t(s.pressed) << 16);
            const bool last = s.cursor == int8_t(s.rows - 1);
            const auto repeat = [&](int8_t target) {
                if (s.repeat < 13) ++s.repeat; else { s.repeat = 12; s.cursor = target; }
            };
            if (s.pressed & 0x10u) { s.repeat = 0; s.cursor = int8_t(nav >> 4); }
            else if ((pad & 0x100010u) == 0x10u) repeat(int8_t(nav >> 4));
            else if (s.pressed & 0x40u) { s.repeat = 0; s.cursor = int8_t(nav & 15); }
            else if ((pad & 0x400040u) == 0x40u) repeat(int8_t(nav & 15));
            else if (s.pressed & 0x80u) {
                s.repeat = 0;
                if (!last) { uint8_t& v = setting(mem(d + 3)); if (v == 1) sound(out, 130, 1); v = 0; }
            } else if (s.pressed & 0x20u) {
                s.repeat = 0;
                if (!last) { uint8_t& v = setting(mem(d + 3)); if (v == 0) sound(out, 130, 1); v = 1; }
            } else if (s.pressed & 0x4000u) {
                uint8_t& v = setting(mem(d + 3)); v ^= 1;
                if (!last) { sound(out, 130, 1); s.pressed &= 0xBFFFu; }
            }
            const uint8_t help = mem(d + 5u + setting(mem(d + 3)));
            if (s.index_base) open_8005382c(mem(d + 4), s.index_base, help);
            else open_8005382c(mem(d + 4), exe_.u32(kMsgTable8008cacc + 4u * help), 255);
            mem(d + 7) = 0x60;
            s.cursor_byte = d + 7;
            s.flags |= 0x00800000u;
            if (old != s.cursor) sound(out, 129, 1);
        }
        ++s.row_draw; s.brightness = uint8_t(s.brightness + 128); s.draw += 7;
        return;
    }
    default: s.draw += 1; return;   // unported (counted by exec)
    }
}

bool MessageWindows::decor_exec(MessageSlot& s) {
    const int16_t m0 = int16_t(s.margin);
    const auto settle = [&](int16_t m) {
        const int16_t w = int16_t(m + s.width * 12), h = int16_t(m + s.height * 12);
        s.box = {{int16_t(-m), int16_t(-m), w, int16_t(-m), int16_t(-m), h, w, h}};
    };
    // Decor 3 / 7 (0x8005AB78) do the same as 2 / 4 (0x8005B85C): margin, settled box.
    if (s.decor == 2 || s.decor == 4 || s.decor == 3 || s.decor == 7) {      // 0x8005B85C / 0x8005AB78
        if (s.decor_state == 0) {
            s.decor_state = 2; s.flags |= 0x06000000u; s.margin = mem(s.read + 2); s.read += 3;
            flags_80098a58 |= 0x00200000u; settle(int16_t(s.margin));
        } else if (s.decor_state == 2) s.flags = (s.flags | 0x10000000u) & 0xF9FFFFFFu;
        return true;
    }
    if (s.decor != 8) return false;
    switch (s.decor_state) {                                                 // 0x8005B98C
    case 0: {                                                                // start collapsed at the middle
        s.decor_step = 8; s.flags |= 0x14000000u; s.decor_saved = s.flags & 0x08000000u; s.flags |= 0x08000000u;
        s.margin = mem(s.read + 2); s.decor_state = 1;
        const int16_t m = int16_t(s.margin), half = int16_t(s.height * 6), w = int16_t(m + s.width * 12);
        s.box = {{int16_t(-m), half, w, half, int16_t(-m), half, w, half}};
        return true;
    }
    case 1:                                                                  // grow by 8 each update
        s.flags |= 0x10000000u;
        s.box[1] = int16_t(s.box[1] - s.decor_step); s.box[3] = int16_t(s.box[3] - s.decor_step);
        s.box[5] = int16_t(s.box[5] + s.decor_step); s.box[7] = int16_t(s.box[7] + s.decor_step);
        if (s.y - m0 < s.y + s.box[1]) return true;
        flags_80098a58 |= 0x00200000u;
        settle(m0); s.decor_state = 2;
        s.flags = ((s.flags | 0x12000000u) & 0xF7FFFFFFu) | s.decor_saved;
        s.read += 3;
        return true;
    case 2:                                                                  // shrink shut
        s.flags |= 0x10000000u;
        s.box[1] = int16_t(s.box[1] + s.decor_step); s.box[3] = int16_t(s.box[3] + s.decor_step);
        s.box[5] = int16_t(s.box[5] - s.decor_step); s.box[7] = int16_t(s.box[7] - s.decor_step);
        if (s.box[5] < s.box[1]) s.flags &= 0xF9FFFFFFu;
        return true;
    default: return true;
    }
}

// 0x8005BF10(pad, value, dst): six digits by the divisors 0x8008AF24 (leading ones as pad glyph
// 0x8008AF3C[pad] until the first non-zero), each as message glyph 0x8008D028[digit], then a
// fixed '0' glyph at dst + 6 (the amounts are shown times ten).
void MessageWindows::print_number_8005bf10(uint8_t pad, uint32_t value, uint32_t dst) {
    bool lead = false;
    for (uint32_t k = 0; k < 6; ++k) {
        const uint32_t div = exe_.u32(kMsgNumberDivisors8008af24 + 4u * k);
        const uint32_t digit = div ? value / div : 0;
        if (lead || digit) { mem(dst + k) = mem(kMsgDigitGlyphs8008d028 + digit); lead = true; }
        else mem(dst + k) = mem(kMsgNumberPads8008af3c + pad);
        if (div) value %= div;
    }
    mem(dst + 6) = mem(kMsgDigitGlyphs8008d028);
}

// Decor 2 frame 0x8005ACA8 with its primitives from 0x8005BCE4.
void MessageWindows::draw_frame(MessageSlot& s, MessageWindowFrame& out) {
    // Decor 7 (0x8005BC90) draws as decor 0 in save mode (0x80098912 == 1), else as decor 4.
    const uint8_t d = s.decor == 7 ? (save_mode_80098912 == 1 ? 0 : 4) : s.decor;
    const uint8_t param = mem(kMsgDecorParam8008af18 + d);
    const uint32_t bg = kMsgFrameColors80097f6c + 6u * param;
    const auto& b = s.box;
    const int X = s.x, Y = s.y;
    const auto edge = [&](int i, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
        FrontendPacket p; p.length = 8; p.bucket = uint16_t(28 + s.ot_layer);
        const uint32_t c = kMsgBorderColors80097f78 + 4u * uint32_t(i);
        p.words = {{0x38000000u | grey(mem(c)), xy(x0, y0), grey(mem(c + 1)), xy(x1, y1),
                    grey(mem(c + 2)), xy(x2, y2), grey(mem(c + 3)), xy(x3, y3), 0}};
        out.packets.push_back(p);
    };
    edge(0, X + b[0] - 1, Y + b[1] - 1, X + b[2] + 1, Y + b[3] - 1, X + b[0], Y + b[1], X + b[2], Y + b[3]);   // +160
    edge(1, X + b[4], Y + b[5], X + b[6], Y + b[7], X + b[4] - 1, Y + b[5] + 1, X + b[6] + 1, Y + b[7] + 1);   // +164
    edge(2, X + b[0] - 1, Y + b[1] - 1, X + b[0], Y + b[1], X + b[4] - 1, Y + b[5] + 1, X + b[4], Y + b[5]);   // +168
    edge(3, X + b[2], Y + b[3], X + b[2] + 1, Y + b[3] - 1, X + b[6], Y + b[7], X + b[6] + 1, Y + b[7] + 1);   // +172
    const auto f4 = [&](uint32_t c) {
        FrontendPacket p; p.length = 5; p.bucket = uint16_t(28 + s.ot_layer);
        p.words = {{0x2A000000u | uint32_t(mem(c)) | (uint32_t(mem(c + 1)) << 8) | (uint32_t(mem(c + 2)) << 16),
                    xy(X + b[0], Y + b[1]), xy(X + b[2], Y + b[3]), xy(X + b[4], Y + b[5]), xy(X + b[6], Y + b[7]), 0, 0, 0, 0}};
        out.packets.push_back(p);
    };
    const auto mode = [&](unsigned abr) {
        FrontendPacket p; p.length = 1; p.bucket = uint16_t(28 + s.ot_layer); p.words[0] = 0xE1000000u | psx_get_tpage(0, abr, 960, 256);
        out.packets.push_back(p);
    };
    f4(bg); mode(0);          // +152: average
    f4(bg + 3); mode(2);      // +156: subtract (drawn first)
}

// 0x80060F84 (k = 0) .. 0x80061224 (k = 3): 31 bubble passes over the 32
// owned part bytes 0x800B5604 (part + 1, 0 = empty); 0x80061304 swaps i and
// i + 1 when i + 1 holds a part and i is empty or its signed stat byte
// 0x8008FE64 + 8 * (part - 1) + k is smaller: descending, empties last.
void MessageWindows::sort_parts_80060f84(int k) {
    auto& a = owned_parts_800b5604;
    const auto stat = [&](uint8_t part) { return int8_t(byte(0x8008FE64u + 8u * uint32_t(part - 1) + uint32_t(k))); };
    for (int pass = 0; pass < 31; ++pass)
        for (int i = 0; i < 31 - pass; ++i) {
            if (!a[size_t(i + 1)]) continue;
            if (!a[size_t(i)] || stat(a[size_t(i)]) < stat(a[size_t(i + 1)])) std::swap(a[size_t(i)], a[size_t(i + 1)]);
        }
    parts_sorted = true;
}

MessageWindowFrame MessageWindows::tick_80053b40(uint16_t held, uint16_t pressed) {
    MessageWindowFrame out;
    ++frame_gp4348;
    if (flags_80098a58 & 0x00400000u) return out;
    flags_80098a58 &= 0x3043FFFFu;
    cursors_gp4808 = 0;
    // 0x800541D4: after each slot (open or not) slots 0..3 OR their cursor
    // (+113, lb) into byte 0x80098B2C + n.
    const auto keep_cursor = [&](int n) {
        if (n < 4) cursors_gp4808 |= uint32_t(int32_t(slots_[size_t(n)].cursor)) << (8 * n);
    };
    for (int n = 0; n < 5; ++n) {
        MessageSlot& s = slots_[size_t(n)];
        if (!(s.flags & 0x80000000u)) { flags_80098a58 &= ~(0x10000u >> s.pad_owner); keep_cursor(n); continue; }
        flags_80098a58 |= 0x80000000u | (kSlotBit >> n);
        if (!(flags_80098a58 & 0x00040000u) && !(flags_80098a58 & (0x10000u >> s.pad_owner))) { s.held = held; s.pressed = pressed; }
        else s.held = s.pressed = 0;
        cur_slot_ = n;
        bool unported = false;
        for (int guard = 0; guard < 4096 && !unported; ++guard) {
            const uint8_t op = mem(s.read);
            bool more;
            if (op < 132) {
                s.timer = int16_t(s.timer - 1);
                if (s.timer > 0) more = false;
                else {
                    const bool fast = !(s.flags & 0x08000000u) && (s.held & 0x1000u) && !(s.flags & 0x00040000u);
                    s.timer = int16_t(fast ? s.speed >> 1 : s.speed);
                    ++s.read;
                    if (!(s.flags & 0x60000000u)) {
                        if (!(s.flags & 0x00040000u)) sound(out, 132, 0);   // 0x8001A008(132, 0, 100)
                        s.flags |= 0x40000000u;
                    }
                    more = s.timer == 0;
                    if (more) s.flags |= 0x40000000u;
                }
            } else more = exec(s, op, out, unported);
            if (more) continue;
            if (s.speed != 0 || (s.flags & 0x10000000u)) break;
        }
        if (unported) { ++out.unported_opcodes; s.flags = 0; keep_cursor(n); continue; }
        // Draw pass.
        s.brightness = 128; s.pen_x = s.x; s.pen_y = int16_t(s.y + s.scroll);
        s.color = s.default_color; s.row_draw = 0; s.draw = s.page;
        for (int guard = 0; s.draw != s.read && guard < 4096; ++guard) {
            const uint8_t op = mem(s.draw);
            if (op >= 132) { draw(s, op, out, n); continue; }
            if (!(s.flags & 0x00100000u)) {
                psxgpu::PolyFt4 q;
                q.code_rgb = 0x2C000000u | grey(s.brightness);
                const int16_t x0 = s.pen_x, y0 = s.pen_y;
                q.x = {{x0, int16_t(x0 + 12), x0, int16_t(x0 + 12)}};
                q.y = {{y0, y0, int16_t(y0 + 12), int16_t(y0 + 12)}};
                const uint8_t u = uint8_t((op % 21) * 12), v = uint8_t((op / 21) * 12);
                q.u = {{u, uint8_t(u + 12), u, uint8_t(u + 12)}};
                q.v = {{v, v, uint8_t(v + 12), uint8_t(v + 12)}};
                q.clut = uint16_t(mem(kMsgClutTable80097f30 + 2u * s.color) | (mem(kMsgClutTable80097f30 + 2u * s.color + 1) << 8));
                q.tpage = kMsgFontTpage;
                FrontendPacket p; p.length = 9; p.bucket = uint16_t(28 + s.ot_layer);
                psxgpu::write_poly_ft4(p.words.data(), q);
                out.packets.push_back(p);
                s.pen_x = int16_t(s.pen_x + mem(kMsgGlyphWidths8008ae7c + op));
            }
            ++s.draw;
        }
        if (s.flags & 0x04000000u) draw_frame(s, out);
        keep_cursor(n);
        s.flags &= 0xAF7FFFFFu;
    }
    // 0x80054240: the font draw mode at OT entry 31.
    FrontendPacket mode; mode.length = 1; mode.bucket = 31; mode.words[0] = 0xE1000000u | kMsgFontTpage;
    out.packets.push_back(mode);
    return out;
}

} // namespace l3
