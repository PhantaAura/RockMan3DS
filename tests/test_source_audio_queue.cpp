#include "legends3ds/source_audio_queue.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>

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
void populate(l3::SourceAudioQueue& queue) {
    for (size_t slot = 0; slot != queue.records.size(); ++slot)
        for (size_t i = 0; i != queue.records[slot].raw.size(); ++i)
            queue.records[slot].raw[i] = static_cast<uint8_t>((slot * 20 + i) * 17 + 3);
}
bool same_records(const l3::SourceAudioQueue& a, const l3::SourceAudioQueue& b) {
    for (size_t i = 0; i != a.records.size(); ++i) if (a.records[i].raw != b.records[i].raw) return false;
    return true;
}
void scalar_tests() {
    l3::SourceAudioQueue queue; populate(queue); const auto original = queue;
    assert(l3::enqueue_source_audio_8001a008(queue, 156, 0, 0) == l3::SourceAudioQueueStatus::NeedsInitialization);
    assert(same_records(queue, original) && queue.count == 0 && !queue.initialized);
    l3::initialize_source_audio_queue_80019e14(queue);
    assert(same_records(queue, original) && queue.count == 0 && queue.initialized);
    const auto before = queue; auto staged = queue;
    assert(l3::enqueue_source_audio_8001a008(staged, 156, 0, 0) == l3::SourceAudioQueueStatus::Queued);
    assert(same_records(queue, before) && queue.count == 0); // Unpublished transaction holds.
    measuring = true;
    for (size_t i = 0; i != 24; ++i)
        assert(l3::enqueue_source_audio_8001a050(queue, 180, {{0x12345678, 0x89abcdef}}) == l3::SourceAudioQueueStatus::Queued);
    const auto full = queue;
    assert(l3::enqueue_source_audio_8001a008(queue, 156, 0, 0) == l3::SourceAudioQueueStatus::DroppedFull);
    assert(same_records(queue, full) && queue.count == 24);
    l3::finish_source_audio_queue_batch_8001a1d0(queue);
    assert(same_records(queue, full) && queue.count == 0 && queue.initialized);
    for (size_t pass = 0; pass != 480; ++pass) {
        for (size_t i = 0; i != 24; ++i)
            assert(l3::enqueue_source_audio_8001a008(queue, 156, 0, 0) == l3::SourceAudioQueueStatus::Queued);
        l3::finish_source_audio_queue_batch_8001a1d0(queue);
    }
    measuring = false;
    assert(allocations == 0);
    queue.count = 25; const auto malformed = queue;
    assert(l3::enqueue_source_audio_8001a008(queue, 156, 0, 0) == l3::SourceAudioQueueStatus::MalformedQueue);
    l3::finish_source_audio_queue_batch_8001a1d0(queue);
    assert(queue.count == 25 && same_records(queue, malformed));
}
void fixture_tests(const char* path) {
    std::ifstream file(path); if (!file) throw std::runtime_error("source audio scalar fixture missing");
    uint64_t kind = 0, count = 0, request = 0, raw0 = 0, raw1 = 0, expected_count = 0;
    size_t cases = 0;
    while (file >> kind >> count >> request >> raw0 >> raw1 >> expected_count) {
        assert(kind <= 1 && count <= 24 && expected_count <= 24);
        l3::SourceAudioQueue queue; populate(queue); l3::initialize_source_audio_queue_80019e14(queue); queue.count = count;
        const auto before = queue;
        std::array<uint8_t, 20> expected{};
        for (auto& byte : expected) { unsigned value = 0; if (!(file >> value)) throw std::runtime_error("truncated source audio fixture"); assert(value <= 255); byte = static_cast<uint8_t>(value); }
        measuring = true;
        const auto result = kind == 0 ? l3::enqueue_source_audio_8001a008(queue, static_cast<uint32_t>(request), static_cast<uint32_t>(raw0), static_cast<uint32_t>(raw1))
            : l3::enqueue_source_audio_8001a050(queue, static_cast<uint32_t>(request), {{static_cast<uint32_t>(raw0), static_cast<uint32_t>(raw1)}});
        measuring = false;
        assert(result == (count == 24 ? l3::SourceAudioQueueStatus::DroppedFull : l3::SourceAudioQueueStatus::Queued));
        assert(queue.count == expected_count && queue.records[count == 24 ? 23 : count].raw == expected);
        for (size_t i = 0; i != queue.records.size(); ++i)
            if (i != count) assert(queue.records[i].raw == before.records[i].raw);
        ++cases;
    }
    assert(cases != 0 && allocations == 0);
    std::cout << "original/native sound producer cases " << cases << "; hot allocations " << allocations << '\n';
}
} // namespace
int main(int argc, char** argv) {
    scalar_tests(); if (argc > 1) fixture_tests(argv[1]);
    std::cout << "source audio request queue fields passed; playback unproved\n";
}
