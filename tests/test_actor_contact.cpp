#include "legends3ds/actor_contact.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using l3::ActorContactWork;
constexpr l3::RetailPlayerBounds bounds{-60, 60, -60, 60, -128, 0};

l3::RuntimeMap rectangle_map(uint16_t code = 1, uint16_t flags = 0) {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area;
    area.start_x = 62; area.start_z = 62; area.width = 4; area.height = 4; area.cell_count = 16;
    map.areas.push_back(area); map.cells.assign(16, 1); map.idx.resize(2);
    map.idx[1].descriptor_index = 0;
    l3::RuntimeMapDescriptor descriptor;
    descriptor.default_contact_index = 0; descriptor.alt_contact_index = 0;
    map.descriptors.push_back(descriptor); map.contact_lists.push_back({0, 1});
    map.contact_groups.push_back({-100, 100, -100, 100, 0, 1});
    map.contact_primitives.push_back({-128, 128, code, flags});
    return map;
}
ActorContactWork work_at(int16_t x, int16_t y, int16_t z, int16_t before_x,
                         int16_t before_y, int16_t before_z) {
    ActorContactWork work;
    work.raw = {x, y, z, 1234}; work.previous_raw = {before_x, before_y, before_z, 4321};
    for (size_t i = 0; i < 4; ++i) {
        work.fixed[i] = int32_t(work.raw[i]) * 65536 + 12345;
        work.previous_fixed[i] = int32_t(work.previous_raw[i]) * 65536 + 23456;
    }
    work.velocity = {321, -654, 987};
    return work;
}
void expect_work(const ActorContactWork& actual, const ActorContactWork& expected) {
    if (actual.raw != expected.raw || actual.fixed != expected.fixed ||
        actual.velocity != expected.velocity || actual.state74 != expected.state74)
        throw std::runtime_error("original/native actor wrapper mutation mismatch");
}
void scalar_tests() {
    auto map = rectangle_map();
    auto work = work_at(-180, -60, 0, -184, -60, 0);
    const auto before = work;
    auto wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::SourceBackedPartial && wall.result == 0);
    assert(l3::apply_actor_contact_result_8004b414(work, wall));
    expect_work(work, before); // Valid zero result preserves fixed fractions.
    work = work_at(-120, -60, 0, -200, -60, 0);
    wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::SourceBackedPartial && wall.result == 0x101);
    assert(wall.scratch.x == -160);
    assert(l3::apply_actor_contact_result_8004b414(work, wall));
    assert(work.raw[0] == -160 && work.fixed[0] == -160 * 65536);
    assert(work.raw[1] == -60 && work.fixed[1] == -60 * 65536 + 12345);
    assert(work.raw[3] == 1234 && work.velocity[0] == 321);
    work = work_at(0, -60, 0, 0, -60, 0);
    wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::Unsupported); // Deep-overlap handler excluded.
    const auto unsupported_before = work;
    assert(!l3::apply_actor_contact_result_8004b414(work, wall));
    expect_work(work, unsupported_before);
    map = rectangle_map(3);
    wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::SourceBackedPartial && wall.scratch.y == -128);
    assert(l3::apply_actor_contact_result_8004b414(work, wall));
    const auto floor = l3::query_actor_floor_contact_80048ea4(map, 0, work, bounds);
    assert(floor.status == l3::RuntimeContactStatus::SourceBackedPartial && floor.result == -128);
    map = rectangle_map(8);
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map = rectangle_map(0x21);
    work = work_at(-160, -60, 0, -164, -60, 0);
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::SourceBackedPartial);
    work.raw[0] = -159;
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map.contact_groups[0].x1 = map.contact_groups[0].x0;
    work.raw[0] = -180;
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map = rectangle_map(0x301);
    work = work_at(0, -60, 0, -4, -60, 0);
    wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::SourceBackedPartial && wall.result == 0);
    assert(l3::query_actor_floor_contact_80048ea4(map, 0, work, bounds).result == 32767);
    map = rectangle_map(); map.cells.assign(16, static_cast<int16_t>(0x8000));
    work = work_at(0, 31, 0, -4, -1, 0); work.state74 = 1;
    wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
    assert(wall.status == l3::RuntimeContactStatus::SourceBackedPartial && wall.result == 0x1005 && wall.scratch.y == -1);
    assert(l3::apply_actor_contact_result_8004b414(work, wall) && work.state74 == 0);
    assert(l3::query_actor_floor_contact_80048ea4(map, 0, work, bounds).result == -1);
    map.cells.clear();
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map = rectangle_map(); map.contact_lists[0].group_count = UINT32_MAX;
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    assert(l3::query_actor_floor_contact_80048ea4(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map = rectangle_map(); map.contact_groups[0].primitive_count = UINT32_MAX;
    assert(l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds).status == l3::RuntimeContactStatus::Unsupported);
    map = rectangle_map(3); map.contact_groups[0].primitive_count = 6;
    map.contact_primitives.clear();
    // Each pass admits one earlier interval after lowering the current best.
    // A still-changing fourth pass must remain explicit rather than returning
    // a partial floor as if the original unbounded minimum had stabilized.
    for (int16_t i = 0; i < 6; ++i)
        map.contact_primitives.push_back({int16_t(-600 + i * 100), int16_t(-627 + i * 100), 3, 0});
    work = work_at(0, 0, 0, -4, 0, 0);
    assert(l3::query_actor_floor_contact_80048ea4(map, 0, work, bounds).status == l3::RuntimeContactStatus::IterationLimit);
    l3::ActorWallContact external;
    external.status = l3::RuntimeContactStatus::SourceBackedPartial; external.result = -1;
    work = work_at(1, 2, 3, 4, 5, 6); work.state74 = 1;
    assert(l3::apply_actor_contact_result_8004b414(work, external));
    assert(work.raw == work.previous_raw && work.fixed == work.previous_fixed && work.state74 == 1 && work.velocity[0] == 321);
    work = work_at(1, 2, 3, 4, 5, 6); work.state74 = 1; work.object_class = 0;
    assert(l3::apply_actor_contact_result_8004b414(work, external));
    assert(work.raw[0] == 4 && work.raw[1] == 5 && work.raw[2] == 6 && work.raw[3] == 1234);
    assert(work.velocity == (std::array<int16_t, 3>{0, 0, 0}) && work.state74 == 0);
}

