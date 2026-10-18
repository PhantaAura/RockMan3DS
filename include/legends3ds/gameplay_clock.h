#pragma once
#include <cstdint>
namespace l3 {
// The measured interpreter trace updates player work once per two VBlanks.
// Nominal 30Hz scheduling is independent of renderer speed. Exact hardware
// wall-clock rate remains HARDWARE_PENDING. A stall retains debt; it never
// enlarges a physics step. UI pause discards wall time without updating gameplay.
class GameplayClock {
public:
    void advance(uint64_t elapsed_us, bool paused) {
        if (paused) { debt_ = 0; return; }
        // Bound the time accounted on one call to avoid integer overflow even
        // after suspend. Discarded time is visible rather than a hidden speedup.
        constexpr uint64_t max_us = 1000000;
        if (elapsed_us > max_us) { discarded_us_ += elapsed_us - max_us; elapsed_us = max_us; }
        debt_ += elapsed_us * 30u;
    }
    bool consume() {
        if (debt_ < 1000000u) return false;
        debt_ -= 1000000u; ++updates_; return true;
    }
    uint64_t pending_updates() const { return debt_ / 1000000u; }
    uint64_t updates() const { return updates_; }
    uint64_t discarded_us() const { return discarded_us_; }
    void reset() { debt_ = 0; }
private:
    uint64_t debt_ = 0, updates_ = 0, discarded_us_ = 0;
};
} // namespace l3
