#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace l3 {
constexpr size_t kSourceAudioQueueCapacity = 24;
constexpr size_t kSourceAudioQueueRecordBytes = 20;
struct SourceAudioQueueRecord {
    std::array<uint8_t, kSourceAudioQueueRecordBytes> raw{};
};
struct SourceAudioQueue {
    std::array<SourceAudioQueueRecord, kSourceAudioQueueCapacity> records{};
    size_t count = 0;
    bool initialized = false;
};
enum class SourceAudioQueueStatus { Queued, DroppedFull, NeedsInitialization, MalformedQueue };

// Original80019E14..24 initializes only the write pointer. Preserve retained
// records/reserved bytes; no retail sound data or invented playback semantics.
void initialize_source_audio_queue_80019e14(SourceAudioQueue& queue) noexcept;
// Exact8001A008 producer: type0, low16 request, rawa1/ rawa2 words.
SourceAudioQueueStatus enqueue_source_audio_8001a008(
    SourceAudioQueue& queue, uint32_t request_bits, uint32_t raw_a1, uint32_t raw_a2) noexcept;
// Exact8001A050 producer: type1, low16 request, two original words at a1+0/+4.
// The native API intentionally does not rename these source words as volume,
// screen position, world coordinates or any unproved sound parameters.
SourceAudioQueueStatus enqueue_source_audio_8001a050(
    SourceAudioQueue& queue, uint32_t request_bits,
    const std::array<uint32_t, 2>& pointed_words) noexcept;
// Original8001A1D0..1DC resets the producer pointer after consumer dispatch.
// Caller must identify whether dispatch was real audio playback or diagnostic
// intent draining; using this function alone does not prove audible sound.
void finish_source_audio_queue_batch_8001a1d0(SourceAudioQueue& queue) noexcept;
} // namespace l3
