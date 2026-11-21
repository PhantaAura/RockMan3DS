#!/usr/bin/env python3
"""compare_recomp_3ds: clears count, extra events fail, order is checked."""
import contextlib, io, sys, tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compare_recomp_3ds as c


def run(oracle, ds, **kw):
    with tempfile.TemporaryDirectory() as t:
        op, dp = Path(t, 'o.csv'), Path(t, 'd.csv')
        op.write_text('frame,flag,value,stage,event\n' + ''.join(f'{fr},{f},{v},0,0\n' for fr, f, v in oracle))
        dp.write_text(''.join(f'# story_flag,frame={fr},flag=0x{f:x},value={v},area=0\n' for fr, f, v in ds))
        with contextlib.redirect_stdout(io.StringIO()):
            return c.compare(op, dp, **kw)


base = [(10, 0x603, 1), (20, 0x1FF, 0), (30, 0x1FF, 1)]
assert run(base, base) == 0, 'identical runs pass'
assert run(base, [(10, 0x603, 1), (30, 0x1FF, 1)]) == 1, 'missing clear fails'
assert run(base, base + [(40, 0x777, 1)]) == 1, 'extra set fails'
assert run(base, base + [(40, 0x777, 1)], allow_extra=True) == 0, 'extra allowed on request'
assert run(base, [(10, 0x603, 1), (30, 0x1FF, 1), (35, 0x1FF, 0)]) == 1, 'clear after set is out of order'
print('test_compare_recomp_3ds OK')
