# 0.0.15.0 validation (cloud session)

| Check | Result |
|---|---|
| Host build (clang 18, Release, -Werror) | PASS |
| Host suite (`ctest -E renderer_gcc_warning`) | 145/145 PASS (143 inherited + `gameplay_gates` + `combat_owner`) |
| `renderer_gcc_warning` | Not meaningful here: Ubuntu arm-none-eabi GCC 13 rejects `-march=armv6k -mfloat-abi=hard`; passes with devkitARM on the Mac |
| ASan+UBSan (GCC 13): combat_owner, gameplay_gates, actor_defeat_runtime, child_runtime, runtime_transactions | PASS |
| Native type check: all 113 Makefile sources vs real libctru/citro3d headers, arm-none-eabi GCC 13, -Wall -Wextra -Werror (-fsyntax-only) | PASS |
| `tools/verify_0146_source.py`, `tools/p2/verify_cumulative_tree.py` | PASS |
| Actual devkitARM link / `.3dsx` | **PENDING — run `tools/m1_mac_build.sh` on the Mac** |
| Owner-data cook with new combat sidecars | **PENDING (Mac)** — synthetic pipeline test passes |
| Azahar / physical Old 3DS | **PENDING** — `AZAHAR_TEST.md` |

Commands (cloud):
```sh
cmake -S . -B ../host -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build ../host -j2 && ctest --test-dir ../host -E renderer_gcc_warning
```
