// Stage loading beyond ST00: the per-stage flow tables (ST02's four-area
// overlay rows), the cooked-pack layout, and the post-win stage request
// carried into a fresh session for the new stage (arrival at the lanes).
#include "legends3ds/runtime_session.h"
#include "legends3ds/st00_flow.h"
#include "legends3ds/source_stage_load.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace l3;

namespace {
int failures = 0;
void require(bool ok, const char* what) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); ++failures; } }
void put32(std::vector<uint8_t>& d, uint32_t addr, uint32_t v) {
    const size_t o = addr - 0x80100000u; for (int i = 0; i < 4; ++i) d[o + i] = uint8_t(v >> (8 * i));
}

// A synthetic overlay with ST02's table addresses: area i has i objects,
// a shared empty aux list and the selector 16*i (as the real overlay).
std::vector<uint8_t> synthetic_st02() {
    const auto& t = *stage_flow_addresses_us(2);
    std::vector<uint8_t> d(0x7000, 0);
    const uint32_t lists = 0x80106000u, aux = 0x80106800u;
    uint32_t cursor = lists;
    for (uint32_t area = 0; area < t.area_count; ++area) {
        put32(d, t.object_ptr_table + 4 * area, cursor);
        put32(d, t.aux_ptr_table + 4 * area, aux);
        put32(d, t.resource_ptr_table + 4 * area, 0x80106E00u);
        put32(d, t.selector_table + 4 * area, 16 * area);
        for (uint32_t i = 0; i < area; ++i) { d[cursor - 0x80100000u] = uint8_t(i); d[cursor - 0x80100000u + 1] = 0x20; cursor += 20; }
        d[cursor - 0x80100000u] = 0xFF; cursor += 4;
        d[t.setup_blob_table - 0x80100000u + 40 * area + 38] = uint8_t(0x10 + area);
        d[t.seed_table - 0x80100000u + 8 * 16 * area] = uint8_t(0xA0 + area);
    }
    d[aux - 0x80100000u] = 0xFF; d[aux - 0x80100000u + 1] = 0xFF;   // -1 sentinel
    return d;
}

RuntimeSession make_session(size_t areas) {
    RuntimeMap map;
    for (size_t i = 0; i < areas; ++i) {
        RuntimeMapArea a; a.start_x = 63; a.start_z = 63; a.width = 2; a.height = 2; a.cell_count = 4; map.areas.push_back(a);
    }
    map.cells.assign(4, int16_t(-32768));
    TutorialRuntimeArea runtime; L3QolSettings settings{}; l3_qol_settings_defaults(&settings);
    return RuntimeSession(std::move(map), std::move(runtime), {}, settings);
}
} // namespace

