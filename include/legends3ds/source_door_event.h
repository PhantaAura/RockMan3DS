#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "legends3ds/player_contact_runtime.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_event_phase6.h"
#include "legends3ds/st00_flow.h"

namespace l3 {
// Player door event, USA ROCK_NEO.EXE 0x80050C2C case 3 (event code 0x3xx).
// The caller (0x8003C4EC) runs it only while story flag 0x200 is clear.
// Story flags use the native bank convention 1<<(id&7).
enum class SourceDoorStatus : uint8_t {
    NoEvent,             // 0x800514FC found nothing (or an unported shape held it)
    PlayerBusy,          // player +0xBD nonzero: 0x80050C2C returns before the switch
    UnsupportedType,     // event type other than 3; not ported here
    GateClosed,          // 0x80051970 false
    NoAuxMatch,          // no scratch 0x1F800050 record passes position/facing/side
    Locked,              // 0x800519E4 returned 1 (flag 0x7E0 or 0x7E0|id)
    LocalDoorOpened,     // record halfword 0 bit 15 clear: flag 0x7B1 set
    TransitionRequested, // bit 15 set: 0x800C3558..71 written, flag 0x7B0 set
};
const char* source_door_status_name(SourceDoorStatus) noexcept;

// Lanes 0x800C3558..0x800C3571 (offset from 0x800C3558) stored by 0x800513E0..80.
struct SourceDoorRequest {
    std::array<uint8_t,26> raw{},known{};
    size_t aux_record_index=0;
    // 0x8002EE90(0x56,5) and 0x8001F6C4 (BGM check from 356E/356F) are
    // recorded for their owners; native audio does not play them yet.
    uint16_t sound_id=0x56,sound_argument=5;
    bool music_check_8001f6c4=true;
};

struct SourceDoorResult {
    SourceDoorStatus status=SourceDoorStatus::NoEvent;
    RuntimeEventQueryStatus query_status=RuntimeEventQueryStatus::NoData;
    std::optional<RuntimeEventPrimitiveSample> sample;
    std::optional<size_t> aux_record_index;
    std::optional<uint16_t> flag_set;        // 0x7B0 (request) or 0x7B1 (local door)
    std::optional<int16_t> locked_message;   // 0x80063BC8(0,0,0,record hw2) when 0x7C0 lock flags clear
    std::optional<SourceDoorRequest> request;
};

// 0x80051970 with a0 = player (0x80050C2C passes null when +1 & 0xF0 != 0).
bool source_door_gate_80051970(const L3PlayerCompatWork& player) noexcept;

// Exact 0x80050C2C prefix + case 3 + 0x800519E4 over one 0x800514FC result.
// Flags are read from `flags` (byte_count bytes); the three source flag writes
// are returned, not applied. No side effect is performed for other types.
SourceDoorResult evaluate_source_door_event_80050c2c(
    const RuntimeEventPrimitiveQuery& query, const L3PlayerCompatWork& player,
    const St00AuxList& aux, const uint8_t* flags, size_t byte_count) noexcept;

// The 0x800C3560..71 part of a door request in the shared transition shape.
// source_function is 0x80050C2C; destination area is 356F, stage 356E.
SourceEventHandoffTransitionIntent source_door_transition_intent(
    const SourceDoorRequest&, uint64_t source_generation) noexcept;

// Event 0 (0x800485E4, started by ST00 poll flag 0x7B0) phase 1 substate 8,
// 0x80048898..0x80048974, after the area load clears 356C: target
// x = 3560 + (rsin(356A)*64 >> 12), y = 3562 - 1, z = 3564 + (rcos(356A)*64 >> 12),
// heading 356A, then the state-3 substate-2 copy 0x80041C64..0x80041D2C into
// +0x14/+0x1C/+0x54 (previous pose first), applied for two frames so the
// previous pose also equals the arrival. Lanes the source copies from
// retained work (+0x43E, +0x434, +0x446) are left unchanged.
bool apply_source_door_arrival_80048898(const SourceEventHandoffTransitionIntent&,
    const ReferenceTables&, L3PlayerCompatWork&) noexcept;

// ---------------------------------------------------------------------------
// Event 0 door sequence (0x800485E4, phases 0x80048620 / 0x80048704 /
// 0x80048A40). Lanes of the event work 0x800C4C48 that the door path uses.
// The ST00 poll 0x80100598 starts it from flag 0x7B0; 0x8001DDE4 zeroes the
// counter. See docs/research_01507/EVENT0_DOOR_SEQUENCE.md.
struct SourceDoorEvent0 {
    bool active=false;        // +0
    uint8_t phase=0;          // +4 (0x8008A778)
    uint8_t substate=0;       // +5 (phase 1: jump table 0x80010D6C, 14 entries)
    uint8_t walk_state=0;     // +10 (0x80048A9C)
    uint8_t walk_limit=0;     // +11 (0x8008AA04[stage], read by walk state 0)
    int32_t counter=0;        // +0x10, +1 per phase-1 frame by 0x8001F070
    int8_t status_356c=0;     // 0x800C356C as event 0 writes and waits on it
    uint16_t forced_pad=0;    // 0x800C0F96, read by 0x8003C07C while +0xBD & 0x20
    // Recorded, not played/applied by native owners yet.
    uint16_t last_sound=0;    // 0x8002EE90(8,128) in phase 0
    uint32_t camera_requests=0; // 0x8001E6C0 calls (substates 0 and 8)
    uint32_t frames=0;        // ticks the event ran (diagnostic)
    std::array<uint8_t,26> lanes{}; // 0x800C3558..71 as the request wrote them
};

// 0x8008AA04[stage]: the second walk ends once counter > this byte. Stage 0 is
// 108; the whole 32-byte row is kept as read from the instructions' table.
uint8_t source_event0_walk_limit_8008aa04(uint8_t stage) noexcept;

// 0x80042238: player hand-over check of phase 0. Writes +0x449 = -1 and
// +0xBD |= 0x40; returns 1 when the event may take the player. State 1 action
// 14 reads byte 0x800C1B93; an unknown value returns nullopt (held).
std::optional<bool> source_player_event_ready_80042238(L3PlayerCompatWork&,
    std::optional<uint8_t> byte_800c1b93) noexcept;
// 0x80041B8C, player state 3 (event-owned pose). Substate 0 snapshots the
// pose into the targets, 1 runs 0x800423A0, 2 copies +0x438.. into the pose
// every frame and requests animation +0x448, 3 runs 0x800423C8 and returns
// the player to state 1 with +0xBD = 8. The trailing 0x80051BAC call and the
// +0xA6 bit-7 sound 0x8001A008(144) are not ported.
void source_player_state3_80041b8c(L3PlayerCompatWork&) noexcept;
// 0x8003C07C input selection for +0x11C/+0x11E: +0xBD bit 0x40 zeroes them,
// bit 0x20 takes the forced pad 0x800C0F96 (+0x11E from 0x800C0F9A, never
// written by event 0, so 0). The bit-0x10 pose copy at 0x8003C164..C210 is
// not ported (event 0 never sets that bit). Returns true when the real pad
// was replaced.
bool source_player_input_select_8003c07c(L3PlayerCompatWork&, uint16_t forced_0f96) noexcept;
// 0x80048A9C forced walk; increments event +5 when done.
void source_event0_forced_walk_80048a9c(SourceDoorEvent0&, L3PlayerCompatWork&, uint8_t stage) noexcept;
// 0x80042314 player release (phase 2), without its 0x8002D004 call.
void source_player_event_release_80042314(L3PlayerCompatWork&) noexcept;
// 0x8003C7B0, player state 0 (re-entry after phase 2 writes +8 word 0):
// resets motion/request lanes and sets state 1. Its 0x800321C8(p,0,5,0) call
// is not ported.
void source_player_state0_8003c7b0(L3PlayerCompatWork&) noexcept;

// ST00 overlay door object, state table 0x8011C928:
// 0 0x801124B0 init, 1 0x80112568 closed, 2 0x801126DC opening,
// 3 0x801127A0 open, 4 0x801128CC closing, 5 0x80112968 restore.
// It owns one cell (0x80048C60 of its position, object +68) and swaps that
// cell's IDX descriptor word (0x8016C008 + cell*12) between the closed one
// (+44) and the one 0x8003AC24 finds for (closed word0 & ~0x1000) (+48).
struct SourceDoorObject {
    uint16_t idx_index=0;          // +68 packed cell (positive = IDX row)
    uint16_t closed_descriptor=0;  // +44
    uint16_t open_descriptor=0;    // +48
    uint8_t state=1;               // +8
    uint8_t frames=0;              // +11
};
struct SourceDoorObjectStep {
    std::optional<bool> flag_7b1,flag_7b2;
    std::optional<uint16_t> descriptor_write; // new descriptor for idx_index
    uint16_t sound=0;                          // 0x8001A050/0x8001A008 id
};
// 0x8003AC24 over the cooked HED descriptors: first descriptor whose word0
// equals key. Native descriptors keep word0 == their HED tag (checked for the
// ST00 door cells 123 and 149).
std::optional<uint16_t> source_descriptor_by_word0_8003ac24(const RuntimeMap&, uint32_t key) noexcept;
// Builds the object for a cell whose descriptor word0 has bit 0x1000 and an
// open variant exists (state 0 init result). nullopt otherwise.
std::optional<SourceDoorObject> source_door_object_init_801124b0(const RuntimeMap&, uint16_t idx_index) noexcept;
// One object update. player_cell is 0x80048C60 of the player position.
// The object +13 lock flag (0x7E0 + id) is not read: the id is object data
// that is not cooked (CANDIDATE: treated as clear).
SourceDoorObjectStep source_door_object_step(SourceDoorObject&, bool flag_7b0, bool flag_7b1,
    std::optional<int16_t> player_cell, const RuntimeMap&) noexcept;
} // namespace l3
