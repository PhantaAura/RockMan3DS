#include "legends3ds/subscreen_menu.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
constexpr uint16_t kGaugeTpage = 0x000D;   // E1 0x0D: GAUGE01u page (832,0)
constexpr uint16_t kStatusClut = 0x3E75;
constexpr uint32_t kMenuMessageId = 0x32, kOptionMessageId = 0x26;
constexpr uint32_t kLayoutTable8008ecdc = 0x8008ECDC;   // {u16 handler, u16 tpage, u32 sprite list} x n
constexpr uint32_t kStatusFrameList8008e09c = 0x8008E09C;   // layout 6
constexpr uint32_t kBank801f2000 = 0x801F2000;           // STFF_FE.MSG (SUB_KEY.BIN)

// SPRT (0x64, rgb 0x808080) as the equal POLY_FT4, like frontend_tiles_80062368.
FrontendPacket sprt(int x, int y, int w, int h, uint8_t u, uint8_t v, uint16_t clut, uint16_t bucket, uint16_t tpage = kGaugeTpage) {
    psxgpu::PolyFt4 q;
    q.code_rgb = 0x2C808080u;
    q.x = {{int16_t(x), int16_t(x + w), int16_t(x), int16_t(x + w)}};
    q.y = {{int16_t(y), int16_t(y), int16_t(y + h), int16_t(y + h)}};
    // A SPRT reaching the page edge (u + w = 256, e.g. the "Mega Buster"
    // name at u 0xC0, 64 wide) must not wrap to u 0: end at 0xFF.
    const uint8_t u1 = uint8_t(std::min(255, int(u) + w)), v1 = uint8_t(std::min(255, int(v) + h));
    q.u = {{u, u1, u, u1}};
    q.v = {{v, v, v1, v1}};
    q.clut = clut; q.tpage = tpage;
    FrontendPacket p; p.length = 9; p.bucket = bucket;
    psxgpu::write_poly_ft4(p.words.data(), q);
    return p;
}
// A chain built in draw order goes to the OT as one block: in AddPrim order
// (prepend) it is the reverse.
void add_chain(std::vector<FrontendPacket>& out, std::vector<FrontendPacket> chain) {
    for (size_t i = chain.size(); i-- > 0;) out.push_back(chain[i]);
}

// 0x80062028(prim, xy, value, digits | flags): right-to-left 8x8 digits
// 7 px apart, u = 0x60 + 8 * digit, v = 0xB0. Flag 0x100 blanks zero digits
// once the value is used up; 0x200 lets the ones digit be blanked too.
void digits_80062028(std::vector<FrontendPacket>& chain, int x, int y, int32_t value, unsigned arg) {
    const int n = int(arg & 255);
    int px = x + (n - 1) * 7;
    for (int i = n - 1; i >= 0; --i, px -= 7) {
        const int digit = value % 10;
        bool draw;
        if (i == n - 1 && !(arg & 0x200)) draw = true;
        else draw = value != 0 || !(arg & 0x100);
        if (draw) chain.push_back(sprt(px, y, 8, 8, uint8_t(0x60 + 8 * digit), 0xB0, kStatusClut, 30));
        value /= 10;
    }
}
} // namespace

SubScreenMenu::SubScreenMenu(ExeExcerpt exe) : windows_(exe), exe_(std::move(exe)) {
    if (!exe_.at(kStatusFrameList8008e09c)) throw std::runtime_error("sub-screen: EXE excerpt lacks the status frame list");
    if (!exe_.at(exe_.u32(kMsgTable8008cacc + 4u * kMenuMessageId))) throw std::runtime_error("sub-screen: menu message missing");
}

void SubScreenMenu::enter() { page_ = 0; options_sub_ = 0; layouts_.fill(-1); parts_dirty_ = false; }

void SubScreenMenu::status_bar_80061e0c(const SubScreenValues& v, std::vector<FrontendPacket>& out) const {
    std::vector<FrontendPacket> chain;
    FrontendPacket mode; mode.length = 1; mode.bucket = 30; mode.words[0] = 0xE100000Du;
    chain.push_back(mode);
    const int cells = (v.life_max_800b5260 - 1) >> 4;
    for (int i = 0; i <= cells; ++i) {
        const int at = 16 * i;
        uint8_t u = 0x80;                                   // empty
        if (at < v.life_800b521e)
            u = (at < v.life_800b521e - 16 || (v.life_800b521e & 15) == 0) ? 0x70 : 0x78;   // full : half
        chain.push_back(sprt(49 + 5 * i, 36, 8, 8, u, 0xB8, kStatusClut, 30));
    }
    digits_80062028(chain, 164, 36, int32_t(v.zenny_800c1b2c), 0x306);
    const uint32_t t = v.play_time_800c1b1c;
    digits_80062028(chain, 262, 36, int32_t(t / 108000u), 2);          // hours (30 Hz * 3600)
    digits_80062028(chain, 282, 36, int32_t((t / 1800u) % 60u), 2);    // minutes
    add_chain(out, std::move(chain));
}

void SubScreenMenu::status_frame_800640f0(const SubScreenValues& v, std::vector<FrontendPacket>& out) const { layout_draw(6, v, out); }

