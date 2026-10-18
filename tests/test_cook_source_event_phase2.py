import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('phase2_cooker', Path(__file__).resolve().parents[1] / 'tools/cook_source_event_phase2.py')
cooker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cooker)


class Phase2CookerGuards(unittest.TestCase):
    def test_unrecognized_or_truncated_owner_image_rejected(self):
        for image in (b'', b'PS-X EXE', bytes(2048), b'PS-X EXE' + bytes(4096)):
            with self.subTest(size=len(image)), self.assertRaises(ValueError):
                cooker.cook(image)


if __name__ == '__main__':
    unittest.main()
