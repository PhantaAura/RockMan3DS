# GPU frame/resource ownership

Before01442, area replacement and script-resource cleanup could free buffers/textures before the next FrameBegin synchronized the previous submitted frame. Shutdown likewise released objects before Citro3D drained its queue. This is a concrete source ordering hazard; it does not establish the original first-frame failure's cause.

01442 acquires a real frame before resource-changing updates and drains before teardown. Pending ownership clears only after successful nonblocking acquisition. Resource releases reject a pending lease. CPU updates, source packet semantics and0143 shader/geometry/camera math are retained. The final preview change preserves a retained live pose's root instead of incorrectly borrowing its cached root.

The allocation-free wait policy has one5second diagnostic budget across display pacing and queue acquisition. It reports distinct clock/queue timeouts, leaves failed resources retained, and keeps error-console polling independent of VBlank. Host tests exercise stalled display/GPU, retries, shared deadline, zero budget and unsigned wrap. A real GPU hang was not induced in Azahar; recovery from a hung physical GPU is not claimed.

Citro3D1.7.1 FrameBegin's synchronization/wait and FrameEnd's queued processing explain the fence placement: https://raw.githubusercontent.com/devkitPro/citro3d/v1.7.1/source/renderqueue.c . The clear-only probe now marks the public target used for transfer without binding3Dstate. That is cleanup of the probe's state usage, not proof that it caused the physical failure. Historical issue35 alone does not establish a defect in this probe: https://github.com/devkitPro/citro3d/issues/35 .

Source preservation hashes and compiled-input identities accompany the build. Contact telemetry retains cumulative receipts and throttles repeated messages; it no longer writes every continuing actor contact to SD.
