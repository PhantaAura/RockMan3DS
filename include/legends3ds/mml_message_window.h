#pragma once
#include "legends3ds/frontend_title.h"
#include <array>
#include <functional>
#include <cstdint>
#include <vector>

// The ROCK_NEO message windows (docs/research_01516/MESSAGE_WINDOW.md):
// five 196-byte slots at 0x800BB6B8, opened by 0x8005382C, run once per
// frame by 0x80053B40 (an execute pass through the exec table 0x8008A91C,
// then a draw pass through the draw table 0x8008AAC4 from the page start to
// the read pointer). The byte stream is the original message data read from
// a copy of the EXE (it is modified in place: the row cursor byte). Only the
// opcodes the OPTION menu (0x8008B69C) and its help/title messages use are
// ported; any other opcode stops the slot and is counted.
namespace l3 {

// EXE excerpt the cook (tools/cook_option_menu.py) copies from the owner's
// ROCK_NEO.EXE: "L3EX0001", u32 count, then count x {u32 addr, u32 bytes,
// data}, then u32 crc32 of everything before it.
struct ExeExcerpt {
    struct Range { uint32_t addr = 0; std::vector<uint8_t> bytes; };
    std::vector<Range> ranges;
    // Byte at a mapped address, or nullptr.
    uint8_t* at(uint32_t addr);
    const uint8_t* at(uint32_t addr) const;
    uint32_t u32(uint32_t addr) const;   // throws when unmapped
};
ExeExcerpt load_exe_excerpt(const std::vector<uint8_t>& file);
std::vector<uint8_t> cook_exe_excerpt(const ExeExcerpt& excerpt);

// The OPTION settings bytes, where the original stores them (pointer table
// 0x8008D0D4 indices 2..6; VERIFIED_1TO1 from the 0xAD row handler
// 0x80055CFC). 0 is the first choice of each row.
struct OptionSettings {
    uint8_t turn_800c1b64 = 0;        // 0 L1/R1, 1 Directional button
    uint8_t free_look_800c1b65 = 0;   // 0 Reverse, 1 Normal
    uint8_t lock_on_800c1b66 = 0;     // 0 On, 1 Off
    uint8_t sound_800c1b68 = 0;       // 0 Stereo, 1 Mono
    uint8_t map_800c1b69 = 0;         // 0 On, 1 Off
};

// Message-window glyph metrics and colours the cook also copies.
constexpr uint32_t kMsgGlyphWidths8008ae7c = 0x8008AE7C;   // u8[132]
constexpr uint32_t kMsgClutTable80097f30 = 0x80097F30;     // u16[16]
constexpr uint32_t kMsgTable8008cacc = 0x8008CACC;         // message pointers
constexpr uint32_t kMsgOptionTable8008d0d4 = 0x8008D0D4;   // setting pointers
constexpr uint32_t kMsgDecorParam8008af18 = 0x8008AF18;    // u8[9]
constexpr uint32_t kMsgNumberDivisors8008af24 = 0x8008AF24; // u32[6]
constexpr uint32_t kMsgNumberPads8008af3c = 0x8008AF3C;     // pad glyph by kind
constexpr uint32_t kMsgDigitGlyphs8008d028 = 0x8008D028;    // digit -> glyph
constexpr uint32_t kMsgFrameColors80097f6c = 0x80097F6C;
constexpr uint32_t kShopPrices8008fd3c = 0x8008FD3C;       // u16 per item id (0xBC / 0xBD)   // u8[6 x n] bg colours
constexpr uint32_t kMsgBorderColors80097f78 = 0x80097F78;  // u8[16] edge vertex greys
constexpr uint32_t kOptionMessage8008b69c = 0x8008B69C;
// GetTPage(0, 0, 960, 256): the font page (KAIFONT.BIN, 0x80054240).
constexpr uint16_t kMsgFontTpage = psx_get_tpage(0, 0, 960, 256);

struct MessageSlot {
    uint32_t flags = 0;         // +0
    int16_t timer = 0;          // +4
    int16_t pad_owner = 0;      // +6 (never written by the ported code: 0)
    int16_t x = 0, y = 0;       // +8, +10 window origin
    int16_t pen_x = 0, pen_y = 0;   // +16, +18
    uint32_t draw = 0;          // +20 draw-pass pointer
    int16_t scroll = 0;         // +56
    uint8_t ot_layer = 3;       // +61
    uint8_t color = 0;          // +62
    uint8_t speed = 1;          // +63
    uint8_t lines = 0;          // +59
    uint32_t index_base = 0;    // +68 (0 when opened with index 255)
    uint32_t page = 0;          // +72
    uint32_t read = 0;          // +108
    uint8_t default_color = 0;  // +112
    int8_t cursor = 0;          // +113
    int8_t rows = 0;            // +114
    int8_t row_draw = 0;        // +115
    uint32_t cursor_byte = 0;   // +116 address of the 0x60 cursor glyph
    uint8_t brightness = 128;   // +120
    std::array<uint8_t, 3> remembered{};   // +121..+123
    uint8_t decor = 0;          // +124
    uint8_t decor_state = 0;    // +125
    uint8_t decor_step = 8;     // +133 (decor 8)
    uint32_t decor_saved = 0;   // +128 (decor 8)
    uint8_t width = 0, height = 0;   // +126, +127 (characters)
    std::array<int16_t, 8> box{};    // +136..+150 corner offsets
    uint16_t margin = 0;        // +180/+182
    uint16_t held = 0, pressed = 0;  // +184, +186
    uint16_t repeat = 0;        // +188
    uint16_t index = 255;       // +194
    uint32_t number_40 = 0;     // +0x40 (0xBA: the zenny amount)
    std::array<uint32_t, 8> exec_stack{}; uint16_t exec_depth = 0;   // +76 / +192 (0xE0 / 0x91)
    std::array<uint32_t, 8> draw_stack{}; uint16_t draw_depth = 0;   // +24 / +190
};

struct MessageWindowFrame {
    // AddPrim order; bucket = front-end OT numbering (0x80098934 + 112 +
    // 4 * layer -> 28 + layer, so layer 3 is entry 31 like the font mode).
    std::vector<FrontendPacket> packets;
    std::vector<std::array<uint8_t, 2>> sounds;   // 0x8001A008(id, a1, ...)
    uint32_t unported_opcodes = 0;
    // Layout calls the list choice 0xDC makes: 0x80063F28(slot) clears a
    // slot (list = -1), 0x80063FC0(slot, 0x20000 | list) starts one.
    std::vector<std::pair<int8_t, int16_t>> layout_calls;
    // 0xBD (0x800572C8): the item bought (money already taken) or sold (money already paid);
    // the caller applies the item-specific effects (0x23 life max +16, 0x24 / 0x25 the canteen
    // bytes 0x800C1B8C/8D) and, after a sale, the part re-equip 0x80061320.
    int16_t shop_bought = -1, shop_sold = -1;
};

class MessageWindows {
public:
    explicit MessageWindows(ExeExcerpt exe);
    // 0x80053A30: clear the five slots (+0 = 0, +194 = 255).
    void clear_80053a30();
    // 0x8005382C(slot, data, index). Returns false when refused.
    bool open_8005382c(int slot, uint32_t data, uint8_t index);
    // 0x80053B40 with the pad words 0x800C0C26 (held) / 0x800C0C2A (pressed).
    MessageWindowFrame tick_80053b40(uint16_t held, uint16_t pressed);

