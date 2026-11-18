#pragma once
// ST03 scene objects (docs/research_01516/STAGE_LOADING.md, "ST03 scene
// objects"): the object sets 0x80103AB0 spawns from 0x80116578, and ST03's
// generic NPC 0x80107B84 (variant 0 of Roll 0x05, Barrell 0x25, Data 0x11,
// the Inspector 0x26 and the officer 0x24). Ported: spawn, init (state 0),
// the tick (state 1) with its range check, floor and animation, behaviour 0
// (idle and fidget), behaviour 1 (walk) and the talk (0x8010A344, behaviour
// 4); the police car (0x801104C0: idle and the waypoint drive; its mesh and
// dust are not drawn); the variant-1 scene NPCs (Barrell 0x8010D0F4, the Inspector 0x8010D968,
// the officer 0x8010C66C: init, tick, all their behaviours; sounds reported).
// Generic behaviours 2, 3, 5..7, shadows and the family-0x60 police car
// are reported, not run.
// Original addresses are identity tokens.
#include "legends3ds/source_angle.h"
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
constexpr uint32_t kSt03CarPaths = 0x80117274u;         // police car waypoints {x, z, w, h}
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

// Check boxes at +0x10: x0, x1, z0, z1, y0, y1. The generic NPC's
// (0x80116E1C) and Barrell variant 1's (0x80117134).
constexpr std::array<int16_t,6> kSt03NpcCheckBox{{-32, 32, -32, 32, -32, 150}};
constexpr std::array<int16_t,6> kSt03Barrell1CheckBox{{-32, 32, -32, 32, -32, 32}};

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
    uint8_t variant = 0;                       // work +4 (record [2]): the class row's column
    bool car = false;                          // the police car, family 0x60 class 0x0E (0x801104C0)
    // Police car: +0x48 strafe, +0x4C acceleration, +0xBC bit 0 restart,
    // +0xC0 timer, +0xC4 waypoint index, 0x8011CB28 last waypoint.
    int16_t strafe_48 = 0, accel_4c = 0, car_timer_c0 = 0, car_index_c4 = 0;
    uint8_t car_last = 0;
    bool car_restart = false;
    bool scene_v1 = false;                     // variant 1 of Barrell 0x25 (0x8010D0F4), the Inspector
                                               // 0x26 (0x8010D968), the officer 0x24 (0x8010C66C)
    std::array<int16_t,6> check_box = kSt03NpcCheckBox;   // +0x10
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
// `generic_classes`: the stage's classes whose variant 0 is the generic NPC
// (from its own class table); null keeps ST03's list (05, 25, 11, 26, 24).
using St03GenericClasses = std::array<bool,128>;
St03Npc spawn_st03_object(const St03ObjectRecord& r, const St03GenericClasses* generic_classes = nullptr);
bool st03_generic_npc_class(uint8_t object_class, uint8_t variant) noexcept;
// A +4 write (and the spawn): picks the object for the class row's column.
void st03_object_set_variant(St03Npc& n, uint8_t variant, const St03GenericClasses* generic_classes = nullptr) noexcept;

struct St03NpcInputs {
    int16_t player_x = 0, player_z = 0;        // 0x800B51C4 / 0x800B51C8
    int16_t player_y = 0;                      // 0x800B51C6
    std::optional<int16_t> floor_y;            // 0x80048E6C(work) (CANDIDATE: the port's floor query)
    std::optional<uint16_t> rand;              // 0x8007FE50 when a behaviour calls it
    std::optional<uint16_t> rand2;             // a second 0x8007FE50 in the same tick (behaviour 1 state 1)
    const ReferenceTables* refs = nullptr;     // 0x80030058 trig (behaviour 1 walk and ahead check)
    const std::array<uint8_t,112>* walk_speeds = nullptr;  // 0x80116DAC by class (0x8010A49C mode 1)
    const std::array<uint8_t,112>* walk_tables = nullptr;  // 0x80116ED4: choice rows, then timer rows
    // Barrell variant 1:
    const SourceAngleSteps* angle_steps = nullptr;         // 0x80031198 for 0x80031BEC
    std::optional<std::array<int16_t,3>> id_target;        // 0x80036294(+0x0C)->+0x14 (resolved by the caller)
    std::optional<std::array<int16_t,3>> id_target2;       // 0x80036294(+0x0D)->+0x14
    // Police car:
    const std::array<uint8_t,88>* car_paths = nullptr;     // 0x80117274 (path A, 6 x 8) + 0x801172A4 (path B, 5 x 8)
    bool flag4 = false;                                    // 0x8001DA58(4): the car is drawn
    bool animation_finished = false;           // +0xA7 < 0
    const std::array<uint8_t,64>* idle_timers = nullptr;   // 0x80116E68
    // Talk (shared NPC talk path):
    bool player_targets_me = false;            // player +0x184 (0x800B5334) == this work
    std::optional<uint16_t> heading_to_player; // 0x80031198(npc +0x14, player +0x14) (result bits)
    bool window_open = false;                  // 0x80098A58 & 0x00200000
    bool talk_active = false;                  // 0x800987B0 (the talk runner's block +0)
    bool save_phase = false;                   // main phase 0x800C1B10 == 6 (Data's Save, see DATA_MENU.md)
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
    bool car_dust = false;                     // the car's 0x80031618 dust (not ported)
    bool id_target_missing = false;            // behaviour 1/2 needed 0x80036294(+0x0C)
    bool clear_player_target = false;          // *(0x800B5334) = 0
    std::optional<uint16_t> flag_set;          // 0x8001DA8C(flag)
    bool talk_unported = false;                // a talk behaviour other than 4 (Data 7, classes 52 / 73)
    bool behaviour_unported = false;           // +9 not ported (1..3, 5..7)
    bool talk_target_taken = false;            // 0x8010A344 switched to the talk behaviour
    std::optional<uint8_t> talk_start;         // 0x80063BC8(npc, 0, 0, +0x0C): message id
    bool checkable = false;                    // 0x80051B68(work, 1, -1, 0): a talk candidate this frame
    std::optional<uint8_t> reopen_menu;        // Data state 3: 0x80053A08(0, 0xDC) after the save screen
};
St03NpcTick step_st03_npc(St03Npc& npc, const St03NpcInputs& in) noexcept;

// 0x8010A44C(class): idle request; 0x8010A3AC(class): fidget request
// (class 0x53 picks 8 or 9 from rand).
uint8_t st03_npc_idle_request(uint8_t object_class) noexcept;
uint8_t st03_npc_fidget_request(uint8_t object_class, uint16_t rand) noexcept;

// The class-0x20 row for one NPC class in the area EBD, with the requests
// the generic NPC issues (0, 1, 8, 9; those the bank has), within `cap`.
St02ActorModel decode_st03_npc_model(const EbdDirectory& ebd, uint8_t object_class, size_t cap) noexcept;

} // namespace l3
