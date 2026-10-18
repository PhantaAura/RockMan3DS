#include "legends3ds/actor_detection_segment.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocation_count = 0;
void* operator new(std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
namespace {
using Status = l3::RuntimeContactStatus;
constexpr l3::RetailPlayerBounds authored_bounds{-25, 25, -35, 35, -70, 10};
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
l3::RuntimeMap rectangle_map(uint16_t code = 1, uint16_t flags = 0, int16_t packed = 1, int16_t raw6 = 0) {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area;
    area.start_x = 62; area.start_z = 62; area.width = 4; area.height = 4; area.cell_count = 16;
    map.areas.push_back(area); map.cells.assign(16, packed); map.idx.resize(2);
    map.idx[1].descriptor_index = 0; map.idx[1].raw6 = raw6;
    l3::RuntimeMapDescriptor descriptor;
    descriptor.default_contact_index = 0; descriptor.alt_contact_index = 0;
    map.descriptors.push_back(descriptor); map.contact_lists.push_back({0, 1});
    map.contact_groups.push_back({-100, 100, -100, 100, 0, 1});
    map.contact_primitives.push_back({-128, 128, code, flags});
    return map;
}
void scalar_guards() {
    auto map = rectangle_map();
    std::array<int16_t, 4> start{{0, -60, 0, 1234}}, end{{300, -60, 300, -1234}};
    auto r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds);
    require(r.status == Status::SourceBackedPartial && r.result == 1 && r.candidate_cells == 1,
            "original rectangle occupancy/candidate dedup mismatch");
    // Rectangles test start occupancy, not a generic ray through the box.
    start[0] = -126; end[0] = 126;
    r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds);
    require(r.status == Status::SourceBackedPartial && r.result == 0,
            "generic segment intersection replaced original step occupancy");
    start = {{0, -60, 0, 1234}}; end = start;
    map.contact_primitives[0].flags = 0x8005;
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).result == 0,
            "classFFFF original flag8000 filter mismatch");
    r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds, 0);
    require(r.status == Status::SourceBackedPartial && r.result == 0x50001,
            "original flagslow4 result encoding mismatch");
    map = rectangle_map(8);
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::Unsupported,
            "unknown slope query guessed clear/blocked");
    map = rectangle_map(0x71);
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::Unsupported,
            "unknown family7 interior guessed segment geometry");
    map = rectangle_map(0x301);
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).result == 0,
            "original firstcode>=256 skip mismatch");
    map = rectangle_map(); map.cells.clear();
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::Unsupported,
            "malformed area cell range accepted");
    map = rectangle_map(); map.contact_lists[0].group_count = UINT32_MAX;
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::Unsupported,
            "malformed group list accepted");
    map = rectangle_map(); map.contact_groups[0].primitive_count = UINT32_MAX;
    require(l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::Unsupported,
            "malformed primitive range accepted");
    map = rectangle_map(1, 0, 0);
    r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds);
    require(r.status == Status::SourceBackedPartial && r.result == 0x40000000,
            "original startzero return mask was not retained");
    start = {{16385, 0, 0, 0}}; end = {{-1, 0, 0, 0}};
    r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds);
    require(r.status == Status::SourceBackedPartial && r.result == (-2147483647 - 1),
            "original sign-cross overflow return mismatch");
    map = rectangle_map(); start = {{-1023, -60, 0, 0}}; end = start;
    r = l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds);
    require(r.status == Status::SourceBackedPartial && r.candidate_cells == 1,
            "invalid neighbor did not stop original candidate gathering");
    map = rectangle_map(); start = {{0, -60, 0, 0}}; end = start;
    const auto allocations = allocation_count;
    bool ready = true;
    for (int i = 0; i < 640; ++i)
        ready &= l3::query_actor_detection_segment_80049c54(map, 0, start, end, authored_bounds).status == Status::SourceBackedPartial;
    require(ready && allocation_count == allocations, "detector segment query allocated");
}
void bounds_format() {
    auto bytes = l3::encode_actor_detection_segment_bounds(authored_bounds);
    require(bytes.size() == 44, "L3DB size mismatch");
    const auto bounds = l3::load_actor_detection_segment_bounds(bytes);
    require(bounds.x0 == authored_bounds.x0 && bounds.x1 == authored_bounds.x1 &&
            bounds.z0 == authored_bounds.z0 && bounds.z1 == authored_bounds.z1 &&
            bounds.y0 == authored_bounds.y0 && bounds.y1 == authored_bounds.y1,
            "L3DB boundsfield order mismatch");
    const auto rejected = [](const std::vector<uint8_t>& input) {
        try { (void)l3::load_actor_detection_segment_bounds(input); return false; }
        catch (const std::runtime_error&) { return true; }
    };
    for (size_t at : {size_t(0), size_t(4), size_t(6), size_t(8), size_t(12), size_t(16),
                      size_t(20), size_t(24), size_t(28), size_t(32)}) {
        auto bad = bytes; bad[at] ^= 0x80u;
        require(rejected(bad), "malformed L3DB header/payload accepted");
    }
    for (size_t length = 0; length < 44; ++length) {
        auto truncated = bytes; truncated.resize(length);
        require(rejected(truncated), "truncated L3DB accepted");
    }
    auto extra = bytes; extra.push_back(0);
    require(rejected(extra), "extra L3DB bytes accepted");
    auto unordered = bytes; unordered[32] = unordered[34]; unordered[33] = unordered[35];
    l3::patch_u32(unordered, 24, l3::crc32(unordered.data() + 32, 12));
    require(rejected(unordered), "CRC-valid unordered L3DB accepted");
}
int64_t number(std::istream& row) {
    int64_t out; require(static_cast<bool>(row >> out), "truncated segment source fixture"); return out;
}
void source_cases(const char* path) {
    std::ifstream file(path); require(static_cast<bool>(file), "segment source fixture unavailable");
    std::string line; std::getline(file, line);
    size_t admitted = 0, held = 0, blocked = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        const auto packed = static_cast<int16_t>(number(row));
        const auto code = static_cast<uint16_t>(number(row));
        const auto flags = static_cast<uint16_t>(number(row));
        const auto raw6 = static_cast<int16_t>(number(row));
        const auto source_class = static_cast<uint32_t>(number(row));
        std::array<int16_t, 4> start{}, end{};
        for (auto& v : start) v = static_cast<int16_t>(number(row));
        for (auto& v : end) v = static_cast<int16_t>(number(row));
        l3::RetailPlayerBounds bounds;
        bounds.x0 = static_cast<int16_t>(number(row)); bounds.x1 = static_cast<int16_t>(number(row));
        bounds.z0 = static_cast<int16_t>(number(row)); bounds.z1 = static_cast<int16_t>(number(row));
        bounds.y0 = static_cast<int16_t>(number(row)); bounds.y1 = static_cast<int16_t>(number(row));
        const auto want = static_cast<int32_t>(number(row));
        const auto candidates = static_cast<uint32_t>(number(row));
        std::string extra; require(!(row >> extra), "extra segment source field");
        const auto map = rectangle_map(code, flags, packed, raw6);
        const auto actual = l3::query_actor_detection_segment_80049c54(map, 0, start, end, bounds, source_class);
        if (actual.status == Status::SourceBackedPartial) {
            if (actual.result != want || actual.candidate_cells != candidates) {
                std::cerr << "row " << admitted + held << " code " << code << " packed " << packed
                          << " result " << actual.result << '/' << want << " cells " << actual.candidate_cells
                          << '/' << candidates << '\n';
                throw std::runtime_error("original/native segment mismatch");
            }
            ++admitted; blocked += actual.result != 0;
        } else {
            require(actual.status == Status::Unsupported, "unexpected source query status"); ++held;
        }
    }
    require(admitted > 1000 && held > 0 && blocked > 100, "insufficient supported/held original segment cases");
    std::cout << "original segment authored cases=" << admitted + held << " supported=" << admitted
              << " held=" << held << " nonzero_results=" << blocked << '\n';
}
void stage_cases(const char* path, const char* owner_pack) {
    const std::string pack(owner_pack);
    const auto map_bytes = l3::read_file(pack + "/st00.l3map");
    const auto render_bytes = l3::read_file(pack + "/st00.l3render");
    auto map = l3::load_l3map(map_bytes);
    const auto render = l3::load_l3render(render_bytes);
    const auto init = l3::load_map_initialization(l3::read_file(pack + "/st00.l3init"));
    l3::apply_map_initialization(map, render, init, l3::crc32(map_bytes.data(), map_bytes.size()),
                               l3::crc32(render_bytes.data(), render_bytes.size()));
    std::ifstream file(path); require(static_cast<bool>(file), "owner stage segment fixture unavailable");
    std::string line; std::getline(file, line);
    size_t admitted = 0, held = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        const auto source_class = static_cast<uint32_t>(number(row));
        std::array<int16_t, 4> start{}, end{};
        for (auto& v : start) v = static_cast<int16_t>(number(row));
        for (auto& v : end) v = static_cast<int16_t>(number(row));
        l3::RetailPlayerBounds bounds;
        bounds.x0 = static_cast<int16_t>(number(row)); bounds.x1 = static_cast<int16_t>(number(row));
        bounds.z0 = static_cast<int16_t>(number(row)); bounds.z1 = static_cast<int16_t>(number(row));
        bounds.y0 = static_cast<int16_t>(number(row)); bounds.y1 = static_cast<int16_t>(number(row));
        const auto want = static_cast<int32_t>(number(row));
        const auto cells = static_cast<uint32_t>(number(row));
        const auto actual = l3::query_actor_detection_segment_80049c54(map, 1, start, end, bounds, source_class);
        if (actual.status == Status::SourceBackedPartial) {
            if (actual.result != want || actual.candidate_cells != cells)
                throw std::runtime_error("materialized original/native segment mismatch");
            ++admitted;
        } else ++held;
    }
    require(admitted > 0 && admitted + held == 21, "missing materialized owner segment coverage");
    std::cout << "materialized original segment cases=" << admitted + held
              << " supported=" << admitted << " held=" << held << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try {
        scalar_guards(); bounds_format();
        if (argc > 1) source_cases(argv[1]);
        if (argc > 3) stage_cases(argv[2], argv[3]);
        std::cout << "PASS: source detectorsegment guards/L3DB and640 allocation-free queries; no whole encounter parity\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
