#pragma once
#include "legends3ds/frontend_title.h"
#include "legends3ds/mml_message_window.h"
#include <cstdint>
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
    MessageWindows windows_;
    ExeExcerpt exe_;
    uint8_t page_ = 0;
    uint8_t options_sub_ = 0;
    bool layout6_ = false;     // layout slot 0 (0x800C3528) running list 6
    uint32_t frame_ = 0;       // 0x1F800002-style counter for mode-1 blink
};

} // namespace l3