// 0x800640F0 for layout list n of 0x8008ECDC (sprite records until a
// negative mode), OT layer 2 (+5) = front-end entry 30.
void SubScreenMenu::layout_draw(uint16_t list, const SubScreenValues& v, std::vector<FrontendPacket>& out) const {
    const uint32_t first = exe_.u32(kLayoutTable8008ecdc + 8u * list + 4u);
    const uint8_t* entry = exe_.at(kLayoutTable8008ecdc + 8u * list);
    if (entry && entry[0] == 1) { icons_8006430c(first, v, out); return; }   // handler table 0x8008DD64[1]
    const uint16_t tpage = entry ? uint16_t(entry[2] | (entry[3] << 8)) : kGaugeTpage;   // the E1 page (+2)
    std::vector<FrontendPacket> chain;
    // Layout slot OT layer 2 (+5) -> entry 28 + 2 = 30, shared with the status
    // bar; added after it, so drawn first (the cells sit on the frame).
    FrontendPacket mode; mode.length = 1; mode.bucket = 30; mode.words[0] = 0xE1000000u | tpage;
    chain.push_back(mode);
    for (uint32_t r = first; ; r += 16) {
        const uint8_t* p = exe_.at(r);
        if (!p || int16_t(uint16_t(p[0] | (p[1] << 8))) < 0) break;
        const int8_t m = int8_t(p[1]);
        const auto u16 = [&](unsigned k) { return uint16_t(p[k] | (p[k + 1] << 8)); };
        int x = int16_t(u16(4)), y = int16_t(u16(6)), w = u16(12), h = u16(14);
        uint8_t u = p[8]; const uint8_t vv = p[9]; const uint16_t clut = u16(10);
        if (m == 1 && (frame_ & 16)) continue;                                   // blink
        if (m == 2 && (((v.play_time_800c1b1c / 15u) % 60u) & 1u)) continue;    // TIME colon
        if (m == 3 && v.life_max_800b5260 >= 81) {                               // LIFE box grows
            const int k = 5 * ((v.life_max_800b5260 - 65) >> 4);
            u = uint8_t(u - k); w += k;
        }
        chain.push_back(sprt(x, y, w, h, u, vv, clut, 30, tpage));
    }
    add_chain(out, std::move(chain));
}

SubScreenFrame SubScreenMenu::update(uint16_t held, uint16_t pressed, const SubScreenValues& v) {
    SubScreenFrame out;
    ++frame_; ++scroll_1f800002;
    for (const auto& p : frontend_tiles_80062368(scroll_1f800002)) out.packets.push_back(p);
    const bool chose = windows_.flags_80098a58 & 0x40000000u;
    switch (page_) {
    case 0:                                                    // 0x8005EC80
        windows_.clear_80053a30();
        windows_.open_8005382c(0, exe_.u32(kMsgTable8008cacc + 4u * kMenuMessageId), 255);
        layouts_[0] = 6;                                       // 0x80063FC0(0, 0x20006)
        status_bar_80061e0c(v, out.packets);
        page_ = 1;
        break;
    case 1:                                                    // 0x8005ECE0
        status_bar_80061e0c(v, out.packets);
        if (chose) {
            const uint8_t c = uint8_t(windows_.flags_80098a58);
            if (c == 4) { page_ = 6; options_sub_ = 0; }
            else if (c == 5) page_ = 8;
            else if (c == 0) { page_ = 2; options_sub_ = 0; }
            else if (c == 1) { page_ = 3; options_sub_ = 0; }
            else if (c == 2) { page_ = 4; options_sub_ = 0; }
            else if (c == 3) { page_ = 5; options_sub_ = 0; }
            else page_ = 0;
        } else if (pressed & 0x0008u) page_ = 8;               // Start (Select page 7 not ported)
        break;
    case 6:                                                    // 0x80060B00
        switch (options_sub_) {
        case 0: windows_.clear_80053a30(); layouts_.fill(-1); ++options_sub_; break;   // + loader 0x8001D440(204)
        case 1: windows_.open_8005382c(0, exe_.u32(kMsgTable8008cacc + 4u * kOptionMessageId), 255); ++options_sub_; break;
        case 2:
            if (chose || (pressed & 0x0008u)) {
                if (pressed & 0x0008u) out.sounds.push_back({{131, 1}});
                windows_.clear_80053a30();
                out.settings_applied = true;                   // 0x800425FC(player, 0x800C1B64)
                ++options_sub_;
            } else pages_80060ec0(pressed, out);              // 0x80060C34
            break;
        default: page_ = 0; options_sub_ = 0; break;           // sw zero, 0(s0)
        }
        break;
    case 2: normal_items_8005ed9c(pressed, v, out); break;
    case 3: special_items_8005f210(pressed, v, out); break;
    case 4: buster_parts_800600cc(pressed, v, out); break;
    case 5: special_weapons_80060618(pressed, v, out); break;
    case 8:                                                    // 0x80060DB8
        windows_.clear_80053a30(); layouts_.fill(-1);
        out.exited = true;
        break;
    default: page_ = 0; break;
    }
    for (const int16_t l : layouts_) if (l >= 0) layout_draw(uint16_t(l), v, out.packets);   // 0x80064060
    windows_.owned_items_800be420 = v.owned_items_800be420;
    windows_.story_flag = v.story_flag;
    windows_.equipped_parts_800b5600 = v.equipped_parts_800b5600;
    if (!parts_dirty_) windows_.owned_parts_800b5604 = v.owned_parts_800b5604;   // a Sort keeps its order
    windows_.parts_sorted = false;
    windows_.phase_800c1b10 = 4;   // main phase 4: Start may cancel a choice
    auto w = windows_.tick_80053b40(held, pressed);
    for (auto& p : w.packets) out.packets.push_back(p);
    for (auto& s : w.sounds) out.sounds.push_back(s);
    out.unported_opcodes = w.unported_opcodes;
    if (windows_.parts_sorted) { parts_dirty_ = true; out.parts_sorted = true; }
    out.owned_parts_800b5604 = windows_.owned_parts_800b5604;
    for (const auto& [slot, list] : w.layout_calls)
        if (slot >= 0 && slot < 4) layouts_[size_t(slot)] = list;
    return out;
}

