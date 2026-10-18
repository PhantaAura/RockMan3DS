#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo "usage: $0 <CDDATA/DAT directory>" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DAT_DIR="$1"
BUILD="${MML_HOST_BUILD_DIR:-$ROOT/../build_flight_qa}"
PACK="$(mktemp -d "${TMPDIR:-/tmp}/legends3ds-flight-pack.XXXXXX")"
trap 'rm -rf "$PACK"' EXIT

printf '[1/6] strict host build\n'
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j2
printf '[2/6] host tests\n'
ctest --test-dir "$BUILD" --output-on-failure
printf '[3/6] fresh tutorial cook\n'
python3 "$ROOT/tools/build_tutorial_pack.py" "$DAT_DIR" "$BUILD" "$PACK" --with-indexed --with-material-cache
printf '[4/6] cooked player/runtime/material probes\n'
"$BUILD/l3player_runtime_probe" "$PACK"
"$BUILD/l3runtime_session_probe" "$PACK/legends3ds/tutorial"
"$BUILD/l3flight_render_probe" "$PACK/legends3ds/tutorial"
for f in "$PACK"/legends3ds/tutorial/materials/area_*.l3mc; do "$BUILD/l3inspect_material_cache" "$f"; done
printf '[5/6] source artifact safety scan\n'
forbidden="$(find "$ROOT" -path "$ROOT/build*" -prune -o -type f \( -iname '*.bin' -o -iname '*.cue' -o -iname '*.chd' -o -iname '*.iso' -o -iname '*.gme' -o -iname '*.pbd' -o -iname '*.tim' -o -iname '*.msg' -o -iname '*.xa' -o -iname '*.vab' -o -iname '*.sep' -o -iname '*.str' -o -iname '*.l3*' \) -print)"
if [[ -n "$forbidden" ]]; then
  echo "FAIL: source tree contains game/cooked assets:" >&2
  echo "$forbidden" >&2
  exit 1
fi
printf '[6/6] 3DS backend source presence\n'
test -f "$ROOT/platform/3ds/Makefile"
test -f "$ROOT/platform/3ds/source/main.cpp"
test -f "$ROOT/platform/3ds/source/vshader0143.v.pica"
printf 'FLIGHT QA: PASS\n'
