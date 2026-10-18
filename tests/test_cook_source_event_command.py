import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SOURCE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(SOURCE/'tools'))
spec=importlib.util.spec_from_file_location('command_cook',SOURCE/'tools/cook_source_event_command.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class CommandCookTests(unittest.TestCase):
    def test_unknown_identity_is_rejected_before_pointer_reads(self):
        for image in (b'',b'PS-X EXE'+bytes(2048),bytes(1000000)):
            with self.assertRaises(ValueError):module.cook(image)

    def test_existing_output_preserved_without_reading_owner(self):
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary)/'sentinel.l3ec';path.write_bytes(b'preserve')
            result=subprocess.run([sys.executable,'-B',str(SOURCE/'tools/cook_source_event_command.py'),
                '--exe',str(Path(temporary)/'missing.exe'),'--out',str(path)],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('Refusing to overwrite',result.stderr+result.stdout)
            self.assertEqual(path.read_bytes(),b'preserve')

if __name__=='__main__':unittest.main()