// 0x80060E64: Triangle or Start leaves an item page (sound 131, windows and
// layouts cleared, loader 0x8001D440(22) = EXIT_SUB.BIN, not ported).
bool SubScreenMenu::exit_check_80060e64(uint16_t pressed, SubScreenFrame& out) {
    if (!(pressed & 0x1008u)) return false;
    out.sounds.push_back({{131, 1}});
    windows_.clear_80053a30(); layouts_.fill(-1);
    return true;
}

// 0x80060EC0: L1 / R1 (pressed 0x400 / 0x800, edge word 0x800C0C2A) step
// the item pages 2..6 (L1 first, wrapping 2 <-> 6): sound 130, windows and
// layouts cleared, ctx +1 (sub-state) and +2..3 zeroed.
void SubScreenMenu::pages_80060ec0(uint16_t pressed, SubScreenFrame& out) {
    if (!(pressed & 0x0C00u)) return;
    out.sounds.push_back({{130, 1}});
    windows_.clear_80053a30(); layouts_.fill(-1);
    if (pressed & 0x0400u) { if (int8_t(--page_) < 2) page_ = 6; }
    else if (int8_t(++page_) >= 7) page_ = 2;
    options_sub_ = 0; ctx_[2] = ctx_[3] = 0;
}

// 0x8006430C: the special item icons of a layout list (see MESSAGE_WINDOW.md).
void SubScreenMenu::icons_8006430c(uint32_t first, const SubScreenValues& v, std::vector<FrontendPacket>& out) const {
    std::vector<FrontendPacket> chain;
    FrontendPacket mode; mode.length = 1; mode.bucket = 30; mode.words[0] = 0xE100000Cu;
    chain.push_back(mode);
    const auto flag = [&](int f) { return v.story_flag && f >= 0 ? int(v.story_flag(uint16_t(f))) : 0; };
    for (uint32_t r = first; ; r += 16) {
        const uint8_t* p = exe_.at(r);
        if (!p || int16_t(uint16_t(p[0] | (p[1] << 8))) < 0) break;
        const auto s16 = [&](unsigned k) { return int16_t(uint16_t(p[k] | (p[k + 1] << 8))); };
        const int cell = p[0], m = int8_t(p[1]), f = s16(2);
        const int x = cell % 6 * 44 + s16(4) + 34, y = cell / 6 * 40 + s16(6) + 60;
        const int w = s16(12), h = s16(14);
        const int s4 = flag(f), n = s4 + flag(f + 1) + flag(f + 2);
        uint8_t u = p[8], vv = p[9]; uint16_t clut = uint16_t(p[10] | (p[11] << 8));
        bool draw = false, box = false;
        switch (m) {
        case 0: case 13: draw = s4 != 0; box = !s4 && m == 0; break;
        case 1: draw = n != 0; box = !n; break;
        case 2: draw = n != 0; break;
        case 3: if (n) { draw = true; u = uint8_t(u + (n - 1) * 8); } break;
        case 4: if (n) { draw = true; u = uint8_t(u + (n - 1) * 32); } else box = true; break;
        case 5: case 7: { const int c = m == 5 ? v.byte_800c1b8c : v.byte_800c1b8d;
                          if (s4 && c >= 10) { draw = true; u = uint8_t(u + c / 10 * 8); } break; }
        case 6: case 8: { const int c = m == 6 ? v.byte_800c1b8c : v.byte_800c1b8d;
                          if (s4) { draw = true; u = uint8_t(u + c % 10 * 8); } break; }
        case 9:  draw = s4 && int8_t(v.byte_800b531a) != 0; break;
        case 10: draw = s4 && (v.byte_800b531b & 2); break;
        case 11: draw = s4 && (v.byte_800b531b & 1); break;
        case 12: draw = s4 && int8_t(v.byte_800b531c) != 0; break;
        default: break;
        }
        if (box) { u = 0x80; vv = 0x40; clut = 0x3C48; }   // 0x3C484080: the empty slot
        if (!draw && !box) continue;
        psxgpu::PolyFt4 q;
        q.code_rgb = 0x2C808080u;
        q.x = {{int16_t(x), int16_t(x + w), int16_t(x), int16_t(x + w)}};
        q.y = {{int16_t(y), int16_t(y), int16_t(y + h), int16_t(y + h)}};
        q.u = {{u, uint8_t(u + w), u, uint8_t(u + w)}};
        q.v = {{vv, vv, uint8_t(vv + h), uint8_t(vv + h)}};
        q.clut = clut; q.tpage = 0x000C;
        FrontendPacket pk; pk.length = 9; pk.bucket = 30;
        psxgpu::write_poly_ft4(pk.words.data(), q);
        chain.push_back(pk);
    }
    add_chain(out, std::move(chain));
}

