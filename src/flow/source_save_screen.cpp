#include "legends3ds/source_save_screen.h"

namespace l3 {
namespace {
// Message pointer table entries (0x8008CACC + 4 * id).
constexpr uint32_t kTitle = 0x8008CAEC, kWhichCard = 0x8008CAF0, kCardRows = 0x8008CAF4, kCardName = 0x8008CAF8,
    kChecking = 0x8008CB00, kNoCard = 0x8008CB04, kSlotList = 0x8008CB20, kSelected = 0x8008CB24,
    kConfirm = 0x8008CB28, kSaving = 0x8008CB2C, kComplete = 0x8008CB30, kFailed = 0x8008CB34, kBadData = 0x8008CB38;
constexpr uint32_t kNames = 0x8008D168, kTimes = 0x8008D184;   // 5 list rows, then [5] the selected row
constexpr uint32_t kStageNames = 0x8008CE94, kNoData = 0x80097FAC, kBroken = 0x80097FC0, kEmptyTime = 0x80097FFC;
constexpr uint32_t kDigitGlyphs = 0x8008D028;
constexpr uint32_t kChoiceMade = 0x40000000u, kNextDone = 0x80000u;
}

SourceSaveScreen::SourceSaveScreen(ExeExcerpt exe) : windows_(std::make_unique<MessageWindows>(std::move(exe))) {}

void SourceSaveScreen::open(int slot, uint32_t entry) {
    const uint32_t data = uint32_t(windows_->byte(entry)) | uint32_t(windows_->byte(entry + 1)) << 8 |
                          uint32_t(windows_->byte(entry + 2)) << 16 | uint32_t(windows_->byte(entry + 3)) << 24;
    (void)windows_->open_8005382c(slot, data, 255);
}

void SourceSaveScreen::begin(const std::array<SaveSlotRecord,5>& slots) {
    slots_ = slots;
    // 0x80066664 (a0 = 1): +4 = 1 (save), +5 = 0, +6 = 0, 0x80067884, then the sub.
    state_ = 2; sub_ = 0; step_ = 0; bad_seen_ = false; waiting_write_ = false; pending_result_.reset();
    windows_->phase_800c1b10 = 6; windows_->save_mode_80098912 = 1;
}

static uint32_t ptr(const MessageWindows& w, uint32_t a) {
    return uint32_t(w.byte(a)) | uint32_t(w.byte(a + 1)) << 8 | uint32_t(w.byte(a + 2)) << 16 | uint32_t(w.byte(a + 3)) << 24;
}

// 0x80066A40..0x80066C70: the list strings per record.
void SourceSaveScreen::patch_list() {
    for (uint8_t i = 0; i < 5; ++i) {
        const uint32_t name = ptr(*windows_, kNames + 4u * i), time = ptr(*windows_, kTimes + 4u * i);
        auto& r = slots_[i];
        if (r.status == 1) {
            for (uint32_t k = 0; k < 13; ++k) windows_->poke(name + k, windows_->byte(kStageNames + 13u * r.stage + k));
            for (uint32_t k = 0; k < 8; ++k) windows_->poke(time + k, r.time_glyphs[k]);
        } else if (r.status == 2) {
            for (uint32_t k = 0; k < 13; ++k) windows_->poke(name + k, windows_->byte(kNoData + k));
            for (uint32_t k = 0; k < 8; ++k) { r.time_glyphs[k] = windows_->byte(kEmptyTime + k); windows_->poke(time + k, r.time_glyphs[k]); }
            r.stage = 0;
        } else {
            for (uint32_t k = 0; k < 13; ++k) windows_->poke(name + k, windows_->byte(kBroken + k));
            for (uint32_t k = 0; k < 8; ++k) { r.time_glyphs[k] = windows_->byte(kEmptyTime + k); windows_->poke(time + k, r.time_glyphs[k]); }
            bad_seen_ = true;
        }
    }
}

// 0x800672D4.. / 0x80067540..: the selected row (pointers [5]) and its slot digit.
void SourceSaveScreen::patch_selected(uint8_t row, uint8_t stage, const uint8_t* glyphs) {
    const uint32_t name = ptr(*windows_, kNames + 20u), time = ptr(*windows_, kTimes + 20u);
    for (uint32_t k = 0; k < 13; ++k) windows_->poke(name + k, windows_->byte(kStageNames + 13u * stage + k));
    for (uint32_t k = 0; k < 8; ++k) windows_->poke(time + k, glyphs[k]);
    windows_->poke(name - 2u, windows_->byte(kDigitGlyphs + 1u + row));
}

void SourceSaveScreen::write_result(int result, uint8_t stage, const std::array<uint8_t,8>& glyphs) {
    pending_result_ = result; pending_stage_ = stage; pending_glyphs_ = glyphs;
}

SourceSaveScreen::Tick SourceSaveScreen::tick(uint16_t held, uint16_t pressed) {
    Tick out;
    if (state_ != 2) { out.done = state_ == 3; return out; }
    const uint32_t f = windows_->flags_80098a58;
    auto sound = [&](uint8_t id) { out.frame.sounds.push_back({id, 1}); };
    switch (sub_) {
    case 0:   // 0x80066764: which card
        if (step_ == 0) { windows_->clear_80053a30(); open(0, kTitle); open(1, kWhichCard); open(2, kCardRows); ++step_; }
        else if (f & kChoiceMade) {
            const int8_t row = int8_t(f & 0xFFu);
            if (row >= 0 && row < 2) { card_ = uint8_t(row); sub_ = 1; step_ = 0; }
            else if (row == 2) { state_ = 3; windows_->clear_80053a30(); out.done = true; return out; }
        }
        break;
    case 1:   // 0x8006689C: "Checking data..."
        if (step_ == 0) {
            windows_->clear_80053a30(); open(0, kTitle); open(1, kCardName + 4u * card_); open(2, kChecking);
            timer_c_ = 15; ++step_;
        } else if (step_ == 1) {
            if (--timer_c_ != 0) break;
            if (card_ != 0) { sub_ = 2; step_ = 0; break; }   // card 2: no card (result -> sub 2)
            bad_seen_ = false; patch_list();
            if (bad_seen_) { open(2, kBadData); ++step_; } else { sub_ = 5; step_ = 0; }
        } else if (f & kNextDone) { sub_ = 5; step_ = 0; }
        break;
    case 2:   // 0x80066D60: no memory card
        if (step_ == 0) { windows_->clear_80053a30(); open(0, kTitle); open(1, kCardName + 4u * card_); open(2, kNoCard); ++step_; }
        else if (f & kNextDone) { sub_ = 0; step_ = 0; }
        break;
    case 5:   // 0x80067174, save steps (table 0x80011440)
        switch (step_) {
        case 0:
            windows_->clear_80053a30(); open(0, kTitle); open(1, kCardName + 4u * card_); open(2, kSlotList);
            timer_10_ = 0x1E; ++step_; break;
        case 1:
            if (f & 0x60000000u) {
                row_ = uint8_t(f & 0xFFu);
                if (row_ == 5) { sound(0x83); sub_ = 0; step_ = 0; break; }
                if (row_ < 5 && slots_[row_].status < 3) {
                    patch_selected(row_, slots_[row_].stage, slots_[row_].time_glyphs.data());
                    open(2, kSelected); open(3, kConfirm); sound(0x82); ++step_;
                } else sound(0x83);
            }
            break;
        case 2:
            if (f & kChoiceMade) {
                if ((f & 0xFFu) != 0) { sub_ = 5; step_ = 0; break; }   // No: the list again
                open(3, kSaving); ++step_; timer_c_ = 15;
            } else if (timer_10_) --timer_10_;
            else timer_10_ = 0x1E;   // 0x80064B60 card poll: card 1 stays present
            break;
        case 3:
            if (!waiting_write_) {
                if (--timer_c_ != 0) break;
                out.write_slot = row_; waiting_write_ = true; break;
            }
            if (!pending_result_) break;
            waiting_write_ = false;
            if (*pending_result_ == 0) {
                slots_[row_].status = 1; slots_[row_].stage = pending_stage_; slots_[row_].time_glyphs = pending_glyphs_;
                patch_selected(row_, pending_stage_, pending_glyphs_.data());
                open(3, kComplete);
            } else open(3, kFailed);
            pending_result_.reset(); ++step_;
            break;
        case 4:
            if (f & kNextDone) { sub_ = 0; step_ = 0; }
            break;
        default: break;
        }
        break;
    default: break;
    }
    auto own = std::move(out.frame.sounds);
    out.frame = windows_->tick_80053b40(held, pressed);
    out.frame.sounds.insert(out.frame.sounds.begin(), own.begin(), own.end());
    return out;
}

} // namespace l3
