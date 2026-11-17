// ST03 object sets (0x80103AB0) and the generic NPC 0x80107B84 on synthetic data.
#include "legends3ds/source_st03_objects.h"
#include <cstdio>

using namespace l3;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

static void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = uint8_t(v >> (8 * i)); }
static void put16(std::vector<uint8_t>& b, size_t o, int16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(uint16_t(v) >> 8); }

int main() {
    // Set 0: Data (0x11, +0x0D = 0 -> behaviour 0) and a family-0x60 object.
    SourceScenePackage pkg; pkg.loaded = true;
    SourceSceneData table{kSt03ObjectSetTable, std::vector<uint8_t>(64)};
    put32(table.bytes, 0, 2); put32(table.bytes, 4, kSt03ObjectSetRecordsA);
    SourceSceneData recs{kSt03ObjectSetRecordsA, std::vector<uint8_t>(40)};
    recs.bytes[0] = 2; recs.bytes[1] = 0x11; recs.bytes[3] = 0x20; recs.bytes[8] = 9; recs.bytes[9] = 0; recs.bytes[10] = 1;
    put16(recs.bytes, 12, -5196); put16(recs.bytes, 14, -1); put16(recs.bytes, 16, 1269); put16(recs.bytes, 18, 0x0E77);
    recs.bytes[20] = 3; recs.bytes[21] = 0x0E; recs.bytes[23] = 0x60;
    pkg.data = {table, recs};
    const auto set = st03_object_set(pkg, 0);
    CHECK(set.size() == 2 && set[0].slot == 2 && set[0].object_class == 0x11 && set[0].position[0] == -5196 &&
          set[0].position[2] == 1269 && set[0].heading == 0x0E77 && set[1].family == 0x60);
    CHECK(st03_object_set(pkg, 1).empty());

    auto data = spawn_st03_object(set[0]);
    auto car = spawn_st03_object(set[1]);
    CHECK(data.generic && !car.generic && data.w.raw_14[0] == -5196 && data.w.rot_54[1] == 0x0E77);

    std::array<uint8_t,64> timers{};
    for (size_t i = 0; i < 64; ++i) timers[i] = uint8_t(10 + i);
    St03NpcInputs in; in.idle_timers = &timers; in.player_x = -5000; in.player_z = 1300; in.floor_y = -2;
    // Init + tick: scale 512, radius 5000, behaviour 0, idle request 1 for Data
    // (init's request 0, then 1 the same frame), timer from row +0x0E & 3 = 1.
    in.rand = 0x0003;
    auto t = step_st03_npc(data, in);
    CHECK(data.w.state_08 == 1 && data.w.state_09 == 0 && data.radius_368 == 5000 && data.w.word_158 == 0x02000200u &&
          data.w.word_15c == 512 && (data.w.flags_00 & 2));
    CHECK(t.in_range && t.rand_used && t.anim_request && *t.anim_request == 1 && data.w.pose_ac == 1);
    CHECK(data.timer_350 == int16_t(timers[16 + 3]) && data.w.sub_0a == 1 && data.w.raw_14[1] == -2 && t.talk_unported);
    // Count down to the fidget (Data: request 1, unchanged -> advance).
    in.rand.reset();
    int n = 0;
    while (data.w.sub_0a == 1 && n < 100) { t = step_st03_npc(data, in); ++n; }
    CHECK(n == timers[19] && data.w.sub_0a == 2 && data.w.pose_ac == 1 && t.anim_advance);
    t = step_st03_npc(data, in); CHECK(data.w.sub_0a == 2);           // waits for +0xA7 < 0
    in.animation_finished = true;
    t = step_st03_npc(data, in); CHECK(data.w.sub_0a == 0);
    t = step_st03_npc(data, in); CHECK(t.rand_missing && data.w.sub_0a == 0);   // rand needed again
    // Out of range: nothing runs.
    in.player_x = 2000;
    t = step_st03_npc(data, in); CHECK(!t.in_range && !t.anim_request && !t.anim_advance);
    // Behaviour other than 0 is reported.
    auto roll_rec = set[0]; roll_rec.object_class = 0x05; roll_rec.word_08[1] = 0x21;
    auto roll = spawn_st03_object(roll_rec);
    in.player_x = -5000; in.rand = 1;
    t = step_st03_npc(roll, in);
    CHECK(roll.w.state_09 == 1 && roll.radius_368 == 5000 && t.behaviour_unported && *t.anim_request == 0);
    auto big = roll_rec; big.word_08[1] = 0x10; big.object_class = 0x26;
    auto insp = spawn_st03_object(big); step_st03_npc(insp, in);
    CHECK(insp.radius_368 == 10000 && insp.w.word_15c == 471);
    // Requests.
    CHECK(st03_npc_idle_request(0x11) == 1 && st03_npc_idle_request(0x05) == 0);
    CHECK(st03_npc_fidget_request(0x05, 0) == 8 && st03_npc_fidget_request(0x11, 0) == 1 && st03_npc_fidget_request(42, 0) == 9 &&
          st03_npc_fidget_request(83, 1) == 9 && st03_npc_fidget_request(83, 2) == 8 && st03_npc_fidget_request(57, 0) == 0);
    if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
    std::printf("source_st03_objects: ok\n");
    return 0;
}
