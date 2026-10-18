# Integrated roadmap sweep WIP

This checkpoint intentionally replaces micro-release development with a single integrated sweep. It keeps evidence-backed retail behavior and private owner-side game data separate.

## Whole-game structural coverage

The exact USA extraction passes the reference verifier. One owner-side command now generates stage map/render packages, all supported EBD/message resources, high-confidence stage object tables, audio source identity metadata, and the exact player trig reference table.

Native probes validate the generated formats rather than trusting Python output alone:

- world: 37/37 map/render packages; 203 areas; 135572 cells; 3891 descriptors; 10569 contact primitives; 9999 render records; 16478 streams; 104789 render primitives;
- contact dispatch: 10569/10569 primitives use proved dispatcher families 0..7;
- scripts: 136 EBD / 4620 rows; 129 MSG / 17594 blocks / 721779 tokens; 739373 cursor events; 1475 AUDIO; 57931 script-bearing events;
- objects: 34 strong tables / 207 lists / 649 raw records; 459 records fit the generic original pool geometry during the neutral probe;
- audio identity: 38 XA + 1 STR, all 2048-byte aligned, native catalog validates, routing deliberately unproved;
- tutorial: five runtime areas load from cooked flow+EBD, owner-side pack is 91 files, integrated session maps and moves in all five areas.

## Exact executable-backed additions

The sweep extends the portable player/contact compatibility layer directly from the exact USA `ROCK_NEO.EXE`, including the original profile-derived directional masks, ordinary movement setup, fixed-point position retention, owner-side 4096-entry trig table, retail contact-list selection, contact dispatch family extraction, and the shared family-0/1 contact geometry classifier at 0x8004E304.

The collision classifier is intentionally neutral: it returns the original small classifier code and does not invent floor/wall meanings before the downstream response code is recovered.

## Integrated runtime state

`RuntimeSession` now holds the portable integration seam for movement, map/contact lookup, cross-location transitions, story flags, checkpoint/save state, dialogue/audio/script request queues, camera ownership, provisional combat scaffolding and retail lower-screen presentation. Provisional combat constants remain explicitly tunable and are not parity claims.

## Remaining blockers

No claim of tutorial-complete 0.1 or playable 0.3 is made. Complete contact response and player state transitions, exact Buster/lock-on ownership, per-frame EBD behaviors, encounter AI, cutscene/camera semantics, XA routing/playback and the 3DS backend remain required.
