import json,re
from pathlib import Path
p=Path(__file__).resolve().parents[1]
h=(p/'platform/3ds/source/checkpoint_version.h').read_text();version=re.search(r'MML_CHECKPOINT_VERSION "([^"]+)"',h)[1];ident=re.search(r'MML_CHECKPOINT_ID "([^"]+)"',h)[1]
assert version==json.loads((p/'state/CURRENT_CHECKPOINT.json').read_text())['version']
make=(p/'platform/3ds/Makefile').read_text();assert 'Legends3DS_'+ident+'_WorldComesAlive' in make
for name in ('main.cpp','hardware_test.cpp'):
 s=(p/'platform/3ds/source'/name).read_text();assert '#include "checkpoint_version.h"' in s;code='\n'.join(line.split('//',1)[0] for line in s.splitlines());assert not re.search(r'0\.0\.14\.[0-9]+|(?:boot|hardware)_014[0-9]+',code)
print('Native checkpoint banners, boot/trace filenames and Makefile agree')
