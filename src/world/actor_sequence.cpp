#include "legends3ds/actor_sequence.h"

namespace l3 {
ActorSequenceCursor actor_sequence_begin(uint16_t index, const ActorSequenceRecord& record) {
    return {index, record[0], record[1], record[2], record[3]};
}

ActorSequenceStatus actor_sequence_reset(ActorSequenceCursor& cursor,
                                         const ActorSequenceRecord* records,
                                         size_t count, uint16_t index) noexcept {
    if (!records || !count || count > kActorSequenceRecordLimit || index >= count)
        return ActorSequenceStatus::MissingRecord;
    cursor = actor_sequence_begin(index, records[index]);
    return ActorSequenceStatus::SourceBackedPartial;
}

uint16_t actor_sequence_next_index(uint16_t index, uint8_t control) {
    if (control < 128u) return static_cast<uint16_t>(index + 1u);
    if (control == 255u) return index;
    return control & 127u;
}

ActorSequenceStep actor_sequence_step(ActorSequenceCursor& cursor,
                                      const ActorSequenceRecord* records,
                                      size_t count) {
    if (!records || !count || count > kActorSequenceRecordLimit || cursor.record_index >= count)
        return {};
    auto next = cursor;
    next.countdown_a5 = static_cast<uint8_t>(next.countdown_a5 - 1u);
    bool changed = false;
    if (next.countdown_a5 == 0) {
        const auto& current = records[next.record_index];
        if (current[3] == 255u) {
            // Source terminal branch reloads only+A5; it does not reload the
            // complete sequencing word or change+A4/+A6/+A7.
            next.countdown_a5 = current[1];
        } else {
            const auto index = actor_sequence_next_index(next.record_index, current[3]);
            if (index >= count) return {};
            changed = index != next.record_index;
            next = actor_sequence_begin(index, records[index]);
        }
    }
    cursor = next;
    return {ActorSequenceStatus::SourceBackedPartial, changed};
}
} // namespace l3
