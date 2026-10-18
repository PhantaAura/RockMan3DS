# Correct child dispatch owner and remaining attack boundary

Status: `SOURCE_BACKED_PARTIAL` for dispatch ownership; complete native child
attack remains `UNKNOWN`.

The inherited01420 Action4 child report incorrectly named resident
`80089820` entries6/9 as the child callbacks. Those entries point to unrelated
message/statistics helpers. The source pool updater `80017210` actually uses
that resident table only when subtype≥128. For subtype<128,
`8001727C..80017294` loads the callback-table pointer from `1F800060`.

Executing the original ST00 initializer `80100324` proves that `1F800060`
receives `8011A5D4`. Its subtype6 entry is `8010F354`; subtype9 is null in this
stage. The independent updater proof follows the original indirect dispatch,
using explicitly authored return-only hooks. A null subtype9 does not become a
made-up fallback callback. These corrected anchors must replace the old
research-only claim wherever used as implementation guidance.

The actual subtype6 callback dispatches byte8 through the stage table
`8011C84C`: states0..4 resolve to `8010F47C`, `8010F674`, `8010F8E8`,
`8010F9F4`, `8010FC88`. It also maintains byte0F, submits proxy information
through800341BC/80051BAC and invokes8002C8CC. Thus an initializer alone is
insufficient to call the child a complete projectile or attack.

Its state0 derives velocities from original sine/cosine helpers and fields
9C/A0; it sets source primitive/material words and uses owner UV references.
It retains previous fixed/raw positions, calls its wall/contact wrapper
8010FDD4, and changes byte8 according to that return. Those owner UV tables
must be retained/imported accurately before child presentation is admitted.

The state1 body checks source work9C masks, may reorient from original player
heading, calls a separate motion helper80110070, restores raw mirrors, invokes
8010FDD4 and advances byte0C. Its threshold150 changes lifecycle state. Contact
and hit/proxy consumers remain required; this is not permission to invent a
150-frame projectile lifetime and ignore the other states.

The collision wrapper8010FDD4 calls8004B414 and owns velocity responses,
positional cue179, effect allocation80031618, reflection8004AC8C and additional
branches under raw stage inputs. States2 and later use source release800318D8
and effect producers. Byte9 subtype absence, hit packet/proxy ownership,
resource/primitive transfer and cleanup timing must remain separate evidence
gates. No native damage/render consumer is fabricated here.

The bounded producer/pool modules added this pass enable later wiring without
reconstructing a different enemy attack. Native Action4 stops at the explicit
child-consumer boundary until these actual consumers are proved and connected.
