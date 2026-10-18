#!/usr/bin/env python3
"""Aggregate-only analysis of the explicitly authorized existing cooked pair."""
import argparse
from collections import Counter, defaultdict
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import zlib

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--pack", type=Path, required=True, help="existing cooked tutorial directory")
parser.add_argument("--out", type=Path, required=True, help="new aggregate JSON outside the pack")
args = parser.parse_args()
PACK = args.pack.resolve()
OUTPUT = args.out.resolve()
if OUTPUT.exists() or OUTPUT == PACK or PACK in OUTPUT.parents:
    raise ValueError("choose a new output outside the existing cooked pack")
render = (PACK / "st00.l3render").read_bytes()
mapping = (PACK / "st00.l3map").read_bytes()
assert render[:4] == b"L3RG" and mapping[:4] == b"L3MP"
assert struct.unpack_from("<HH", render, 4) == (3, 80)
assert struct.unpack_from("<HH", mapping, 4) == (1, 84)
assert zlib.crc32(render[80:]) == struct.unpack_from("<I", render, 64)[0]
assert zlib.crc32(mapping[84:]) == struct.unpack_from("<I", mapping, 80)[0]
rc = struct.unpack_from("<6I", render, 12)
rd, rr, rm, rf, rs, rp = struct.unpack_from("<6I", render, 36)
mc = struct.unpack_from("<9I", mapping, 12)
ma, mt, mi, md, ml, mg, mp = struct.unpack_from("<7I", mapping, 48)
idx = [struct.unpack_from("<hhhhHH", mapping, mi + i * 12) for i in range(mc[4])]


def geometry(descriptor, lod=1):
    ri = struct.unpack_from("<H", render, rd + descriptor * 12 + lod * 2)[0]
    if ri == 65535:
        return [], 0
    first, count = struct.unpack_from("<IH", render, rr + ri * 24 + 12)
    out, primitive_count = [], 0
    for j in range(count):
        si = struct.unpack_from("<I", render, rf + (first + j) * 4)[0]
        shift = render[rs + si * 20 + 1]
        first_primitive, primitive_count_stream = struct.unpack_from("<II", render, rs + si * 20 + 4)
        primitive_count += primitive_count_stream
        for pi in range(first_primitive, first_primitive + primitive_count_stream):
            start = rp + pi * 24
            xs = struct.unpack_from("4b", render, start + 12)
            zs = struct.unpack_from("4b", render, start + 16)
            ignored = struct.unpack_from("4b", render, start + 20)
            raw = [xs[0], zs[0], ignored[0], xs[1], ignored[1], xs[2],
                   zs[1], ignored[2], zs[2], ignored[3], xs[3], zs[3]]
            out.extend(tuple(raw[p + axis] * (1 << shift) for axis in range(3))
                       for p in range(0, 12, 3))
    return out, primitive_count


def bounds(points):
    return tuple(value for axis in range(3)
                 for value in (min(p[axis] for p in points), max(p[axis] for p in points)))


stream_shifts, primitive_shifts = Counter(), Counter()
for si in range(rc[2]):
    shift = render[rs + si * 20 + 1]
    primitive_count = struct.unpack_from("<I", render, rs + si * 20 + 8)[0]
    stream_shifts[shift] += 1
    primitive_shifts[shift] += primitive_count

descriptor_bounds, primitive_counts = {}, {}
lod_compare, descriptor_spans = Counter(), Counter()
for di in range(mc[5]):
    points, primitives = geometry(di)
    if not points:
        continue
    bbox = bounds(points)
    descriptor_bounds[di] = bbox
    primitive_counts[di] = primitives
    descriptor_spans[(bbox[1] - bbox[0], bbox[5] - bbox[4])] += 1
    for lod in (0, 2):
        lod_points, _ = geometry(di, lod)
        if lod_points:
            other = bounds(lod_points)
            lod_compare[f"mid_vs_{'near' if lod == 0 else 'far'}_pairs"] += 1
            if bbox == other:
                lod_compare[f"mid_vs_{'near' if lod == 0 else 'far'}_identical_xyz_bounds"] += 1

contact_metrics, ratios, margins = Counter(), Counter(), Counter()
for di, bbox in descriptor_bounds.items():
    for list_offset in (4, 6):
        li = struct.unpack_from("<H", mapping, md + di * 44 + list_offset)[0]
        if li == 65535:
            continue
        first, count = struct.unpack_from("<II", mapping, ml + li * 8)
        if not count:
            continue
        groups = [struct.unpack_from("<4h", mapping, mg + (first + j) * 16)
                  for j in range(count)]
        cb = (min(v[0] for v in groups), max(v[1] for v in groups),
              min(v[2] for v in groups), max(v[3] for v in groups))
        rb = bbox[:2] + bbox[4:6]
        contact_metrics["contact_render_pairs"] += 1
        for axis, offset in (("x", 0), ("z", 2)):
            gs, cs = rb[offset + 1] - rb[offset], cb[offset + 1] - cb[offset]
            if gs:
                ratios[(axis, str(Fraction(cs, gs)))] += 1
        for extra in range(4):
            delta = max(abs((rb[i] << extra) - cb[i]) for i in range(4))
            margins[f"extra_{extra}_max_bound_delta_le_1"] += delta <= 1
            margins[f"extra_{extra}_max_bound_delta_le_8"] += delta <= 8