int main() {
    // Stage tables: ST00 and ST02 are traced, others are not.
    require(stage_flow_addresses_us(0) && stage_flow_addresses_us(0)->area_count == 5 &&
            stage_flow_addresses_us(0)->object_ptr_table == 0x8011AB3Cu, "ST00 table (0x8010038C)");
    const auto* st02 = stage_flow_addresses_us(2);
    require(st02 && st02->area_count == 4 && st02->object_ptr_table == 0x80106940u &&
            st02->resource_ptr_table == 0x801068E0u && st02->selector_table == 0x80106C18u, "ST02 table (0x801000B0)");
    require(stage_flow_addresses_us(3) && stage_flow_addresses_us(3)->area_count == 9 && stage_flow_addresses_us(3)->aux_ptr_table == 0x80114B7Cu,
            "ST03 table (0x80100524)");
    require(!stage_flow_addresses_us(1) && !stage_flow_addresses_us(4), "untraced stages have no table");

    // Parse -> cook -> load keeps four areas and each area's rows.
    const auto flow = parse_stage_flow_us(synthetic_st02(), *st02);
    require(flow.area_count == 4 && flow.areas[3].objects.records.size() == 3 && flow.areas[0].objects.records.empty() &&
            flow.areas[2].setup_selector == 32 && flow.areas[1].setup_seed[0] == 0xA1 && flow.areas[3].setup_blob[38] == 0x13,
            "ST02 flow rows parsed");
    const auto cooked = cook_l3st00_flow(flow);
    require(inspect_l3st00_flow(cooked, true).area_count == 4, "L3SF carries 4 areas");
    const auto loaded = load_l3st00_flow(cooked, true);
    require(loaded.area_count == 4 && loaded.areas[3].objects.records.size() == 3 && loaded.areas[3].setup_blob[38] == 0x13,
            "L3SF round trip");
    bool threw = false;
    try { TutorialRuntimeArea a = load_tutorial_runtime_area(cooked, {}, 4, true); (void)a; } catch (const std::exception&) { threw = true; }
    require(threw, "area 4 of a four-area stage is refused");

    // Pack layout.
    require(stage_asset_path(0, "st00.l3map") == "st00.l3map" && stage_asset_path(0, "ebd/area_00.l3ebd") == "ebd/area_00.l3ebd",
            "ST00 keeps the pack root");
    require(stage_asset_path(2, "st00.l3map") == "stages/st02/map.l3map" && stage_asset_path(2, "st00.l3flow") == "stages/st02/flow.l3flow" &&
            stage_asset_path(2, "materials/area_00.l3mc") == "stages/st02/materials/area_00.l3mc" &&
            stage_asset_path(0x1C, "st00.l3init") == "stages/st1c/init.l3init", "other stages under stages/stNN/");

    // ST00 event 2's lanes: 356C = 2, 356E = 2, 356F = 0, 3560..3565 = 0, 356A = 0.
    std::array<uint8_t,18> lanes{}; lanes[12] = 2; lanes[14] = 2; lanes[15] = 0; lanes[16] = lanes[17] = 0xFF;
    const auto request = source_stage_request_from_lanes(lanes, 0x8010194Cu, 7);
    const auto arrival = source_stage_arrival(request);
    require(request.stage == 2 && request.area == 0 && request.generation == 7 &&
            arrival.position == std::array<int16_t,3>{0, 0, 0} && arrival.heading == 0, "event 2 lanes -> stage 2 area 0 at (0,0,0)");
    lanes[0] = 0x34; lanes[1] = 0x12; lanes[4] = 0xFE; lanes[5] = 0xFF; lanes[10] = 0x00; lanes[11] = 0x08;
    const auto moved = source_stage_arrival(source_stage_request_from_lanes(lanes, 0, 0));
    require(moved.position == std::array<int16_t,3>{0x1234, 0, -2} && moved.heading == 0x800, "lanes are signed little-endian");

    // A session for stage 0 never offers its own stage as a stage request;
    // the carry moves flags, life and frames into a stage-2 session.
    auto st00 = make_session(5);
    require(st00.stage() == 0 && !st00.source_stage_request(), "no request without a hand-off");
    st00.reset_player({0, 0, 0}, 0, 60, 80);
    st00.set_story_flag(0x383, true); st00.set_story_flag(0x600, true);
    st00.initialize_fresh_source_statistics_8001326c();
    const auto carry = st00.make_stage_carry();
    require(carry.hp == 60 && carry.hp_max == 80 && carry.stats_selector == 1, "carry: life and selector");

    auto st2 = make_session(4);
    st2.set_stage(2);
    for (size_t i = 1; i < 4; ++i) { TutorialRuntimeArea a; a.area_index = i; st2.register_area(i, a); }
    require(st2.apply_stage_load(carry, arrival), "stage 2 area 0 load applies");
    const auto p = st2.player_position();
    require(st2.map_area_index() == 0 && p.x == 0 && p.y == 0 && p.z == 0 && st2.player_yaw() == 0, "arrival at the lanes");
    require(st2.story_flag(0x383) && st2.story_flag(0x600) && !st2.story_flag(0x606), "story flags survive");
    require(st2.hp() == 60 && st2.hp_max() == 80 && st2.source_stats_selector() == 1, "life and selector survive");
    SourceStageArrival a1 = arrival; a1.area = 1; a1.position = {100, -5, 200};
    auto st2b = make_session(4); st2b.set_stage(2);
    TutorialRuntimeArea r1; r1.area_index = 1; st2b.register_area(1, r1);
    require(st2b.apply_stage_load(carry, a1) && st2b.map_area_index() == 1 && st2b.player_position().x == 100, "arrival into area 1");
    SourceStageArrival wrong = arrival; wrong.stage = 3;
    require(!st2.apply_stage_load(carry, wrong), "a load for another stage is refused");
    // ST02 event 4: the first-visit poll sets 0x600 and starts it; it ends
    // with the stage 3 area 1 request (3570 = 2) and clears 0x600.
    {
        auto s4 = make_session(4); s4.set_stage(2);
        SourcePostWinProfile p4; p4.loaded = true; p4.event = 4;
        p4.timeline.token = 0x8010738Cu;
        p4.timeline.records = {{{0x50000u, 0x80101000u}}, {{0xffu, 0u}}};
        for (uint8_t i = 0; i < 5; ++i) { std::array<uint8_t,20> r{}; r[0] = i; p4.spawns.push_back(r); }
        require(s4.bind_source_post_win_profile(p4) && s4.source_post_win_profile_bound(4), "event 4 profile binds");
        s4.reset_player({0, 0, 0}, 0, 80, 80);
        s4.tick({});
        require(s4.story_flag(0x600) && s4.source_post_win_event().active && s4.source_post_win_event().event == 4,
                "first visit: 0x80100204 sets 0x600, table 0x80106C28 starts event 4");
        int guard = 0;
        while (!s4.pending_source_post_win_cd() && guard++ < 10) s4.tick({});
        require(s4.pending_source_post_win_cd() == 0x20000004u, "XA 0x20000004 requested");
        s4.acknowledge_source_post_win_cd(0x20000004u);
        guard = 0;
        while (!s4.source_post_win_handoff() && guard++ < 300) s4.tick({});
        const auto r4 = s4.source_stage_request();
        require(r4 && r4->stage == 3 && r4->area == 1 && r4->source_function == 0x80101CC0u &&
                s4.source_post_win_handoff()->raw[16] == 2 && s4.source_post_win_handoff()->raw[17] == 0xFF,
                "event 4 requests stage 3 area 1 (3570 = 2)");
        require(!s4.story_flag(0x600) && !s4.source_post_win_transition(), "0x600 cleared; not an area transition");
        s4.tick({});
        require(!s4.source_post_win_event().active, "event 4 does not restart (visit byte counted)");
    }
    // Event 4's records: slot commands and the 0x801002C8 area cut (356C = 0xFF).
    {
        auto ev = start_source_post_win_event(4);
        SourcePostWinStep out; SourcePostWinInputs in;
        run_source_post_win_callback(ev, 0x80100728u, in, out);
        require(out.area_cut == 1 && out.sound_8002ee90 && (ev.slots[0].flags_00 & 2) && (ev.slots[4].flags_00 & 2) &&
                !(ev.slots[3].flags_00 & 2), "0x80100728: cut to area 1, slots 0..2/4 drawn, slot 3 hidden");
        run_source_post_win_callback(ev, 0x801014BCu, in, out);
        require(ev.slots[0].state_09 == 7 && ev.slots[0].fixed_1c == int32_t(0xF6C00000u), "event 4's 0x801014BC (state 7)");
        auto ev2 = start_source_post_win_event(2);
        run_source_post_win_callback(ev2, 0x801014BCu, in, out);
        require(ev2.slots[0].state_09 == 5, "event 2's 0x801014BC is a different callback (state 5)");
        ev.counters.c10 = 108; run_source_post_win_callback(ev, 0x80100874u, in, out);
        require(ev.slots[0].state_09 == 2, "0x80100874 at t = 108 writes slot 0 (a1 kept)");
        const auto cut = source_st02_event_area_cut(3);
        require(cut.event_cut() && cut.raw[14] == 2 && cut.raw[15] == 3 && cut.raw[16] == 0xFF, "0x801002C8 lanes");

        auto s4 = make_session(4); s4.set_stage(2);
        for (size_t i = 1; i < 4; ++i) { TutorialRuntimeArea a; a.area_index = i; s4.register_area(i, a); }
        SourcePostWinProfile p4; p4.loaded = true; p4.event = 4;
        p4.timeline.token = 0x8010738Cu;
        p4.timeline.records = {{{0x50000u, 0x80100728u}}, {{0x50001u, 0x80100A28u}}, {{0xffu, 0u}}};
        for (uint8_t i = 0; i < 5; ++i) { std::array<uint8_t,20> r{}; r[0] = i; p4.spawns.push_back(r); }
        require(s4.bind_source_post_win_profile(p4), "event 4 profile binds (cuts)");
        s4.reset_player({10, 0, 20}, 0, 80, 80);
        int guard = 0, cuts = 0;
        std::vector<size_t> areas;
        while (!s4.source_stage_request() && guard++ < 400) {
            s4.tick({});
            if (const auto cd = s4.pending_source_post_win_cd()) s4.acknowledge_source_post_win_cd(*cd);
            if (const auto t = s4.source_post_win_transition()) {
                require(t->source_function == 0x801002C8u && s4.can_commit_source_post_win_transition(), "cut offered");
                const uint32_t ticks = s4.source_post_win_event().ticks;
                s4.tick({});
                require(s4.source_post_win_event().ticks == ticks, "the event waits for the cut's load");
                require(s4.commit_source_post_win_transition(), "cut commits");
                ++cuts; areas.push_back(s4.map_area_index());
                require(s4.source_post_win_event().active && s4.player_position().x == 10 && s4.player_position().z == 20,
                        "cut keeps the event running and the player in place");
            }
        }
        require(cuts == 2 && areas == std::vector<size_t>({1, 0}), "event 4 cuts to area 1, then area 0");
        require(s4.source_stage_request() && s4.source_stage_request()->stage == 3, "event 4 still ends with stage 3");
    }
    if (failures) return EXIT_FAILURE;
    std::puts("stage_load: ok");
    return 0;
}