// 0x8005F210 Special Items: the grid (message 26) and icons (layout 2). The
// use dispatch (table 0x80011284, story events) is not ported: a pick
// returns to the grid (CANDIDATE).
void SubScreenMenu::special_items_8005f210(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out) {
    const bool chose = windows_.flags_80098a58 & 0x40000000u;
    switch (options_sub_) {
    case 0: windows_.clear_80053a30(); layouts_.fill(-1); ++options_sub_; break;   // + loader 0x8001D440(204)
    case 1:
        windows_.open_8005382c(1, kBank801f2000, 25);              // "Special Items" -> grid 26
        layouts_[0] = 2;                                            // 0x80063FC0(0, 0x20002)
        ++options_sub_;
        break;
    case 2:
        if (chose) { ++options_sub_; item_code_ = uint8_t(windows_.cursors_gp4808 >> 16); }   // byte 0x80098B2E
        else if (exit_check_80060e64(pressed, out)) { page_ = 0; options_sub_ = 0; }   // sw zero, 0(s0)
        else pages_80060ec0(pressed, out);                         // 0x8005F31C
        break;
    case 3: special_use_80011284(v, out); break;                   // 0x8005F32C
    default: options_sub_ = 2; break;
    }
}

// 0x8005F32C: the use dispatch (table 0x80011284, code = item_code_, step =
// ctx +2). Every handler ends with "wait for window 0 to close, then sub 2".
void SubScreenMenu::special_use_80011284(const SubScreenValues& v, SubScreenFrame& out) {
    uint8_t& step = ctx_[2];
    const auto flag = [&](int f) { return v.story_flag && v.story_flag(uint16_t(f)); };
    const auto open = [&](uint8_t id) { windows_.open_8005382c(0, kBank801f2000, id); };
    const auto snd = [&](uint8_t id) { out.sounds.push_back({{id, 1}}); };
    const auto closed = [&] { return !(windows_.flags_80098a58 & 0x08000000u); };
    const auto done = [&] { options_sub_ = 2; step = 0; };
    const uint8_t* map = exe_.at(0x8008DAF4u + item_code_);              // code -> owned flag 1312 + n
    const int own = map ? 1312 + *map : -1;
    const uint8_t c = item_code_;
    if (c >= 18) return;
    if (c == 0) {                                                        // 0x8005F3D0: life from the reserve
        const int life = v.life_800b521e, max = v.life_max_800b5260, res = v.byte_800c1b8c;
        if (step == 0) {
            if (!flag(1316)) { snd(131); step += 2; }
            else if (life == max) { snd(130); open(64); step += 2; }
            else if (res == 0) { snd(130); open(65); step += 2; }
            else { open(66); step += 1; }
        } else if (step == 1) {
            const uint32_t g = windows_.flags_80098a58;
            if (!(g & 0x40000000u)) return;
            if (!uint8_t(g)) {
                const int need = (max - (uint16_t(life) & 0xF0)) >> 4;
                if (res < need) { open(68); out.life_800b521e = (uint16_t(life) & 0xF0) + res * 16; out.byte_800c1b8c = 0; }
                else { open(67); out.byte_800c1b8c = res - need; out.life_800b521e = max; }
            }
            step += 1;
        } else if (step == 2 && closed()) done();
        return;
    }
    if (c <= 4) {                                                        // 0x8005F608: equipment toggles
        const bool on1 = v.byte_800b531a != 0, on2 = v.byte_800b531b & 2, on3 = v.byte_800b531b & 1, on4 = v.byte_800b531c != 0;
        if (step == 0) {
            if (!flag(own)) { snd(131); step += 2; return; }
            open(c == 1 ? (on1 ? 51 : 50) : c == 2 ? (on2 ? 57 : 56) : c == 3 ? (on3 ? 53 : 52) : (on4 ? 73 : 72));
            step += 1;
        } else if (step == 1) {
            const uint32_t g = windows_.flags_80098a58;
            if (!(g & 0x40000000u)) return;
            if (!uint8_t(g)) {
                if (c == 1) { open(on1 ? 61 : 60); out.byte_800b531a = v.byte_800b531a ^ 1; }
                else if (c == 2) { open(on2 ? 63 : 62); out.byte_800b531b = v.byte_800b531b ^ 2; }
                else if (c == 3) { open(on3 ? 55 : 54); out.byte_800b531b = v.byte_800b531b ^ 1; }
                else if (on4) { open(75); out.byte_800b531c = 0; }
                else { open(74); out.byte_800b531c = flag(1338) + flag(1339) + flag(1340); }
            }
            step += 1;
        } else if (step == 2 && closed()) done();
        return;
    }
    if (c == 12) {                                                       // 0x8005FBB4: the warp item (table 0x800112CC)
        if (step == 0) {
            if (flag(own)) { snd(130); open(76); step += 1; }
            else { snd(131); done(); }
        } else if (step == 1) {
            int dest = -1, sub = 0;
            const int r = warp_target_8005fde4(v, dest, sub);
            if (r == 0) { open(77); warp_ = int16_t(dest); warp_sub_ = int16_t(sub); step += 2; }
            else { open(r == 1 ? 78 : 79); step += 1; }
        } else if (step == 2) { if (closed()) done(); }
        else if (step == 3) {
            // 0x8005FD18: once closed, 0x8003B140, area bytes +80/+81 <- +121/+122,
            // 0x80017C30(23), 0x8001D854(16) (not ported: the caller warps).
            if (closed()) { out.warp_800c1b60 = warp_; out.warp_800c1b61 = warp_sub_; step += 1; }
        } else if (step == 4) {
            // 0x8005FD6C: stage reload calls, then 0x800C1B10 = 5, +1 = 2 (leave the sub-screen).
            windows_.clear_80053a30(); layouts_.fill(-1);
            out.exited = true; page_ = 0; options_sub_ = 0; step = 0;
        }
        return;
    }
    if (step == 0) {                                                     // 0x8005FAE4 / 0x8005FFBC: info
        bool have;
        uint8_t id;
        if (c >= 14) { have = flag(own) || flag(own + 1) || flag(own + 2); id = 71; }
        else { have = flag(own); id = c == 5 ? 69 : c <= 8 ? 70 : 71; ctx_[5] = id; }
        if (have) { snd(130); open(id); } else snd(131);
        step += 1;
    } else if (closed()) done();
}

