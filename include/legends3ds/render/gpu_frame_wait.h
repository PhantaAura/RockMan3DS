#pragma once
#include <cstdint>

namespace l3 {

struct GpuDisplayCounters {
    uint32_t top = 0;
    uint32_t bottom = 0;
};

enum class GpuFrameWaitStatus : uint8_t {
    Acquired,
    DisplayClockTimeout,
    QueueTimeout,
};

struct GpuFrameWaitResult {
    GpuFrameWaitStatus status = GpuFrameWaitStatus::QueueTimeout;
    uint64_t elapsed_ticks = 0;
};

// Platform adapter: Now returns a monotonic unsigned tick counter; Counters
// samples the two display clocks; TryBegin must attempt frame acquisition
// without blocking, opening a frame only when it returns true. Pause must
// return promptly (the native adapter sleeps 1 ms). No callback may allocate
// or wait indefinitely. Counter subtraction tolerates one unsigned wrap;
// the timeout must be shorter than the tick counter's full wrap period.
// One budget covers display pacing AND GPU queue acquisition. The caller
// owns a frame only on Acquired; timeout never makes an acquisition attempt
// at/after the deadline. This policy does not supply a retail timing value.
template<class Now, class Counters, class TryBegin, class Pause>
GpuFrameWaitResult wait_for_gpu_frame(uint64_t timeout_ticks,
                                    bool wait_for_display,
                                    Now now,
                                    Counters counters,
                                    TryBegin try_begin,
                                    Pause pause) {
    const uint64_t start = now();
    const auto initial = wait_for_display ? counters() : GpuDisplayCounters{};
    bool top_ready = !wait_for_display;
    bool bottom_ready = !wait_for_display;
    for (;;) {
        const uint64_t elapsed = uint64_t(now() - start);
        if (elapsed >= timeout_ticks)
            return {top_ready && bottom_ready ? GpuFrameWaitStatus::QueueTimeout
                                             : GpuFrameWaitStatus::DisplayClockTimeout,
                    elapsed};
        if (!top_ready || !bottom_ready) {
            const auto current = counters();
            top_ready = top_ready || current.top != initial.top;
            bottom_ready = bottom_ready || current.bottom != initial.bottom;
        }
        if (top_ready && bottom_ready) {
            // Counter observation can consume the remaining budget too.
            const uint64_t before_acquire = uint64_t(now() - start);
            if (before_acquire >= timeout_ticks)
                return {GpuFrameWaitStatus::QueueTimeout, before_acquire};
            if (try_begin())
                return {GpuFrameWaitStatus::Acquired, uint64_t(now() - start)};
        }
        pause();
    }
}

} // namespace l3
