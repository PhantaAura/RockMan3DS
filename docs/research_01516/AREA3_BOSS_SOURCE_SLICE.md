# ST00 Area 3 boss: event ownership and intro slice

Status: **SOURCE_BACKED_PARTIAL**. This is not a complete boss encounter and
does not claim 1:1 AI, combat, camera, HUD, defeat, or progression.

## Original evidence

The Area 3 source object list has 11 records. Its first record is class 00,
event ID 3; there is no placed class-40 boss. The Area 3 EBD row 0 is class
`40`, subtype `02`. A PSXRecomp run of the owner-supplied US SLUS-00603 game
reached Area 3 after flag `0x622` and showed that event 3 allocates class-40
slot 0 at `0x800C3580`.

The live class-40 wrapper is `0x801076E8`. Its state table is at `0x8011C054`;
state 0 selects `0x801078B8`. A clean pre-fight save (slot 2) was replayed for
420 original frames. Event 3 replaces a stale class-40 slot at frame 49 and
initializes the boss at frame 52 with:

- class/subtype/parameter `40/02/01`;
- spawn position `(-256,-385,1001)`, heading `0`;
- Q12 scale `(512,512,512)`, shade `0x4210`;
- HP `1024` at work `+0x6E`;
- action/phase/substate `1/0/1`;
- pose request/record `0/0`;
- a 12-part articulated model from Area 3 EBD row 0.

Initializer `0x801078B8` allocates two owned class-37/subtype-10 children at
work `+0x514` and `+0x518`, obtains HP from EBD stats byte 8 shifted left 5,
and selects action 1 for parameter 1. The wrapper dispatches action 1 through
`0x80107B48`; the intro uses substates `0x80107F40`, `0x80108C18`,
`0x80107FAC`, and `0x801084D4`.

The observed entry sequence is:

- Mega Man arrives at z `-1920`, is briefly placed at `-1984`, then is forced
  forward in 16-unit steps to z `-1632`;
- phase 0 waits for that endpoint, then phase 3 installs request 3 and a
  12-tick countdown;
- phase 1/request 1 moves the boss from z `1001` to `-327` in 83 steps of 16;
- phase 2/request 2 moves it from `-327` to `-1223` in 28 steps of 32;
- one handoff update returns to action 1/phase 0, then action 6 phase 0 sets
  work `+0x4E4` to 80;
- the first action-6 presentation cycle advances through phases 1..5 with
  request/countdown pairs `11/12`, `11/9`, `12/19`, `12/9`, and `13/14`,
  then returns to action 1/phase 0. The next attack selection is not yet
  translated.

Ghidra headless analysis of the preserved owner-local ST00 overlay confirms
that `0x80107FAC` initializes phase 0, compares work `+0x80` against
`0xC8000`, decrements the 80-tick decision timer while distant, turns and
moves through shared helpers, and selects actions 2, 3, or 6. The clean replay
above covers the near-player branch and its shared animation phases; movement,
collision and the other distance branch remain outside the native slice.

The next boundary is now statically resolved as well. `0x80107B48` is the
outer action-1 wrapper, not another attack body. It refreshes work `+0x4E0`
bit `0x20` through `0x8003648C`, optionally calls `0x8010A448` for bit 1,
then dispatches work byte `+9` through the table at `0x8011C068`. Substate 1
selects `0x80107FAC`. When the dispatched body returns with work `+0x4E0`
bit 0 set and HP remains positive, the wrapper resets work `+0x0A` and chooses
substate 1 when the proximity bit is set or the prior substate was 2; otherwise
it routes 0/4 through 3, restores 3 from work `+0x0D`, or returns the remaining
states to 0. HP below 1 selects substate 0 and begins the separate death path.
This chooser is source evidence for the next implementation slice; it is not
yet native encounter behavior.

`0x8003648C` is not a stateless radius comparison that can be replaced with a
guessed distance. It passes actor work `+0x504`, the actor pointer and heading
`+0x56` to `0x800364B0`. That body first compares work `+0x80` with a
profile-table byte shifted by 20, then advances a state byte at detection
work `+7`, samples the global player position through `0x80030628`, stores
relative halfwords at detection work `+8..+0x0C`, and uses collision/status
helpers before returning the boolean consumed as work `+0x4E0` bit `0x20`.
Therefore the pure native chooser accepts the already-refreshed bit. Wiring
it into live combat requires the original `+0x504` detection state and work
`+0x80` writer; this boundary is intentionally not replaced by a Euclidean
shortcut.

