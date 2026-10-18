#include "legends3ds/render/gpu_frame_wait.h"
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace {
constexpr uint64_t never = std::numeric_limits<uint64_t>::max();

struct FakeDevice {
    uint64_t tick = 0, elapsed = 0;
    uint64_t top_at = 1, bottom_at = 1, queue_at = 1;
    uint32_t top_base = 0, bottom_base = 0;
    unsigned attempts = 0, samples = 0, sleeps = 0;
    bool frame_open = false;

    l3::GpuFrameWaitResult wait(uint64_t budget, bool display = true) {
        return l3::wait_for_gpu_frame(budget, display,
            [&] { return tick; },
            [&] {
                ++samples;
                return l3::GpuDisplayCounters{
                    uint32_t(top_base + uint32_t(elapsed >= top_at)),
                    uint32_t(bottom_base + uint32_t(elapsed >= bottom_at))};
            },
            [&] {
                ++attempts;
                assert(!frame_open);
                frame_open = elapsed >= queue_at;
                return frame_open;
            },
            [&] { ++tick; ++elapsed; ++sleeps; });
    }
};
}

int main() {
    using Status = l3::GpuFrameWaitStatus;
    {
        FakeDevice d; d.top_at=2; d.bottom_at=3; d.queue_at=5;
        const auto r=d.wait(10);
        assert(r.status==Status::Acquired && r.elapsed_ticks==5);
        assert(d.frame_open && d.attempts==3 && d.sleeps==5);
    }
    for (bool stalled_top : {false,true}) {
        FakeDevice d;
        if(stalled_top)d.top_at=never;else d.bottom_at=never;
        const auto r=d.wait(8);
        assert(r.status==Status::DisplayClockTimeout && r.elapsed_ticks==8);
        assert(!d.frame_open && d.attempts==0 && d.sleeps==8);
    }
    {
        FakeDevice d; d.top_at=1; d.bottom_at=2; d.queue_at=never;
        const auto r=d.wait(7);
        assert(r.status==Status::QueueTimeout && r.elapsed_ticks==7);
        assert(!d.frame_open && d.attempts==5);
    }
    {
        // Four ticks of display waiting leave only one tick for the queue.
        FakeDevice d; d.top_at=4; d.bottom_at=4; d.queue_at=6;
        const auto r=d.wait(5);
        assert(r.status==Status::QueueTimeout && r.elapsed_ticks==5);
        assert(!d.frame_open && d.attempts==1);
    }
    {
        // An acquisition at the deadline must never open a frame.
        FakeDevice d; d.queue_at=5;
        const auto r=d.wait(5);
        assert(r.status==Status::QueueTimeout && !d.frame_open);
        assert(d.attempts==4);
    }
    {
        // Time spent observing clocks cannot extend the queue deadline.
        uint64_t tick=0; unsigned samples=0,attempts=0;
        const auto r=l3::wait_for_gpu_frame(5,true,[&]{return tick;},
            [&] {
                if(samples++==0)return l3::GpuDisplayCounters{};
                tick=7;return l3::GpuDisplayCounters{1,1};
            },
            [&]{++attempts;return true;},[&]{++tick;});
        assert(r.status==Status::QueueTimeout && r.elapsed_ticks==7);
        assert(attempts==0);
    }
    {
        FakeDevice d; d.tick=never-2; d.top_base=UINT32_MAX;
        d.bottom_base=UINT32_MAX; d.queue_at=3;
        const auto r=d.wait(6);
        assert(r.status==Status::Acquired && r.elapsed_ticks==3);
        assert(d.tick==0 && d.frame_open);
    }
    {
        FakeDevice d; d.tick=never-1; d.queue_at=never;
        const auto r=d.wait(4);
        assert(r.status==Status::QueueTimeout && r.elapsed_ticks==4);
        assert(d.tick==2 && !d.frame_open);
    }
    {
        // A resource-release fence need not wait for another display tick.
        FakeDevice d; d.top_at=never; d.bottom_at=never; d.queue_at=0;
        const auto r=d.wait(1,false);
        assert(r.status==Status::Acquired && r.elapsed_ticks==0);
        assert(d.frame_open && d.samples==0 && d.attempts==1);
    }
    {
        FakeDevice d;
        const auto r=d.wait(0);
        assert(r.status==Status::DisplayClockTimeout && r.elapsed_ticks==0);
        assert(!d.frame_open && d.attempts==0 && d.sleeps==0);
        const auto queue_only=d.wait(0,false);
        assert(queue_only.status==Status::QueueTimeout && !d.frame_open);
    }
}
