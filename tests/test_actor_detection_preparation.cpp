#include "legends3ds/actor_detection_preparation.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { size_t allocations = 0; bool measuring = false; }
void* operator new(std::size_t size) {
    if (measuring) ++allocations;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
namespace {
using PrepStatus = l3::ActorDetectionPreparationStatus;
std::array<int64_t, 10> fields(const l3::ActorDetectionWork& w) {
    return {{w.origin_34c[0], w.origin_34c[1], w.origin_34c[2], w.profile_352, w.phase_353,
             w.vector_354[0], w.vector_354[1], w.vector_354[2], w.countdown_35a, w.offset_35b}};
}
l3::RuntimeMap rectangle_map(int16_t packed, uint16_t code, uint16_t flags, int16_t raw6) {
    l3::RuntimeMap map; l3::RuntimeMapArea area;
    area.start_x = 62; area.start_z = 62; area.width = 4; area.height = 4; area.cell_count = 16;
    map.areas.push_back(area); map.cells.assign(16, packed); map.idx.resize(2);
    map.idx[1].descriptor_index = 0; map.idx[1].raw6 = raw6;
    l3::RuntimeMapDescriptor descriptor; descriptor.default_contact_index = 0; descriptor.alt_contact_index = 0;
    map.descriptors.push_back(descriptor); map.contact_lists.push_back({0, 1});
    map.contact_groups.push_back({-100, 100, -100, 100, 0, 1});
    map.contact_primitives.push_back({-128, 128, code, flags}); return map;
}
struct Row {
    std::vector<int64_t> values; size_t at = 0;
    int64_t next() { if (at == values.size()) throw std::runtime_error("truncated preparation fixture"); return values[at++]; }
    std::array<int16_t, 3> shorts() { return {{static_cast<int16_t>(next()), static_cast<int16_t>(next()), static_cast<int16_t>(next())}}; }
    l3::ActorDetectionWork work() {
        l3::ActorDetectionWork w; w.origin_34c = shorts(); w.profile_352 = static_cast<uint8_t>(next());
        w.phase_353 = static_cast<uint8_t>(next()); w.vector_354 = shorts();
        w.countdown_35a = static_cast<uint8_t>(next()); w.offset_35b = static_cast<int8_t>(next()); return w;
    }
};
void check_result(Row& row, const l3::ActorDetectionPreparation& actual, size_t ordinal) {
    std::array<int64_t, 10> expected{}; for (auto& value : expected) value = row.next();
    const bool detected = row.next() != 0, notified = row.next() != 0, sounded = row.next() != 0;
    const auto query_count = static_cast<size_t>(row.next());
    const auto point0 = row.shorts(); const int32_t segment0 = static_cast<int32_t>(row.next());
    const auto point1 = row.shorts(); const int32_t segment1 = static_cast<int32_t>(row.next());
    if (row.at != row.values.size()) throw std::runtime_error("extra preparation fixture fields");
    if (actual.status != PrepStatus::Ready) {
        assert(fields(actual.before) == fields(actual.next));
        assert(!actual.detector.detected && !actual.detector.sound156_intent && !actual.detector.notify_global_800bbd84);
        assert(actual.status == PrepStatus::SegmentUnsupported || actual.status == PrepStatus::SourceMathRejected);
        return;
    }
    bool points_match = true;
    for (size_t i = 0; i != 3; ++i) {
        if (query_count > 0) points_match &= actual.updated_point[i] == point0[i];
        if (query_count > 1) points_match &= actual.midpoint[i] == point1[i];
    }
    if (fields(actual.next) != expected || actual.detector.detected != detected ||
        actual.detector.notify_global_800bbd84 != notified || actual.detector.sound156_intent != sounded ||
        size_t(actual.segment0_queried) + size_t(actual.segment1_queried) != query_count || !points_match ||
        (query_count > 0 && actual.segment0.result != segment0) || (query_count > 1 && actual.segment1.result != segment1)) {
        std::cerr << "preparation divergence row " << ordinal << " status " << unsigned(actual.status)
                  << " queries " << actual.segment0_queried + actual.segment1_queried << '/' << query_count
                  << " results " << actual.segment0.result << '/' << segment0 << ' ' << actual.segment1.result << '/' << segment1 << '\n';
        const auto got = fields(actual.next);
        for (size_t i = 0; i != expected.size(); ++i) if (got[i] != expected[i]) std::cerr << " field " << i << ": " << got[i] << " expected " << expected[i] << '\n';
        throw std::runtime_error("original/native detector composition mismatch");
    }
}
void scalar_tests() {
    l3::ActorDetectionWork work; work.origin_34c = {{101, 202, 303}}; work.phase_353 = 1;
    work.profile_352 = 4; work.vector_354 = {{12, 34, 56}}; work.countdown_35a = 18;
    l3::ActorDetectionPreparationInputs input; input.source.player_raw = {{100, 200, 300}};
    const auto before = fields(work);
    auto p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::DetectorHeld && p.detector.status == l3::ActorDetectionStatus::NeedsProfile);
    assert(fields(work) == before && fields(p.next) == before);
    input.source.profile = l3::ActorDetectionProfile{10, 20, 15};
    p = l3::prepare_actor_detection_800364b0(work, input); assert(p.status == PrepStatus::NeedsMap);
    auto map = rectangle_map(-32768, 1, 0, 0); input.map = &map;
    p = l3::prepare_actor_detection_800364b0(work, input); assert(p.status == PrepStatus::NeedsBounds);
    input.segment_bounds = l3::RetailPlayerBounds{25, -25, -35, 35, -70, 10};
    p = l3::prepare_actor_detection_800364b0(work, input); assert(p.status == PrepStatus::NeedsBounds);
    assert(fields(work) == before && fields(p.next) == before);
    input.segment_bounds = l3::RetailPlayerBounds{-25, 25, -35, 35, -70, 10};
    p = l3::prepare_actor_detection_800364b0(work, input); assert(p.status == PrepStatus::NeedsNormalizationLookup);
    assert(fields(work) == before && fields(p.next) == before);
    input.source.distance_work80 = 15U << 20; input.map = nullptr; input.segment_bounds.reset();
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::Ready && p.next.phase_353 == 0 && !p.phase1_math_prepared);
    assert(l3::actor_detection_preparation_matches(p, work));
    auto stale = work; stale.vector_354[2] ^= 1; assert(!l3::actor_detection_preparation_matches(p, stale));
    input.source.distance_work80 = 0; input.source.player_raw = {{-26751, 202, 303}}; work.origin_34c[0] = 0;
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::Ready && p.next.phase_353 == 0 && !p.phase1_math_prepared);
    // A caller-supplied invented clear helper cannot bypass exact preparation.
    input.source.player_raw = {{100, 202, 303}}; input.source.phase1 = l3::ActorDetectionPhase1{};
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::NeedsMap && !p.source.phase1);
    input.map = &map; input.segment_bounds = l3::RetailPlayerBounds{-25, 25, -35, 35, -70, 10};
    l3::SourceNormalizationLookup lookup{}; lookup.fill(4096); input.normalization_lookup = &lookup;
    input.source.global_800bbd96 = 0; input.source.scratch_1f800075 = 2;
    // Real valid clear packed cell setup; both query points remain inside area.
    work.origin_34c = {{0, -96, 0}}; work.vector_354 = {{0, 0, 0}}; input.source.player_raw = {{0, -96, 0}};
    input.source.scratch_1f800075.reset();
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::DetectorHeld && p.detector.status == l3::ActorDetectionStatus::NeedsScratchMode);
    assert(fields(work) == fields(p.next) && p.segment0_queried && p.segment1_queried);
    input.source.scratch_1f800075 = 2; input.source.global_800bbd96.reset();
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::DetectorHeld && p.detector.status == l3::ActorDetectionStatus::NeedsPlayerGlobal);
    assert(fields(work) == fields(p.next));
    // Authored extreme reference context reaches the original normalizer's
    // excluded signed-ADD overflow domain on the interpolated candidate.
    lookup.fill(32767); work.vector_354 = {{32767, 32767, 32767}};
    input.source.player_raw = {{1, -95, 1}}; input.source.global_800bbd96 = 0;
    p = l3::prepare_actor_detection_800364b0(work, input);
    assert(p.status == PrepStatus::SourceMathRejected);
    assert(fields(work) == fields(p.next));
}
void authored_cases(const char* path) {
    std::ifstream file(path); if (!file) throw std::runtime_error("authored preparation fixture missing");
    std::string line; std::getline(file, line); size_t count = 0, matched = 0, held = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        Row row; std::istringstream parser(line); int64_t number;
        while (parser >> number) row.values.push_back(number);
        const int16_t packed = static_cast<int16_t>(row.next()); const uint16_t code = static_cast<uint16_t>(row.next());
        const uint16_t flags = static_cast<uint16_t>(row.next()); const int16_t raw6 = static_cast<int16_t>(row.next());
        auto map = rectangle_map(packed, code, flags, raw6); l3::ActorDetectionPreparationInputs input;
        input.segment_bounds = l3::RetailPlayerBounds{static_cast<int16_t>(row.next()), static_cast<int16_t>(row.next()),
            static_cast<int16_t>(row.next()), static_cast<int16_t>(row.next()), static_cast<int16_t>(row.next()), static_cast<int16_t>(row.next())};
        input.map = &map; input.source.heading56 = static_cast<uint16_t>(row.next());
        const int16_t sine = static_cast<int16_t>(row.next()), cosine = static_cast<int16_t>(row.next());
        l3::ReferenceTables refs; const int32_t heading = input.source.heading56 < 32768 ? input.source.heading56 : int32_t(input.source.heading56) - 65536;
        const int32_t angle = 2048 - heading; refs.trig[static_cast<uint32_t>(angle < 0 ? -angle : angle) & 4095U] = {sine, cosine};
        input.source.reference = &refs; l3::SourceNormalizationLookup lookup{}; lookup.fill(static_cast<int16_t>(row.next())); input.normalization_lookup = &lookup;
        const auto work = row.work(); const auto before = fields(work);
        input.source.distance_work80 = static_cast<uint32_t>(row.next()); input.source.actor_raw = row.shorts(); input.source.player_raw = row.shorts();
        input.source.profile = l3::ActorDetectionProfile{static_cast<uint8_t>(row.next()), static_cast<uint8_t>(row.next()), static_cast<uint8_t>(row.next())};
        input.source.global_800bbd96 = static_cast<int16_t>(row.next()); input.source.scratch_1f800075 = static_cast<uint8_t>(row.next());
        measuring = true; const auto prepared = l3::prepare_actor_detection_800364b0(work, input); measuring = false;
        assert(fields(work) == before && l3::actor_detection_preparation_matches(prepared, work));
        check_result(row, prepared, count);
        if (prepared.status == PrepStatus::Ready) ++matched; else ++held;
        ++count;
    }
    assert(count != 0 && matched != 0 && held != 0 && allocations == 0);
    std::cout << "full original/native authored detector cases=" << count << " matched=" << matched << " held=" << held << " hot_allocations=" << allocations << '\n';
}
void owner_cases(const char* path, const char* pack_path, const char* exe_path) {
    const std::string pack(pack_path); const auto map_bytes = l3::read_file(pack + "/st00.l3map");
    const auto render_bytes = l3::read_file(pack + "/st00.l3render"); auto map = l3::load_l3map(map_bytes);
    const auto render = l3::load_l3render(render_bytes); const auto init = l3::load_map_initialization(l3::read_file(pack + "/st00.l3init"));
    l3::apply_map_initialization(map, render, init, l3::crc32(map_bytes.data(), map_bytes.size()), l3::crc32(render_bytes.data(), render_bytes.size()));
    const auto exe = l3::read_file(exe_path); if (exe.size() < 2048) throw std::runtime_error("owner EXE header missing");
    const uint32_t base = l3::rd_u32(exe.data() + 24);
    const auto halfword = [&](uint32_t address) {
        const uint64_t offset = uint64_t(address) - base + 2048; if (address < base || offset + 2 > exe.size()) throw std::runtime_error("owner scalar read escaped EXE");
        const uint16_t bits = l3::rd_u16(exe.data() + offset); return bits < 32768 ? static_cast<int16_t>(bits) : static_cast<int16_t>(int32_t(bits) - 65536);
    };
    l3::ReferenceTables refs; for (size_t i = 0; i != refs.trig.size(); ++i) refs.trig[i] = {halfword(0x80092ec4 + uint32_t(i) * 4), halfword(0x80092ec6 + uint32_t(i) * 4)};
    l3::SourceNormalizationLookup lookup; for (size_t i = 0; i != lookup.size(); ++i) lookup[i] = halfword(0x80092d38 + uint32_t(i) * 2);
    const size_t profile_offset = 0x800899ac + 4 * 4 - base + 2048;
    const l3::ActorDetectionProfile profile{exe[profile_offset], exe[profile_offset + 1], exe[profile_offset + 2]};
    const l3::RetailPlayerBounds bounds{halfword(0x80097d0c), halfword(0x80097d0e), halfword(0x80097d10), halfword(0x80097d12), halfword(0x80097d14), halfword(0x80097d16)};
    std::ifstream file(path); if (!file) throw std::runtime_error("owner preparation fixture missing");
    std::string line; std::getline(file, line); size_t count = 0, matched = 0, held = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        Row row; std::istringstream parser(line); int64_t number; while (parser >> number) row.values.push_back(number);
        const auto work = row.work(); l3::ActorDetectionPreparationInputs input; input.map = &map; input.area_index = 1;
        input.source.profile = profile; input.source.reference = &refs; input.normalization_lookup = &lookup; input.segment_bounds = bounds;
        input.source.distance_work80 = static_cast<uint32_t>(row.next()); input.source.actor_raw = row.shorts(); input.source.player_raw = row.shorts();
        input.source.global_800bbd96 = static_cast<int16_t>(row.next()); input.source.scratch_1f800075 = static_cast<uint8_t>(row.next());
        measuring = true; const auto prepared = l3::prepare_actor_detection_800364b0(work, input); measuring = false;
        check_result(row, prepared, count); if (prepared.status == PrepStatus::Ready) ++matched; else ++held; ++count;
    }
    assert(count != 0 && matched != 0 && allocations == 0);
    std::cout << "full materialized original/native detector cases=" << count << " matched=" << matched << " held=" << held << " hot_allocations=" << allocations << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try { scalar_tests(); if (argc > 1) authored_cases(argv[1]); if (argc > 4) owner_cases(argv[2], argv[3], argv[4]);
        std::cout << "detector exact-helper composition passed; retail encounter parity unproved\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
