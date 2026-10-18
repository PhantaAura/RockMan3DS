# Original player proxy lease prerequisite

Status: **SOURCE_BACKED_PARTIAL**. This module removes a bounded prerequisite of
`80034EB8`; it does not implement target selection, lock-on or Buster spawning.

The complete leaf function `80035508` loads the five packed matrix words and
three signed translations from `1F800008` into GTE controls 0..7. It masks out the
low byte of `80098798`, walks the remaining 24 bits from most significant to
least significant, resolves corresponding pointers from `800BE478`, and loads
the owner's two words atBC/C0 intoVXY0/VZ0. The original instruction is
RTPS(sf1, lm0). It clears both owner words after the projection, readsSZ3,
arithmetic-shifts it by 2 into local stack storage and storesSXY2 to another
local stack word. Those local writes are not a persistent enemy target, damage
packet or lock-on request.

`source_player_consume_leases_80035508()` reproduces the camera transfer,
iteration order, GTE register effects and owner clearing. Repeated source
owner pointers see the earlier clear on later iterations. The adapter stages
all operations and retains every owner and GTE field on an unavailable premise;
this transaction policy bounds failure safely without inventing source data.
The camera transfer still occurs when no high mask bit is set. Projection
controls are required only if an actual RTPS is reached.

`SourceGteRtpsState` separately admits original matrix/translation and resident
projection controlsOFX/OFY/H/DQA/DQB. A captured retail GTE state must be bound
at the original consumer boundary. A floating-point renderer camera is not an
acceptable substitute. The default state admits neither owner.

The restricted arithmetic implementation uses signed fixed-point operations,
source register narrowing, FIFO order, saturation and the hardware UNR division
mechanism. It holds any intermediate outside the signed 44-bit MAC domain,
instead of guessing overflow wrapping. It supports only the reachedRTPS
command and does not add a general PS1 CPU/GTE emulator to the native engine.
The hardware arithmetic reference is the
[PSX specifications GTE documentation](https://psx-spx.consoledev.net/geometrytransformationenginegte/).
The hardware reciprocal scalar is generated from the documented arithmetic;
no game reciprocal table or source payload is compiled into the program.

The host test checks original unchanged CPU instructions with authored owners,
matrices and projection controls. Its GTE instruction adapter follows the
published hardware arithmetic; therefore this is not an independent hardware
GTE differential or retail parity. Live original-emulator before/after data,
when captured, must be reported separately. The original helper is not hooked
or replaced, and no instruction listing or commercial payload is exported.

The native test additionally checks unknown matrix/mask/owner/projection holds,
atomic failure after a valid earlier owner, repeated-pointer clear order,
near-plane divide overflow, UNR clipping without divide overflow, unused GTE
register preservation and 4096 allocation-free ticks with 24 active owners.

`tools/compare_player_lease_reference.py` independently admits the read-only
Redux observer's owner hash/function, pairs exact entry/return calls, verifies
mask/table stability and physical alias consistency, then invokes
`test_source_player_proxy --reference` against private normalized scalars.
The native checker compares every raw CP2C/CP2D field and every selected owner
word. It reports all divergent fields. A guard failure, missing provenance,
pending call or zero completed calls remains UNKNOWN. Its fixture is kept out
of the source release; authored fixtures are not relabeled as emulator traces.

Pending downstream owners remain `80034EB8` target/window selection,
`80035724` allocator/leases/pose muzzle production, subsequent projectile
callbacks and final retail Buster damage/reaction chronology. Completing this
leaf function must not promote any of those readiness entries to 1:1.

An independent original Redux snapshot contains 323 complete leaf calls. All 64
raw register fields match the native camera-transfer path. None sets a high
lease bit, so these observations do not verify RTPS arithmetic or a Buster
lease. Raw CP2D storage for register 15 remains unchanged; the architectural
CPU read is exposed separately and mirrors register 14. The source checker
retains this backing field and compares it explicitly.
