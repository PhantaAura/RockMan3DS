# Source action9 body and independent contact producer

Status: **SOURCE_BACKED_PARTIAL**. This implements original ST00 action9
`80103B58` and a separately admitted `8002FEA4(work,52,0,0)` contact
preparation. It does not establish complete callback, encounter or retail parity.

The first angle getter receives player position `800B51C4` then actor work+14;
its low16 difference from unsigned player heading `800B5206` is always stored
at work36E. The later turn helper receives actor position then player position.
Those two getter results are separate optional inputs. The original player entry
`8003BE40` passes base `800B51B0` to `8003BE6C`; base+56 is `800B5206`.
The native owner is the raw little-endian halfword at
`RuntimeSession::compat_player().bytes[0x56..0x57]`. Camera heading and a masked
accessor are not equivalent to this source unsigned load.

Phase0 sets phase1, request1, timer90, work330=0 and falls through. For phase0/1,
work80 above00480000 plus detection flag10 selects action3 and clears both
phase0A and aux0B. Other phases only retain work36E. The active moving route
uses source turn32, forward52, original fixed accumulator/raw conversion,
wall result and floor stores. The reducer retains the original 33-unit floor
rollback across all four raw and fixed lanes, contactF00 route to action1 with
restore0E=4, timer/range action10 and action4 boundaries, and absent-detection
common-transition flag.

Action9 sound176 has a distinct source position owner. The source saves a
pointer to work+14, then performs movement and wall mutation, then calls
`8001A050`, then performs floor stores. Thus the two queued words represent
**post-wall raw14..1B before floor stores or rollback**. The action9 result
carries those exact words. Action2's earlier cue uses its own pre-motion point;
one shared pre-body sound snapshot would be incorrect here.

`8002FEDC` is the delay-slot instruction within `8002FEA4`'s call to80030058,
not a separately callable motion function. The independent action9 proof checks
actual call arguments52,0,0 and executes the original8002FEA4 accumulator and
80030150 conversion. Its GTE delta is an explicitly declared boundary: authored
trig fixtures or private owner trig produce the supplied exact fixed delta.
Original turn, wall/floor and sound queue writer execute. No original instruction
listings, trig payloads or owner positions are included in the source checkpoint.

Dependencies remain explicit. Missing either angle, player heading, distance,
cue, motion or needed sound consumer holds every source-used state field and
suppresses all intents. Unsupported contact publishes no prepared motion. The
preparation snapshots include source action/phase/flags/heading, work80, turn
angle, cue and complete actor/contact work so runtime integration can reject
stale preparation. Source RNG ownership, the common tail, action4/10 bodies,
child attacks and damage delivery remain separate gates.

Evidence:

- 2,496 original action9/native reducer differential cases; 192 original cue calls.
- 288 authored contact cases:216 exact supported and72 explicitly held code8.
- 24 materialized owner-stage cases:24 exact supported.
- 926,476 original instructions for reducer/contact evidence.
- 48 original player-turn→action9 heading cases plus one player-base prefix;
  2,983 original instructions.
- Both native functions execute640 repeated admitted calls without allocations.

Source-safe fixtures are `tests/fixtures/class20_03_action9_01420.tsv` and
`tests/fixtures/actor_action9_contact_01420.tsv`. Source hashes, external
premises and exact native commands/results are recorded in
`docs/validation_01420/action9-*-evidence.json` and
`docs/validation_01420/action9-native-differential.json`.

```sh
python3 -B docs/research_01420/class20_03_action9_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/owner-local/action9-proof
python3 -B docs/research_01420/action9_player_heading_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/owner-local/action9-heading-proof
```
