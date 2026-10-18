# Tutorial class20/03 source evidence

**SOURCE_BACKED_PARTIAL.** Class20/03 has a source path for spatial spawning,
player-directed movement, health/damage, and an attack child. No retail enemy
name, completed encounter, complete AI, or combat timing parity is assigned.

The five ordinary ST00 object lists contain no class20/03 records. The two
inherited class20/35 objects remain the separately proved neutral actor profile.
The missing source records are in 24-byte conditional lists reached by E0/00.

Area1 C0/21 at ordinary list index5 calls `0x800317AC` with slot31 from callback
`0x80119BE8`, creating E0/subtype0. Its callback `0x801116DC` selects state0
`0x80111718`, state1 `0x80111810`, and state2 `0x80111CD0`. State0 supplies the
area's 12-byte spatial descriptors. State1 selects a descriptor when its signed
qualifier differs from work+1C and the player's signed X/Z grid coordinates fall
inside the descriptor rectangle. State2 runs on the following tick and creates
its conditional records. Descriptor `0x8011A40C` has qualifier1, id2, X grid
[-3,1] and Z grid[-16,-15], and points to list `0x8011A274`.

That list contains class20/03 records at `0x8011A274`, `0x8011A28C`, and
`0x8011A2A4`, with parameters0/128/128. Record predicates `0x63F..0x641` call
`0x8001DA58`; zero admits allocation. The known low predicates use MSB-first
bits from `0x800BE378`. The creator commits qualifier to work+1C, descriptor id
to work+1D, and returns state+8 to1. The event agent owns catalog retention and
spatial gate integration; this evidence does not invent a wave order.

The actor dispatch is `0x8011A4DC →0x8011A480 →0x8010226C`; state0 entry
`0x8011BE9C` selects initializer `0x80102410`. At `0x80102444` it calls
`0x8002F9C4` with a null variant pointer, selecting the first class/subtype row.
ST00_01/02 row0 is class20/subtype03/variant0 with resource offsets
`0x314`, `0x14A4`, `0x364C`. It uses the previously decoded class20 descriptor,
mesh packet, vertex, and packed-pose consumers. The initializer sets Q12 scale
512 on all axes and calls pose-request helper `0x800322A8` with request0.

The native profile admits only parameters0/128/160 and dispatch byte0. It binds
pose0/record0 automatically in `spawn_imported_actors`; the retail-selection
flag is true for this initializer-selected pose. Shade remains UNKNOWN, with
the existing flat neutral mesh presentation. Body bounds `0x8011BED4` are
installed before the floor call, so the existing zero-bound actor floor subset
returns Unsupported for this profile.

An owner-local native probe of the first source record at (367,-1,-6946), heading
1024, decodes five enabled parts, 89 logical vertices, 60 triangles, 49 quads,
and 3,252 owned element bytes. Pose0/record0 selects frame0 at EBD offset5452.
All three source instances decode and release their actor/resource accounting
on unload. These are static presentation probes with diagnostic placement.

The HP owner `0x80064718` reads raw selector byte `0x800C1B6B`. Selector2
installs stats table `0x8008F060` at `0x80098844` (GP+0xFE0), giving class20/03
stats-index0 HP byte8. Other selector values install `0x8008EDC4`, giving byte4.
The actor initializer stores byte<<4 at work+6E and work+B0: 128 and64
respectively. The initial static branch interpretation was reversed; executing
the bounded source initializer corrected it before handoff. The native helper
`class20_03_initial_hp` requires both raw selector and raw context high byte.
It returns no value for context bit0x40, whose additional arithmetic remains
outside the implemented subset. No global selector defaults or instance HP
are supplied by the actor constructor.

State1 `0x80102600` invokes hit consumer `0x801041B0`. For a nonzero packet
damage field and packet flags masked by `0x38000000`, it subtracts the low24
bits of work+33C from signed16 health at work+6E. Zero/negative health sets
work+328 bit0x40. Five bounded synthetic cases reproduce damage, ignored flags,
and the defeat latch. The consumer retains work+33C, so repeated delivery and
packet lifetime cannot be inferred from this isolated function.

Movement action `0x801029B4` calls `0x80031BEC` with actor position work+14 and
player position `0x800B51C4`. Attack call `0x801030EC →0x80104078` allocates a
class60/subtype06 child through `0x80031518`. Player damage delivery, projectile
collision, animation advancement, the complete defeat path, floor handling,
texture residency and encounter completion remain outside this proof.

The initial C0/03 record is a RGB555 palette timeline. Its storage at
`0x800AD168` must not be executed as commands or treated as progression flags.

Reproduction scripts:

- `research_01415/class20_03_evidence_probe.py`
- `research_01415/tutorial_actor_identity_probe.py`

Evidence: `validation_01415/class20-03-source-evidence.json` contains selected
addresses, source hashes, scalar checks, native counts and placement bounds.
It contains no retail payload, disassembly, original function table or assets.