// 0x8005FDE4 on 0x800C1B10: 0 = warp to +121 / +122, 1 = not here, 2 = not
// yet (byte +120 0, flag missing, or the sub-area does not allow it). Table
// 0x800112E4 by area (+80) - 3.
int SubScreenMenu::warp_target_8005fde4(const SubScreenValues& v, int& dest, int& sub) const {
    const auto flag = [&](int f) { return v.story_flag && v.story_flag(uint16_t(f)); };
    if (!v.byte_800c1b88) return 2;
    const int area = v.area_800c1b60, s81 = v.area_800c1b61;
    switch (area) {
    case 3: case 27:
        if (!flag(792) || s81 >= 3) return 2;
        dest = flag(85) ? 27 : 3; sub = 1; return 0;
    case 5: if (!flag(793)) return 2; dest = 5; sub = 0; return 0;
    case 6: case 15: if (!flag(794)) return 2; dest = 6; sub = 0; return 0;
    case 8:
        if (!flag(795)) return 2;
        if (s81 >= 3 && s81 != 5) return 2;
        dest = 8; sub = 0; return 0;
    case 12: if (!flag(796) || s81 != 0) return 2; dest = 12; sub = 0; return 0;
    case 18: if (!flag(799)) return 2; dest = 18; sub = 0; return 0;
    case 25: if (!flag(797) || s81 < 2) return 2; dest = 25; sub = 3; return 0;
    default: return 1;
    }
}

// 0x800605DC: stat targets (0x8006147C) then the animated gauges
// (0x800602A4 mode 0 -> 8 x 0x80061A2C). The "with part" targets use the
// same equipped parts (0x80061740 is not traced: no candidate part, CANDIDATE).
void SubScreenMenu::gauges_800605dc(const SubScreenValues& v, std::vector<FrontendPacket>& out) {
    static constexpr int kNow[4] = {17, 25, 33, 41}, kWith[4] = {20, 28, 36, 44}, kMax[4] = {70, 70, 70, 40};
    int now[4] = {0, 0, 0, 0};
    const int slots = (v.story_flag && v.story_flag(0x526)) ? 3 : 2;
    for (int i = 0; i < slots; ++i) {
        const uint8_t part = v.equipped_parts_800b5600[size_t(i)];
        if (!part) continue;
        const uint8_t* st = exe_.at(uint32_t(0x8008FE64 + 8 * (part - 1)));
        if (!st) continue;
        for (int k = 0; k < 4; ++k) now[k] += 10 * int8_t(st[k]);
    }
    for (int k = 0; k < 4; ++k) {
        const int t = std::clamp(now[k], 0, kMax[k]);
        ctx_[size_t(kNow[k])] = ctx_[size_t(kWith[k])] = uint8_t(t);
    }
    animate_800602a4();
    static constexpr int kTriplets[10] = {16, 19, 24, 27, 32, 35, 40, 43, 48, 51};
    std::vector<FrontendPacket> chain;
    const auto quad = [&](int x0, int x1, int y0, int y1, uint8_t u0, uint8_t u1, uint8_t v0, uint8_t v1, uint16_t clut) {
        psxgpu::PolyFt4 q; q.code_rgb = 0x2C808080u;
        q.x = {{int16_t(x0), int16_t(x1), int16_t(x0), int16_t(x1)}};
        q.y = {{int16_t(y0), int16_t(y0), int16_t(y1), int16_t(y1)}};
        q.u = {{u0, u1, u0, u1}}; q.v = {{v0, v0, v1, v1}};
        q.clut = clut; q.tpage = 0x000C;
        FrontendPacket p; p.length = 9; p.bucket = 30; psxgpu::write_poly_ft4(p.words.data(), q);
        chain.push_back(p);
    };
    for (int row = 0; row < 4; ++row)
        for (int half = 0; half < 2; ++half) {                               // 0x80061A2C(203, y, value, half)
            const int x = 203, y = 40 + 10 * row, value = ctx_[size_t(kTriplets[2 * row + half] + 2)];
            const int y0 = y + 2 * half, y1 = y0 + 5;
            const uint8_t v0 = uint8_t(144 + 8 * half), v1 = uint8_t(149 + 8 * half);
            quad(x, x + value + 6, y0, y1, 96, 97, v0, v1, 0x3E83);           // body
            quad(x + value + 6, x + value + 10, y0, y1, 111, 115, v0, v1, 0x3E83);   // cap
            if (half == 0 && ((y != 70 && value == 70) || (y == 70 && value == 40)))   // "max"
                quad(value == 70 ? x + 24 : x + 10, (value == 70 ? x + 24 : x + 10) + 24, y, y + 6, 0x28, 0x40, 0xF0, 0xF6, 0x3E82);
        }
    add_chain(out, std::move(chain));
}

