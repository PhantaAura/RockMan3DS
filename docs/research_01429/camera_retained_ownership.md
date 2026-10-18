# Retained scripted-camera vectors — 0.0.14.29

Status: SOURCE_BACKED_PARTIAL. No live retail chronology or hardware parity is
claimed. The field labels remain their original offsets.

The first supported `8001EC0C` command prefix ultimately integrates three
vectors. The third position vector is source scene work `800BC738+68/+6C/+70`
(addresses `800BC7A0/A4/A8`), with corresponding velocity at `+78/+7C/+80`.
The 01428 constructor retained the position vector and explicitly zeroed its
velocity. Native validity therefore correctly held when retained positions had
no known owner.

The executable cold entry provides an exact owner. Original `80068000..80068020`
clears `[80098158,800C5688)` in four-byte words. This range contains the entire
188-byte source scene camera. The last word is `800C5684`; the exclusive end is
preserved. The probe fills all 185648 bytes with nonzero authored input and guard
words on both sides, executes unchanged original instructions, and checks the
complete zero result and unchanged guards. Four cases execute 928256 original
instructions with zero patches and zero hooks.

The fresh gameplay camera initializer is a different owner. Its measured prefix
`80038648..80038860` clears controller work beginning at `800A3958`, writes player
position/rotation and other controller fields, and **preserves all 188 retained
scene camera bytes** in 24 nonzero authored cases (8808 original instructions).
Therefore calling this initializer alone cannot justify initializing retained
scene vectors to zero.

A supported explicit sequence was exercised: cold executable clear, measured
fresh controller prefix with authored player position, original pool resets,
then the unchanged phase5 constructor using the supplied supported overlay and
area EBD. The constructor executes 7560 original instructions, reaches phase6,
and retains zero `+68/+6C/+70` with zero velocity `+78/+7C/+80`. Its first source
callback prefix `8010102C → 80100738`, under explicit first-counter=0/input=0
premises, executes another 53 instructions and preserves those vectors.

The cold boot premise can admit these fields in the existing explicitly named
fresh-executable-bootstrap subset. It must be a deliberate initialization before
construction, with a source owner attached to the validity markers. It cannot
be implemented inside the generic constructor as a missing-field fallback, and
must not run on an area transition, saved-state load or captured warm scene.
Future camera/script updates must preserve or replace the same field owner.

This proof does not execute the complete title/NewGame chronology or all interim
camera owners. GTE/render calls beyond the measured controller prefix remain
outside the proof. The first callback counter/input are explicit probe premises,
not a claim that the retail menu sequence naturally reached them. Warm retained
camera fields require a capture or their own proven writers. Camera command
execution, the complete timeline and scene completion remain separate work.

The source probe exports only hashes, addresses, scalar checks and instruction
counts. Actual overlay/EBD bytes and private disassembly remain outside the
source release.

Exact command from the source directory:

```sh
python3 -B docs/research_01429/camera_retained_probe.py --exe ../../owner-data/extracted/ROCK_NEO.EXE --st00 ../../owner-data/extracted/CDDATA/DAT/ST00.BIN --area ../../owner-data/extracted/CDDATA/DAT/ST00_00.BIN --out ../pass-20261011-worldalive-01429/camera-retained-oracle-r2
```

Result: PASS. The source probe raises only its instrumentation step budget from
10000 to 1000000 so the original cold loop can finish; it does not alter code or
substitute helper behavior.
