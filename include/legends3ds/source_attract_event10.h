#pragma once
// ST07 event 10, the title attract scene (docs/research_01516/ATTRACT_EVENT10.md):
// handler 0x80100330 (phases 0x8010036C / 0x80100598 / 0x801006D8) on the
// shared scene runtime (source_scene_runtime.h), and the record callbacks'
// background and area effects. Actor spawns/releases in the callbacks are not ported yet;
// the steps report them. Original addresses are identity tokens.
#include "legends3ds/source_bg_layers.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_scene_runtime.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// The event's tables, bound from the cooked L3CS package by original address:
// timelines 0x80105E1C / 0x801060D4, camera programs 0x80105DD0 / 0x80105E2C,
// the background rows 0x80106124 (cook_cutscene.py --extra) and the
// importer's callback -> area requests (0x80100CD4).
struct SourceAttractProfile {
    bool loaded = false;
    std::array<SourceSceneTimeline,2> timelines;
    std::array<SourceSceneCameraProgram,2> cameras;
    std::array<std::array<uint8_t,8>,8> bg_rows{};
    std::vector<SourceSceneAreaRequest> area_requests;
    std::vector<std::array<uint8_t,20>> records;   // 0x80105B8C.. (29 spawn records)
    std::vector<uint32_t> servbot_behaviours;      // 0x8010663C[id] (class 0x20/0x00 by +0xC)
    std::vector<uint8_t> teisel_face;              // 0x80106914..: face scripts + tables 0x80106944/0x80106950
};
bool bind_source_attract_profile(const SourceScenePackage& package, SourceAttractProfile& out);

struct SourceAttractEvent10State {
    uint8_t phase = 0, sub = 0;
    uint8_t flags_0a = 0, count_0b = 0;
    uint32_t callback_counter_80108b0c = 0;
    uint8_t timeline = 0, camera_program = 0;   // which cooked table is current
    size_t record = 0;
    SourceEventCounters event;
    SourceCameraBlock camera;
    bool done = false;
};
struct SourceAttractEvent10Step {
    uint8_t fade_type = 0;          // 0x80017C30 request this tick
    bool cd_command = false;        // 0x8001D58C(0x20000005)
    int8_t area_request = -1;       // 0x80100CD4(area)
    uint8_t actor_callbacks = 0;    // callbacks whose actor spawns/releases are not ported
    uint8_t release_slots = 0;      // 0x80031824 on event+0x24.. slots 0..n-1 this tick
    int8_t spawn_first = -1;        // 0x8001EAE8 on records [spawn_first, spawn_first + spawn_count)
    uint8_t spawn_count = 0;
    uint32_t callback = 0;          // record callback run this tick (0 = none)
    uint8_t fx_spawn = 0;           // class-0x80 emitters: 1 = 0x8010086C at 130, 2 = 0x801009B8 at 130
    bool fx_clear = false;          // 0x801009B8 first tick: 0x80031988 on all 64 slots of 0x800A3B40
};
// One tick of 0x80100330. cd_done: 0x8001D878; fade_busy: 0x800A8A72;
// start: pad bit 8 (0x800C0C2A). layers: the 0x800BBA90 records (area 0
// set already initialised by the stage init).
SourceAttractEvent10Step step_source_attract_event10_80100330(SourceAttractEvent10State& st,
    const SourceAttractProfile& profile, SourceBgLayers& layers, bool cd_done, int16_t fade_busy, bool start) noexcept;

