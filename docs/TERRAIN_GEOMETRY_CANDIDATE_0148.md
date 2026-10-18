# Terrain geometry recovery evidence, 0.0.14.8

Status: **CANDIDATE**. The supplied cooked data strongly distinguishes a consecutive XYZ layout and PS1 strip vertex order from the inherited terrain gather. No original executable was read, no raw game files were processed, and no cooking/extraction ran in this pass. Runtime and retail comparison are separate evidence requirements.

The existing `SourceRenderPrimitive` stores all twelve compressed bytes reversibly in `local_x`, `local_z`, and `ignored_raw`. The inherited gather sets X from byte indices `0,3,5,10`, Z from `1,6,8,11`, and forces all Y values to the tile origin. Its supporting executable anchors in the inherited evidence ledger are candidates; the comment calling the gather exact does not establish a complete XYZ interpretation.

The candidate helper first inverts that storage gather, then groups original bytes into four consecutive XYZ triples. It keeps the existing tile origin, coordinate scale, and quarter-turn transform conventions. Its local Y interpretation is explicitly candidate. UVs, prefix, material identity, stream order, and vertex index associations are retained. It submits `(0,1,2)` and `(1,3,2)` without sorting vertices, making a hull, discarding records, or replacing uncertain faces.

Aggregate observations for the 2,493 stored cooked records:

| Measurement | Inherited flattened gather | Candidate XYZ |
|---|---:|---:|
| Axis-aligned rectangles | 0 | 1,642 |
| Coplanar XYZ records | Flat by construction | 2,493 |
| Four-vertex projected hulls | 717 | 2,323 |
| Three-vertex projected hulls | 1,761 | 170 |
| Strip coverage only | 495 | 2,425 |
| Fan coverage only | 682 | 0 |
| Either coverage | 117 | 66 |
| Neither coverage | 1,184 | 2 |
| Zero-area records | 15 | 0 |

The candidate XYZ records have 2,403 four-point records and 90 three-point records. All first triangles have nonzero area. There are 829 constant-X, 717 constant-Y, and 643 constant-Z faces. These observations account for vertical terrain that the prior flattened expansion cannot represent. Projection uses a face-normal axis for the aggregate hull comparison only; the runtime never uses the hull.

The two neither-coverage records remain unresolved. Their prefix high bytes are 128 and 160, but no operation or topology is assigned to those values. Original order is preserved. Hull coverage does not prove original shape for concave records, so these two results do not justify a correction or removal.

The 0143 fallback retains its inherited expander and triangulation. The recovery helper is a separate API intended for an independently gated enhanced renderer. Physics, collision contacts, floor snapping, material palette ownership, and original camera behavior are not proved by this geometry measurement.

Reproduce aggregates from an existing cooked file:

```sh
python3 tools/terrain_geometry_evidence.py /absolute/path/to/st00.l3render
```

Machine-readable results are in `terrain_geometry_candidate_0148.json`, including the input SHA-256. The analyzer validates L3RG version, section bounds, payload CRC, and file size, performs synthetic strip/fan diagnostic self-checks, and emits no original vertex coordinates. The C++ candidate tests cover signed extremes, all four rotations, scales, a vertical wall, UV association, optional/malformed descriptor references, shifts, overflows, and unchanged baseline behavior.
