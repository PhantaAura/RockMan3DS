#!/usr/bin/env python3
from pathlib import Path
import argparse,json,re

def strings(b):return [x.decode('ascii','ignore') for x in re.findall(rb'[ -~]{5,}',b)]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('root');ap.add_argument('-o','--output');a=ap.parse_args();r=Path(a.root)
 xa=sorted(p.name for p in r.rglob('*.XA')); aliases={}
 for p in r.rglob('ST00*.BIN'):
  try:s=strings(p.read_bytes())
  except:continue
  e=[x for x in s if '.EBD' in x.upper()]
  if e:aliases[p.name]=e
 out={'xa_count':len(xa),'xa_names':xa,'embedded_ebd_paths':aliases,'note':'metadata only; semantics not inferred'}
 s=json.dumps(out,indent=2);Path(a.output).write_text(s+'\n') if a.output else print(s)
if __name__=='__main__':main()
