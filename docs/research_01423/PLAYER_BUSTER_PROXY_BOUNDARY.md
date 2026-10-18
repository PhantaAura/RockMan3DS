# Player, targeting and projectile prerequisites after the root pose bridge

Status: **SOURCE_BACKED_PARTIAL RESEARCH; NOT IMPLEMENTED**. These are original
read/call/write boundaries from the verified owner EXE. No source payload or
instruction listings are retained. Function names below describe bounded
observed behavior; they do not establish complete Buster semantics.

`8003450C` calls80034EB8, then800358F0 with a0=800B51B0, before resetting
registration count and executing the already translated pair loop. The existing
pair proof returns from those two calls explicitly. That boundary remains.

## 80034EB8 screen selection and firing calls

The function first calls80035508. That helper loads the matrix/translation at
scratch1F800008 into GTE control registers0..7, scans the high24 registration
mask80098798 and leased owner pointers800BE478, projects each owner+BC through
RTPS, then clears the owner's two pointed words at+BC/+C0. This is additional
lease and GTE ownership, not the kind0 paired overlap predicate.

The selection body reads:

- Player800B51B0 byte00 activation and raw XYZ800B51C4.
- Signed state byte800B5320 and conditional signed byte800B56E8.
- Source mode-admission bytes GP+049E/049F (GP=80097864).
- Proxy count800988E0 and records800A2F48; composite kind fields can consume the
  next proxy record and advance the loop.
- Pointed target+4 bit80, which rejects a target for this selection.
- Scratch camera matrix/translation1F800008 and resident GTE projection state.
- Owner-local signed selection bounds beginning8008994C, selected in12B rows.
- Player byte800B5324 flags20/10 to request respective80035724 calls.

RTPS outputs SZ3 and SXY2 drive depth/window selection. The first projection
without a valid original camera matrix and GTE control state is an actual
prerequisite gap. Float native camera projection cannot silently substitute for
the original GTE divide, saturation, screen bounds and depth comparison.

`80035724` allocates through80031598, optionally leases target identity through
8003545C, then consumes player work170/171, work3C8 XYZ or3E0 XYZ, owner-local
table byte80089A8B in12B rows, halfword4FA in8B rows, work01 and heading56.
It writes the allocated work02/03/0C/0D/14..24/1A/64 fields and may decrement
the selected halfword4FA. Thus allocation, lease lifecycle, muzzle-point pose
production, table selection and subsequent callback dispatch must be recovered
before describing it as a complete projectile launch.

## 800358F0 proximity and target ownership

This body takes player work800B51B0, calls80036D40 for four20B entries starting
8009A300, copies player pointed8B+14, reads GP+049E/049F and traverses admitted
proxies. It rejects target+4 bit80 and class identity>=128, computes supported
single-record or composite midpoints, performs signed XYZ bounds±2000 and
squared distance<=4,000,000, then uses target flags & (player byte01+1) to select
between80036C50 bookkeeping and nearest target selection. GP+0D00/+0D04 track
identity/countdown;80036294 resolves the chosen identity.

The four-entry80036D40 consumer additionally requires identity resolution,
camera eye800A39F0 XYZ, player heading800B5206 and helper8003081C visibility;
later state branches have further helpers. Its point/range and ownership inputs
are not present in the current neutral paired proxy bank. The function itself
is not simply a GTE-based damage writer.

## Actual later proxy writer candidates

The original functions at800441CC and800476A4 contain later registrations at
80044400 and80047784, after indirect callback dispatch. They consume callback
work38 XYZ, signed halfword3E extent, work01 identity and byte03 branches.
Both set target flags via extra128 and use fixed global packet targets
800986C0/800986D8, respectively. They can choose modeA000/C000 or4000 according
to distinct source branches.800441CC also reads signed byte0D to select an
owner-local byte at80097DDC and includes branch-specific high packet words;
800476A4 uses distinct producer words (the observed ordinary registration word
is10000008, special branch30000000).

These are candidate follow-on projectile/attack owners with exact source
prerequisites. Their dispatch-table references8008A20C/8008A70C, allocation
path, selected byte provenance, full scalar callbacks, motion/contact and
retirement need independent original-instruction differentials. The values do
not justify replacing the desktop fallback projectile with guessed source
damage or pretending the authored mode1 chain is retail Buster combat.

The root collision point and class03 tail are reusable completed bounded
pieces. Full player firing, high-mask leases, targeting/GTE, reaction motion
and defeat remain pending. No live retail parity or complete combat claim is
made by this research.
