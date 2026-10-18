# Class20/03 initializer floor bounds

Status: `SOURCE_BACKED_PARTIAL`. The actor sampler now accepts the verified
class20/03 initializer bounds. This does not establish complete retail movement,
combat, texture residency or encounter parity.

Initializer `0x80102410` installs pointer `0x8011BED4` at work+0x78 before
`0x801025A8` calls floor wrapper `0x80048E6C`. The six signed halfwords are
`{x0=-60, x1=60, z0=-60, z1=60, y0=-128, y1=0}`. Their 12-byte SHA256 is
`62de884fbddc639e9afbfddb9e73def0d7025aed21a74684c1a3581d6ca932e0`.
Class20/35 initializer `0x80106974` calls the floor wrapper while its bounds
pointer is null; scratch helper `0x8004B3C0` therefore installs six zeros for
that fresh query. The profiles retain those distinct inputs.

The fresh floor wrapper supplies source XYZ, the installed bounds, zero Y
delta, and class20 to core `0x80048EA4`. Its rectangle classifier expands X/Z
halfwords by the opposite bound, retains strict edges, and subtracts the actor
bottom/top bounds from the floor interval. The actor sampler uses the shared
`runtime_contact_floor_interval_8004a060` helper for rectangle codes1/3/15 and
the separately verified floor interval codes8..11. It does not run actor XYZ
through the player movement solver.

`class20_03_evidence_probe.py` replays 1,944 authored rectangle cases through
the original floor core, covering both profiles, strict X/Z edges, vertical
admission, three low primitive codes and the class20 filter bit. All cases
match; 132 return a floor. The probe executes 479,570 original instructions,
hooks only candidate gathering to provide one synthetic cell index, and
confirms that the source XYZ input remains unchanged. The shared slope helper
has a separate 240-case source ledger. The actor test also checks its integration
and repeated allocation-free sampling.

The actual area1 map probes still retain `Unsupported` at all three source
record origins. Each has four candidates. Positive IDX94/descriptor41 at cell451
has no selected contact list for origin `(367,-1,-6946)`. Positive IDX123/
descriptor32 at cell538 has no selected contact list for the other two origins,
`(-300,-1,-5654)` and `(-200,-1,-5654)`. Negative packed cells use their separate
supported floor path. The sampler reports two unsupported visits as the query
repeats; it returns no floor and leaves the actor's source Y unchanged. Missing
contact-list admission remains a separate source-proof gate.

Machine-readable evidence:

- `../validation_01415/class20-03-source-evidence.json`: model, HP, hit consumer,
  installed bounds, original rectangle replay, and actual native map query results.
- `../validation_01415/class20-03-floor-source-evidence.json`: bounded floor replay
  summary and source hashes.
- `../validation_01415/class20-03-owner-floor.json`: exact unavailable candidate
  bindings for the three source origins.
- `../validation_01415/floor08-source-evidence.json`: shared floor interval proof.

Owner-local reproduction uses the existing extracted executable/ST00 inputs,
the existing cooked area1 EBD and ST00 map, and the built
`l3probe_actor_class20_03`. Pass `--cooked-map` to
`class20_03_evidence_probe.py` for actual map queries. The native probe accepts
optional `st00.l3map area_index` arguments and distinguishes verified floor
bounds from `NOT_SAMPLED` or an actual query status. No retail bytes, instruction
listings, additional assets or live saves are exported or modified.
