#include "legends3ds/subscreen_menu.h"
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
FrontendPacket sprt(int x, int y, int w, int h, uint8_t u, uint8_t v, uint16_t clut, uint16_t bucket) {
    psxgpu::PolyFt4 q;
    q.code_rgb = 0x2C808080u;
    q.x = {{int16_t(x), int16_t(x + w), int16_t(x), int16_t(x + w)}};
    q.y = {{int16_t(y), int16_t(y), int16_t(y + h), int16_t(y + h)}};
    q.u = {{u, uint8_t(u + w), u, uint8_t(u + w)}};
    q.v = {{v, v, uint8_t(v + h), uint8_t(v + h)}};
    q.clut = clut; q.tpage = kGaugeTpage;
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

void SubScreenMenu::enter() { page_ = 0; options_sub_ = 0; layouts_.fill(-1); }

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
    std::vector<FrontendPacket> chain;
    // Layout slot OT layer 2 (+5) -> entry 28 + 2 = 30, shared with the status
    // bar; added after it, so drawn first (the cells sit on the frame).
    FrontendPacket mode; mode.length = 1; mode.bucket = 30; mode.words[0] = 0xE1000000u | kGaugeTpage;
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
        chain.push_back(sprt(x, y, w, h, u, vv, clut, 30));
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
            else page_ = 0;   // pages 3..5 not ported: reopen the menu (CANDIDATE)
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
            }                                                  // else 0x80060EC0 (L1/R1 pages: not ported)
            break;
        default: page_ = 0; options_sub_ = 0; break;           // sw zero, 0(s0)
        }
        break;
    case 2: normal_items_8005ed9c(pressed, out); break;
    case 8:                                                    // 0x80060DB8
        windows_.clear_80053a30(); layouts_.fill(-1);
        out.exited = true;
        break;
    default: page_ = 0; break;
    }
    for (const int16_t l : layouts_) if (l >= 0) layout_draw(uint16_t(l), v, out.packets);   // 0x80064060
    windows_.owned_items_800be420 = v.owned_items_800be420;
    windows_.phase_800c1b10 = 4;   // main phase 4: Start may cancel a choice
    auto w = windows_.tick_80053b40(held, pressed);
    for (auto& p : w.packets) out.packets.push_back(p);
    for (auto& s : w.sounds) out.sounds.push_back(s);
    out.unported_opcodes = w.unported_opcodes;
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

// 0x8005ED9C Normal Items (sub-state in options_sub_, jump table 0x80011264).
void SubScreenMenu::normal_items_8005ed9c(uint16_t pressed, SubScreenFrame& out) {
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
        } else if (exit_check_80060e64(pressed, out)) options_sub_ = 7;   // (0x80060EC0 L1/R1 pages: not ported)
        break;
    case 3:
        // Items 0..3 (codes 64..67) have their own use flows (0x8005D95C,
        // 0x80037604, ...): not ported, they take the generic message.
        windows_.open_8005382c(0, kBank801f2000, item_code_ < 88 ? 90 : 91);
        options_sub_ = 5;
        break;
    case 5: case 6:
        if (windows_.flags_80098a58 & 0x08000000u) break;           // window 0 still open
        windows_.flags_80098a58 &= ~0x4000u;
        options_sub_ = 2;
        break;
    default: page_ = 0; options_sub_ = 0; break;                    // 7: loader idle -> menu
    }
}

} // namespace l3
