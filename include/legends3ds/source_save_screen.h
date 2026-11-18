#pragma once
// The memory-card save screen (main phase 6, 0x800665FC; docs/research_01516/DATA_MENU.md),
// save mode only, on the ported message windows. The 3DS save files stand in for memory
// card 1 (always present and formatted); card 2 reads as absent. Not ported: the format
// flow (sub 3/4), load mode, the 15-frame card polls beyond presence.
// Original addresses are identity tokens.
#include "legends3ds/mml_message_window.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace l3 {

struct SaveSlotRecord {          // one 0x180-byte header read (0x800BAF38 + i * 0x180)
    uint8_t status = 2;          // +0x109: 1 valid, 2 empty, 3 broken
    uint8_t stage = 0;           // +0x100
    std::array<uint8_t,8> time_glyphs{};   // +0x101
};

class SourceSaveScreen {
public:
    explicit SourceSaveScreen(ExeExcerpt exe);
    // State 0 (0x80066664) with a0 = 1 (save); `slots` are what card 1 holds.
    void begin(const std::array<SaveSlotRecord,5>& slots);
    struct Tick {
        MessageWindowFrame frame;
        std::optional<uint8_t> write_slot;   // 0x80064D74(card, name[slot], 0x800B5DB0, 0x1F00) now
        bool done = false;                   // state 3: phase 6 returns 1 -> main phase 3, sub 2
    };
    // One phase-6 update: the sub-state, then 0x80053B40 (pads 0x800C0C26 / 0x800C0C2A).
    Tick tick(uint16_t held, uint16_t pressed);
    // The write's result (0 ok, else an error) and the written block's stage / time glyphs.
    void write_result(int result, uint8_t stage, const std::array<uint8_t,8>& time_glyphs);
    bool active() const noexcept { return state_ == 2; }
    MessageWindows& windows() { return *windows_; }
    uint8_t sub() const noexcept { return sub_; }
    uint8_t step() const noexcept { return step_; }

private:
    void open(int slot, uint32_t pointer_entry);
    void patch_list();
    void patch_selected(uint8_t row, uint8_t stage, const uint8_t* glyphs);
    std::unique_ptr<MessageWindows> windows_;
    std::array<SaveSlotRecord,5> slots_{};
    uint8_t state_ = 0, sub_ = 0, step_ = 0, card_ = 0, row_ = 0;
    bool bad_seen_ = false;
    int timer_c_ = 0, timer_10_ = 0;
    std::optional<int> pending_result_;
    uint8_t pending_stage_ = 0; std::array<uint8_t,8> pending_glyphs_{};
    bool waiting_write_ = false;
};

} // namespace l3
