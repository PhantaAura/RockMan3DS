# Source reaction actions7/8

Status: SOURCE_BACKED_PARTIAL. The source class20/subtype03 hit consumer already
subtracts HP and selects reaction actions7/8. This pass implements those two
previously unsupported action consumers and their native motion/wall preparation.
It does not establish a complete Buster hit, enemy defeat, or live encounter.

The immutable owner EXE is SHA256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`;
the ST00 overlay is
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.

Source dispatch8010267C..8010269C selects action7 entry80103654 and action8
entry8010388C. These functions are distinct from the HP consumer801041B0.
Their phases0..2 narrow motion arguments through8002FEA4, accumulate fixed work,
convert raw coordinates with80030150, and call8004B414. Neither action calls
a separate floor helper. Work370 retains the complete wall result. Mask0F00
clears forward velocity; mask1000 queues sound222, increments byte0B by two,
and either repeats the source reaction pair or enters recovery phase3.

Action7 phase0 installs animation7, forward velocity-16, zero work330, and the
first timer from8011BEE0. Action8 installs animation12 and retains a table owner
at work364. Its ordinary branch uses8011BEEC and velocity16. Parameter40 uses
8011BEF8, velocity8, random work32A=7, and the exact selected statistics record
byte0B. Missing or mismatched record ownership holds the native transaction.
Both clear flag8 initially and clear flag4 when signed HP is nonpositive.

The timer decrement uses LHU and the original32bit difference before shift and
argument narrowing. Native motion uses the low16bits of the original forward
and vertical arguments; velocity46 retains the original low16bit shift by3.
This preserves negative/large timer cases without floating point or a new
gravity constant. The bounded table consumer admits byte0B indexes0..9;
larger indexes hold rather than reading outside the retained12byte tables.

After the fifth ordinary even-indexed landing, positive HP enters phase3 with
animation17 and timer18; nonpositive HP sets flag1. Phase3 decrements signed
timer work and either sets flags9 on expiry or turns with original80031BEC
limit128. The source heading helper input remains explicit.

`tools/cook_reaction_profile.py` creates a NEW owner-local100byte
`actor_reactions.l3r7` sidecar with three12byte tables. Its loader checks exact
header, CRC, and ST00 identity. No timer table values are embedded in source.

## Verification

The original instruction probe runs6,556 authored table/motion/wall cases:
717,682 original instructions,2,016 source sound222 cases, and336 transitions
into recovery. All24 observed action fields and sound pointed words match.
The independent motion/wall probe runs648 authored geometry cases and529,301
original instructions. Native preparation admits608, holds40 unsupported
geometry cases, and matches all608 complete wall results/work fields;36
admitted cases receive nonzero correction. GTE deltas are explicit authored
boundaries. Original argument narrowing, fixed accumulation, raw conversion,
turn helper, wall wrapper and positional audio writer execute.

The native test also checks missing profile/HP/record/table/heading/audio,
stale fourth raw lane and velocity receipts, invalid table indexes, metadata
corruption, reducer connectivity, and a1024iteration allocation-free recovery
loop. Private fixtures remain outside source. Derived evidence and host results
are in `docs/validation_01432/reaction-*`.

## Remaining input owners

The live actor bridge now stages its shared proxy target alias(packet33C,
flags340,point344,word34A) after exact pool identity and pending-packet conflict
checks. `tick_imported_actor_state1` performs this staging automatically for
bound owners. Its separate pose/proxy tail returns NeedsBodyTransaction while
either native alias has a pending packet, including an incoming packet not yet
staged into behavior. A held body can no longer lose its packet through that tail.

`prepare_imported_actor_reaction_contact` consumes the real hit fields on a
read-only provisional state before preparing source-selected action7/8 motion
and heading. Its positional audio prerequisite comes from the actual initialized
queue. Relative origin8011BF08 remains explicit when word34A bit0 is clear;
absolute-origin axis queries need no invented angle table. A zero-packet valid
reaction uses the receipt in the actor body/pose/audio transaction and retains
velocity44, wall state74 and table364. Stale receipts hold all body work.

Nonzero packet body commits remain held until the complete outer callback,
effect and packet-clear owners are admitted. The new adapter stages and
preserves those pending hits; it does not bypass missing consumers or write HP
as a substitute for that final transaction. Reaction profile ownership,
matching byte0B for parameter40, source angle and actual animation/audio remain
required on the source branches that use them.

The source defeat tail801027CC..80102848 also allocates effect128/parameter65,
marks class20 release through80031824, calls reward consumer80047228, and queues
sound161 at pose point180. The reward consumer selects a statistics record word0
and dispatches80046B34. Those reward/effect consumers and live pool release
ownership remain separate holds. The new reactions do not bypass them.
