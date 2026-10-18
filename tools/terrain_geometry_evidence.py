#!/usr/bin/env python3
"""Aggregate geometry evidence from an existing cooked L3RG v3 file only.

No raw game reader or cooker is invoked. No vertex payloads are emitted.
Hull comparisons diagnose decoded geometry; they never reorder runtime vertices.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zlib


def turn(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def hull(points):
    points = sorted(set(points))
    if len(points) < 2:
        return points
    low, high = [], []
    for point in points:
        while len(low) > 1 and turn(low[-2], low[-1], point) <= 0:
            low.pop()
        low.append(point)
    for point in reversed(points):
        while len(high) > 1 and turn(high[-2], high[-1], point) <= 0:
            high.pop()
        high.append(point)
    return low[:-1] + high[:-1]


def area2(points):
    return abs(sum(a[0] * b[1] - a[1] * b[0]
                   for a, b in zip(points, points[1:] + points[:1])))


def coverage(points):
    polygon = hull(points)
    target = area2(polygon)
    if target == 0:
        return "degenerate", len(polygon)
    strip = (abs(turn(points[0], points[1], points[2]))
             + abs(turn(points[1], points[3], points[2])) == target
             and turn(points[1], points[2], points[0])
             * turn(points[1], points[2], points[3]) <= 0)
    fan = (abs(turn(points[0], points[1], points[2]))
           + abs(turn(points[0], points[2], points[3])) == target
           and turn(points[0], points[2], points[1])
           * turn(points[0], points[2], points[3]) <= 0)
    label = "both" if strip and fan else "strip_only" if strip else "fan_only" if fan else "neither"
    return label, len(polygon)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def diagnose(cooked):
    if coverage([(0, 0), (2, 0), (0, 3), (2, 3)])[0] != "strip_only":
        raise RuntimeError("strip diagnostic self-check failed")
    if coverage([(0, 0), (2, 0), (2, 3), (0, 3)])[0] != "fan_only":
        raise RuntimeError("fan diagnostic self-check failed")
    if len(cooked) < 80 or cooked[:4] != b"L3RG":
        raise ValueError("expected cooked L3RG")
    if struct.unpack_from("<HH", cooked, 4) != (3, 80):
        raise ValueError("only retained-byte L3RG v3 is supported")
    if struct.unpack_from("<I", cooked, 8)[0] & 3 != 3:
        raise ValueError("retained-byte/current-gather flags missing")
    counts = struct.unpack_from("<6I", cooked, 12)
    offsets = struct.unpack_from("<6I", cooked, 36)
    file_bytes, crc = struct.unpack_from("<II", cooked, 60)
    if file_bytes != len(cooked) or zlib.crc32(cooked[80:]) != crc:
        raise ValueError("cooked file size or CRC mismatch")
    if offsets[0] < 80 or list(offsets) != sorted(offsets) or offsets[-1] > file_bytes:
        raise ValueError("cooked section offsets invalid")
    sizes = [counts[0] * 12, counts[1] * 24, counts[4] * 4,
             counts[3] * 4, counts[2] * 20, counts[5] * 24]
    if any(offset + size > limit for offset, size, limit
           in zip(offsets, sizes, list(offsets[1:]) + [file_bytes])):
        raise ValueError("cooked section bounds invalid")
    baseline, candidate, prefixes = Counter(), Counter(), {}
    for primitive_index in range(counts[5]):
        start = offsets[5] + primitive_index * 24
        xs = struct.unpack_from("4b", cooked, start + 12)
        zs = struct.unpack_from("4b", cooked, start + 16)
        ignored = struct.unpack_from("4b", cooked, start + 20)
        # Invert the supplied cooked gather; every compressed byte is retained.
        raw = [xs[0], zs[0], ignored[0], xs[1], ignored[1], xs[2],
               zs[1], ignored[2], zs[2], ignored[3], xs[3], zs[3]]
        old_points = list(zip(xs, zs))
        label, hull_vertices = coverage(old_points)
        baseline[label] += 1
        baseline[f"hull_vertices_{hull_vertices}"] += 1
        if (len(set(old_points)) == 4 and len(set(xs)) == 2
                and len(set(zs)) == 2):
            baseline["axis_aligned_rectangles"] += 1
        points = [tuple(raw[i:i + 3]) for i in range(0, 12, 3)]
        edges = [subtract(point, points[0]) for point in points[1:]]
        normal = cross(edges[0], edges[1])
        determinant = sum(a * b for a, b in zip(normal, edges[2]))
        candidate["coplanar" if determinant == 0 else "noncoplanar"] += 1
        candidate[f"unique_vertices_{len(set(points))}"] += 1
        candidate["first_triangle_nonzero" if any(normal) else "first_triangle_zero"] += 1
        constant_axes = [axis for axis in range(3)
                         if len(set(point[axis] for point in points)) == 1]
        for axis in constant_axes:
            candidate[f"constant_{'xyz'[axis]}"] += 1
        # Use a face-normal projection solely for area/overlap diagnostics.
        projection_axis = max(range(3), key=lambda axis: abs(normal[axis]))
        projected = [tuple(point[axis] for axis in range(3) if axis != projection_axis)
                     for point in points]
        label, hull_vertices = coverage(projected)
        candidate[label] += 1
        candidate[f"hull_vertices_{hull_vertices}"] += 1
        if (len(set(points)) == 4 and len(constant_axes) == 1
                and all(len(set(point[axis] for point in points)) == 2
                        for axis in range(3) if axis != constant_axes[0])):
            candidate["axis_aligned_rectangles"] += 1
        prefix_high_byte = str(cooked[start + 3])
        prefixes.setdefault(prefix_high_byte, Counter())[label] += 1
    return {
        "status": "CANDIDATE",
        "input_sha256": hashlib.sha256(cooked).hexdigest(),
        "input_bytes": len(cooked),
        "stored_primitive_count": counts[5],
        "baseline_cross_gather_xz": dict(sorted(baseline.items())),
        "candidate_consecutive_xyz": dict(sorted(candidate.items())),
        "candidate_prefix_high_byte_coverage": {key: dict(sorted(value.items()))
                                                for key, value in sorted(prefixes.items())},
        "runtime_vertex_order": [0, 1, 2, 1, 3, 2],
        "uv_vertex_association": "original indices retained",
        "limitations": [
            "No executable semantics were recovered or verified in this pass.",
            "Coplanarity and hull coverage are structural measurements, not original-game parity.",
            "Neither-coverage records remain submitted in original order; no hull replacement.",
            "Global height/scale conventions and gameplay floor contact need runtime comparison."
        ]
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cooked_render", type=Path)
    args = parser.parse_args()
    source = args.cooked_render
    if source.suffix.lower() != ".l3render" or "owner-data" in source.parts or source.is_symlink():
        parser.error("only a regular existing cooked .l3render file is accepted")
    if not source.is_file() or source.stat().st_size > 4 * 1024 * 1024:
        parser.error("cooked input must be a regular file no larger than4MiB")
    print(json.dumps(diagnose(source.read_bytes()), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
