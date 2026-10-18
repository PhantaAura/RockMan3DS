# Projection owner initialization

Status: **SOURCE_BACKED_PARTIAL**.

The first scripted camera operation requires unsigned halfword `800BC81C`.
Its original writer is the 40-byte area configuration copy in `800371A0`.
The destination starts at `800BC7F8`; record offset `0x24` therefore reaches
`800BC81C`. The writer copies the last eight bytes through unaligned transfer
instructions at `80037260..8003727C`, explaining why a search for a direct
halfword store missed it.

This field supplies the original fog helper. It is not the native renderer's
FOV, and a generic projection value cannot substitute for its owner.

The ST00 caller `80100528..80100564` selects the configuration at
`8011AC54 + area * 40` and invokes the writer. The supported area IDs are
0 through 4. The earlier fresh caller `80013648..80013660` selects its own
EXE configuration record. These callers establish distinct lifecycle owners;
the earlier record cannot be assumed current after ST00 configuration loads.

`800371A0` copies the complete record to `800BC7F8`, `8009A350`, and
`800C0DC0`. It also reads record `+0x24` directly for `8007A1CC`, passing
denominator 384. The source setter for scripted operation 33, `800369C0`,
loads the retained halfword from `800BC81C`, doubles it only when the result
is below 13107, and invokes the same fog helper. The helper divides the
signed numerator `-320 * selected_input` by 384, truncating toward zero;
the leaf setter narrows DQA through signed 16 bits. DQB receives `01400000`.
The other source shift variants were measured separately.

The runtime must admit this field from a validated owner-local area profile
at the corresponding fresh area initialization. It must retain the field as
unknown when that profile or lifecycle owner is absent. Copying the field is
not evidence of a complete current camera matrix, gameplay camera, or script
completion.

The retained probe executes 80 authored writer cases, including all four
source and destination alignment offsets, followed by 240 source shift
consumers. It also executes all five original ST00 area selections and their
15 shift consumers, plus the earlier fresh writer. The run executes **43,096
unchanged original instructions**, with zero instruction patches and zero
helper hooks. Both the EXE and ST00 overlay are guarded by their exact hashes.

The source release retains the probe and its public hash/case-count proof.
The authored fixture contains artificial records. The separate local owner
proof contains only the selected scalar observations; no original configuration
records, executable instructions, or assets are included in the source release.

These are original instruction comparisons using the inherited CPU/GTE
control-transfer interpreter. They do not establish independent hardware GTE
behavior, live retail timing, renderer acceptance, or physical Old 3DS testing.

Reproduce with a new output directory outside the source tree:

```sh
python3 -B docs/research_01429/projection_source_probe.py \
  --exe /path/to/owner/ROCK_NEO.EXE \
  --st00 /path/to/owner/CDDATA/DAT/ST00.BIN \
  --out /path/to/new/private/projection-proof
```