// A Servbot (family 0x20 class 0x00, per-type table 0x801052DC[4] =
// 0x8010205C). Spawn 0x8001E968: +0x14/+0x1C position from rec[12..17],
// heading +0x56 = rec[18..19]; 0x8001EAE8: +0xC..+0xF = rec[8..11] (id,
// animation request, size flag, +0xF).
struct SourceAttractServbot {
    uint8_t slot = 0, id = 0, request = 0, big = 0, byte_0f = 0;
    std::array<int16_t,3> raw{};        // +0x14
    std::array<int32_t,3> fixed{};      // +0x1C (16.16)
    uint16_t heading = 0;               // +0x56
    uint8_t state = 0, started = 0;     // +8, +9
    uint32_t behaviour = 0;             // 0x8010663C[id]
    std::array<int16_t,3> scale() const noexcept { const int16_t v = big == 1 ? 1024 : 512; return {{v, v, v}}; }
};
bool spawn_source_attract_servbot(const std::array<uint8_t,20>& record, const SourceAttractProfile& profile,
                                  SourceAttractServbot& out) noexcept;
struct SourceAttractAnimation { bool reset = false; uint8_t request = 0; bool step = false; bool shadow = false; };
// One update of 0x8010205C: state 0 runs the init 0x8010212C (the floor query
// 0x80048E6C is the caller's), then every update saves nothing the port uses
// and runs the behaviour 0x8010663C[+0xC] with event+0x10 (after the
// timeline's increment this tick).
SourceAttractAnimation update_source_attract_servbot_8010205c(SourceAttractServbot& servbot, uint32_t event_10,
                                                              const ReferenceTables& refs) noexcept;

// The bridge's static cast (family 0x20, per-class table [+4 = 10]): class
// 0x28 (0x80102D84) and class 0x2B (0x80102EA8). Init: drawn flag, model,
// scale +0x158 = (512, 512, 512), floor query (y += floor), animation request
// 19 (0x28) or 0 (0x2B); then 0x80032488 each update (shadow 0x8002C8CC with
// radius 48 not ported). Placement as 0x8001E968.
struct SourceAttractStatic {
    uint8_t object_class = 0, slot = 0, request = 0;
    std::array<int16_t,3> raw{};
    uint16_t heading = 0;
};
bool spawn_source_attract_static(const std::array<uint8_t,20>& record, SourceAttractStatic& out) noexcept;

// Teisel (family 0x40, class 0x0B, per-type table [10] = 0x8010384C), record
// 28. Init: drawn flag, model, scale 512, floor query, animation request 0,
// face set 0x80103A24(expression 0). Then each update: in shot e8 = 4 the
// expression changes (e9 1: tick 10 -> 2, tick 90 -> 0; e9 2: tick 1 -> 2,
// tick 50 -> 1), 0x80032488, then the face step 0x80103A6C.
// Face (+1248): per channel (eyes: script 0x80106944[expr], parts 17 -> 16;
// mouth: 0x80106950[expr], 19 -> 18) the index advances unless the next byte
// is 126 (hold) or 127 (back to 0); a frame change copies the source part's
// UVs with offset 0x8002D404(frame, 42, 64, 6).
struct SourceAttractFaceChannel { uint32_t script = 0; int8_t index = 0; int16_t previous = -1; };
struct SourceAttractTeisel {
    uint8_t slot = 0, state = 0;
    std::array<int16_t,3> raw{};
    uint16_t heading = 0;
    std::array<SourceAttractFaceChannel,2> face{};
};
bool spawn_source_attract_teisel(const std::array<uint8_t,20>& record, SourceAttractTeisel& out) noexcept;
struct SourceAttractFaceCopy { uint8_t destination_part = 0, source_part = 0; uint32_t offset = 0; };
struct SourceAttractTeiselStep {
    bool reset = false, step = false;        // animation request 0 / 0x80032488
    std::array<SourceAttractFaceCopy,2> copies{};
    size_t copy_count = 0;
};
SourceAttractTeiselStep update_source_attract_teisel_8010384c(SourceAttractTeisel& teisel, const SourceAttractProfile& profile,
                                                               const SourceEventCounters& event) noexcept;

