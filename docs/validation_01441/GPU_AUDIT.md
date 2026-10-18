# Bounded GPU audit

Installed Citro3D 1.7.1-2 and libctru 2.7.0-1 were inspected read-only, including their installed static libraries. The shared 240x400 RGBA8/depth24 target and RGB8 output are standard settings. C3D_FrameEnd(0) flushes the linear heap; adding arbitrary VBO flushes is not an established fix. The default command buffer is 0x40000 bytes; libctru overflow raises a user-break panic rather than silently dropping all draws. A continuously updating bottom map is evidence against that particular failure, though no physical command trace has been acquired.

The supplied 14.40 emulator trace has 33 sampled rows, each 12 draws and 1,478 triangles. About 992 triangles use the 59,520-byte terrain VBO, leaving roughly 486 immediate triangles and an estimated 70KB of vertex commands before state. This is an estimate for those emulator samples, not a hardware worst-case measurement. The new runtime logs C3D_GetCmdBufUsage after FrameEnd, which describes the completed submission. No physics, palettes, shaders or gameplay have been replaced based on speculation.

Primary reference: https://raw.githubusercontent.com/devkitPro/citro3d/master/source/renderqueue.c
Primary reference: https://raw.githubusercontent.com/devkitPro/libctru/master/libctru/source/gpu/gpu.c

The early startup HID chord was sampled once, and the old SD AUTO_START policy could bypass the menu. The automatic diagnostic removes that dependency; the exact device-side reason the held chord was absent is not yet observed.
