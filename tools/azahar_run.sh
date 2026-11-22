#!/bin/zsh
# The one shared Azahar runner. Every session uses this script, so runs never
# overlap on the shared SD folder (2026-11-21: two runners without a common
# lock swapped and then deleted the owner's Legends3DS folder).
#
# Usage:
#   OWNER=<session-name> PASS=pass-YYYYMMDD-HHMMSS-m2-01516 ROUTE=<route.txt> \
#   [STAGE="stage 2 area 0"] [MAXMIN=15] [WAITMIN=180] [BOOTMIN=20] [STOPRE=<regex>] \
#   tools/azahar_run.sh <outdir>
#
# Rules it enforces:
#  - one run at a time: mkdir of $SDROOT/.legends3ds_run.lock (atomic); the
#    lock holds owner, run id and pid, and is removed only after the restore;
#  - it never moves a folder onto an existing name (refuses if the backup exists);
#  - it only deletes a Legends3DS that carries its own .installed_by marker;
#  - any failure that leaves the backup in place keeps the lock and says how to
#    recover, so nobody else starts on a half-restored folder.
# A stale lock (its pid is gone) is reported, never taken over automatically:
# check $SDROOT, put the original folder back by hand, then rmdir the lock.
# The screen must be unlocked (Azahar stalls behind the lock screen).
# BOOTMIN: minutes to wait for the first trace line; under heavy load the
# game has taken 16 minutes to boot (run58, load average ~60).
set -u
setopt nullglob

OUT=${1:?usage: tools/azahar_run.sh <outdir>}
OWNER=${OWNER:?set OWNER to your session name}
WORK=${WORK:-$HOME/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work}
W="$WORK/${PASS:?set PASS to the build pass directory name}"
ROUTE=${ROUTE:?set ROUTE to an autoplay route file}
SDROOT=${SDROOT:-$HOME/Library/Application Support/Azahar/sdmc/3ds}   # override for tests only
D="$SDROOT/Legends3DS"
B="$SDROOT/Legends3DS.bak-run"
LOCK="$SDROOT/.legends3ds_run.lock"
NOTES=${NOTES:-$HOME/Library/Application Support/Claude/side-session-notes/local_eacea414-6259-4158-b2aa-efacb210c6dc/azahar_runs.md}
RUNID="$OWNER-$(basename "$OUT")-$$"

note() { echo "$(date '+%F %H:%M') $RUNID: $*" >> "$NOTES" 2>/dev/null; echo "$*"; }
locked_screen() {
    ioreg -n Root -d1 -a 2>/dev/null | python3 -c 'import sys; o=sys.stdin.read(); i=o.find("CGSSessionScreenIsLocked"); sys.exit(0 if i>=0 and "<true/>" in o[i:i+60] else 1)'
}

[ -f "$W/sd_out/3ds/Legends3DS/Legends3DS.3dsx" ] || { echo "no build in $W"; exit 9; }
[ -f "$ROUTE" ] || { echo "no route $ROUTE"; exit 9; }

# 0. Runners that predate this script do not take the lock: also wait while
#    Azahar runs or another runner's backup folder exists.
foreign_busy() {
    [ -z "${AZAHAR:-}" ] && pgrep -x azahar >/dev/null && return 0
    for f in "$SDROOT"/Legends3DS.bak-boss-slope "$SDROOT"/Legends3DS.bak-stage-run4; do [ -e "$f" ] && return 0; done
    [ -n "$(find "$SDROOT" -maxdepth 1 -name 'Legends3DS.bak-*' -mmin -180 2>/dev/null)" ]
}
n=0
while foreign_busy; do
    n=$((n + 1)); [ $n -ge $(( ${WAITMIN:-180} * 3 )) ] && { echo "Azahar or another runner still busy"; exit 2; }
    sleep 20
done