// Class 0x80 type 8 (ST07 0x801055B8[8] = 0x801046A0; +13 == 0: emitter
// 0x80106C14[+3], else puff 0x80106C1C[+3]), pool 0x800A3B40 (64 x 0x48).
// Emitters (flags 1, never drawn) spawn a puff when (+9 & 7) == 0, then
// +9 += 1. Set 1 (0x8010086C, shot 2:0 tick 130): x -256, y -672 - 32i,
// z 280 + 63i, +9 = 6 - 2i, i 0..3; state 0 (0x8010470C) puffs: life 20,
// sprite 0x3D48000E, colour 0x2E808080, uv (32, 0), scale (rand & 7) + 16;
// each update (0x801047E0) drawn only while event+8 == 1; at life 15/10/5
// u += 32; y -= 1 + life / 3, scale += 1 + life / 6, life -= 1; released
// at 0. Set 2 (0x801009B8, shot 0:2 tick 130): (-18, -79, 200) +9 = 8 and
// (28, -79, 200) +9 = 4; state 1 (0x80104904) puffs: life 32, z += rand & 3,
// x += rand & 3, sprite 0x3D4E002E, colour 0x2E404040, uv (32, 0), scale
// (rand & 3) + 18; each update (0x801049F4) drawn only while event+8 == 3
// and event+0x10 > 0; colour += 0xFFFDFDFE, y -= life < 17 ? 2 : 1,
// life -= 1. rand is 0x8007FE50 (the LCG; its seed here is CANDIDATE).
// New puffs are not updated on their spawn pass (CANDIDATE slot order).
struct SourceAttractFx {
    bool emitter = false;
    uint8_t state = 0, counter = 0, u = 0, v = 0;   // +3, +9, +0x22, +0x23
    int8_t life = 0;                                 // +14
    std::array<int16_t,3> pos{};                     // +0x1C
    uint16_t scale = 0;                              // +0x24
    uint32_t material = 0, colour = 0;               // +0x14, +0x18
    bool visible = false;                            // flags bit 1
};
struct SourceAttractFxPool { std::vector<SourceAttractFx> objects; uint32_t rand_next = 1; };
void spawn_source_attract_fx(SourceAttractFxPool& pool, uint8_t set) noexcept;
void update_source_attract_fx(SourceAttractFxPool& pool, const SourceEventCounters& event) noexcept;

// Fade type 20 (the scene's natural end, 0x80017C30 -> mode 2, sequence
// 0x800821E0[2] = steps 0, 4, 1): step 4 is the iris wipe 0x80018BB8. Its
// first pass builds the ring (no draw) and sets counter 60; then per fade
// pass (one per game tick): state 1 draws inner radius 4c, outer 210
// (T * 105 >> 11), then c -= 1 and at 0 goes to state 2 with c = 5; state 2
// does c -= 1 first, draws inner 4c, outer 16c + 16, and at c = 0 ends
// (busy 0x80098A72 clears after the sequence). Ring: 32 semi POLY_G4 (code
// 0x3A; inner colour 0, outer 0xFFFFFF) around (160, 120), T[i] =
// (rcos, rsin)(i * 128), after draw mode E1000740 (ABR 2: B - F), OT entry 30.
struct SourceIrisWipe { uint8_t state = 0; int8_t counter = 0; };
struct SourceIrisFrame { bool draw = false, done = false; int16_t inner = 0, outer = 0; };
void start_source_iris_80018bb8(SourceIrisWipe& iris) noexcept;
SourceIrisFrame step_source_iris_80018bb8(SourceIrisWipe& iris) noexcept;
// The ring's packets for one frame (draw mode last, so it draws first after
// the OT prepend). widescreen adds a solid ring from the outer radius to 400
// (3DS: the 400-px screen's corners lie beyond 210; the oracle also shows
// black outside the state-2 ring).
std::vector<FrontendPacket> build_source_iris_packets(const SourceIrisFrame& frame, const ReferenceTables& refs,
                                                        bool widescreen);
} // namespace l3
