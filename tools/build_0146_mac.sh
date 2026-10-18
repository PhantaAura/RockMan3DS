#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: $0 /absolute/path/to/extracted/CDDATA/DAT" >&2
  exit 2
fi
DAT="$1"
if [ ! -d "$DAT" ]; then
  echo "DAT directory not found: $DAT" >&2
  exit 2
fi

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITARM:=$DEVKITPRO/devkitARM}"
export DEVKITPRO DEVKITARM

cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --target \
  test_core test_push2 \
  l3cook_st00 l3cook_render l3cook_st00_flow l3cook_pbd \
  l3cook_player_comp l3cook_anim l3cook_skeleton l3cook_msg l3cook_ebd \
  l3cook_texture l3cook_palette l3cook_material_cache l3cook_player_material_cache \
  l3flight_material_probe l3flight_animation_probe -j2
ctest --test-dir build-host --output-on-failure

rm -rf tutorial_pack sd_out
python3 tools/build_tutorial_pack.py \
  --with-indexed --with-material-cache \
  "$DAT" build-host tutorial_pack

TROOT="$ROOT/tutorial_pack/legends3ds/tutorial"
"$ROOT/build-host/l3flight_material_probe" "$TROOT"
"$ROOT/build-host/l3flight_animation_probe" "$TROOT/player/main.l3anim" 0x0C

cd "$ROOT/platform/3ds"
make clean
make -j1
cd "$ROOT"

python3 tools/build_flight_sd.py \
  tutorial_pack sd_out \
  --3dsx platform/3ds/Legends3DS_FlightProbe_0146.3dsx

python3 tools/verify_0146_source.py

echo
echo "0146 build complete. SD tree: $ROOT/sd_out/3ds/Legends3DS"
echo "3DSX: $ROOT/platform/3ds/Legends3DS_FlightProbe_0146.3dsx"
