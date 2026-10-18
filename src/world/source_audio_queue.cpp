#include "legends3ds/source_audio_queue.h"

namespace l3 {
namespace {
void put_word(std::array<uint8_t, kSourceAudioQueueRecordBytes>& bytes,
              size_t offset, uint32_t value, unsigned width) noexcept {
    for (unsigned i = 0; i != width; ++i) bytes[offset + i] = static_cast<uint8_t>(value >> (8U * i));
}
SourceAudioQueueStatus enqueue(SourceAudioQueue& queue, uint8_t type,
                              uint32_t request, uint32_t word4, uint32_t word8) noexcept {
    if (!queue.initialized) return SourceAudioQueueStatus::NeedsInitialization;
    if (queue.count > kSourceAudioQueueCapacity) return SourceAudioQueueStatus::MalformedQueue;
    if (queue.count == kSourceAudioQueueCapacity) return SourceAudioQueueStatus::DroppedFull;
    auto& bytes = queue.records[queue.count].raw;
    bytes[0] = type;
    put_word(bytes, 2, request, 2);
    put_word(bytes, 4, word4, 4);
    put_word(bytes, 8, word8, 4);
    ++queue.count;
    return SourceAudioQueueStatus::Queued;
}
} // namespace
void initialize_source_audio_queue_80019e14(SourceAudioQueue& queue) noexcept {
    queue.count = 0; queue.initialized = true;
}
SourceAudioQueueStatus enqueue_source_audio_8001a008(
    SourceAudioQueue& queue, uint32_t request_bits, uint32_t raw_a1, uint32_t raw_a2) noexcept {
    return enqueue(queue, 0, request_bits, raw_a1, raw_a2);
}
SourceAudioQueueStatus enqueue_source_audio_8001a050(
    SourceAudioQueue& queue, uint32_t request_bits,
    const std::array<uint32_t, 2>& pointed_words) noexcept {
    return enqueue(queue, 1, request_bits, pointed_words[0], pointed_words[1]);
}
void finish_source_audio_queue_batch_8001a1d0(SourceAudioQueue& queue) noexcept {
    if (queue.initialized && queue.count <= kSourceAudioQueueCapacity) queue.count = 0;
}
} // namespace l3
