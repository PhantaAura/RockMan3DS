# Class40 original local states and request banks

Evidence: `source-scene-actor40-state-proof.json`. These are the original
ST00 class40/subtype5 callbacks selected by table `8011C4DC`, through
`8010D7A4` and ending at `8010D800`, before the source shadow producer. This restores the state
handlers that 01435 omitted after the initial pose. Request identities come
from original call sites; animation meaning is not inferred from their order.

| State09 | Request IDs consumed by its original handler |
|---|---|
| 0 | 0 |
| 1 | 16, 17, 18, 19, 20 |
| 2 | 21 |
| 3 | 22 |
| 4 | 23, 24 |
| 5 | 25 |
| 6 | 48, 34 |
| 7 | 26 |
| 8 | 27 |
| 9 | 28 |
| 10 | 29 |
| 11 | 30 |
| 12 | 31, 32 |

The shared reset is the work-owned `800322A8` transaction, selecting an
explicit resident request/record rather than assuming request0. Sequence
advancement reuses the exact scalar `80032488` record controls. The transform
producer keeps its integer/GTE rounding and hierarchy rules; original state4
changes to mesh-index lanes do not change transform hierarchy. Native 01436 prepares separately owned geometry for alternate mesh indices
20/21 through a load-time variant.

The original fixed-position producer `80030150` narrows `(raw>>16)+bit15`,
including negative words and low-half rounding. State6 preserves the source
frame38 request transition and 47 additions to fixedX, including the final
addition after the countdown reaches zero. State9 preserves the exceptional
frame32 branch that restores its pre-add X word. The native routine performs
no allocations or owner-data decoding in these updates.

Original pointer globals are read through the event owner at offsets28,30,60,
and64. Peer e0 actor stores require occupied source bitmap/header identities.
The state4 family80 destruction uses the original deferred-release OR/AND
mask and clears active00; allocation bits survive until the cleanup owner.
Missing pointer, header, bitmap, or deferred-release ownership holds the
entire transaction. State2/state12 emit original audio request144/145 receipts;
playback delivery remains the audio owner's responsibility.

Verification executed 1,216,398 unchanged original instructions: 59 authored
branch cases across states0..12 and 86 consecutive state6 calls. All 145 native
results matched every known work byte and all external actor/pending-release
bytes, with 1,018 atomic guards and zero hot allocations. The private TSV
contains original actor/resource-derived work snapshots and is excluded from
source releases. No original instruction or helper was patched/replaced.

This is `SOURCE_BACKED_PARTIAL`: synthetic consecutive calls do not establish
retail frame scheduling. Final player halfword mirroring is now admitted through the separate guarded
tail proof. `8002C8CC` shadow output, `8010C820` timed UV cues, native audio
delivery, and physical Old 3DS behavior remain independent gates. These results are not retail
parity or complete tutorial progression.
