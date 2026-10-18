# Commands and results

The mutable source folder retains its historical name to reuse the valid host cache. It contains version 0.0.14.43. There is no Git repository, branch or commit.

Successful pipeline, run from the project workspace:

```sh
python3 -B work/Legends3DS_0.0.14.41_WorldComesAlive/tools/build_hardware_test.py --pack work/pass-20261014-worldalive-01440/pack-runtime-r1 --out work/pass-20261015-native-01443/native-r2 --host-build work/pass-20261014-worldalive-01441/native-r1/host
```

It ran CMake configure/build, all CTest checks, existing-pack validation and wrapping, runtime/inspection probes, and a clean `make -j4` of the actual 3DS target. Exact expanded commands and return codes are in `build-result.json`.

Result: 139/139 host tests pass in 5.30 seconds; actual native compilation passes with no warning/error lines. The 874,656-byte `.3dsx` has SHA-256 `b12b1dadd956f98d8eab7b9d7f53bcdedc951bef577a3b1e1861a0f348e3bd13`. All 280 native input hashes match the active source and staged snapshot. No extraction or payload recooking ran.

The initial `native-r1` attempt stopped with 138/139 tests passing because the intentionally changed enhanced shader no longer matched its frozen hash. That failure log is retained. The source verifier was updated to pin the deliberate UV revision while preserving the original 0143 hash; no failing assertion was suppressed.

```sh
python3 -B work/Legends3DS_0.0.14.41_WorldComesAlive/tools/verify_0146_source.py
/usr/local/bin/dkp-pacman -Q devkitARM libctru citro3d
/opt/devkitpro/devkitARM/bin/arm-none-eabi-g++ --version
```

Source-policy validation passes. Toolchain: devkitARM r68-1, GCC 16.1.0, libctru 2.7.0-1, Citro3D 1.7.1-2. ARMv6K/mpcore hard-float flags are preserved. The portable host build/tests pass; no standalone desktop visual build was tested.

Azahar 2126.0 was launched with isolated XDG config/data/cache and an owner-local SD layout. Configuration: `is_new_3ds=false`, CPU 100%, Vulkan, resolution factor 1. GUI testing exercised RGB/cyan, B, LEFT, RIGHT, X, A, movement/jump/fire inputs and normal START exit. Each mode’s first 3D GPU submission synchronized. Original native 400×480 captures, exact log paths/hashes and sampled counters are in `emulator-observation.json`. The owned emulator PID 20974 was absent after GUI quit. These checks do not prove physical hardware compatibility or retail parity.
