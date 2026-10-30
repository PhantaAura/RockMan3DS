#pragma once
#include "legends3ds/reference_tables.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

// The title Servbot parade (docs/research_01516/FRONTEND_TITLE.md): the
// spawner 0x80015428, the class-20 callback 0x800153EC with its states
// 0x80014AC0 (init), 0x800150E0 (script + motion) and 0x800153CC (release),
// and the script reader 0x80014BCC. Scripts come from the owner's EXE
// (tools/cook_title_servbots.py, L3TS).
namespace l3 {

// 0x80038370 on the title points (0x800979DC target, 0x800979EC), as the
// cooker evaluates it: the arguments it passes to 0x80037FB4.
struct TitleServbotCamera {
    std::array<int32_t, 3> target{};
    int32_t distance = 0, yaw = 0, pitch = 0;
};

struct TitleServbotScripts {
    uint32_t base = 0;
    std::vector<uint8_t> data;   // EXE bytes base .. base + size
    std::optional<TitleServbotCamera> camera;   // L3TS0002 only
    std::optional<int16_t> halfword(uint32_t address) const noexcept;
    std::optional<uint32_t> word(uint32_t address) const noexcept;
    // Script pointer of slot `index` in wave `wave` (0x800820E0), 0 = none.
    std::optional<uint32_t> script(uint8_t wave, uint8_t index) const noexcept;
};
TitleServbotScripts load_title_servbot_scripts(const std::vector<uint8_t>& bytes);

// The used fields of one class-20 work (0x3C0 bytes at 0x8009F108 + slot).
struct TitleServbot {
    bool active = false;              // +0
    // Spawned by the title setup (state 1) this update: its callback first
    // runs on the next update (oracle: +0 = 1, then 3).
    bool fresh = false;
    uint8_t index = 0;                // +3
    uint8_t state = 0;                // +8: 0 init, 1 run, 2 release
    uint8_t wave = 0;                 // +0x0C
    uint8_t steps_on = 0, step_mode = 0;   // +0x0D, +0x0E
    int8_t step_timer = 0;            // +0x0F
    std::array<int16_t, 3> raw{};     // +0x14
    std::array<int32_t, 3> fixed{};   // +0x1C
    std::array<int16_t, 3> velocity{};     // +0x44 (forward/vertical/strafe or world xyz)
    std::array<int16_t, 3> acceleration{}; // +0x4C
    uint16_t heading = 0;             // +0x56
    uint8_t anim_ac = 0xFF, anim_ad = 0xFF;   // +0xAC request, +0xAD previous
    uint8_t byte_110 = 0;             // +0x110
    std::array<int16_t, 3> scale{};   // +0x158 (0x400 at init)
    // Script state at +0x348.
    uint32_t script = 0;              // +0: pointer
    uint16_t pc = 0;                  // +4: halfword index
    int16_t delay = 0;                // +6
    int16_t heading_step = 0;         // +8
    std::array<int16_t, 3> scale_step{};   // +0x0A
    uint8_t world_motion = 0;         // +0x12
    uint8_t byte_13 = 0xFF, byte_14 = 0xFF;
};

struct TitleServbotParade {
    std::array<TitleServbot, 16> slots{};
    int16_t timer_06 = 0, wave_08 = 0, phase_0a = 0;   // front-end work +6/+8/+10
};

struct TitleServbotFrame {
    std::vector<uint8_t> sounds;        // 0x8001A008 ids (a1 = a2 = 0)
    uint8_t released_8002ee90 = 0;      // slots 0x8002EE90(2, 0x80) released this frame
    std::array<bool, 16> animation_restart{};   // 0x800322A8 (else 0x80032488)
};

// 0x80015428 with the front-end state byte (+0) and the fade counter.
void title_servbot_spawn_80015428(TitleServbotParade& parade, uint8_t frontend_state,
                                  int16_t fade_80098a72, TitleServbotFrame& out);
// The class callbacks for every active slot (0x80031AA4 order: slot 0..15).
void title_servbot_update_800153ec(TitleServbotParade& parade, const TitleServbotScripts& scripts,
                                   const ReferenceTables& refs, TitleServbotFrame& out);

} // namespace l3
