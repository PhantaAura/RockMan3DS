# Bounded original-game contact reference

The owner-supplied USA disc ran in the installed official PCSX-Redux
`f78f8ef8` interpreter with bundled OpenBIOS `0b0359a7`. The user reached and
controlled original tutorial gameplay. These are original PS1 emulator
measurements, not native 3DS execution, PS1 hardware proof or retail parity.
Both memory-card slots are disabled in an isolated portable session. Owner
saves and the installed application were not changed.

## Live tutorial measurements

`../validation_01415/redux-tutorial-live-profile.json` records matching hashes
for three live RAM code ranges against the owner executable: geometry
selected-list admission, floor selected-list admission and the stats selector.
The three ST00 IDX identities also match their source positions/descriptors.
Player work flags byte 0=3, raw slot marker byte 1=0, state bytes 8/9/A=1/0/1,
and XYZ=(930,-1,-10135). Byte 1=0 is a raw slot marker; it must not be used as
an inactive generic-actor class guard.

IDX78, IDX94 and IDX123 have **static raw6=0 but live raw6=1**. Their live
selectors choose the non-null alternate contact lists. This is
`MATCHES_MEASURED_FIELDS` for these records in the captured state. It does not
prove the writer, timing, domain or a universal initialization rule.
`selected-contact-admission-audit.json` retains both the static audit and this
measured difference.

Actual RAM word `0x00000008` was zero and raw stats selector `0x800C1B6B`
was 1. These tutorial measurements extend the retained intro-only profile.
No active class20/subtype03 was present in the latest all-16-slot snapshot,
so live context/HP for that enemy remains unmeasured. The Python snapshot
masks the class/slot marker's top bits; the older running Lua observer
selected raw marker20 only. Its empty actor list is not exhaustive.

The source selector reads descriptor+4 when IDX+6 low12 bits are nonzero,
and descriptor+8 otherwise. Geometry reads selected-pointer+8 at
`0x8004B9EC`, then tests the loaded word at `0x8004B9F4`. Floor reads it at
`0x80049018`, then tests the loaded word at `0x80049020`. Neither audited
block has an explicit selected-pointer NULL guard. RAM8=0 does not justify
native NULL-as-empty admission: the three measured live records select
non-null lists, and a NULL-selected gameplay floor call was not observed.

## Exact blocker and bounded research

The static cooked selector is not yet reconstructed as live per-area state.
Bounded searches of the owner executable and ST00 overlay did not identify
the exact raw6 writer. Halfword+6 stores, aligned word+4 stores, 12-byte
record walks and packed-word flag updates were considered; no candidate was
proved to write these IDX entries. Writer, initialization timing and domain
remain `UNKNOWN`. Native contact code was not changed to toggle all flags,
substitute alternate lists, or ignore missing selected lists.

Next step: during an owner-authorized new-game/area reload, observe debugger
write callbacks over IDX78/94/123 word+4. Addresses `0x8015C3AC`,
`0x8015C46C` and `0x8015C5C8`, width 4, cover raw6 halfword writes and an
aligned word store. Capture the original writer PC and verify the source
instruction, inputs, timing and record domain before implementation. The
[official breakpoint API](https://pcsx-redux.consoledev.net/Lua/breakpoints/)
documents `PCSX.addBreakpoint(address, 'Write', width, cause, callback)`;
retaining returned objects prevents garbage collection. No write callback
was installed in this pass, no RAM was modified and the user's current
session was not restarted.

## Reproduction and evidence

`redux_live_contact_snapshot.py` uses the official read-only RAM endpoint,
checks the exact owner executable and live code hashes, saves only bounded
scalar/hash observations, discards the 2 MiB response and refuses to replace
an earlier snapshot. Example from the source root:

```sh
python3 -B docs/research_01415/redux_live_contact_snapshot.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --api http://127.0.0.1:18081 --out /tmp/new-owner-reference-profile.json
```

`redux_contact_observer.lua` contains bounded scalar reads, frame counters
and isolated emulator pause/resume/quit controls. It has no RAM writes, pad
overrides or save API. The saved future-session copy now handles all class20
slot markers. Pool base is `0x8009F108`, 16 slots, stride `0x3C0`; stats index
is +0x0C, context high byte +0x0F and signed HP +0x6E. Context is not +0x13.

`redux-tutorial-controller-proof.json` preserves source-API button counters:
Start 107 frames, Cross 289 frames at observer vsync 91814, with both released
in that snapshot. The counters establish delivered input without injecting
it. They are observer-session fields, not gameplay trace timing.

The installed app lacked a bundle identifier. A local copy adds only bundle
identifier/display/name metadata; executable SHA-256 remains
`4a1333a3ab2abbd3269352a5fc201175bcba86c32bfaeb04bd5af4de54623523`.
Earlier CUA coordinate input returned `NoWindowsAvailable`. With isolated
Pad0 input Keyboard (enum 2), later CUA keys and manual user input reached
gameplay. The r4 reference remains running under user control.

Local reference root is `work/pass-20261004-worldalive/ps1-contact-reference/`;
app copy is `work/pass-20261004-worldalive/PCSX-Redux Reference.app`.
A genuine original tutorial capture shows the player against a wall with
the PS1 HUD. Capture paths/hashes and historical failures are preserved in
`../validation_01415/redux-contact-reference-result.json`. Screenshot and
scalar reads are separate observations, not a synchronized retail trace.
No capture, raw RAM, retail executable, disc, model, texture, audio or save
is included in this source checkpoint. Old 3DS hardware remains
`HARDWARE_PENDING`.

## Exact keyboard source audit

`../validation_01415/redux-keyboard-source-audit.json` links and hashes the
installed changeset's official pad and GUI sources. Return is PS1 Start;
X is Cross; arrows are D-pad. F5 resumes, F6 pauses and Escape toggles the
emulator menu. Source polls `ImGui::IsKeyDown` without game-viewport-focus or
`WantCaptureKeyboard` gates. Auto prefers a connected controller and
otherwise uses keyboard. Configuration → Controls → Device type → Keyboard
was applied only to the copied session. The read-only
`PCSX.SIO0.slots[1].pads[1].getButton` observer establishes delivered button
state; no pad override endpoint was installed.