// 0x800602A4: shown = target - (target - from) * n / 4 while n (+6) counts down.
void SubScreenMenu::animate_800602a4() {
    static constexpr int kTriplets[10] = {16, 19, 24, 27, 32, 35, 40, 43, 48, 51};
    const int n = int8_t(ctx_[6]);
    for (int t : kTriplets) {
        const int from = ctx_[size_t(t)], target = ctx_[size_t(t + 1)];
        ctx_[size_t(t + 2)] = uint8_t(n ? target - (((target - from) * n) >> 2) : target);
    }
    if (n) ctx_[6] = uint8_t(n - 1);
}

// 0x80061840: a weapon's five stats (attack, energy, range, rapid, special)
// from table 0x8008D7E8 (20 bytes per class 0x8008DB08[id], 4 per stat,
// indexed by the weapon's level byte); 255 = INFINITY (88 + flag).
void SubScreenMenu::weapon_stats_80061840(int weapon, const SubScreenValues& v) {
    static constexpr int kTarget[5] = {17, 25, 33, 41, 49}, kFlag[5] = {22, 30, 38, 46, 54};
    for (int k = 0; k < 5; ++k) {
        int value = 0, inf = 0;
        if (weapon >= 0 && weapon < 14) {
            const uint8_t* cls = exe_.at(uint32_t(0x8008DB08 + 2 * weapon));
            const int idx = cls ? (cls[0] | (cls[1] << 8)) : 0;
            const uint8_t* b = exe_.at(uint32_t(0x8008D7E8 + idx * 20 + 4 * k + v.weapon_levels[size_t(weapon)][size_t(k)]));
            value = b ? *b : 0;
            if (value == 255) { value = 88; inf = 1; }
        }
        ctx_[size_t(kTarget[k])] = uint8_t(value); ctx_[size_t(kFlag[k])] = uint8_t(inf);
    }
}

// 0x800602A4 mode 1: five 0x80061C50(164, 158 + 10k, shown, infinity) bars.
void SubScreenMenu::bars_800602a4_mode1(std::vector<FrontendPacket>& out) {
    animate_800602a4();
    static constexpr int kShown[5] = {18, 26, 34, 42, 50}, kFlag[5] = {22, 30, 38, 46, 54};
    std::vector<FrontendPacket> chain;
    const auto quad = [&](int x0, int x1, int y0, int y1, uint8_t u0, uint8_t u1, uint8_t v0, uint8_t v1, uint16_t clut) {
        psxgpu::PolyFt4 q; q.code_rgb = 0x2C808080u;
        q.x = {{int16_t(x0), int16_t(x1), int16_t(x0), int16_t(x1)}};
        q.y = {{int16_t(y0), int16_t(y0), int16_t(y1), int16_t(y1)}};
        q.u = {{u0, u1, u0, u1}}; q.v = {{v0, v0, v1, v1}};
        q.clut = clut; q.tpage = 0x000C;
        FrontendPacket p; p.length = 9; p.bucket = 30; psxgpu::write_poly_ft4(p.words.data(), q);
        chain.push_back(p);
    };
    for (int k = 0; k < 5; ++k) {
        const int x = 164, y = 158 + 10 * k, value = ctx_[size_t(kShown[k])];
        quad(x, x + value, y, y + 8, 0x48, 0x49, 0x90, 0x98, 0x3E83);         // body
        quad(x + value, x + value + 4, y, y + 8, 0x4C, 0x50, 0x90, 0x98, 0x3E83);   // cap
        if (ctx_[size_t(kFlag[k])] && value == 88)                            // INFINITY
            quad(x + 24, x + 64, y, y + 8, 0x28, 0x50, 0xF8, 0xFF, 0x3E82);
    }
    add_chain(out, std::move(chain));
}

