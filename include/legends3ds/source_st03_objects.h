#pragma once
// ST03 scene objects (docs/research_01516/STAGE_LOADING.md, "ST03 scene
// objects"): the object sets 0x80103AB0 spawns from 0x80116578, and ST03's
// generic NPC 0x80107B84 (variant 0 of Roll 0x05, Barrell 0x25, Data 0x11,
// the Inspector 0x26 and the officer 0x24). Ported: spawn, init (state 0),
// the tick (state 1) with its range check, floor and animation, behaviour 0
// (idle and fidget), behaviour 1 (walk) and the talk (0x8010A344, behaviour
// 4). Behaviours 2, 3, 5..7, shadows and the family-0x60 police car are
// reported, not run.
// Original addresses are identity tokens.
#include "legends3ds/source_scene_runtime.h"
#include "legends3ds/source_st02_event4_actors.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

constexpr uint32_t kSt03ObjectSetTable = 0x80116578u;   // 8 sets x {count, records}
constexpr uint32_t kSt03ObjectSetRecordsA = 0x801154B0u, kSt03ObjectSetRecordsB = 0x801158E8u;
constexpr uint32_t kSt03NpcIdleTimers = 0x80116E68u;    // 4 rows x 16 bytes
constexpr uint32_t kSt03NpcWalkSpeeds = 0x80116DACu;    // byte per class
constexpr uint32_t kSt03NpcWalkTables = 0x80116ED4u;    // 4 choice rows + 3 timer rows
constexpr size_t kSt03ObjectSlots = 10;

struct ReferenceTables;

struct St03ObjectRecord {
    uint8_t slot = 0, object_class = 0, subtype = 0, family = 0, byte_04 = 0;
    std::array<uint8_t,4> word_08{};          // -> work +0x0C..+0x0F
    std::array<int16_t,3> position{};
    uint16_t heading = 0;
};
// Records of set n, read from the cooked event-8 data (empty when absent).
std::vector<St03ObjectRecord> st03_object_set(const SourceScenePackage& package, uint8_t n);

struct St03Npc {
    St02EventActorWork w;                      // shared actor work (slot, pose, scale, flags)
    uint8_t object_class = 0, family = 0;
    uint16_t flags_348 = 0;                    // +0x348 (bit 1: talk handler on)
    int16_t timer_350 = 0;                     // +0x350
    std::array<int16_t,3> home_354{};          // +0x354
    uint16_t heading_352 = 0;                  // +0x352
    uint16_t radius_368 = 5000;                // +0x368
    uint8_t pose_ad = 0;                       // +0xAD previous request
    uint8_t byte_0e = 0, byte_0f = 0;          // +0x0E / +0x0F (record [10..11])
    bool generic = false;                      // handled by 0x80107B84 (family 0x20, +3 == 0)
    // Talk block +0x348..: +0x34A saved request, +0x34B talk sub-state,
    // +0x34C saved heading, +0x34E turn step.
    uint8_t saved_34a = 0, talk_34b = 0;
    uint16_t saved_heading_34c = 0;
    int16_t turn_step_34e = 0;
    // Behaviour 1 (walk): +0x44 speed, +0x64 target heading, +0x35C leg
    // direction (0 out, 1 back), +0x35E steps left, +0x366 leg length.
    int16_t speed_44 = 0;
    uint16_t target_heading_64 = 0;
    int16_t walk_dir_35c = 0, leg_left_35e = 0, leg_len_366 = 0;
};

// 0x8001EAE8 + 0x8001E968 for one record, then 0x80107BC0 (state 0) when
// the class is one of the generic NPCs.
St03Npc spawn_st03_object(const St03ObjectRecord& r);
bool st03_generic_npc_class(uint8_t object_class, uint8_t byte_04) noexcept;

struct St03NpcInputs {
    int16_t player_x = 0, player_z = 0;        // 0x800B51C4 / 0x800B51C8
    std::optional<int16_t> floor_y;            // 0x80048E6C(work) (CANDIDATE: the port's floor query)
    std::optional<uint16_t> rand;              // 0x8007FE50 when a behaviour calls it
    std::optional<uint16_t> rand2;             // a second 0x8007FE50 in the same tick (behaviour 1 state 1)
    const ReferenceTables* refs = nullptr;     // 0x80030058 trig (behaviour 1 walk and ahead check)
    const std::array<uint8_t,112>* walk_speeds = nullptr;  // 0x80116DAC by class (0x8010A49C mode 1)
    const std::array<uint8_t,112>* walk_tables = nullptr;  // 0x80116ED4: choice rows, then timer rows
    bool animation_finished = false;           // +0xA7 < 0
    const std::array<uint8_t,64>* idle_timers = nullptr;   // 0x80116E68
    // Talk (shared NPC talk path):
    bool player_targets_me = false;            // player +0x184 (0x800B5334) == this work
    std::optional<uint16_t> heading_to_player; // 0x80031198(npc +0x14, player +0x14) (result bits)
    bool window_open = false;                  // 0x80098A58 & 0x00200000
    bool talk_active = false;                  // 0x800987B0 (the talk runner's block +0)
};
struct St03NpcTick {
    bool in_range = false;                     // 0x8010A4EC
    std::optional<uint8_t> anim_request;       // 0x800322A8(work, +0xAC, 0) (init's request 0 included)
    bool anim_advance = false;                 // 0x80032488 (after any request this tick)
    bool rand_used = false;
    bool rand2_used = false;
    bool rand_missing = false;
    bool table_missing = false;                // walk_speeds / walk_tables absent
    bool refs_missing = false;                 // refs absent (no walk, no ahead check)
    bool moved = false;                        // 0x8002FEA4 ran
    bool talk_unported = false;                // a talk behaviour other than 4 (Data 7, classes 52 / 73)
    bool behaviour_unported = false;           // +9 not ported (1..3, 5..7)
    bool talk_target_taken = false;            // 0x8010A344 switched to the talk behaviour
    std::optional<uint8_t> talk_start;         // 0x80063BC8(npc, 0, 0, +0x0C): message id
    bool checkable = false;                    // 0x80051B68(work, 1, -1, 0): a talk candidate this frame
};
St03NpcTick step_st03_npc(St03Npc& npc, const St03NpcInputs& in) noexcept;
// The NPC's check box at +0x10 (0x80116E1C): x0, x1, z0, z1, y0, y1.
constexpr std::array<int16_t,6> kSt03NpcCheckBox{{-32, 32, -32, 32, -32, 150}};

// 0x8010A44C(class): idle request; 0x8010A3AC(class): fidget request
// (class 0x53 picks 8 or 9 from rand).
uint8_t st03_npc_idle_request(uint8_t object_class) noexcept;
uint8_t st03_npc_fidget_request(uint8_t object_class, uint16_t rand) noexcept;

// The class-0x20 row for one NPC class in the area EBD, with the requests
// the generic NPC issues (0, 1, 8, 9; those the bank has), within `cap`.
St02ActorModel decode_st03_npc_model(const EbdDirectory& ebd, uint8_t object_class, size_t cap) noexcept;

} // namespace l3
