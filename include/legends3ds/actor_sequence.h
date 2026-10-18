#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace l3 {
using ActorSequenceRecord = std::array<uint8_t, 8>;
constexpr size_t kActorSequenceRecordLimit = 256;

struct ActorSequenceCursor {
    uint16_t record_index = 0;
    uint8_t frame_a4 = 0;
    uint8_t countdown_a5 = 0;
    uint8_t opaque_a6 = 0;
    uint8_t opaque_a7 = 0;
};
enum class ActorSequenceStatus : uint8_t { SourceBackedPartial, MissingRecord };
struct ActorSequenceStep {
    ActorSequenceStatus status = ActorSequenceStatus::MissingRecord;
    bool record_changed = false;
};

ActorSequenceCursor actor_sequence_begin(uint16_t record_index,
                                         const ActorSequenceRecord& record);
// Checked scalar reset for an already prepared request. The class03 callback's
// changed-request branch selects record0; selecting which request is valid is
// the caller's job. Invalid bounds preserve the existing cursor.
ActorSequenceStatus actor_sequence_reset(ActorSequenceCursor& cursor,
                                         const ActorSequenceRecord* records,
                                         size_t record_count,
                                         uint16_t record_index = 0) noexcept;
// Source80032488 takes the signed control from current record byte3, not from
// work+A7. Positive controls advance one record, -1 repeats its countdown,
// other negative controls jump to an absolute seven-bit record index.
uint16_t actor_sequence_next_index(uint16_t current_index, uint8_t raw_control);

// Allocation-free scalar sequencer only. Records are an already validated,
// owner-data-derived load-time cache. Missing/over-budget records reject the
// step and preserve cursor. Matrix/pose consumption and callback cadence are
// separate gates; this function does not declare any actor request semantics.
ActorSequenceStep actor_sequence_step(ActorSequenceCursor& cursor,
                                      const ActorSequenceRecord* records,
                                      size_t record_count);
} // namespace l3
