#pragma once
// ST02 event 4's scene actors (docs/research_01516/STAGE_LOADING.md,
// "ST02 event 4 actors"), all variant 4: slot 0 = 40/05 (0x80103360),
// slot 1 = 40/06 (0x80103D18), slot 2 = 40/07 (0x80104154), slot 3 = 20/0A
// (the Flutter seen from outside, 0x80102888), slot 4 = 20/27 (0x80102990,
// the prop 40/06 holds: 0x80103D8C writes its +0x0C and +0x58 every frame). The event's
// record callbacks write their +0 / +9 / position fields (source_st00_post_win.h
// SourceSceneSlotCommands); the update functions below are hand-ported and run
// once per object update. Rendering-only work (the 0x8002F9C4 model lookup,
// 0x80032538 sequence step, 0x800334C4 draw submit) and the overlay effects
// (0x80105724 smoke, 0x80105C28) are reported, not performed.
// Original addresses are identity tokens.
#include "legends3ds/actor_animation_bank.h"
#include "legends3ds/actor_sequence.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_st00_post_win.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace l3 {

struct St02EventActorWork {
    bool live = false;
    uint8_t slot = 0, subtype = 0;               // event slot, work byte [2]
    uint8_t flags_00 = 0;                        // bit 1 = drawn
    uint8_t state_08 = 0, state_09 = 0, sub_0a = 0;
    uint8_t byte_03 = 0;                         // spawn record [4]; 40/05 and 40/07 run only when 0
    uint8_t byte_0b = 0, byte_0c = 0, byte_0d = 0;   // +0x0C/+0x0D: spawn record [8..9]
    uint8_t pose_ac = 0;                         // +0xAC: last 0x800322A8 request
    uint8_t byte_143 = 0, byte_147 = 0;          // 40/06 init stores 0xFF (face channel bytes, CANDIDATE)
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

enum class St02ActorStatus : uint8_t {
    Ok, NoReferenceTables, UnknownState,
    SequenceUnavailable,   // a key test needed *(+0x98) and no sequence data is bound
    RollTableUnavailable,  // 0x80103D8C needed the cooked 0x801079E4 tables (or read past them)
};

// The class-40 pose sequence a 40/xx handler drives: 0x800322A8(work, request,
// 0) restarts at the request's first 8-byte record, 0x80032488 steps it, and
// the handlers test byte 0 of the current record (*(+0x98)). A port without
// decoded sequence data returns nullopt from key().
struct St02ActorSequence {
    virtual ~St02ActorSequence() = default;
    virtual void restart(uint8_t slot, uint8_t request) = 0;
    virtual void advance(uint8_t slot) = 0;
    virtual std::optional<uint8_t> key(uint8_t slot) const = 0;
};

struct St02EventActorTick {
    St02ActorStatus status = St02ActorStatus::Ok;
    bool model_init = false;                     // 0x8002F9C4
    std::optional<uint8_t> model_variant;        // its variant byte (nullopt = null pointer)
    bool pose_restart = false;                   // 0x800322A8
    std::optional<uint8_t> pose_request;         // its request (40/xx; 20/xx restart request 0)
    uint8_t sequence_steps = 0;                  // 0x80032538 (20/xx) or 0x80032488 (40/xx) calls
    bool face_tick = false;                      // 40/05 0x80102C44 / 40/06 0x801035F4 ran (not ported)
    bool prop_written = false;                   // 40/06 0x80103D8C wrote slot 4
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

struct St02ActorContext {
    const ReferenceTables* refs = nullptr;       // 20/0A states 3 and 6
    uint16_t frame_1f800002 = 0;                 // smoke cadence
    St02ActorSequence* sequence = nullptr;       // 40/xx poses and key tests
    St02EventActorWork* prop = nullptr;          // event slot 4 (*(0x800C4C7C)), for 40/06
    const std::vector<uint8_t>* roll_801079e4 = nullptr;   // cooked 0x801079E4..0x80107AAA
};

// One object update of the actor's handler. States that need missing inputs
// report a status and skip only that work, as listed in St02ActorStatus.
St02EventActorTick step_st02_event4_actor(St02EventActorWork& w, const St02ActorContext& ctx) noexcept;
St02EventActorTick step_st02_event4_actor(St02EventActorWork& w, const ReferenceTables* refs,
                                          uint16_t frame_1f800002) noexcept;
// The class-40 sequences of slots 0..2 from the stage EBD: 0x8002F9C4 with a
// null variant takes the first (0x40, subtype) row; the requests decoded are
// the ones the handlers issue (40/05: 16..26, 40/06: 1..9, 40/07: 1..3).
// restart/advance/key follow 0x800322A8 / 0x80032488 / +0xA4 over the
// decoded records (actor_sequence.h). A request missing from a bank leaves
// that slot keyless (key() == nullopt) until the next restart.
struct St02ActorBankSequence final : St02ActorSequence {
    std::array<std::shared_ptr<const ActorAnimationBank>,3> banks{};
    std::array<const ActorAnimationRequestCache*,3> current{};
    std::array<ActorSequenceCursor,3> cursor{};
    void restart(uint8_t slot, uint8_t request) override;
    void advance(uint8_t slot) override;
    std::optional<uint8_t> key(uint8_t slot) const override;
};
// Decodes one slot's bank (nullptr when the row is missing or the decode fails
// or exceeds `cap` bytes).
std::shared_ptr<const ActorAnimationBank> decode_st02_event4_actor_bank(const EbdDirectory& ebd, uint8_t subtype,
                                                                        size_t cap) noexcept;
constexpr uint32_t kSt02PropRollTable = 0x801079E4u;
constexpr size_t kSt02PropRollBytes = 0xC6;

} // namespace l3
