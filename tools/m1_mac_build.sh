#!/usr/bin/env bash
# Legends3DS 0.0.15.16 (M2 pop-out Reaverbot) — one-command Mac build + Azahar-ready SD folder.
#
# Runs the existing owner-local pipeline (tools/build_mml3ds.py): host build,
# full host test suite, tutorial cook (cache reused when unchanged), actual
# devkitARM Old 3DS build, SD layout. Then copies the trace-observed RNG profile
# (source_rng.l3rg) from an earlier private pack, because it cannot be cooked
# from the EXE alone. Nothing in earlier checkpoints is modified.
#
# Override any path with environment variables, e.g.
#   OWNER=/path/to/extracted RNG=/path/to/source_rng.l3rg tools/m1_mac_build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${PROJECT:-$HOME/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca}"
OWNER="${OWNER:-$PROJECT/owner-data/extracted}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${OUT:-$PROJECT/work/pass-$STAMP-m2-01516}"
CACHE="${CACHE:-$PROJECT/work/mml3ds-cache-01500}"
CUE="${CUE:-$HOME/Downloads/MML_Discoken/Mega Man Legends (USA).cue}"
RNG="${RNG:-$PROJECT/work/pass-20261017-worldalive-01447/native-r1/sd_out/3ds/Legends3DS/data/legends3ds/tutorial/source_rng.l3rg}"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"

echo "Source : $ROOT"
echo "Owner  : $OWNER"
echo "Output : $OUT"
[ -d "$OWNER" ] || { echo "Owner data folder not found: $OWNER" >&2; exit 1; }
[ -f "$DEVKITARM/3ds_rules" ] || { echo "devkitARM not found at $DEVKITARM (set DEVKITARM)" >&2; exit 1; }

python3 "$ROOT/tools/build_mml3ds.py" "$OWNER" --out "$OUT" --cache "$CACHE" --tutorial-only

SD="$OUT/sd_out/3ds/Legends3DS"
DATA="$SD/data/legends3ds/tutorial"
[ -f "$SD/Legends3DS.3dsx" ] || { echo "Native build did not produce $SD/Legends3DS.3dsx" >&2; exit 1; }

if [ -f "$DATA/source_rng.l3rg" ]; then
    echo "RNG profile already present."
elif [ -f "$RNG" ]; then
    cp -n "$RNG" "$DATA/source_rng.l3rg"
    echo "Copied private RNG profile: $RNG"
else
    echo "WARNING: no source_rng.l3rg found ($RNG)."
    echo "         The tutorial enemy will hold at its first random draw (it will not move or attack)."
fi

# Audio (owner-derived, private): effect bank from the DAT VABs, and the
# message voice clips from the full CUE/BIN when present. Cached by tool hash.
mkdir -p "$DATA/audio" "$CACHE/audio"
SFX_KEY="$(shasum -a 256 "$ROOT/tools/cook_sfx_bank.py" | cut -c1-16)"
SFX="$CACHE/audio/sfx_st00-$SFX_KEY.l3sb"
[ -f "$SFX" ] || python3 "$ROOT/tools/cook_sfx_bank.py" --exe "$OWNER/ROCK_NEO.EXE" --dat "$OWNER/CDDATA/DAT" --out "$SFX"
cp "$SFX" "$DATA/audio/sfx_st00.l3sb"
if [ -f "$CUE" ]; then
    VOICE_KEY="$(cat "$ROOT/tools/cook_xa_voice.py" "$ROOT/tools/cook_xa_stream.py" | shasum -a 256 | cut -c1-16)"
    VOICE="$CACHE/audio/voice_st00-$VOICE_KEY.l3vb"
    [ -f "$VOICE" ] || python3 "$ROOT/tools/cook_xa_voice.py" --cue "$CUE" --exe "$OWNER/ROCK_NEO.EXE" --out "$VOICE" --requests 0x0-0x2d
    cp "$VOICE" "$DATA/audio/voice_st00.l3vb"
else
    echo "WARNING: no CUE at $CUE; Roll's voice clips are not installed (text stays silent)."
fi

cat <<EOF

BUILD OK
  SD folder for Azahar : $OUT/sd_out
  App to load          : $SD/Legends3DS.3dsx
  Build report         : $OUT/build-result.json

Azahar: Emulation > Configure > System: New 3DS mode OFF.
        Emulation > Configure > Storage: Use custom storage, SDMC = $OUT/sd_out
        File > Load File... > $SD/Legends3DS.3dsx
Then follow docs/validation_01516/AZAHAR_TEST.md and send back:
        $SD/boot_01516.log and $SD/hardware_01516*.csv
EOF
