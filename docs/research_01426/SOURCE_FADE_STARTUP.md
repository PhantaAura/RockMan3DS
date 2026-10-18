# Event1 phase4 fade startup

This bounded consumer owns only original80017C30 with argument2. It does not own
fade rendering, callback advancement, BIOS threads or scene completion. The
source EXE identity is
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`;
the ST00 program identity is
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.

Original80101184 decrements work+0C, stores that count, and when it reaches zero
increments work+04 before entering80017C30. The helper return has no completion
predicate. Native code publishes count0/phase5 only after the startup has either
performed its exact busy no-op or written its constructor state and retained a
bounded, explicitly unconsumed scheduler intent. Missing owners leave work,
fade context and intent storage unchanged.

The helper reads signed halfword80098A72 (work80098A68+0A). Any nonzero value
returns without modifying the context or packets. With zero busy, argument2
writes mode+00=0, duration+0A from owner table80097A44, and step+0C from80097A4C.
It preserves all other context bytes. Two16-byte TILE packets at80098A28 and
80098A38 receive tag length3, command62, origin0, and extent320x240. Their links
and RGB bytes remain retained. Two12-byte DRAW_MODE packets at80098A10 and
80098A1C receive length2, E1000140 and word0; their links remain retained. These
packet constants come from the original constructor operations, whereas the
two table scalars are supplied through the owner-local L3FDv1 profile.

GetTPage8007C478 and SetDrawMode8007DF38 execute unchanged in the probe. The
GPU-type branch in8007E438 produces the same type2 draw word for all256 authored
GPU-type bytes. It therefore adds no native prerequisite for this slice.

The next original call is80012E10(slot2,callback80017E94). Its BIOS OpenThread
depends on SP at801F8210 and GP at801F8244, produces the handle at801F8208, and
marks801F8200 active. None of these thread owners is admitted by this module.
The native fixed-capacity queue records the exact pending call and supplies no
thread handle, completion receipt or automatic timer. Original callback
80017E94 and its rendering/yield loop remain excluded. In particular, a pending
intent must not be treated as fade completion or as presentation parity.

The existing source executable boot premise proves busy0. Reset8001D928 does
not clear it. A native area transition may clear its own pending intent storage,
but cannot infer a fresh original fade busy value from that reset. Runtime
bindings must retain or invalidate fade state according to its explicit owner.

L3FDv1 is68 bytes: magic/version/header64, decoder1, reserved0, record count1,
total68, payload CRC, record bytes4, and the32-byte source EXE identity. Its
payload contains only the two selected little-endian halfwords. The cooker
uses the existing OwnerExe identity/bounds guard and creates a new private
directory. Owner-derived profile bytes remain excluded from source releases.

`source_fade_startup_probe.py` executes517 authored cases through unchanged
ST00 phase4 and original EXE helper instructions, with zero code patches or
helper hooks. Each case proves phase5/count0 at helper entry;512 stop before
80012E10 and five nonzero-busy cases return normally. The fixture contains only
authored input scalars and resulting authored packets. The native always-on
test compares packet bytes and validity, checks malformed profile and missing
owner rollback, bounded queue overflow, exact pending arguments, and4096
allocation-free updates. This is source-backed partial behavior, not a live
retail trace or whole fade parity.
