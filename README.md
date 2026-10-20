# Mega Man Legends 3DS — 0.0.15.15 (tutorial radio)

0.0.15.15: walking down the first corridor now triggers Roll's radio call, as
in the original. Mega Man stops, a message box on the bottom screen shows her
lines and the firing and lock-on instructions, and "Read instructions again?"
offers Yes/No. Choosing No releases him, and the next trigger ("Turn right
there.") follows. This is the original ST00 tutorial trigger controller,
ported from the game's own code.
[Results](docs/validation_01515/RESULTS.md) · [Resume](RESUME_01515.md).

---

# Mega Man Legends 3DS — 0.0.15.14 (Buster shot, combat loop)

0.0.15.14: Buster shots now hit. The port was firing the wrong weapon lane: a
subtype-0x80 object tied to Mega Man's feet. Running the original game in
PCSX-Redux showed the real shot is subtype 0x81, fired from the hand, moving
100 units a frame for 18 frames. That shot and its impact effect are now ported.
Scripted Azahar runs (trace evidence) show a Reaverbot shot to 0 HP, its reward
spawning and being collected, and the first door working both ways.
[Results](docs/validation_01514/RESULTS.md) · [Resume](RESUME_01514.md).

---

# Mega Man Legends 3DS — 0.0.15.13 (enemy attacks)

0.0.15.13: Reaverbots froze when Mega Man got close, and Buster shots could not
hit them. Their close-range lunge (ST00 action 10) was missing, so they stopped
updating, hitbox included. The lunge, the hit knockback (action 6) and the
damaged bounce and self-destruct (action 5) are now ported from the original
code. Host 153/153 and the native build pass; Azahar pending.
[Research](docs/research_01513/LATE_ACTIONS_5_6_10.md) · [Resume](RESUME_01513.md).

---

# Mega Man Legends 3DS — 0.0.15.12 (enemy reward pickups)

0.0.15.12: defeated Reaverbots drop the original reward pickups
(`0x80046B34`). They arc out, bounce, rest, blink and despawn, and Mega Man
collects them for money and energy. The pickup model is still a diagnostic
marker. Host 152/152 and the native build pass; Azahar pending.
[Results](docs/validation_01512/RESULTS.md) · [Resume](RESUME_01512.md).

---

# Mega Man Legends 3DS — 0.0.15.11 (visible Buster shot)

0.0.15.11: in 0.0.15.10, pressing Y fired a Buster shot, but nothing appeared on
screen. The shot was spawned, but its diagnostic ribbon always had zero length:
the original code copies the shot's end point over its start point right after
registering the hit segment. The ribbon now draws the registered segment, from
Mega Man to the end of the shot. The shot's length (a fixed forward ray) is
still CANDIDATE. In Azahar, 5 of 5 presses spawn and draw a shot (trace). On
screen it is only a short sliver, because the camera sees the ray end-on.
[Research](docs/research_01511/BUSTER_SHOT_PRESENTATION.md) ·
[Results](docs/validation_01511/RESULTS.md) · [Resume](RESUME_01511.md).

---

# Mega Man Legends 3DS — 0.0.15.10 (original lock-on)

0.0.15.10: the gameplay camera now also produces its original `0x1F800008`
matrix (`0x80037FB4`). It matches the unchanged R3000 code on 640 states. The
original lock-on `0x80034EB8` uses it, so a Y shot is aimed at the tutorial
enemy when it is ahead of Mega Man. It also fixes the camera's `rsin`/`rcos`
table. Not yet checked in Azahar.
[Results](docs/validation_01510/RESULTS.md) · [Resume](RESUME_01510.md).

---

# Mega Man Legends 3DS — 0.0.15.9 (original gameplay camera)

0.0.15.9: the 3DS gameplay view now uses the original follow camera
(`0x80038894`). It turns toward Mega Man at the original rate instead of
snapping, backs out while he runs and eases back in. Host 150/150 and the native
build pass; not yet checked in Azahar.
[Results](docs/validation_01509/RESULTS.md) · [Resume](RESUME_01509.md).

---

# Mega Man Legends 3DS — 0.0.15.8 (M2 door terrain patch)

0.0.15.8: the 3DS renderer now follows door IDX swaps without rebuilding the
area (the swapped cell's baked terrain is hidden and its live descriptor drawn
in a small overlay). The data shows the ST00 doors' open and closed
descriptors draw the same geometry, so the doors still look closed; the door
opening must come from the door object's own draw, which is not ported.
[Research](docs/research_01508/DOOR_TERRAIN_DESCRIPTORS.md) ·
[Results](docs/validation_01508/RESULTS.md) · [Resume](RESUME_01508.md).

0.0.15.7: the original door sequence (event 0, `0x800485E4`) now runs. After A
at the first Area 1 door, the door opens, Mega Man walks through on his own,
the game loads Area 2, its door opens and he walks through it too, then control
returns. The door leaf is not shown yet, so he passes through the visible
slab. Host replay passes; Azahar check pending.
[Results](docs/validation_01507/RESULTS.md) · [Resume](RESUME_01507.md).

0.0.15.6: pressing A (the original Circle) while facing the first Area 1 door
now takes Mega Man to Area 2, using the original door event `0x80050C2C`. He
arrives where the original places him, (−256,−2,6208). The door animation,
walk-in and fades are not ported yet, so he stops in front of the closed Area 2
door. Host replay passes; Azahar check pending.
[Results](docs/validation_01506/RESULTS.md) · [Resume](RESUME_01506.md).

0.0.15.5: the 3DS boot package now applies the original ST00 startup map
initialization. Without it the floor in front of the first Area 1 door was
missing, and Mega Man sank through the door and fell out of the world (Oct 19
Azahar playtest). Host replay of the recorded input now passes; interactive
Azahar re-check pending. [Results](docs/validation_01505/RESULTS.md) ·
[Resume](RESUME_01505.md).

0.0.15.4: the enhanced build now boots at the original Area 1 handoff
(2822,-1,-4833) instead of the Area 0 inspection camera; confirmed in Azahar.
[Results](docs/validation_01504/RESULTS.md) · [Resume](RESUME_01504.md).

Ports the original deep-overlap recovery `0x80050400` and the `0x8004B6AC`
correction history. Both are verified against the original R3000 instructions
and cross-checked with Ghidra 12.1.4. When Mega Man is embedded in geometry, he
now moves out to the nearest face under the original rules instead of holding
his pre-move pose. At corners, the previous push is undone as the original
does, so he slides along the free axis. Diagonal walls, Area 1 containment and
source combat are unchanged. The host suite passes 147/147 and the Old 3DS
build passes. Azahar shows boot only; no gameplay collision was checked there.
Details: [docs/validation_01503/RESULTS.md](docs/validation_01503/RESULTS.md) ·
[research](docs/research_01503/DEEP_OVERLAP_80050400.md) ·
Resume: [RESUME_01503.md](RESUME_01503.md). `PHYSICS_1TO1=false`.

---

# Mega Man Legends 3DS — 0.0.15.2 (M2b diagonal walls)

Default boot now starts where the original first gives Mega Man to the player
(Area 1 handoff). Source-backed Buster/hit/defeat is the default combat on the
enhanced renderer. The Area 0/1 map-edge escape remains fixed. Original
diagonal wall classifiers and response paths for families `0x21`, `0x31`,
`0x41`, and `0x51` now drive native contact and sliding. Executable-derived
vectors match for all four families; the owner-local Area 1 audit records zero
unsupported frames, escapes, or falls across 600,000 frames.
Build on the Mac with `tools/m1_mac_build.sh`; test with
[docs/validation_01502/AZAHAR_TEST.md](docs/validation_01502/AZAHAR_TEST.md).
Resume: [RESUME_01502.md](RESUME_01502.md). `PHYSICS_1TO1=false`.

---

# Mega Man Legends 3DS — 0.0.14.51

Cumulative native Old 3DS/XL technical prototype. The imported class-20/03
enemy now completes a bounded source-backed defeat transaction after Buster
damage reaches zero: reaction timing, effect `0x80/0x41`, sound `0xA1`, actor
removal, proxy cleanup, and one exact reward handoff.

The L3CB version-4 owner-local profile retains the source reward descriptor. A
regression drives two eight-damage packets through 16→8→0 HP and proves cleanup,
effect, sound and reward publication happen once without heap allocation.
Missing source evidence or queue capacity holds before partial publication.

The exact RNG-driven pickup expansion, pickup collection/progression and full
defeat-effect visual callback remain incomplete. The host suite (144/144),
owner-local package/replay pipeline and actual ARMv6K build pass. No 0.0.14.51
emulator or physical-hardware result is claimed. WCA is not finished.
`PHYSICS_1TO1=false`.

[Report](docs/validation_01451/FINAL_REPORT.md) · [Tests](docs/validation_01451/TEST_COMMANDS.md) · [Readiness](state/CURRENT_READINESS.json) · [Resume](RESUME_01451.md)

Source releases contain no retail payloads, decompiler exports, DSP, or saves.
Owner-derived cooked data remains local and is excluded from distributable ZIPs.
