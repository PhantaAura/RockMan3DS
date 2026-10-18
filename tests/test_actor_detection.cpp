#include "legends3ds/actor_detection.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
size_t allocations = 0;
bool measure_allocations = false;
}
void* operator new(std::size_t size) {
    if (measure_allocations) ++allocations;
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
using l3::ActorDetectionWork;
std::array<int64_t, 10> fields(const ActorDetectionWork& w) {
    return {{w.origin_34c[0], w.origin_34c[1], w.origin_34c[2], w.profile_352,
             w.phase_353, w.vector_354[0], w.vector_354[1], w.vector_354[2],
             w.countdown_35a, w.offset_35b}};
}
struct Row {
    std::vector<int64_t> values;
    size_t cursor = 0;
    int64_t next() {
        if (cursor == values.size()) throw std::runtime_error("truncated detection scalar fixture");
        return values[cursor++];
    }
    std::array<int16_t, 3> shorts() {
        return {{static_cast<int16_t>(next()), static_cast<int16_t>(next()), static_cast<int16_t>(next())}};
    }
};
void fixture_tests(const char* file) {
    std::ifstream input(file); if (!input) throw std::runtime_error("missing detector fixture");
    std::string line; size_t count = 0;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        Row row; std::istringstream tokens(line); int64_t value = 0;
        while (tokens >> value) row.values.push_back(value);
        l3::ActorDetectionInputs inputs;
        inputs.heading56 = static_cast<uint16_t>(row.next());
        const int16_t sine = static_cast<int16_t>(row.next());
        const int16_t cosine = static_cast<int16_t>(row.next());
        ActorDetectionWork work;
        work.origin_34c = row.shorts(); work.profile_352 = static_cast<uint8_t>(row.next());
        work.phase_353 = static_cast<uint8_t>(row.next()); work.vector_354 = row.shorts();
        work.countdown_35a = static_cast<uint8_t>(row.next()); work.offset_35b = static_cast<int8_t>(row.next());
        inputs.distance_work80 = static_cast<uint32_t>(row.next());
        inputs.actor_raw = row.shorts(); inputs.player_raw = row.shorts();
        inputs.profile = l3::ActorDetectionProfile{static_cast<uint8_t>(row.next()),
            static_cast<uint8_t>(row.next()), static_cast<uint8_t>(row.next())};
        l3::ActorDetectionPhase1 prepared;
        prepared.normalized_8007a694 = row.shorts();
        prepared.interpolated_80033f04 = row.shorts();
        prepared.segment0_80049c54 = static_cast<int32_t>(row.next());
        prepared.segment1_80049c54 = static_cast<int32_t>(row.next());
        inputs.phase1 = prepared; inputs.global_800bbd96 = static_cast<int16_t>(row.next());
        inputs.scratch_1f800075 = static_cast<uint8_t>(row.next());
        l3::ReferenceTables refs;
        const int32_t signed_heading = inputs.heading56 < 32768 ? inputs.heading56 : int32_t(inputs.heading56) - 65536;
        const int32_t angle = 2048 - signed_heading;
        const uint32_t index = static_cast<uint32_t>(angle < 0 ? -angle : angle) & 4095U;
        refs.trig[index] = {sine, cosine}; inputs.reference = &refs;
        std::array<int64_t, 10> expected{}; for (auto& field : expected) field = row.next();
        const bool detected = row.next() != 0, notified = row.next() != 0, sounded = row.next() != 0;
        if (row.cursor != row.values.size()) throw std::runtime_error("extra detector fixture values");
        measure_allocations = true;
        const auto result = l3::step_actor_detection_800364b0(work, inputs);
        measure_allocations = false;
        if (result.status != l3::ActorDetectionStatus::SourceBackedPartial || fields(work) != expected ||
            result.detected != detected || result.notify_global_800bbd84 != notified || result.sound156_intent != sounded) {
            std::cerr << "detection mismatch row " << count << " status " << unsigned(result.status)
                      << " result " << result.detected << " expect " << detected << '\n';
            const auto actual = fields(work);
            for (size_t i = 0; i < actual.size(); ++i)
                if (actual[i] != expected[i]) std::cerr << " field " << i << ": " << actual[i] << " expected " << expected[i] << '\n';
            throw std::runtime_error("original/native detector scalar mismatch");
        }
        ++count;
    }
    assert(count != 0 && allocations == 0);
    std::cout << "original/native detector scalar cases " << count << "; hot allocations " << allocations << '\n';
}
void globals_fixture_tests(const char* file) {
    std::ifstream input(file); if (!input) throw std::runtime_error("missing globals fixture");
    int before94 = 0, before96 = 0, after94 = 0, after96 = 0; size_t count = 0;
    while (input >> before94 >> before96 >> after94 >> after96) {
        auto globals = l3::initialize_actor_detection_globals_8005c258();
        globals.halfword_800bbd94 = static_cast<int16_t>(before94);
        globals.halfword_800bbd96 = static_cast<int16_t>(before96);
        measure_allocations = true;
        l3::tick_actor_detection_globals_8005c2b8(globals);
        measure_allocations = false;
        assert(globals.halfword_800bbd94 == after94 && globals.halfword_800bbd96 == after96);
        ++count;
    }
    assert(count != 0 && allocations == 0);
    std::cout << "original/native global countdown cases " << count << '\n';
}
void scalar_tests() {
    ActorDetectionWork w; w.origin_34c = {101, 202, 303}; w.phase_353 = 1;
    w.vector_354 = {111, 222, 333}; w.countdown_35a = 19;
    const auto before = fields(w); l3::ActorDetectionInputs inputs;
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsProfile);
    assert(fields(w) == before);
    inputs.profile = l3::ActorDetectionProfile{8, 20, 15}; inputs.player_raw = {0, 0, 0};
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsPhase1Helpers);
    assert(fields(w) == before);
    l3::ActorDetectionPhase1 p; p.interpolated_80033f04 = {0, 0, 0}; inputs.phase1 = p;
    w.origin_34c = {0, 0, 0}; const auto prior_admission = fields(w);
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsScratchMode);
    assert(fields(w) == prior_admission);
    inputs.scratch_1f800075 = 0;
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsPlayerGlobal);
    assert(fields(w) == prior_admission);
    w.phase_353 = 2; const auto phase2_before = fields(w);
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsPlayerGlobal);
    assert(fields(w) == phase2_before);
    inputs.global_800bbd96 = 0;
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::NeedsReference);
    assert(fields(w) == phase2_before);
    inputs.distance_work80 = 15U << 20;
    assert(l3::step_actor_detection_800364b0(w, inputs).status == l3::ActorDetectionStatus::SourceBackedPartial);
    assert(w.phase_353 == 0 && w.countdown_35a == 19);
    auto global = l3::initialize_actor_detection_globals_8005c258();
    assert(global.halfword_800bbd94 == 0 && global.halfword_800bbd96 == 0 && global.byte_800bbd84 == 0);
    global.halfword_800bbd94 = -1; l3::set_actor_detection_countdown_8005ebb4(global, 1800);
    l3::notify_actor_detection_8005d4c0(global);
    measure_allocations = true;
    for (size_t i = 0; i != 3600; ++i) l3::tick_actor_detection_globals_8005c2b8(global);
    measure_allocations = false;
    assert(global.halfword_800bbd94 == -1 && global.halfword_800bbd96 == 0 && global.byte_800bbd84 == 1 && allocations == 0);
}
} // namespace
int main(int argc, char** argv) {
    scalar_tests();
    if (argc >= 2) fixture_tests(argv[1]);
    if (argc >= 3) globals_fixture_tests(argv[2]);
    std::cout << "actor detector supported scalar fields passed; retail parity false\n";
}
