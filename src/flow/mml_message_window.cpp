#include "legends3ds/mml_message_window.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
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
        if (s.decor != 2 && s.decor != 4) { unported = true; return false; }
        // Decor 2/4 exec 0x8005B85C.
        if (s.decor_state == 0) {
            s.decor_state = 2; s.flags |= 0x06000000u; s.read += 3;
            s.margin = mem(r + 2); flags_80098a58 |= 0x00200000u;
            const int16_t m = int16_t(s.margin), w = int16_t(m + s.width * 12), h = int16_t(m + s.height * 12);
            s.box = {{int16_t(-m), int16_t(-m), w, int16_t(-m), int16_t(-m), h, w, h}};
        } else if (s.decor_state == 2) s.flags = (s.flags | 0x10000000u) & 0xF9FFFFFFu;
        return false;
    }
    case 0x95: return false;                                                 // 0x80054BAC: halt
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
        if (mem(r) == 0xE8 || mem(r) == 0xEA) { unported = true; return false; }
        flags_80098a58 |= 0x40000000u | uint8_t(s.cursor);
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
    case 0xA0: case 0xAE: s.read += 3; return true;                          // 0x800543E0
    case 0xA1: s.flags |= 0x20000000u; s.read += 1; return true;             // 0x8005563C
    case 0xAD: s.repeat = 0; ++s.rows; s.flags |= 0x00404000u; s.read += 7; return true;   // 0x80055CC4
    case 0x96: s.repeat = 0; ++s.rows; s.flags |= 0x00400800u; s.read += 3; return true;   // 0x80054BB4
    case 0xAF: ++s.lines; s.read += 2; return true;                          // 0x80056128
    case 0xDB: s.read += 5; return true;                                     // 0x800543F8
    case 0xED: s.read += 1; return true;                                     // 0x800543B0
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
    case 0x96: s.draw += 3; return;
    case 0xA0: s.pen_x = int16_t(s.pen_x + int16_t(u16(d + 1))); s.draw += 3; return;   // 0x800555F4
    case 0xAE: s.brightness = setting(mem(d + 1)) == mem(d + 2) ? 128 : 64; s.draw += 3; return;   // 0x800560D0
    case 0xAF: s.pen_x = s.x; s.color = s.default_color; s.pen_y = int16_t(s.pen_y + mem(d + 1)); s.draw += 2; return;   // 0x80056148
    case 0xB0: s.brightness = mem(d + 1); s.draw += 2; return;               // 0x80056180
    case 0xDB: s.pen_x = int16_t(u16(d + 1)); s.pen_y = int16_t(u16(d + 3)); s.draw += 5; return;   // 0x80058D64
    case 0xED: s.pen_x = int16_t(s.pen_x + 12); s.draw += 1; return;        // 0x8005A858
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

// Decor 2 frame 0x8005ACA8 with its primitives from 0x8005BCE4.
void MessageWindows::draw_frame(MessageSlot& s, MessageWindowFrame& out) {
    const uint8_t param = mem(kMsgDecorParam8008af18 + s.decor);
    const uint32_t bg = kMsgFrameColors80097f6c + 6u * param;
    const auto& b = s.box;
    const int X = s.x, Y = s.y;
    const auto edge = [&](int i, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {
        FrontendPacket p; p.length = 8; p.bucket = s.ot_layer;
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
        FrontendPacket p; p.length = 5; p.bucket = s.ot_layer;
        p.words = {{0x2A000000u | uint32_t(mem(c)) | (uint32_t(mem(c + 1)) << 8) | (uint32_t(mem(c + 2)) << 16),
                    xy(X + b[0], Y + b[1]), xy(X + b[2], Y + b[3]), xy(X + b[4], Y + b[5]), xy(X + b[6], Y + b[7]), 0, 0, 0, 0}};
        out.packets.push_back(p);
    };
    const auto mode = [&](unsigned abr) {
        FrontendPacket p; p.length = 1; p.bucket = s.ot_layer; p.words[0] = 0xE1000000u | psx_get_tpage(0, abr, 960, 256);
        out.packets.push_back(p);
    };
    f4(bg); mode(0);          // +152: average
    f4(bg + 3); mode(2);      // +156: subtract (drawn first)
}

MessageWindowFrame MessageWindows::tick_80053b40(uint16_t held, uint16_t pressed) {
    MessageWindowFrame out;
    if (flags_80098a58 & 0x00400000u) return out;
    flags_80098a58 &= 0x3043FFFFu;
    for (int n = 0; n < 5; ++n) {
        MessageSlot& s = slots_[size_t(n)];
        if (!(s.flags & 0x80000000u)) { flags_80098a58 &= ~(0x10000u >> s.pad_owner); continue; }
        flags_80098a58 |= 0x80000000u | (kSlotBit >> n);
        if (!(flags_80098a58 & 0x00040000u) && !(flags_80098a58 & (0x10000u >> s.pad_owner))) { s.held = held; s.pressed = pressed; }
        else s.held = s.pressed = 0;
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
        if (unported) { ++out.unported_opcodes; s.flags = 0; continue; }
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
                FrontendPacket p; p.length = 9; p.bucket = s.ot_layer;
                psxgpu::write_poly_ft4(p.words.data(), q);
                out.packets.push_back(p);
                s.pen_x = int16_t(s.pen_x + mem(kMsgGlyphWidths8008ae7c + op));
            }
            ++s.draw;
        }
        if (s.flags & 0x04000000u) draw_frame(s, out);
        s.flags &= 0xAF7FFFFFu;
    }
    // 0x80054240: the font draw mode at OT entry 31.
    FrontendPacket mode; mode.length = 1; mode.bucket = 31; mode.words[0] = 0xE1000000u | kMsgFontTpage;
    out.packets.push_back(mode);
    return out;
}

} // namespace l3
