#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

/*
 * References game-derived message blocks instead of copying dialogue into the
 * source tree. The platform message renderer resolves these through the locally
 * cooked L3MS banks at runtime.
 */
struct MessageRef {
    uint16_t bank = 0;
    uint16_t block = 0;

    bool operator==(const MessageRef& other) const {
        return bank == other.bank && block == other.block;
    }
};

struct DialogueHistoryEntry {
    MessageRef message{};
    uint32_t sequence = 0;
};

class DialogueHistory {
public:
    static constexpr size_t kCapacity = 16;

    void clear();
    void push(MessageRef message);
    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }

    /* Oldest -> newest, suitable for a scrollable player-facing log. */
    std::vector<DialogueHistoryEntry> ordered() const;

private:
    std::array<DialogueHistoryEntry, kCapacity> entries_{};
    size_t start_ = 0;
    size_t count_ = 0;
    uint32_t next_sequence_ = 1;
};

} // namespace l3
