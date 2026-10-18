# Combat mod log — siblingpass-20261011-worldalive-01432

Read the reverse-engineering skill and used only the supplied immutable owner
EXE and ST00 overlay. Frozen01431, archives and sources/ were untouched.

Confirmed actions7/8 dispatch, work offsets, timer shifts, wall result masks,
sound argument positions, recovery turn and profile pointer ownership from the
original instructions. Added clean native reducers, explicit metadata loader
and cooker, and an allocation-free contact preparation adapter. Wired the
bounded reactions into class20_03_step_state1. Every missing prerequisite
retains the existing source work and returns no sound intent.

Added live actor alias staging with exact pool target identity and conflict
validation. Incoming packet/context fields commit together independently of
HP. The actor tick automatically stages bound targets; pending nonzero hits
remain held until the complete source callback owners are available. The
separate pose/proxy tail now rejects any pending packet instead of clearing a
held hit. Native reaction preparation uses the staged post-hit action/heading;
zero-packet valid reaction receipts commit velocity/state74 with pose/audio.
Focused runtime tests cover these paths, stale receipts, unchanged HP/audio
during preparation/holds, wrong identities and conflicting pending packets.

Both private original-instruction differentials pass. Total original
instructions exercised:1,246,983. Native action/control cases:6,556.
Native motion/wall cases:608 admitted/40 held/36 corrected. The native build
uses -Wall -Wextra -Wpedantic -Werror. No retail instructions, assets, timer
values or private fixtures were placed in distributed source.

Combat readiness remains SOURCE_BACKED_PARTIAL for these consumers only.
The Buster→HP→reaction→defeat loop and Old3DS encounter parity remain unproved.
See COMBAT_REACTION.md for concrete missing input owners and source addresses.