A separate sampled active encounter, retained only as a later-state witness,
had:

- class/subtype/parameter `40/02/01`;
- position `(-256,-385,-1223)`, heading `4095`;
- Q12 scale `(512,512,512)`, shade `0x4210`;
- HP `1024` at work `+0x6E`;
- action/phase `6/5`;
- pose request `13`, record `9`, countdown `1`;
- the same 12-part articulated model.

The callback uses the shared animation (`0x80032488`), packed-pose
(`0x80033A28`), hierarchy (`0x8003346C`), contact/damage, and class-40 draw
paths. Those later observations define the next translation boundaries; they
are not acknowledged as implemented combat behavior.

## Native slice

### Event-owned player entry

The 2026-10-27 PSXRecomp slot-2 replay resolves an important ownership detail.
With injected input cleared, frame history reports pad `0xFFFF` on every frame
7354 through 7382. Player work `+0x18` nevertheless advances from z `-1952`
through `-1920`, `-1888`, ... to exactly `-1632`. Each coordinate is visible
for two 60 Hz presents, proving a 16-unit step per 30 Hz fixed update. The
event therefore owns the authored player position; it does not synthesize Up
input or route the step through ordinary terrain collision. This is
`VERIFIED_1TO1` for the neutral pad, fixed increment and endpoint only.

`source_area3_boss_spawn_spec_801076e8` admits exactly one class-00 event ID 3.
Event IDs are area-local, so `append_source_area3_boss_801076e8` additionally
requires ST00 source area index 3; an early Azahar run exposed that Areas 1 and
2 also use event ID 3, and the area guard prevents false boss ownership there.
The appender then decodes the first class `40/02` EBD row, binds fresh request
0 and a bounded immutable bank for original requests 0, 1, 2, 3, 11, 12 and
13, and appends one dynamic owner to the existing actor area.
`tick_source_area3_boss_intro_80107b48` runs independently of diagnostic actor
gates and is owned by the Area 3 event rather than the gameplay camera. It
commits only the bounded entry position until z `-1632`, retains that endpoint
through the recovered presentation cycle, uses a neutral pad and bypasses
ordinary map collision, so neither the doorway threshold nor the still-live
door-arrival state can restore the room spawn. Dialogue, cutscene and
source-player gates remain intact. It follows
the observed phase
path and movement increments, advances only resident pose data, executes the
first bounded action-6 presentation cycle, and stops at the next action-1
selection boundary. It does not synthesize a placed object or change the
established class-20 runtime.

Owner-local probe result on the current cooked Area 3 package:

```text
status=SourceBackedPresentation actors=1 row=0 parts=12
pose_request=0 pose_record=0 hp=1024 action=1 phase=0 substate=1
spawn_z=1001 decoded_bytes=46398
```

Host tests cover absent, unique, and duplicate event-3 records, fresh source
constants, non-Area-3 rejection, the complete admitted intro/first action-6
cycle, exact endpoints, zero tick-time allocations and the pure action-1
post-dispatch chooser (including HP-zero/death-bit behavior). The real-package probe is
`l3probe_area3_boss`; retail bytes remain outside Git.

The complete host suite passes 170/170 and the Old 3DS target builds. Continuous
Old-3DS-mode Azahar trace `hardware_01516_4002111785105.csv` reaches flag
`0x622` at frame 9772, enters the original owner, reaches CombatReady at frame
10090 and keeps Mega Man at `(-256,-385,-1632)` through frame 10800. It exits
cleanly at frame 10854. Earlier trace `hardware_01516_4002111246033.csv` is
retained as negative evidence: releasing ownership at the endpoint let the
door-arrival player state restore z `-1984` while the boss continued. The
validated build retains event ownership only until intro leg 6.

## Next exact step

Bind the recovered action-1 chooser to the original `+0x504` detection owner
and work `+0x80` writer, then translate the movement/contact branches of
`0x80107FAC`. Recover the wrapper's
`0x800341BC` radius-208 hit proxy, bind Buster damage to HP `+0x6E`, and add
the original boss gauge. Child allocation identities are proved, but their
live behavior is not ported. Do not add a synthetic defeat or transition.
