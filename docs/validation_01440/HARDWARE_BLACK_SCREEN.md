# Original Old 3DS XL top-screen failure

User report,2026-10-14:0.0.14.39 runs the bottom map but top is pure black. HoldingSELECT for0143fallback also stays black. This is real user-reported hardware evidence and invalidates any assumption that Azahar boot proves hardware rendering. No device logs/photo acquired; exact renderer flags, package version, frame admission and GPU command usage are not yet measured.

0.0.14.40 explicitly selects2D(left-eye output), checks C3D_FrameBegin/C3D_FrameDrawOn return values, and adds an opt-in L+R startup bisect. HoldL+R while launching, pressA on the existing start prompt if it appears. Test1 writes red/green/blue directly to the top CPU framebuffer and flushes that cache. PressA for test2: a brightcyan GPU target clear without geometry, textures, camera, shader draw or fade. PressA again to continue normal gameplay. START exits through the normal error/cleanup flow. This diagnostic is not a game render or a confirmed black-screen fix.

Interpretation: CPUtestblack points to presentation/display setup; CPUcolors+GPUblack narrows to GPU target/transfer; bothvisible+gameblack narrows to draw/camera/state/command submission. These are hypotheses to follow after an actual device report, not established causes. Preserve boot_01440.log and hardware_01440_*.csv from the SD app directory. No reset used.
