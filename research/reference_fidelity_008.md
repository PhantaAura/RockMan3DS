# Tutorial Core 0.0.8 — reference fidelity and player-experience bridge

## Authority order

For parity-critical behavior, Legends3DS now uses this evidence order:

1. **Actual user-owned USA project source** — CHD / extracted disc / `ROCK_NEO.EXE` / stage containers / XA / STR.
2. **Direct executable and data-flow reconstruction** from those bytes.
3. **Existing public reverse-engineering/decomp research** as a cross-check and accelerator.
4. **Video capture** for perceptual timing/presentation comparisons that are not directly encoded in a single data structure.
5. **Project memory/notes** only as navigation hints, never as final parity evidence.

A remembered value must be revalidated before it becomes a gameplay constant.

## Exact reference set verified in this checkpoint

The project Drive extraction contains the expected `ROCK_NEO.EXE`, `SLUS_006.03`, `SYSTEM.CNF`, 205 DAT containers, 38 XA files and one STR file. Critical tutorial-stage, executable, audio and movie files are fingerprinted by `tools/verify_mml_us_reference.py`.

The exact project CHD is 217,298,243 bytes with SHA-256:

`573c1fa958db9a84a5f57b2894bf7f7b71e51f77df70dac1cd34a5a3b66d1f94`

That CHD hash identifies the project's current reference file. It is not claimed as a universal hash for every independently recompressed CHD representing the same disc.

## Cook stability

A fresh indexed tutorial cook from the verified extraction produced 85 files. Comparing every output byte against the previously cooked 0.0.7 pack found **zero SHA-256 differences**. The new QoL/UI/reference tooling therefore does not alter existing cooked game data.

## Direct executable analysis

`tools/psxexe_to_elf.py` creates a local analysis-only ELF wrapper around the PS-X EXE text payload. The exact project executable reports:

- entry: `0x80068000`
- text load address: `0x80010000`
- text payload size: `0x000C9000` (823,296 bytes)

`llvm-objdump` on that wrapper independently reproduces the known firing-gate helpers:

- `0x800423A0` — clear shot-enable state and copy compatibility bytes.
- `0x800423C8` — set the enable bit.

The generated ELF contains copyrighted executable bytes and must remain local.

## Bottom-screen design rule

The lower screen is a **player interface**, not a parity/debug console.

Retail builds may present area/status, HP, Buster state, lock-on state, objective reminders, dialogue history, menu/save/loading affordances and optional touch shortcuts. Debug-only data such as source object index, EBD candidate, coordinates, trigger selector, collision cells, frame time or memory usage belongs in a separate developer build/overlay and is never part of the normal bottom-screen command model.

During original cutscenes the bottom screen should become visually quiet. This avoids turning a 1:1 cinematic into a dual-screen redesign and keeps the top-screen composition authoritative.

## QoL containment rule

Classic is the parity baseline. Enhanced options may change ergonomics/presentation, but must not mutate original progression, enemy behavior, damage, timing gates or script decisions. When a QoL can affect timing (for example faster text), the engine must preserve the original event/synchronization semantics rather than simply skipping arbitrary frames.
