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
} // namespace l3
