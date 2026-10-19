# Resume 0.0.15.2 (World Comes Alive M2b)

Repo: GitHub `PhantaAura/RockMan3DS`, branch `main`. Parent: 0.0.15.1.

## Completed
- Recovered the four diagonal family classifiers at `0x8004E490`,
  `0x8004E750`, `0x8004EA10`, and `0x8004ECD0`.
- Ported the source dispatcher paths through `0x8004F8BC`, `0x8004FBE8`, and
  `0x8004FF14`: signed 32-bit projection arithmetic, vertical-vs-horizontal
  response choice, direction bits, and corner axis resolution.
- Families `0x21`, `0x31`, `0x41`, and `0x51` now slide in the normal native
  runtime. Four original-executable vectors match exactly. Related subtype-2
  records share the verified response; unsupported diagonal subtypes still hold.
- Preserved the Area 1 containment fixes and source combat. Owner-local Area 1
  audit, 200 seeds x 3000 frames: 0 unsupported, 0 escapes, 0 falls, 53 cells.

## Validation
- Host: `cmake -S . -B /private/tmp/l3-host-01502 -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`
- Host: `cmake --build /private/tmp/l3-host-01502 -j4`
- Host: `ctest --test-dir /private/tmp/l3-host-01502 --output-on-failure` — 147/147 PASS.
- Real-data audit: `l3edge_audit PACK 1 2822 -1 -4833 200 3000` — PASS as above.
- Native: `tools/m1_mac_build.sh` — PASS; 924,136-byte ARMv6K `.3dsx`, SHA-256 `41abf4b4ba762744b7c108d2a31dc46c2b487ff05e7ea5ea4d46ca1647f1c16f`.
- Azahar: unavailable this pass because macOS reported the screen locked; no emulator result claimed.

## Remaining M2 work
`0x80050400` deep-overlap recovery remains untranslated. Full collision parity
also needs retail traces for corner chronology, steps/slopes/ledges, additional
areas, and physical Old 3DS validation. `PHYSICS_1TO1=false`.

Recommended next objective: recover `0x80050400`, then capture paired PS1/native
movement traces through Area 1 corners before expanding the area-transition set.
