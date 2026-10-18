#!/usr/bin/env python3
"""Authored trace fixtures test evidence handling, not retail gameplay parity."""

import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[1] / "tools" / "summarize_native_trace.py"
SPEC = importlib.util.spec_from_file_location("native_trace_summary", TOOL)
summary = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(summary)

HEADER = "# build=0.0.14.29,model_query_rc=0,new3ds=0,memory_layout=2,stack_reserved=262144\n"
CSV_HEADER = ",".join(summary.NUMERIC_COLUMNS) + "\n"
EVENT = "# source_event,frame=70,area=0,status=12,committed=0,event=1,phase=6,consumer=8010102c\n"
COMMAND = "# source_command_service,frame=70,status=10,committed=0,popped=0,phase=2,pending_CD=1,param_count=0,params=0:0:0:0,CD_status_raw=34,transport_receipts=0,source_request=00000000,missing=00000000,native_transport=NATIVE_XA_PLAYBACK_CANDIDATE\n"
FADE = "# source_fade_callback,frame=77,status=12,committed=1,ack=0,running=0,iterations=19,terminated=1,presentation=10,grayscale=0,native_scheduler_pass=true,cadence=SOURCE_BACKED_PARTIAL\n"
SCENE = "# source_scene_constructor,frame=69,status=0,committed=1,phase=6,pool40=80000000,pool80=80000000,poolE0=fffe0000,record_resources=OWNER_EBD,script_callbacks=UNKNOWN,scene_render_complete=false\n"
SCENE_NEW = SCENE.replace("script_callbacks=UNKNOWN", "script_callback_prefix=SOURCE_BACKED_PARTIAL")
CALLBACK = "# source_scene_callback,frame=70,status=3,committed=1,callback=80100738,awaiting_camera=0,awaiting_timeline=0,awaiting_caller=1,next_consumer=801011e8,scene_complete=false\n"
CAMERA = "# source_scene_camera,frame=70,fog_controls_known=1,dqa=fffff956,dqb=01400000,source_shift_known=1,source_shift=1,renderer_camera_bound=false\n"


def numeric(frame):
    values = {name: 0 for name in summary.NUMERIC_COLUMNS}
    values.update(frame=frame, y=-1, grounded=-1, render_us=16311, draw_calls=12, triangles=1478)
    return ",".join(str(values[name]) for name in summary.NUMERIC_COLUMNS) + "\n"


class NativeTraceSummaryTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "authored.csv"

    def read(self, text):
        self.path.write_bytes(text if isinstance(text, bytes) else text.encode("utf-8"))
        original = self.path.read_bytes()
        result = summary.summarize_native_trace(self.path)
        self.assertEqual(self.path.read_bytes(), original)
        self.assertEqual(result["input"]["sha256"], hashlib.sha256(original).hexdigest())
        return result

    def test_event_and_command_phases_remain_separate(self):
        result = self.read(HEADER + CSV_HEADER + EVENT + COMMAND + numeric(120) + numeric(120) + FADE + SCENE)
        self.assertEqual(result["current_run"]["event_phase"], 6)
        self.assertEqual(result["current_run"]["command_queue_phase"], 2)
        event = result["last_valid_records"]["source_event"]
        self.assertEqual(event["event"], 1)
        self.assertEqual(event["consumer"], "0x8010102C")
        self.assertNotIn("phase", event)
        self.assertNotIn("phase", result["last_valid_records"]["source_command_service"])
        self.assertEqual(result["current_run"]["render_rows"], 2)
        self.assertEqual(result["current_run"]["last_numeric_frame"], 120)
        self.assertFalse(result["current_run"]["fade_running"])
        self.assertTrue(result["current_run"]["last_fade_termination"]["terminated"])
        self.assertFalse(result["last_valid_records"]["source_scene_constructor"]["scene_render_complete"])
        self.assertEqual(result["header"]["build"], "0.0.14.29")
        self.assertEqual(result["header"]["new3ds"], 0)
        self.assertEqual(result["visual_acceptance"], "UNKNOWN")
        self.assertEqual(result["physical_hardware_validation"], "UNKNOWN")

    def test_loading_has_no_invented_event_or_visual_acceptance(self):
        result = self.read(HEADER + CSV_HEADER + "# area_event=package_before,area=0\n" + numeric(0))
        self.assertIsNone(result["current_run"]["event_phase"])
        self.assertIsNone(result["current_run"]["command_queue_phase"])
        self.assertIsNone(result["current_run"]["fade_running"])
        self.assertEqual(result["visual_acceptance"], "UNKNOWN")

    def test_old_and_new_scene_records_keep_each_owner_separate(self):
        result = self.read(HEADER + CSV_HEADER + EVENT + COMMAND + SCENE + SCENE_NEW + CALLBACK + CAMERA)
        scene = result["last_valid_records"]["source_scene_constructor"]
        self.assertEqual(scene["event_phase"], 6)
        self.assertEqual(scene["script_callback_prefix"], "SOURCE_BACKED_PARTIAL")
        self.assertNotIn("script_callbacks", scene)
        self.assertEqual(result["counts"]["valid_by_kind"]["source_scene_constructor"], 2)
        callback = result["last_valid_records"]["source_scene_callback"]
        self.assertTrue(callback["awaiting_caller"])
        self.assertFalse(callback["awaiting_camera"])
        self.assertFalse(callback["awaiting_timeline"])
        self.assertEqual(result["current_run"]["scene_callback_next_consumer"], "0x801011E8")
        camera = result["last_valid_records"]["source_scene_camera"]
        self.assertTrue(camera["fog_controls_known"])
        self.assertEqual(camera["dqa"], "0xFFFFF956")
        self.assertEqual(camera["dqb"], "0x01400000")
        self.assertFalse(camera["renderer_camera_bound"])
        self.assertEqual(result["current_run"]["event_phase"], 6)
        self.assertEqual(result["current_run"]["command_queue_phase"], 2)
        self.assertEqual(result["visual_acceptance"], "UNKNOWN")

    def test_incomplete_new_scene_records_cannot_replace_valid_observations(self):
        text = HEADER + CALLBACK + CAMERA
        text += CALLBACK.replace(",awaiting_caller=1", "")
        text += CAMERA.replace("dqa=fffff956", "dqa=wrong")
        text += SCENE_NEW.replace("script_callback_prefix=SOURCE_BACKED_PARTIAL", "script_callbacks=UNKNOWN,script_callback_prefix=SOURCE_BACKED_PARTIAL")
        text += CALLBACK.rstrip("\n")
        result = self.read(text)
        self.assertEqual(result["current_run"]["scene_callback_next_consumer"], "0x801011E8")
        self.assertEqual(result["last_valid_records"]["source_scene_camera"]["dqa"], "0xFFFFF956")
        self.assertIsNone(result["last_valid_records"]["source_scene_constructor"])
        self.assertEqual(result["counts"]["malformed_lines"], 3)
        self.assertEqual(result["counts"]["truncated_final_lines"], 1)

    def test_malformed_and_truncated_records_preserve_last_valid(self):
        text = HEADER + CSV_HEADER + EVENT + COMMAND + numeric(120)
        text += EVENT.replace("status=12", "status=wrong")
        text += EVENT.replace("phase=6", "phase=6,phase=2")
        text += numeric(999).replace(",16311,", ",bad,")
        text += numeric(-1)
        text += "121,0,1\n"
        text += "# source_event,frame=999,area=0,status=12,committed=0,event=1,phase=9,consumer=8010102c"
        result = self.read(text)
        self.assertEqual(result["current_run"]["event_phase"], 6)
        self.assertEqual(result["current_run"]["last_numeric_frame"], 120)
        self.assertEqual(result["counts"]["malformed_lines"], 5)
        self.assertEqual(result["counts"]["truncated_final_lines"], 1)
        self.assertFalse(result["input"]["complete_final_line"])

    def test_invalid_schema_and_utf8_do_not_supply_records(self):
        text = HEADER + "frame,phase\n70,6\n"
        text += EVENT.replace("consumer=8010102c", "consumer=8010102g")
        text += FADE.replace("grayscale=0", "grayscale=256")
        result = self.read(text.encode("utf-8") + b"\xff\n")
        self.assertIsNone(result["last_valid_numeric_row"])
        self.assertIsNone(result["last_valid_records"]["source_event"])
        self.assertIsNone(result["last_valid_records"]["source_fade_callback"])
        self.assertEqual(result["counts"]["malformed_lines"], 5)

    def test_new_run_header_clears_prior_run_observations(self):
        result = self.read(HEADER + CSV_HEADER + EVENT + COMMAND + numeric(120) + FADE + HEADER)
        self.assertEqual(result["counts"]["build_headers"], 2)
        self.assertIsNone(result["current_run"]["event_phase"])
        self.assertIsNone(result["current_run"]["last_numeric_frame"])
        self.assertIsNone(result["current_run"]["last_fade_termination"])
        self.assertEqual(result["current_run"]["render_rows"], 0)

    def test_command_only_does_not_become_event_phase(self):
        result = self.read(COMMAND)
        self.assertIsNone(result["header"])
        self.assertIsNone(result["current_run"]["event_phase"])
        self.assertEqual(result["current_run"]["command_queue_phase"], 2)

    def test_empty_trace_and_cli_read_only_output(self):
        result = self.read("")
        self.assertEqual(result["input"]["lines"], 0)
        self.assertIsNone(result["header"])
        completed = subprocess.run([sys.executable, "-B", str(TOOL), str(self.path)],
                                   check=True, capture_output=True, text=True)
        self.assertEqual(json.loads(completed.stdout), result)
        self.assertEqual(completed.stderr, "")


if __name__ == "__main__":
    unittest.main()
