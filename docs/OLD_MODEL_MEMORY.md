# Azahar 2126.0: Old-model 3DSX memory allocation

Checked 2026-09-30 against the official `2126.0` release tag. Read-only source investigation; no emulator settings, application code, or game files changed.

**Finding:** The observed `new3ds=0`, `memory_layout=2`, `app_region_bytes=100663296` is expected for direct 3DSX loading in this release. It is Old-model emulation with a 96 MiB application region, not evidence that New 3DS mode was accidentally enabled.

1. `AppLoader_THREEDSX` derives from `AppLoader` without overriding `LoadKernelMemoryMode()`. See [3dsx.h, lines 18–49](https://github.com/azahar-emu/azahar/blob/2126.0/src/core/loader/3dsx.h#L18).
2. The inherited method explicitly returns `Kernel::MemoryMode::Dev1` when no internal override exists. Its comment identifies 96 MiB allocated to the application. See [loader.h, lines 138–149](https://github.com/azahar-emu/azahar/blob/2126.0/src/core/loader/loader.h#L138).
3. `System::Load()` initially selects Old-model `Prod`, then replaces it with the application's requested mode for a direct launch. Thus the loader's `Dev1` wins. See [core.cpp, lines 321–375](https://github.com/azahar-emu/azahar/blob/2126.0/src/core/core.cpp#L321).
4. The Old-model memory table assigns layout 0 an application region of `0x04000000` (64 MiB), and layout 2 `0x06000000` (96 MiB). The selected mode and region size are written into the emulated configuration memory read by libctru. See [memory.cpp, lines 28–65](https://github.com/azahar-emu/azahar/blob/2126.0/src/core/hle/kernel/memory.cpp#L28).

**Supported setting:** No application-memory selector was found in the release's settings definitions or System configuration implementation. The exposed `is_new_3ds` toggle selects model, not the 3DSX loader's inherited 96 MiB request. See [settings.h](https://github.com/azahar-emu/azahar/blob/2126.0/src/common/settings.h) and [configure_system.cpp](https://github.com/azahar-emu/azahar/blob/2126.0/src/citra_qt/configuration/configure_system.cpp#L347). Internal loader overrides and FIRM-launch parameters exist, but they are not an ordinary supported 64 MiB setting for directly opening this 3DSX. The plugin-specific override visible in `core.cpp` requests `Dev2` (80 MiB), not 64 MiB.

**Evidence limit:** Keep the test labeled “Azahar 2126.0, Old-model mode, 96 MiB application layout 2.” It cannot establish standard 64 MiB loader compatibility or physical Old 3DS performance. Small live malloc usage does not change the selected memory layout, and reserved heaps must not be presented as live allocation. A standard-64 MiB claim requires a separate verified execution with layout 0 or the relevant physical Old 3DS loader; no such execution was performed for this investigation.
