#include "legends3ds/dialogue_history.h"

namespace l3 {

void DialogueHistory::clear() {
    entries_ = {};
    start_ = 0;
    count_ = 0;
    next_sequence_ = 1;
}

void DialogueHistory::push(MessageRef message) {
    size_t slot = 0;
    if (count_ < kCapacity) {
        slot = (start_ + count_) % kCapacity;
        ++count_;
    } else {
        slot = start_;
        start_ = (start_ + 1) % kCapacity;
    }
    entries_[slot] = DialogueHistoryEntry{message, next_sequence_++};
}

std::vector<DialogueHistoryEntry> DialogueHistory::ordered() const {
    std::vector<DialogueHistoryEntry> out;
    out.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
        out.push_back(entries_[(start_ + i) % kCapacity]);
    }
    return out;
}

} // namespace l3