# 1. The lock.
n=0
until mkdir "$LOCK" 2>/dev/null; do
    holder=$(cat "$LOCK/owner" 2>/dev/null)
    pid=${${holder##*pid=}%% *}
    if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
        echo "stale lock ($holder): its process is gone. Check $SDROOT by hand, restore Legends3DS, then rmdir \"$LOCK\"."
        exit 6
    fi
    n=$((n + 1)); [ $n -ge $(( ${WAITMIN:-180} * 3 )) ] && { echo "lock still held: $holder"; exit 2; }
    sleep 20
done
echo "$RUNID pid=$$ since=$(date '+%F %H:%M')" > "$LOCK/owner"
release() { rm -f "$LOCK/owner"; rmdir "$LOCK" 2>/dev/null; }
keep_lock() { note "$1 Lock kept at $LOCK; backup at $B. Restore by hand, then rmdir the lock."; exit 5; }

# 2. Preconditions, under the lock.
[ -z "${AZAHAR:-}" ] && locked_screen && { release; echo "screen locked; not starting"; exit 8; }
[ -z "${AZAHAR:-}" ] && pgrep -x azahar >/dev/null && { release; echo "Azahar is running outside the lock; not starting"; exit 2; }
[ -e "$B" ] && keep_lock "Backup name already exists (an interrupted run?); not moving anything."
[ -d "$D" ] || keep_lock "No Legends3DS folder to back up."

# 3. Back up, install, mark.
mv "$D" "$B" || keep_lock "mv to backup failed."
touch "$B"
note "Azahar run started ($(basename "$W"), $(basename "$ROUTE")); SD folder at Legends3DS.bak-run."
restore() {
    if [ -e "$D" ]; then
        if [ "$(cat "$D/.installed_by" 2>/dev/null)" = "$RUNID" ]; then rm -rf "$D"
        else keep_lock "Legends3DS is not the folder this run installed; left both in place."; fi
    fi
    mv "$B" "$D" || keep_lock "Restore mv failed."
}
PID=""
on_signal() { [ -n "$PID" ] && kill -9 "$PID" 2>/dev/null; sleep 1; restore; release; note "Interrupted; SD folder restored."; exit 130; }
trap on_signal INT TERM HUP

cp -R "$W/sd_out/3ds/Legends3DS" "$D" || { rm -rf "$D"; restore; release; echo "install copy failed"; exit 7; }
echo "$RUNID" > "$D/.installed_by"
cmp -s "$W/sd_out/3ds/Legends3DS/Legends3DS.3dsx" "$D/Legends3DS.3dsx" || { restore; release; echo "installed 3dsx differs"; exit 7; }
cp "$ROUTE" "$D/diagnostic_autoplay.txt"
[ -n "${STAGE:-}" ] && echo "$STAGE" > "$D/diagnostic_stage_01516.txt"
cp "$B/dspfirm.cdc" "$D/" 2>/dev/null

# 4. Run with a cap; stop on the end marker or when the trace stops growing.
mkdir -p "$OUT"
${AZAHAR:-/Applications/Azahar.app/Contents/MacOS/azahar} -w "$D/Legends3DS.3dsx" > "$OUT.azahar.log" 2>&1 &
PID=$!
MAXN=$(( ${MAXMIN:-15} * 12 )); last=-1; still=0; n=0
while kill -0 $PID 2>/dev/null && [ $n -lt $MAXN ]; do
    sleep 5; n=$((n + 1))
    sz=$(cat /dev/null "$D"/hardware_*.csv 2>/dev/null | wc -c | tr -d ' ')
    if [ "$sz" = "$last" ]; then still=$((still + 1)); else still=0; last=$sz; fi
    if [ $still -ge 36 ] || { [ $n -ge $(( ${BOOTMIN:-20} * 12 )) ] && [ "$sz" -eq 0 ]; }; then kill -9 $PID; echo "stalled (no trace growth)"; fi
    if grep -qE "${STOPRE:-# game_over,frame|diagnostic_autoplay_end}" /dev/null "$D"/hardware_*.csv 2>/dev/null; then
        sleep 10; kill -9 $PID 2>/dev/null; echo "stopped (end marker)"
    fi
done
kill -0 $PID 2>/dev/null && { kill -9 $PID; echo "timeout"; }
wait $PID 2>/dev/null

# 5. Collect, restore, unlock.
cp "$D"/boot_*.log "$D"/hardware_*.csv "$OUT"/ 2>/dev/null
restore
release
note "Azahar free, SD folder restored."
echo done
