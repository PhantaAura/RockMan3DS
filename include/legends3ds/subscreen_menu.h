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
// Pages 2..5 (Normal / Special Items, Buster Parts, Special Weapons) and the
// L1/R1 page switch 0x80060EC0 are ported; item use, the Select page (7) and
// the loader calls 0x8001D440/0x8001D7AC are not.
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
    std::array<int16_t, 14> weapon_energy_1274{};   // +1274 + 8 * id: current energy
    uint8_t byte_800bbd87 = 0;                       // item 64 state (0x8005D95C)
};

struct SubScreenFrame {
    std::vector<FrontendPacket> packets;
    std::vector<std::array<uint8_t, 2>> sounds;
    bool exited = false;          // page 8 reached: resume gameplay
    bool settings_applied = false;   // Options page closed (0x800425FC reinstalled)
    uint32_t unported_opcodes = 0;
    bool parts_sorted = false;   // a Sort pick reordered the parts: write owned_parts_800b5604 back
    std::array<uint8_t, 64> owned_parts_800b5604{};
    // Normal Items use (0x8005F074), for the caller to apply: item code 64..67
    // on Yes; 64 = 0x8005D9C4 (byte 0x800BBD87 = 0, then 0x8005D528); 65 = the
    // on-hand weapon's energy (+1274); 66 / 67 = halfwords 0x800BBD96 /
    // 0x800BBD94; then, when the message closes, 0x8001DCD0 clears story flag
    // 1344 + item (the owned bit in 0x800BE420).
    uint8_t item_used = 0;
    int16_t weapon_energy_1274 = -1;
    uint16_t timer_800bbd94 = 0, timer_800bbd96 = 0;
    int16_t clear_story_flag = -1;
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
    void normal_items_8005ed9c(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out);
    int weapon_energy_max_80037604(const SubScreenValues& v) const;
    void special_items_8005f210(uint16_t pressed, SubScreenFrame& out);
    void buster_parts_800600cc(uint16_t pressed, const SubScreenValues& v, SubScreenFrame& out);
    void pages_80060ec0(uint16_t pressed, SubScreenFrame& out);
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
    uint16_t used_flag_ = 0;   // page +12: story flag 1344 + item, cleared after use
    // ctx 0x800A38F0 +6 (animation count) and +16..+54 (stat triplets
    // {from, target, shown}: attack now/with, energy, range, rapid).
    std::array<uint8_t, 64> ctx_{};
    // Arm changes the Equip flow made (0x80060848..: +1103 / +368), for the caller.
    int8_t arm_368_ = 0; uint8_t flag_1103_ = 0; bool arm_dirty_ = false;
    bool parts_dirty_ = false;   // menu copy of the parts after a Sort
    uint32_t frame_ = 0;       // 0x1F800002-style counter for mode-1 blink
};

} // namespace l3