struct Row {
    std::vector<int64_t> fields;
    size_t cursor = 0;
    int64_t next() {
        if (cursor >= fields.size()) throw std::runtime_error("truncated source scalar fixture");
        return fields[cursor++];
    }
    void done() const { if (cursor != fields.size()) throw std::runtime_error("extra source scalar fields"); }
};
template<class Function> void rows(const std::string& path, Function function) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read source scalar fixture");
    std::string line; size_t count = 0;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line); Row row;
        int64_t value = 0; while (stream >> value) row.fields.push_back(value);
        try { function(row); row.done(); }
        catch (const std::exception& error) { throw std::runtime_error(path + " row " + std::to_string(count + 1) + ": " + error.what()); }
        ++count;
    }
    if (!count) throw std::runtime_error("empty source scalar fixture");
}
ActorContactWork read_work(Row& row) {
    ActorContactWork work;
    work.object_class = static_cast<uint8_t>(row.next()); work.state74 = static_cast<uint8_t>(row.next());
    for (auto& value : work.raw) value = static_cast<int16_t>(row.next());
    for (auto& value : work.fixed) value = static_cast<int32_t>(row.next());
    for (auto& value : work.previous_raw) value = static_cast<int16_t>(row.next());
    for (auto& value : work.previous_fixed) value = static_cast<int32_t>(row.next());
    for (auto& value : work.velocity) value = static_cast<int16_t>(row.next());
    return work;
}
ActorContactWork read_output(Row& row, ActorContactWork work) {
    for (auto& value : work.raw) value = static_cast<int16_t>(row.next());
    for (auto& value : work.fixed) value = static_cast<int32_t>(row.next());
    for (auto& value : work.velocity) value = static_cast<int16_t>(row.next());
    work.state74 = static_cast<uint8_t>(row.next());
    return work;
}
void shared_player_wrapper(const ActorContactWork& input, const l3::ActorWallContact& core,
                           const ActorContactWork& expected) {
    L3PlayerCompatWork work{};
    const auto put = [&](size_t offset, size_t width, uint32_t value) {
        for (size_t i = 0; i < width; ++i) work.bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
    };
    work.bytes[1] = input.object_class; work.bytes[0x74] = input.state74;
    for (size_t i = 0; i < 4; ++i) {
        put(0x14 + i * 2, 2, static_cast<uint16_t>(input.raw[i]));
        put(0x1c + i * 4, 4, static_cast<uint32_t>(input.fixed[i]));
        put(0x2c + i * 2, 2, static_cast<uint16_t>(input.previous_raw[i]));
        put(0x34 + i * 4, 4, static_cast<uint32_t>(input.previous_fixed[i]));
    }
    for (size_t i = 0; i < 3; ++i) put(0x44 + i * 2, 2, static_cast<uint16_t>(input.velocity[i]));
    const int32_t result = l3_pl00_apply_contact_result_8004b414(&work, core.result, &core.scratch);
    if (result != core.result) throw std::runtime_error("shared player wrapper return mismatch");
    const auto get = [&](size_t offset, size_t width) {
        uint32_t value = 0;
        for (size_t i = 0; i < width; ++i) value |= uint32_t(work.bytes[offset + i]) << (i * 8);
        return value;
    };
    ActorContactWork actual = input;
    for (size_t i = 0; i < 4; ++i) {
        actual.raw[i] = static_cast<int16_t>(get(0x14 + i * 2, 2));
        actual.fixed[i] = static_cast<int32_t>(get(0x1c + i * 4, 4));
    }
    for (size_t i = 0; i < 3; ++i) actual.velocity[i] = static_cast<int16_t>(get(0x44 + i * 2, 2));
    actual.state74 = work.bytes[0x74]; expect_work(actual, expected);
}
void source_fixtures(const std::string& prefix) {
    uint32_t wrapper_count = 0, core_count = 0, unsupported_count = 0, floor_count = 0, corrections = 0;
    rows(prefix + ".wrapper.tsv", [&](Row& row) {
        l3::ActorWallContact core; core.status = l3::RuntimeContactStatus::SourceBackedPartial;
        core.result = static_cast<int32_t>(row.next());
        core.scratch.x = static_cast<int16_t>(row.next()); core.scratch.y = static_cast<int16_t>(row.next());
        core.scratch.z = static_cast<int16_t>(row.next()); core.scratch.contact_state = static_cast<uint8_t>(row.next());
        auto work = read_work(row);
        const auto result = static_cast<int32_t>(row.next()); const auto expected = read_output(row, work);
        shared_player_wrapper(work, core, expected);
        if (result != core.result || !l3::apply_actor_contact_result_8004b414(work, core)) throw std::runtime_error("wrapper return mismatch");
        expect_work(work, expected); ++wrapper_count;
    });
    rows(prefix + ".synthetic.tsv", [&](Row& row) {
        const auto packed = static_cast<int16_t>(row.next()); const auto code = static_cast<uint16_t>(row.next());
        const auto flags = static_cast<uint16_t>(row.next()); const auto raw6 = static_cast<int16_t>(row.next());
        auto map = rectangle_map(code, flags); map.cells.assign(16, packed); map.idx[1].raw6 = raw6;
        auto work = read_work(row);
        const auto result = static_cast<int32_t>(row.next()); const auto expected = read_output(row, work);
        const auto floor_result = static_cast<int32_t>(row.next());
        const auto wall = l3::query_actor_wall_contact_8004b6ac(map, 0, work, bounds);
        if (wall.status == l3::RuntimeContactStatus::SourceBackedPartial) {
            if (wall.result != result) throw std::runtime_error("original/native geometry result mismatch: " + std::to_string(wall.result) + "/" + std::to_string(result));
            if (!l3::apply_actor_contact_result_8004b414(work, wall)) throw std::runtime_error("supported wrapper rejected");
            expect_work(work, expected); ++core_count; if (result != 0) ++corrections;
        } else ++unsupported_count;
        // Independent floor proof accepts the actual original post-wall mirrors
        // as an explicit input even when its separate wall handler is excluded.
        const auto floor = l3::query_actor_floor_contact_80048ea4(map, 0, expected, bounds);
        if (floor.status == l3::RuntimeContactStatus::SourceBackedPartial) {
            if (floor.result != floor_result) throw std::runtime_error("original/native floor result mismatch: " + std::to_string(floor.result) + "/" + std::to_string(floor_result));
            ++floor_count;
        }
    });
    if (wrapper_count != 176 || core_count < 100 || corrections < 100 || floor_count < 100 || unsupported_count == 0)
        throw std::runtime_error("insufficient original/native contact coverage");
    std::cout << "source wrapper cases=" << wrapper_count << ", supported core cases=" << core_count
              << ", corrections=" << corrections << ", excluded core cases=" << unsupported_count
              << ", supported floor cases=" << floor_count << '\n';
}
void stage_fixtures(const std::string& prefix, const std::string& pack) {
    const auto map_bytes = l3::read_file(pack + "/st00.l3map");
    const auto render_bytes = l3::read_file(pack + "/st00.l3render");
    auto map = l3::load_l3map(map_bytes);
    const auto render = l3::load_l3render(render_bytes);
    const auto initialization = l3::load_map_initialization(l3::read_file(pack + "/st00.l3init"));
    l3::apply_map_initialization(map, render, initialization,
        l3::crc32(map_bytes.data(), map_bytes.size()), l3::crc32(render_bytes.data(), render_bytes.size()));
    uint32_t count = 0;
    rows(prefix + ".stage.tsv", [&](Row& row) {
        auto work = read_work(row); const auto result = static_cast<int32_t>(row.next());
        const auto expected = read_output(row, work); const auto floor_result = static_cast<int32_t>(row.next());
        const auto wall = l3::query_actor_wall_contact_8004b6ac(map, 1, work, bounds);
        if (wall.status != l3::RuntimeContactStatus::SourceBackedPartial || wall.result != result)
            throw std::runtime_error("materialized original/native wall query mismatch status=" + std::to_string(static_cast<int>(wall.status)));
        if (!l3::apply_actor_contact_result_8004b414(work, wall)) throw std::runtime_error("stage wrapper rejected");
        expect_work(work, expected);
        const auto floor = l3::query_actor_floor_contact_80048ea4(map, 1, work, bounds);
        if (floor.status != l3::RuntimeContactStatus::SourceBackedPartial || floor.result != floor_result)
            throw std::runtime_error("materialized original/native floor query mismatch");
        ++count;
    });
    if (count != 18) throw std::runtime_error("incomplete owner stage query coverage");
    std::cout << "source stage wall/floor cases=" << count << '\n';
}
} // namespace

int main(int argc, char** argv) {
    try {
        scalar_tests();
        if (argc >= 2) source_fixtures(argv[1]);
        if (argc >= 3) stage_fixtures(argv[1], argv[2]);
        std::cout << "PASS: actor contact source subset and transactional wrapper; whole encounter parity unclaimed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
