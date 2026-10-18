# Next detector-selected action boundary

Status: **SOURCE_BACKED_PARTIAL**; research only. No native action9 reducer was
added in this checkpoint. Unknown RNG ownership and hit/attack consumers remain
independent gates. All owner executable/overlay payloads stay outside source.

The original state1 indirect dispatch at80102698 selects action9 entry80103B58
and action4 entry80102FC0. The action9 body ends before80103DF8. It has no
direct RNG or hit-consumer call; the outer callback still consumes its BIOS
sample unconditionally, and source common-tail transitions retain their own
random dependencies.

Every action9 invocation calls80031198 with player raw position800B51C4 first
and actor work+14 second. It subtracts unsigned global halfword800B5206 and
stores the low16 difference at work36E, including inactive phases. This angle
getter argument order is distinct from the later turn helper's actor/player
order. The player-heading global must have explicit source-equivalent ownership;
a generic camera heading or default zero is not justified.

Phase0 stores phase1, animation request1, timer32C=90 and work330=0, then enters
the same body as phase1. For these active phases, unsigned work80 greater than
00480000 together with flags328 mask10 chooses action3 and clears the complete
halfword0A/0B. This route has no motion, wall, floor, cue, allocator or direct
RNG call. It is the smallest useful next reducer, once target-angle/player-heading
dependencies are independently admitted. Other phases retain only work36E.

The remaining active route executes original turn80031BEC with limit32 and
stores its signed result at work36C. Heading56 adds that result modulo4096.
It calls8002FEA4(work,52,0,0), then8004B414(work), stores contact at work370,
conditionally calls sound176 when signed cueA6 equals1, and calls80048E6C(work).
Sound's second argument is actor raw position work+14, supplied in the original
delay slot. Motion8002FEA4 must be proven as its own producer; existing prepared
action2/3 data cannot silently stand in for it.

After floor stores, an absolute raw-Y change of33 or greater restores all four
previous raw/fixed lanes, then chooses action1 with complete0A/0B clear and
restore0E=4. Contact maskF00 chooses that action/phase/restore route without the
full position rollback. A zero timer35E plus work80 at most00011FFF chooses
action10 and clears flags328 mask01. Otherwise a zero timer35C, work80 at most
0011FFFF, absolute signed turn36C below16 and
`((work36E+1024)&4095)<2048` choose action4 with complete0A/0B clear. With no
such route, absent detection mask10 sets common-transition mask01. These
conditions remain partial source arithmetic; no animation semantic names or
retail gameplay parity are inferred.

Action4 is relevant because action9 can select it. Its phase0 chooses request21;
the original phase2 expiry increments that request to22 and calls overlay
helper80104078. Its lifecycle/consumer behavior is not admitted by this bounded
investigation. Other action4 phases call the same motion/wall/floor family;
porting a child attack or damage delivery requires separate evidence.

`class20_03_action9_source_probe.py` reads the exact supported owner EXE and
overlay and writes a NEW owner-local scalar report. It covers distance/flag
and inactive-phase boundaries, unsigned heading differences, floor32/33,
contact masks, cue equality and timer branches. Original turn instructions
execute; angle, zero-motion, wall, floor and sound are declared authored hooks.
No original instruction bytes, listings, tables or assets are serialized.

```sh
python3 -B docs/research_01419/class20_03_action9_source_probe.py \
  --exe /path/to/ROCK_NEO.EXE --dat /path/to/CDDATA/DAT \
  --out /new/owner-local/action9-evidence
```

Next admissible API should preserve atomic staging and optional target angle,
unsigned player heading, work80 and phase0/1 ownership. A missing dependency
must hold before changing work36E, animation or phase. Start with the source
non-motion transition route, then add an independently proved preparation API
for8002FEA4/wall/floor/cue. No default RNG seed or speculative hit consumer is
needed to describe this boundary.
