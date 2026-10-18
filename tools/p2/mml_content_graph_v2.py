#!/usr/bin/env python3
from pathlib import Path
import argparse,json,hashlib,re
STAGE=re.compile(r'^ST([0-9A-Fa-f]{2})(?:_([0-9A-Fa-f]{2}))?\.BIN$')
def sha(p):
 h=hashlib.sha256();
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()
def classify(p):
 s=p.suffix.upper(); name=p.name.upper()
 if s=='.XA':return 'xa'
 if s=='.STR':return 'str'
 if s in ('.EXE','.03'):return 'program'
 if s=='.BIN' and STAGE.match(name):return 'stage_state'
 if s=='.BIN':return 'dat_resource'
 return 'other'
def build(root):
 nodes=[];families={}
 for p in sorted(x for x in Path(root).rglob('*') if x.is_file()):
  rel=p.relative_to(root).as_posix();k=classify(p);m=STAGE.match(p.name.upper())
  node={'id':'file:'+rel,'path':rel,'kind':k,'size':p.stat().st_size,'sha256':sha(p),'geometry_identity':None}
  if m:
   fam='ST'+m.group(1).upper();state=m.group(2)
   node['stage_family']=fam;node['gameplay_state_identity']=p.name.upper();families.setdefault(fam,[]).append(node['id'])
  nodes.append(node)
 return {'format':'mml-content-graph-v2','nodes':nodes,'stage_families':families,'identity_policy':'geometry identity never merges gameplay/event state'}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('root');ap.add_argument('-o','--output');a=ap.parse_args();r=build(a.root);s=json.dumps(r,indent=2);Path(a.output).write_text(s+'\n') if a.output else print(s)
if __name__=='__main__':main()