// 0x80060618 Special Weapons (jump table 0x8001134C). Equip changes the arm
// bytes (+1103 / +368) only in this menu's copy (no weapons in the port yet);
// 0x8005C218 and the "next" arrow are not ported.
void SubScreenMenu::special_weapons_80060618(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out) {
    const uint32_t g = windows_.flags_80098a58;
    const bool chose = g & 0x40000000u;
    const auto bars = [&] { bars_800602a4_mode1(out.packets); };
    switch (options_sub_) {
    case 0:
        windows_.clear_80053a30(); layouts_.fill(-1); ctx_[5] = 0;      // + loader 0x8001D440(205)
        if (!arm_dirty_) { arm_368_ = v.arm_368; flag_1103_ = v.flag_1103; }
        ++options_sub_;
        break;
    case 1:
        windows_.open_8005382c(0, kBank801f2000, arm_368_ ? 92 : 93);
        if (!(windows_.flags_80098a58 & 0x04000000u)) windows_.open_8005382c(1, kBank801f2000, 47);
        if (v.weapon_370 == 0) layouts_[0] = 28;
        layouts_[1] = 9; layouts_[2] = 23; layouts_[3] = 8;
        weapon_stats_80061840(0, v);
        bars();
        ++options_sub_;
        break;
    case 2:
        if (chose) {
            const uint8_t c = uint8_t(g);
            if (c == 0) { out.sounds.push_back({{130, 1}}); options_sub_ = 3; }
            else if (c == 1) {
                if (v.weapon_370 == 0) windows_.open_8005382c(0, kBank801f2000, 97);
                else if (v.energy_1102 <= 0) windows_.open_8005382c(0, kBank801f2000, 96);
                else if (flag_1103_ == 0) { windows_.open_8005382c(0, kBank801f2000, 94); flag_1103_ = 1; arm_368_ = v.weapon_370; arm_dirty_ = true; }
                else { windows_.open_8005382c(0, kBank801f2000, 95); flag_1103_ = 0; arm_368_ = 0; arm_dirty_ = true; }
                options_sub_ = 5;
            } else if (c == 2) {
                out.sounds.push_back({{131, 1}});
                windows_.clear_80053a30(); layouts_.fill(-1);               // + loader 22
                options_sub_ = 6;
            }
        }
        bars();
        break;
    case 3: {                                                               // browse the weapon grid (slot 1)
        const int cursor = uint8_t(windows_.cursors_gp4808 >> 8);         // byte 0x80098B2D (slot 1 cursor)
        const int id = cursor ? cursor + 1 : 0;
        ctx_[4] = uint8_t(cursor); ctx_[5] = uint8_t(id);
        const bool owned = cursor < 13 && v.story_flag && v.story_flag(uint16_t(1216 + id));
        if (owned) {
            layouts_[1] = int16_t(9 + cursor);
            layouts_[0] = v.weapon_370 == id ? 28 : -1;
            weapon_stats_80061840(id, v);
        } else {
            ctx_[5] = 0; layouts_[1] = 22; layouts_[0] = -1;
            weapon_stats_80061840(-1, v);
        }
        if (windows_.slot_bits_gp4812 & 0x40000u) {                        // 0x80060248
            for (int t : {16, 19, 24, 27, 32, 35, 40, 43, 48, 51}) ctx_[size_t(t)] = ctx_[size_t(t + 2)];
            ctx_[6] = 3;
        }
        if (chose) {
            if (cursor == 13) { out.sounds.push_back({{130, 1}}); options_sub_ = 5; }
            else if (owned) { out.sounds.push_back({{130, 1}}); windows_.open_8005382c(0, kBank801f2000, uint8_t(228 + cursor)); options_sub_ = 4; }
            else out.sounds.push_back({{131, 1}});
        } else if (pressed & 0x1008u) { out.sounds.push_back({{131, 1}}); options_sub_ = 5; }
        else pages_80060ec0(pressed, out);                                  // 0x80060A60
        bars();
        break;
    }
    case 4: if (!(windows_.flags_80098a58 & 0x08000000u)) options_sub_ = 3; bars(); break;
    case 5: if (!(windows_.flags_80098a58 & 0x08000000u)) { options_sub_ = 1; ctx_[2] = 0; } bars(); break;
    default: page_ = 0; options_sub_ = 0; break;                           // 6: loader idle -> menu
    }
}

// 0x800600CC Buster Parts (message 29 -> 30, header layout 7, gauges). The
// Equip list / Sort work on the port's parts (none yet); 0x80061C50 (the
// equip list's part bars) and 0x80062118 are not ported.
void SubScreenMenu::buster_parts_800600cc(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out) {
    switch (options_sub_) {
    case 0: windows_.clear_80053a30(); layouts_.fill(-1); ++options_sub_; break;   // + loader 0x8001D440(205)
    case 1:
        windows_.open_8005382c(0, kBank801f2000, 29);
        layouts_[2] = 7;                                                    // 0x80063FC0(2, 0x20007)
        gauges_800605dc(v, out.packets);
        ++options_sub_;
        break;
    case 2: {
        const uint32_t g = windows_.flags_80098a58;
        if ((g & 0x480000FFu) == 0x48000002u) { ++options_sub_; break; }   // Close (+ loader 22)
        if (!(g & 0x08000000u)) {
            const uint32_t w = windows_.slot_bits_gp4812;
            if ((w & 0x10000u) ? (w & 0x40000u) : (w & 0x80000u)) {        // 0x80060248: re-snapshot
                for (int t : {16, 19, 24, 27, 32, 35, 40, 43, 48, 51}) ctx_[size_t(t)] = ctx_[size_t(t + 2)];
                ctx_[6] = 3;
            }
        }
        gauges_800605dc(v, out.packets);
        pages_80060ec0(pressed, out);                                       // 0x8006020C
        break;
    }
    default: page_ = 0; options_sub_ = 0; break;
    }
}

