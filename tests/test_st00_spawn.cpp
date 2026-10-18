#include "legends3ds/st00_spawn.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <functional>
#include <stdexcept>

static bool rejects(const std::function<void()>& f) {
    try { f(); } catch (const std::runtime_error&) { return true; }
    return false;
}
int main() {
    l3::St00SpawnCatalog input;
    l3::St00SpawnRegion r;
    r.id = 2; r.qualifier = 1; r.min_x = -3; r.min_z = -16; r.width = 5; r.depth = 2;
    l3::St00ConditionalSpawn o; o.object.raw[0] = 1; o.object.raw[1] = 0x20;
    o.object.raw[2] = 3; o.object.raw[3] = 128; o.predicate = 1599;
    r.records.push_back(o); input.areas[1].regions.push_back(r);
    assert(l3::st00_spawn_region_contains(r, -1536, -8192));
    assert(!l3::st00_spawn_region_contains(r, -1537, -8192));
    assert(l3::st00_spawn_region_contains(r, 1023, -7169));
    assert(!l3::st00_spawn_region_contains(r, 1024, -7169));
    assert(!l3::st00_spawn_region_contains(r, 0, -7168));
    assert(l3::st00_spawn_selection(input.areas[1], -1, 0, -8000) == 0);
    assert(!l3::st00_spawn_selection(input.areas[1], 1, 0, -8000));
    input.areas[1].regions.push_back(r);
    assert(l3::st00_spawn_selection(input.areas[1], -1, 0, -8000) == 1);
    auto bytes = l3::cook_l3st00_spawn(input);
    const auto h = l3::inspect_l3st00_spawn(bytes);
    assert(h.area_count == 5 && h.region_count == 2 && h.record_count == 2);
    const auto output = l3::load_l3st00_spawn(bytes);
    assert(output.areas[0].regions.empty());
    assert(output.areas[1].regions[0].records[0].predicate == 1599);
    assert(output.areas[1].regions[0].records[0].object.raw[3] == 128);
    assert(l3::cook_l3st00_spawn(output) == bytes);
    auto bad = bytes; bad.back() ^= 1;
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad); }));
    bad = bytes; l3::patch_u32(bad, 12, 0xffffffffu);
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    bad = bytes; l3::patch_u32(bad, h.off_areas + 8, 1);
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    bad = bytes; l3::patch_u32(bad, h.off_regions + 8, 1);
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    bad = bytes; l3::patch_u32(bad, h.off_records + 20, 2048);
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    bad = bytes; bad[h.off_records] = 128;
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    bad = bytes; bad.push_back(0);
    assert(rejects([&] { (void)l3::load_l3st00_spawn(bad, false); }));
    assert(rejects([&] { (void)l3::parse_st00_spawn_us({1,2,3}); }));
}
