#pragma once
// ST02 event 4's family-0x20 scene actors (docs/research_01516/STAGE_LOADING.md,
// "ST02 event 4 actors"): slot 3 = 20/0A (the Flutter seen from outside,
// 0x80102888) and slot 4 = 20/27 (0x80102990), both variant 4. The event's
// record callbacks write their +0 / +9 / position fields (source_st00_post_win.h
// SourceSceneSlotCommands); the update functions below are hand-ported and run
// once per object update. Rendering-only work (the 0x8002F9C4 model lookup,
// 0x80032538 sequence step, 0x800334C4 draw submit) and the overlay effects
// (0x80105724 smoke, 0x80105C28) are reported, not performed.
// Original addresses are identity tokens.
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_st00_post_win.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {

struct St02EventActorWork {
    bool live = false;
    uint8_t slot = 0, subtype = 0;               // event slot, work byte [2]
    uint8_t flags_00 = 0;                        // bit 1 = drawn
    uint8_t state_08 = 0, state_09 = 0, sub_0a = 0;
    uint8_t byte_0c = 0, byte_0d = 0;            // spawn record [8..9]
    std::array<int16_t,3> raw_14{};              // 0x80030150 shorts
    std::array<int32_t,3> fixed_1c{};            // 16.16 position
    std::array<int32_t,3> rot_44{};              // 16.16 rotation accumulators
    std::array<uint16_t,3> rot_54{};             // rotation shorts (+0x56 = heading)
    uint32_t word_78 = 0, word_158 = 0, word_15c = 0;
    uint16_t colour_15e = 0;
    bool released = false;                       // 0x80031824
};

// 0x80105724(work, a1, a2, a3, [sp+16]) and 0x80105C28(work, a1, a2, a3).
struct St02ActorEffect {
    uint32_t function = 0;
    int16_t a1 = 0, a2 = 0, a3 = 0;
    int32_t a4 = 0;
};

enum class St02ActorStatus : uint8_t { Ok, NoReferenceTables, UnknownState };

struct St02EventActorTick {
    St02ActorStatus status = St02ActorStatus::Ok;
    bool model_init = false;                     // 0x8002F9C4
    std::optional<uint8_t> model_variant;        // its variant byte (nullopt = null pointer)
    bool pose_restart = false;                   // 0x800322A8(work, 0, 0)
    uint8_t sequence_steps = 0;                  // 0x80032538 calls
    uint8_t draw_submits = 0;                    // 0x800334C4 calls
    std::array<St02ActorEffect,3> effects{};
    uint8_t effect_count = 0;
};

// 0x8001EAE8 for one of event 4's spawn records: work +3 = record [4],
// +4 = record [2], +0x0C..+0x0F = record [8..11]; 0x8001E968 places it at
// record [12..17] (shorts) with heading record [18..19].
St02EventActorWork spawn_st02_event4_actor(const std::array<uint8_t,20>& record) noexcept;

// Applies the writes the record callbacks made to `slot` since the last call
// and clears the one-shot ones (state, position, heading), so the actor's own
// progress is not overwritten by stale values.
void apply_st02_event4_slot_writes(St02EventActorWork& w, SourceSceneSlotCommands& slot) noexcept;

// One object update: 20/0A 0x80102888 or 20/27 0x80102990. `frame_1f800002`
// is the frame counter halfword the smoke cadence reads. `refs` may be null;
// states that need it (20/0A states 3 and 6) then report NoReferenceTables
// and leave the position unchanged.
St02EventActorTick step_st02_event4_actor(St02EventActorWork& w, const ReferenceTables* refs,
                                          uint16_t frame_1f800002) noexcept;

} // namespace l3