    // gp+4596 (0x80098A58): bit 31 any window, 0x08000000 >> slot open,
    // bit 30 + low byte the last choice made by 0x97.
    uint32_t flags_80098a58 = 0;
    // 0x800C1B10 main phase: 4 lets Start cancel a choice (front end: 3).
    int8_t phase_800c1b10 = 3;
    // 0x80098912: the save/load screen's mode (1 save, 0 load); decor 7 reads it.
    uint16_t save_mode_80098912 = 0;
    uint32_t zenny_800c1b2c = 0;   // 0xBA, 0xBC / 0xBD (0x8004327C, 0x80043294)
    uint8_t shop_item_gp10cc = 0;  // 0x80098930: the list row's item (0xBC)
    uint8_t shop_short_gp1308 = 0; // 0x80098B6C: 1 = not enough money for it (0xBC)
    uint8_t shop_kind_gp0fec = 0;  // 0x80098850 (setting 14): 0 parts, 1 items (0xB8)
    uint8_t shop_scroll0_gp1090 = 0;   // 0x800988F4: the list's starting scroll (0xB8)
    uint8_t row_block_gp0f94 = 0;      // 0x800987F8 (setting 13): the block each 0xB7 row calls
    uint8_t cancel_slot_gp1200 = 0, cancel_block_gp1201 = 0;   // 0x80098A64/65: 0xB7's Triangle message
    int16_t life_max_800b5260 = 0;     // 0xB8: the life-up price follows the max life
    uint8_t canteen_800c1b8d = 0;      // 0xB8: the canteen row and its price; capacity for 0xC2..0xC4
    uint8_t canteen_fill_800c1b8c = 0; // 0xC2..0xC4: the canteen's current fill
    int16_t word_800c1b70 = 0;         // 0xC3: >= 0x80 allows the free refill branch (meaning UNKNOWN)
    // 0xB8 (0x80056D10): build the shop list into 0x800BE2F8 from the flag rows of table
    // 0x80097F64/68 and pick the window block by the row count.
    std::array<uint8_t,256> flags_800be378{};   // the story flag bytes the list reads (caller mirror)
    std::function<void(uint16_t, bool)> set_story_flag;   // 0x8001DA8C / 0x8001DCD0 (unset: dropped)
    // Map more original bytes (a stage message bank at its load address) next to the excerpt.
    void map_range(uint32_t addr, std::vector<uint8_t> bytes) { exe_.ranges.push_back({addr, std::move(bytes)}); }
    // Item list state (0xDF/0xDD/0xDC/0xE0): owned normal items, a 64-bit
    // field at 0x800BE420 (bit 0x80 >> (id & 7) of byte id >> 3).
    std::array<uint8_t, 8> owned_items_800be420{};
    std::function<bool(uint16_t)> story_flag;   // 0x8001DA58 for 0x9C (unset: all clear)
    // Buster parts (player work 0x800B51B0 + 1104 / + 1108): equipped slots
    // (part + 1, 0 = empty) and the owned list, for 0xE1.
    std::array<uint8_t, 4> equipped_parts_800b5600{};
    std::array<uint8_t, 64> owned_parts_800b5604{};
    bool parts_sorted = false;   // set by a Sort pick; the caller writes owned_parts_800b5604 back
    void sort_parts_80060f84(int k);
    uint8_t name_index_gp4044 = 0;               // 0xC1 name (table 0x8008CBA4)
    std::array<uint8_t, 128> list_800be2f8{};   // list ids, 255 = empty
    uint8_t list_scroll_gp4408 = 0, list_max_scroll_gp4464 = 0, list_picked_gp4077 = 255;
    uint32_t list_count_gp4420 = 0, slot_bits_gp4812 = 0, frame_gp4348 = 0;
    uint32_t cursors_gp4808 = 0;   // bytes 0x80098B2C..2F: cursors of slots 0..3 after the last tick
    OptionSettings settings() const;
    void set_settings(const OptionSettings& s);
    const MessageSlot& slot(int i) const { return slots_[size_t(i)]; }
    uint8_t byte(uint32_t addr) const;
    // Write one mapped byte (the save screen patches its list strings, 0x80066A40..).
    void poke(uint32_t addr, uint8_t v) { mem(addr) = v; }
    void print_number_8005bf10(uint8_t pad, uint32_t value, uint32_t dst);
    const ExeExcerpt& excerpt() const { return exe_; }

private:
    uint8_t& mem(uint32_t addr);
    uint16_t u16(uint32_t addr) { return uint16_t(mem(addr) | (mem(addr + 1) << 8)); }
    uint8_t& setting(uint8_t table_index);
    bool exec(MessageSlot& s, uint8_t op, MessageWindowFrame& out, bool& unported);
    void draw(MessageSlot& s, uint8_t op, MessageWindowFrame& out, int slot_index);
    void draw_frame(MessageSlot& s, MessageWindowFrame& out);
    // Decor exec 0x8008AAC0[decor]: 2/4 = 0x8005B85C, 8 = 0x8005B98C (grows open / shrinks shut).
    bool decor_exec(MessageSlot& s);
    void sound(MessageWindowFrame& out, uint8_t id, uint8_t a1) { out.sounds.push_back({{id, a1}}); }

    ExeExcerpt exe_;
    std::array<MessageSlot, 5> slots_{};
    std::array<uint8_t, 16> ram_800c1b60{};   // settings bytes 0x800C1B60..6F
    uint8_t dummy_8009879c = 0;               // pointer table index 7 (Close row)
    int cur_slot_ = 0;                        // slot the tick is running
    uint8_t list_at(int index) const { return index >= 0 && index < 128 ? list_800be2f8[size_t(index)] : 255; }
    uint32_t name_ptr(int8_t id) const { return exe_.u32(uint32_t(0x8008CCA4 + 4 * int32_t(id))); }   // item names
};

} // namespace l3
