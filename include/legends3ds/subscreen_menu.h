#pragma once
#include "legends3ds/frontend_title.h"
#include "legends3ds/mml_message_window.h"
#include <cstdint>
#include <functional>
#include <vector>

// The in-game Start sub-screen (main phase 4), ROCK_NEO 0x8005EC34: each
// update draws the tile backdrop 0x80062368, then runs the page task
// 0x8008DBB0[page] (page byte 0x800A38F0). Ported pages: 0 0x8005EC80
// (open the menu message 0x32, start status layout 6), 1 0x8005ECE0 (status
// bar 0x80061E0C, the choice), 6 0x80060B00 (Options: message 0x26), 8 exit.
// The item/parts/weapon pages (2..5) and the Select page (7) are not ported:
// choosing them reopens the menu (CANDIDATE presentation). The loader calls
// 0x8001D440/0x8001D7AC on the Options page are not ported.
namespace l3 {

struct SubScreenValues {
    int16_t life_800b521e = 0, life_max_800b5260 = 0;
    uint32_t zenny_800c1b2c = 0;
    uint32_t play_time_800c1b1c = 0;   // 30 Hz updates
    std::array<uint8_t, 8> owned_items_800be420{};   // normal items owned (bit 0x80 >> (id & 7))
    std::function<bool(uint16_t)> story_flag;          // 0x8001DA58 (special item icons)
    uint8_t byte_800c1b8c = 0, byte_800c1b8d = 0;      // counters shown as digits (modes 5..8)
    uint8_t byte_800b531a = 0, byte_800b531b = 0, byte_800b531c = 0;   // player work (modes 9..12)
    std::array<uint8_t, 4> equipped_parts_800b5600{};   // buster parts (part + 1, 0 = empty)
    std::array<uint8_t, 64> owned_parts_800b5604{};
    // Special weapons (player work 0x800B51B0): +370 weapon on hand, +368 arm
    // in use, +1102 / +1103, and five level bytes per weapon at +1268 + 8 * id.
    int8_t weapon_370 = 0, arm_368 = 0, energy_1102 = 0; uint8_t flag_1103 = 0;
    std::array<std::array<int8_t, 5>, 14> weapon_levels{};
};

struct SubScreenFrame {
    std::vector<FrontendPacket> packets;
    std::vector<std::array<uint8_t, 2>> sounds;
    bool exited = false;          // page 8 reached: resume gameplay
    bool settings_applied = false;   // Options page closed (0x800425FC reinstalled)
    uint32_t unported_opcodes = 0;
};

class SubScreenMenu {
public:
    explicit SubScreenMenu(ExeExcerpt exe);
    // 0x8005EBE4: page 0 on entry.
    void enter();
    SubScreenFrame update(uint16_t held, uint16_t pressed, const SubScreenValues& v);
    uint8_t page() const { return page_; }
    uint8_t options_substate() const { return options_sub_; }
    MessageWindows& windows() { return windows_; }
    uint16_t scroll_1f800002 = 0;

private:
    void status_bar_80061e0c(const SubScreenValues& v, std::vector<FrontendPacket>& out) const;
    void status_frame_800640f0(const SubScreenValues& v, std::vector<FrontendPacket>& out) const;
    void layout_draw(uint16_t list, const SubScreenValues& v, std::vector<FrontendPacket>& out) const;
    void normal_items_8005ed9c(uint16_t pressed, SubScreenFrame& out);
    void special_items_8005f210(uint16_t pressed, SubScreenFrame& out);
    void buster_parts_800600cc(const SubScreenValues& v, SubScreenFrame& out);
    void gauges_800605dc(const SubScreenValues& v, std::vector<FrontendPacket>& out);
    void special_weapons_80060618(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out);
    void weapon_stats_80061840(int weapon, const SubScreenValues& v);
    void bars_800602a4_mode1(std::vector<FrontendPacket>& out);
    void animate_800602a4();
    void icons_8006430c(uint32_t first, const SubScreenValues& v, std::vector<FrontendPacket>& out) const;
    bool exit_check_80060e64(uint16_t pressed, SubScreenFrame& out);
    MessageWindows windows_;
    ExeExcerpt exe_;
    uint8_t page_ = 0;
    uint8_t options_sub_ = 0;
    std::array<int16_t, 4> layouts_{{-1, -1, -1, -1}};   // slots 0x800C3528: running list, -1 = off
    uint8_t item_code_ = 255;  // page +4
    // ctx 0x800A38F0 +6 (animation count) and +16..+54 (stat triplets
    // {from, target, shown}: attack now/with, energy, range, rapid).
    std::array<uint8_t, 64> ctx_{};
    // Arm changes the Equip flow made (0x80060848..: +1103 / +368), for the caller.
    int8_t arm_368_ = 0; uint8_t flag_1103_ = 0; bool arm_dirty_ = false;
    uint32_t frame_ = 0;       // 0x1F800002-style counter for mode-1 blink
};

} // namespace l3
