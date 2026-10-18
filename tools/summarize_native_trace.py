#!/usr/bin/env python3
"""Summarize a native diagnostic CSV without changing it.

Event phase and CD command queue phase belong to different owners. Keep them
separate when diagnosing a held event. A trace can establish which records were
logged; it cannot establish that a screenshot shows that run or that rendering
looks correct. Visual acceptance and physical hardware testing stay UNKNOWN.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
from typing import Any


NUMERIC_COLUMNS = (
    "frame", "area", "held", "pressed", "x", "y", "z", "heading",
    "projectiles", "frame_before_log_us", "interval_us", "update_us",
    "render_us", "bottom_ui_us", "draw_calls", "triangles", "owned_linear",
    "owned_texture", "malloc_live", "malloc_arena_free", "linear_free",
    "vram_free", "action", "phase", "animation", "animation_pose",
    "vertical_velocity", "grounded", "contact_status", "psx_held",
    "psx_pressed",
)

# Require the complete records emitted by the native frontend. An unfinished
# line must not overwrite the last valid observation with a partial record.
RECORD_FIELDS = {
    "source_event": {
        "frame": "uint", "area": "uint", "status": "uint",
        "committed": "flag", "event": "uint", "phase": "uint",
        "consumer": "address",
    },
    "source_command_service": {
        "frame": "uint", "status": "uint", "committed": "flag",
        "popped": "flag", "phase": "uint", "pending_CD": "uint",
        "param_count": "uint", "params": "params", "CD_status_raw": "uint",
        "transport_receipts": "uint", "source_request": "address",
        "missing": "address", "native_transport": "token",
    },
    "source_fade_callback": {
        "frame": "uint", "status": "uint", "committed": "flag",
        "ack": "flag", "running": "flag", "iterations": "uint",
        "terminated": "flag", "presentation": "uint", "grayscale": "byte",
        "native_scheduler_pass": "bool", "cadence": "token",
    },
    "source_scene_constructor": {
        "frame": "uint", "status": "uint", "committed": "flag",
        "phase": "uint", "pool40": "address", "pool80": "address",
        "poolE0": "address", "record_resources": "token",
        "script_callbacks": "token", "scene_render_complete": "bool",
    },
    "source_scene_callback": {
        "frame": "uint", "status": "uint", "committed": "flag",
        "callback": "address", "awaiting_camera": "flag",
        "awaiting_timeline": "flag", "awaiting_caller": "flag",
        "next_consumer": "address", "scene_complete": "bool",
    },
    "source_scene_camera": {
        "frame": "uint", "fog_controls_known": "flag",
        "dqa": "address", "dqb": "address", "source_shift_known": "flag",
        "source_shift": "byte", "renderer_camera_bound": "bool",
    },
}


def _fields(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in text.split(","):
        key, separator, value = item.partition("=")
        if not separator or not key or not value or key in out:
            raise ValueError("missing, empty, or duplicate field")
        out[key] = value
    return out


def _value(text: str, kind: str) -> Any:
    if kind in ("uint", "flag", "byte"):
        if not re.fullmatch(r"[0-9]+", text):
            raise ValueError("unsigned decimal required")
        value = int(text)
        maximum = {"uint": (1 << 64) - 1, "flag": 1, "byte": 255}[kind]
        if value > maximum:
            raise ValueError("numeric value outside supported range")
        return bool(value) if kind == "flag" else value
    if kind == "address":
        if not re.fullmatch(r"[0-9a-fA-F]{8}", text):
            raise ValueError("eight hexadecimal address digits required")
        return "0x" + text.upper()
    if kind == "params":
        values = text.split(":")
        if len(values) != 4:
            raise ValueError("four CD parameter bytes required")
        return [_value(item, "byte") for item in values]
    if kind == "bool":
        if text not in ("true", "false"):
            raise ValueError("literal true or false required")
        return text == "true"
    if kind == "token":
        if not re.fullmatch(r"[A-Za-z0-9_]+", text):
            raise ValueError("diagnostic token required")
        return text
    raise ValueError("unsupported field type")


def _record(name: str, text: str, line: int) -> dict[str, Any]:
    fields = _fields(text)
    required = RECORD_FIELDS[name]
    if name == "source_scene_constructor" and "script_callback_prefix" in fields:
        # 01428 records held the callback entirely. 01429 reports the measured
        # prefix separately. Both schemas remain valid historical evidence.
        required = {key: kind for key, kind in required.items() if key != "script_callbacks"}
        required["script_callback_prefix"] = "token"
    if set(fields) != set(required):
        raise ValueError("record fields differ from the supported native schema")
    result = {key: _value(fields[key], kind) for key, kind in required.items()}
    result["line"] = line
    if name == "source_command_service":
        result["queue_phase"] = result.pop("phase")
    elif name == "source_event":
        result["event_phase"] = result.pop("phase")
    elif name == "source_scene_constructor":
        result["event_phase"] = result.pop("phase")
    return result


def _build_header(text: str, line: int) -> dict[str, Any]:
    fields = _fields(text)
    if not {"build", "model_query_rc", "new3ds"}.issubset(fields):
        raise ValueError("build/model header fields missing")
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+){3}[A-Za-z0-9_-]*", fields["build"]):
        raise ValueError("unsupported build label")
    result: dict[str, Any] = {"build": fields.pop("build"), "line": line}
    for key, value in fields.items():
        result[key] = _value(value, "uint")
    if result["new3ds"] not in (0, 1):
        raise ValueError("new3ds must be zero or one")
    return result


def summarize_native_trace(path: str | Path) -> dict[str, Any]:
    """Read a native trace and return bounded metadata and final valid records."""
    path = Path(path).resolve()
    digest = hashlib.sha256()
    size = 0
    line_count = 0
    complete_final_line = True
    header = None
    columns = None
    records: dict[str, Any] = {name: None for name in RECORD_FIELDS}
    last_numeric = None
    last_termination = None
    run_render_rows = 0
    run_max_frame = None
    counts = {
        "build_headers": 0, "valid_numeric_rows": 0,
        "valid_comment_records": 0, "malformed_lines": 0,
        "truncated_final_lines": 0, "ignored_lines": 0,
    }
    valid_by_kind = {name: 0 for name in RECORD_FIELDS}
    issues: list[dict[str, Any]] = []

    def issue(line: int, reason: str) -> None:
        if len(issues) < 8:
            issues.append({"line": line, "reason": reason})

    with path.open("rb") as stream:
        for line_count, raw in enumerate(stream, 1):
            digest.update(raw)
            size += len(raw)
            complete_final_line = raw.endswith(b"\n")
            if not complete_final_line:
                counts["truncated_final_lines"] += 1
                issue(line_count, "final line has no newline; observation ignored")
                continue
            try:
                text = raw.decode("utf-8").rstrip("\r\n")
                if text.startswith("# build="):
                    header = _build_header(text[2:], line_count)
                    counts["build_headers"] += 1
                    # A repeated build header starts a new run. Never carry a
                    # prior run's events into a later run still at its menu.
                    columns = None
                    records = {name: None for name in RECORD_FIELDS}
                    last_numeric = None
                    last_termination = None
                    run_render_rows = 0
                    run_max_frame = None
                    continue
                if text.startswith("# "):
                    name, separator, payload = text[2:].partition(",")
                    if separator and name in RECORD_FIELDS:
                        record = _record(name, payload, line_count)
                        records[name] = record
                        counts["valid_comment_records"] += 1
                        valid_by_kind[name] += 1
                        if name == "source_fade_callback" and record["terminated"]:
                            last_termination = record
                    else:
                        counts["ignored_lines"] += 1
                    continue
                if text.startswith("frame,"):
                    candidate = tuple(next(csv.reader([text], strict=True)))
                    if candidate != NUMERIC_COLUMNS:
                        columns = None
                        raise ValueError("unsupported numeric CSV header")
                    columns = candidate
                    continue
                if text and (text[0].isdigit() or text[0] == "-"):
                    if columns is None:
                        raise ValueError("numeric row has no supported CSV header")
                    fields = next(csv.reader([text], strict=True))
                    if len(fields) != len(columns):
                        raise ValueError("numeric CSV row field count differs from header")
                    if any(not re.fullmatch(r"-?[0-9]+", item) for item in fields):
                        raise ValueError("numeric CSV row contains a noninteger field")
                    values = [int(item) for item in fields]
                    if any(item < -(1 << 63) or item > (1 << 64) - 1 for item in values):
                        raise ValueError("numeric CSV value outside supported range")
                    candidate_row = dict(zip(columns, values))
                    if not 0 <= candidate_row["frame"] <= (1 << 32) - 1:
                        raise ValueError("frame outside uint32 range")
                    last_numeric = candidate_row
                    last_numeric["line"] = line_count
                    counts["valid_numeric_rows"] += 1
                    run_render_rows += 1
                    run_max_frame = max(run_max_frame or 0, last_numeric["frame"])
                    continue
                counts["ignored_lines"] += 1
            except (ValueError, UnicodeDecodeError, csv.Error) as error:
                counts["malformed_lines"] += 1
                issue(line_count, str(error))

    event = records["source_event"]
    command = records["source_command_service"]
    fade = records["source_fade_callback"]
    scene_callback = records["source_scene_callback"]
    return {
        "schema_version": 1,
        "input": {
            "path": str(path), "sha256": digest.hexdigest(), "bytes": size,
            "lines": line_count, "complete_final_line": complete_final_line,
        },
        "header": header,
        "last_valid_records": records,
        "last_valid_numeric_row": last_numeric,
        "numeric_column_owners": {"action": "player_work_09", "phase": "player_work_0A"},
        "current_run": {
            "render_rows": run_render_rows,
            "last_numeric_frame": None if last_numeric is None else last_numeric["frame"],
            "maximum_numeric_frame": run_max_frame,
            "event_phase": None if event is None else event["event_phase"],
            "command_queue_phase": None if command is None else command["queue_phase"],
            "fade_running": None if fade is None else fade["running"],
            "last_fade_termination": last_termination,
            "scene_callback_next_consumer": (None if scene_callback is None
                                             else scene_callback["next_consumer"]),
        },
        "counts": {**counts, "valid_by_kind": valid_by_kind},
        "issues": issues,
        "visual_acceptance": "UNKNOWN",
        "physical_hardware_validation": "UNKNOWN",
        "limits": [
            "Event phase and CD command queue phase are separate states.",
            "The numeric row's action/phase columns are player work bytes 09/0A.",
            "A trace does not identify a screenshot's run or prove visible gameplay.",
            "The model query does not distinguish emulator execution from physical hardware.",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path, help="native hardware diagnostic CSV to read")
    args = parser.parse_args()
    try:
        result = summarize_native_trace(args.trace)
    except OSError as error:
        parser.exit(1, f"Could not read trace: {error}\n")
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