areas = []
for area in range(mc[0]):
    ax, az, width, height, first, count = struct.unpack_from("<hhHHII", mapping, ma + area * 24)
    references, cell_counts = defaultdict(list), Counter()
    deltas = Counter()
    for local in range(count):
        packed = struct.unpack_from("<h", mapping, mt + (first + local) * 2)[0]
        cell_counts["positive" if packed > 0 else "negative" if packed < 0 else "zero"] += 1
        if packed <= 0:
            continue
        gx, gz = ax + local % width, az + local // width
        ix, iy, iz, flags, di, reserved = idx[packed]
        references[packed].append((gx, gz))
        deltas[(gx * 512 - 32512 - ix, gz * 512 - 32512 - iz)] += 1
    rectangles = Counter()
    for points in references.values():
        xspan = max(p[0] for p in points) - min(p[0] for p in points) + 1
        zspan = max(p[1] for p in points) - min(p[1] for p in points) + 1
        rectangles[(xspan, zspan, len(points))] += 1
    areas.append({
        "area": area, "cell_counts": dict(cell_counts),
        "distinct_positive_idx_references": len(references),
        "occurrences_per_idx_histogram": dict(Counter(len(v) for v in references.values())),
        "reference_cell_rectangles": [{"width_cells": k[0], "height_cells": k[1],
                                        "occurrences": k[2], "count": v}
                                       for k, v in sorted(rectangles.items())],
        "relative_grid_center_minus_idx_offsets": [{"dx": k[0], "dz": k[1], "count": v}
                                                   for k, v in sorted(deltas.items())],
        "current_occurrence_primitive_count": sum(primitive_counts.get(idx[k][4], 0) * len(v)
                                                  for k, v in references.items()),
        "hypothetical_unique_reference_primitive_count": sum(primitive_counts.get(idx[k][4], 0)
                                                             for k in references),
    })

result = {
    "status": "SOURCE_BACKED_PARTIAL",
    "input_sha256": {"map": hashlib.sha256(mapping).hexdigest(),
                     "render": hashlib.sha256(render).hexdigest()},
    "source_semantics_unverified": True,
    "current_global_extra_shift": 0,
    "stream_base_shift_counts": dict(stream_shifts),
    "stored_primitive_base_shift_counts": dict(primitive_shifts),
    "mid_descriptor_xz_spans": [{"span_x": k[0], "span_z": k[1], "count": v}
                               for k, v in sorted(descriptor_spans.items())],
    "lod_bound_comparison": dict(lod_compare),
    "contact_render_pairs": dict(contact_metrics),
    "contact_to_render_span_ratios": [{"axis": k[0], "ratio": k[1], "count": v}
                                     for k, v in sorted(ratios.items())],
    "contact_bound_margin_comparison": dict(margins),
    "areas": areas,
    "idx_metadata": {"rows": len(idx), "raw6_zero_rows": sum(row[3] == 0 for row in idx),
                     "x_low2_zero_rows": sum(row[0] & 3 == 0 for row in idx),
                     "z_low2_zero_rows": sum(row[2] & 3 == 0 for row in idx)},
    "findings": [
        "No supported global scale increase: extra shift0 already matches many contact footprints within one unit per bound.",
        "Positive reference identity is reused across adjacent grid cells. Current renderer emits each occurrence at a grid center and rotates by reference-index low bits.",
        "IDX X/Z coordinates match the center of each repeated reference cell rectangle. This is strong placement evidence but the intended renderer semantics remain unverified.",
        "Current contact-group helper adds grid center and applies no index-low-bit rotation. The supplied terrain helper applies that rotation; their placement conventions differ.",
        "Negative cells are omitted by the runtime terrain frontend. Their renderer/material semantics are not supplied by the current cooked package.",
        "Inherited executable anchors0x80021834,0x80021c3c,0x800221e0,0x8002246c are CANDIDATE. A register/argument trace or fuller authorized source notes are required to verify placement, rotation and global scale."
    ],
    "changes_applied": False,
    "payload_policy": "Aggregate spans and relative offsets only; no vertex payloads or original absolute coordinates."
}
out = OUTPUT
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
print(json.dumps({"output": str(out), "lod": dict(lod_compare), "margin": dict(margins),
                  "area_counts": [{"area": a["area"], "occurrence_primitives": a["current_occurrence_primitive_count"],
                                   "hypothetical_unique_reference_primitives": a["hypothetical_unique_reference_primitive_count"]}
                                  for a in areas]}, indent=2))
