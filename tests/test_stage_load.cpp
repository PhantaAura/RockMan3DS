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
    require(!stage_flow_addresses_us(1) && !stage_flow_addresses_us(3), "untraced stages have no table");

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
    if (failures) return EXIT_FAILURE;
    std::puts("stage_load: ok");
    return 0;
}
