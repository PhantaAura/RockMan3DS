# Class20/subtype03 scalar transactions

Status: **SOURCE_BACKED_PARTIAL**. The native reducer now implements reusable,
atomic transactions for original state1, its common tail, and the outer
callback's source-used scalar stores. It preserves the previous action0/1 API.
This boundary does not establish a retail encounter, BIOS seed, admitted
detection solver, projectile delivery, pose engine, effect engine or actor
defeat lifecycle.

The read-only owner inputs remain the established USA `ROCK_NEO.EXE`, SHA-256
`10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0`, and
ST00 PROGBIN SHA-256
`9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c`.
No original code bytes, instruction listings, profile tables or asset payloads
are included in the implementation, documentation or probe.

`Class20_03CallbackState` adds the fields missing from the narrow action
kernel: auxiliary byte0B, restore byte0E, optional health ownership, packet33C,
detection phase353, common timers and complete raw/fixed/angle snapshot blocks.
Behavior heading56 is authoritative for the second angle lane. The full raw
and fixed blocks retain their fourth lanes even though actions0/1 do not move.

The four entry points have deliberately different boundaries:

| Entry point | Admitted work | Explicit remaining ownership |
| --- | --- | --- |
| `class20_03_initialize_scalar_state` | Known initializer scalar stores, original floor or parameter40 +32Y route | HP/profile may remain unknown; resource and pose consumers excluded |
| `class20_03_step_state1_tail` | Original transition priority, byte0A/0B clear, parameter/flag clear, signed timer decrements | Random sample only if selector reached; health only if action6 transition reached; expiring defeat lifecycle held |
| `class20_03_step_state1` | Detection flag, bounded hit reducer, actions0/1, prepared action2 and common tail | Explicit detection result or proved range rejection, packet/predicate/heading as read; prepared motion/contact/floor/cue and sound consumer readiness |
| `class20_03_step_callback` | Snapshot copies, request comparison, sample store, bounded state1, source consumer intents, final packet clear | One explicit BIOS sample always; animation/effect/proxy/sound consumers acknowledged separately |

All entry points stage changes in a local copy. Any unsupported required
input/path leaves every modeled field unchanged. A held result emits no
consumer intents. A caller can additionally stage this reducer on its own copy,
preflight the exact requested pose bank, then commit behavior and presentation
together. Merely setting a consumer readiness flag does not implement that
consumer's excluded side effects; successful results retain the partial label.

The source-used state1 entry point does not copy callback snapshots, store an
unconditionally generated sample, advance pose/effects/proxies or clear
packet33C. It can therefore advance a supported branch whose inputs are known
even when that branch never reads the unavailable BIOS sample. Once its common
tail needs `80103FAC`, the missing sample holds the entire state1 transaction.
The full callback entry point always requires the original BIOS sample, even
for stationary actions. This distinction is visible in the API and tests.

The initializer can retain unknown HP and detection-profile ownership as
`std::nullopt` while populating independently proved scalar stores. It does
not choose a combat statistics global. Its normal route requires an explicit
source floor word; parameter40 follows the original +32 raw-Y/action8 route.
The existing source pose initialization call is excluded, so a caller must
prepare its exact pose request separately. The normal initialized flags are
14, while parameter20 initializes flags to0. A known zero packet from the
initializer allows the hit consumer's immediate return without knowing HP.

Detection range uses the packed four-byte EXE profile, not a pointer table.
Original core `800364B0` reads the profile byte at
`800899AE + selector352*4` and compares unsigned work80 with that byte shifted
left20. Equality rejects. A proved rejection resets phase353 to0 and returns
false; an admitted range requires the remaining persistent and geometric
helpers. The bounded reducer never substitutes a cosine cone or proximity
test. For the owner profile4 context, the range byte is15; the caller must
retain its independently owned profile context. Parameter20 bypasses the
helper, clears flags328 mask10 and leaves the persistent phase unchanged.

The bounded hit reducer executes only these source-established routes:

- Packet payload `packet & 0x00FFFFFF` equal to0 returns without HP/profile reads.
- Nonzero payload with none of masks `0x38000000` admits sound163 without damage.
- A damage packet with mask `0x20000000`, flags328 mask08 absent, known HP and
  a positive signed result after the original low-halfword subtraction writes
  HP6E, previous HPB2, work15E=32767, clears parameter40, and requests sound162
  for supported actions0/1. The reducer retains the source wrapping behavior.

Lethal results, flags328 mask08 and damage paths requiring selected combat
record byte9 remain `NeedsHitConsumer`. Missing sound readiness holds before
commit. These branches prove arithmetic and source call intent; they do not
prove native delivery of a projectile or hit packet.