// 0x8005ED9C Normal Items (sub-state in options_sub_, jump table 0x80011264).
// 0x80037604 for the weapon on hand (+370): the energy of its level byte
// (+1269 + 8w) from the level table 0x80089EAC[w] (+2 of an 8-byte entry), or
// -1 when 0x8008D7EC + 20 * class(0x8008DB08[w]) + level holds 255.
int SubScreenMenu::weapon_energy_max_80037604(const SubScreenValues& v) const {
    const int w = v.weapon_370;
    if (w < 0 || w >= 14) return -1;
    const int lvl = v.weapon_levels[size_t(w)][1];
    const uint8_t* c = exe_.at(0x8008DB08u + 2u * uint32_t(w));
    if (!c) return -1;
    const uint8_t* gate = exe_.at(uint32_t(0x8008D7EC + (c[0] | (c[1] << 8)) * 20 + lvl));
    if (!gate || *gate == 255) return -1;
    if (!exe_.at(0x80089EACu + 4u * uint32_t(w))) return -1;
    const uint8_t* e = exe_.at(exe_.u32(0x80089EACu + 4u * uint32_t(w)) + 8u * uint32_t(lvl) + 2u);
    return e ? int(int16_t(uint16_t(e[0] | (e[1] << 8)))) : -1;
}

void SubScreenMenu::normal_items_8005ed9c(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out) {
    const bool chose = windows_.flags_80098a58 & 0x40000000u;
    switch (options_sub_) {
    case 0: windows_.clear_80053a30(); layouts_.fill(-1); ++options_sub_; break;   // + loader 0x8001D440(204)
    case 1: windows_.open_8005382c(1, kBank801f2000, 20); ++options_sub_; break;   // "Normal Items" -> list 21
    case 2:
        if (chose) {
            item_code_ = uint8_t(windows_.slot_bits_gp4812);          // byte 0x80098B30
            if (item_code_ != 255) {
                out.sounds.push_back({{130, 1}});
                windows_.flags_80098a58 |= 0x4000u;
                ++options_sub_;
            } else out.sounds.push_back({{131, 1}});
        } else if (exit_check_80060e64(pressed, out)) options_sub_ = 7;
        else pages_80060ec0(pressed, out);                            // 0x8005EED0
        break;
    case 3: {                                                       // 0x8005EEE0: per item
        uint8_t id = item_code_ < 88 ? 90 : 91;
        bool ask = false;
        if (item_code_ == 64) { ask = v.byte_800bbd87 != 0; id = ask ? 80 : 88; }   // 0x8005D95C
        else if (item_code_ == 65) {
            // Weapon energy: class 0x8008DB08[w], level byte +1269 + 8w; 255 in
            // 0x8008D7EC + 20 * class + level = no refill (59); full (58); else ask (82).
            const int w = v.weapon_370;
            const int max = weapon_energy_max_80037604(v);
            if (max < 0) id = 59;
            else if (max == v.weapon_energy_1274[size_t(w)]) id = 58;
            else { id = 82; ask = true; }
        } else if (item_code_ == 66) { id = 84; ask = true; }
        else if (item_code_ == 67) { id = 86; ask = true; }
        windows_.open_8005382c(0, kBank801f2000, id);
        options_sub_ = ask ? 4 : 5;
        break;
    }
    case 4: {                                                       // 0x8005F074: yes / no
        const uint32_t g = windows_.flags_80098a58;
        if (!(g & 0x40000000u)) break;
        if (uint8_t(g)) { options_sub_ = 5; break; }                 // No
        if (item_code_ == 64) { windows_.open_8005382c(0, kBank801f2000, 81); out.item_used = 64; used_flag_ = 1344; }   // + 0x8005D9C4
        else if (item_code_ == 65) {
            windows_.open_8005382c(0, kBank801f2000, 83);           // 0x80037594: energy = max
            out.item_used = 65; out.weapon_energy_1274 = int16_t(weapon_energy_max_80037604(v)); used_flag_ = 1345;
        } else if (item_code_ == 66) { windows_.open_8005382c(0, kBank801f2000, 85); out.item_used = 66; out.timer_800bbd96 = 1800; used_flag_ = 1346; }
        else if (item_code_ == 67) { windows_.open_8005382c(0, kBank801f2000, 87); out.item_used = 67; out.timer_800bbd94 = 900; used_flag_ = 1347; }
        options_sub_ = 6;
        break;
    }
    case 5: case 6:                                                 // 0x8005F188
        if (windows_.flags_80098a58 & 0x08000000u) break;           // window 0 still open
        if (options_sub_ == 6) out.clear_story_flag = int16_t(used_flag_);   // 0x8001DCD0(ctx +12): the item is used up
        windows_.flags_80098a58 &= ~0x4000u;
        options_sub_ = 2;
        break;
    default: page_ = 0; options_sub_ = 0; break;                    // 7: loader idle -> menu
    }
}

} // namespace l3