Outer packet finalization happens after all preflight checks. The full
callback always clears the complete packet33C word, including when the hit
consumer is skipped. A two-callback native test delivers one admitted hit,
observes the committed clear, and then supplies that cleared word: HP remains
unchanged on the second callback. The state1 subset retains the packet because
its source boundary excludes that outer finalization.

The common tail preserves the original priority: action1 restores byte0E;
action6 with signed HP below51 chooses action5; detection mask10 chooses
action9; otherwise the explicitly supplied low-four-bit sample selects the
existing action/turn result. It then clears both bytes0A/0B, parameter40 and
flags328 mask01. Action0's phase2 start also clears the complete0A halfword;
the wider wrapper supplies the byte0B clear missing from the earlier scalar
kernel. Nonzero signed timers35C,35E and332 decrement with 16-bit wrapping,
including negative values. An expiring defeat timer360 holds the whole
transaction because its allocation/free/effect/sound consumers are unported.

Prepared action2 is admitted through `Class20_03PreparedAction2`. Its raw and
fixed arrays contain the work state after source motion and wall correction,
before the action's floor stores. Its floor word, contact word and optional
signed cue byteA6 are explicit inputs. The larger actor transaction must retain
and commit the wall wrapper's velocity44/46/48 and byte74 changes alongside
these scalar results. Supplying only a contact return word does not satisfy
the preparation contract. Unsupported stage families reject preparation.

For phase0/1, the reducer requests animation1 on initialization, clears
work330, retains the source turn/heading arithmetic modulo4096, installs the
prepared raw/fixed work, and performs the original floor stores. The absolute
signed raw-Y change at33 or greater restores all four raw/fixed lanes from the
entry state. Wall maskF00 also chooses action1 without that full rollback.
Both routes clear bytes0A/0B and choose restore byte0E=4 with detection mask10
or2 otherwise; the changed heading remains. With neither route, detection
mask10 sets the transition flag so the same source common tail chooses
action9. Phases above1 return without requesting movement, floor or cue.

Unknown cue byteA6 holds before any motion commits. Cue1 requests source
sound176 and requires the sound consumer to be ready. A bounded hit can
request sound162 earlier in the same callback; the result retains that first
intent and sound176 in `secondary_sound_8001a050`, preserving source order.
Neither readiness nor prepared scalar fixtures establishes native sound or
contact execution; those producers/consumers remain explicit.

The owner-local reproducer is
[class20_03_transaction_source_probe.py](class20_03_transaction_source_probe.py).
It executes original instructions with explicitly declared authored helper
premises. The final pass covers 2,309 cases across 65 scalar fields per case:
71 outer callbacks, 1,198 state1 bodies, 1,024 common tails and16 initializer
contexts. The callback/state1 cases include961 prepared action2 contexts.
It interpreted 289,285 instructions including the authored hooks.
The cases cover raw/fixed/angle snapshot sizes, animation restart/advance
choice, effect/proxy order, real source detection rejection, every selector
low nibble, health50/51 transition boundary, signed timer wrap, phase2
distance/predicate/packet starts, source hit wrapping and packet finalization.
Prepared action2 cases cover phases0/1/2, detection paths, floor changes at
both sides of the32/33 boundary, wall maskF00, nontransition contact1000,
negative contact words, wrapped headings, cue0/1 and two source sound intents.
Their motion delta and contact/floor results are declared authored premises;
the original movement accumulator/raw conversion and action/state1/callback
instructions execute. A caller must independently source-validate the actual
actor motion/contact producer.

The native C++17 differential test passes every field and consumer intent.
Fixture SHA-256:
`a53d2de7521fcae59d7f4646585e3459f17e871acb34a70a530f8cb56aa68a28`.
Owner execution evidence and its original generated fixture remain local at
`work/pass-20261006-worldalive-01418/enemy-callback/r4/`. The code-only source
package includes a copy of the authored synthetic scalar fixture at
`tests/fixtures/class20-03-transaction-source-cases.tsv`, so the full original
instruction comparison can run through CTest without owner payloads. These
are authored work premises and scalar outputs, with declared helper hooks;
they contain no harvested code, original tables or assets. The bare test target
also checks missing input/consumer atomic holds, the source-used subset,
unknown HP/profile preservation, rejection equality and consecutive packet
delivery. The existing 2,018 action/selector and675 distance cases still pass.

```sh
python3 docs/research_01418/class20_03_transaction_source_probe.py \
  --exe ../../owner-data/extracted/ROCK_NEO.EXE \
  --dat ../../owner-data/extracted/CDDATA/DAT \
  --out ../pass-20261006-worldalive-01418/enemy-callback/new-run

test_class20_03_callback \
  ../pass-20261006-worldalive-01418/enemy-callback/new-run/class20-03-transaction-source-cases.tsv
```
