#!/usr/bin/env python3
"""Unchanged original class40 scalar prefix oracle. Retail fixtures stay private."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[3]
sys.path[:0]=[str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe,OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe
WORK=0x800c3580;SCRIPT=0x1000

def fill(c,at,data):
    for i,n in enumerate(data):c.put(at+i,1,n)
def read(c,at,count):return bytes(c.get(at+i,1) for i in range(count))
def authored(seed):return bytes((seed*37+i*17)&255 for i in range(1444))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--overlay',type=Path);p.add_argument('--route',choices=('global','phase5'),default='phase5')
    for name in ('exe','ebd','out'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if out.exists() or out.is_relative_to(SOURCE):raise ValueError('Choose NEW private evidence directory')
    image=a.exe.read_bytes();owner=OwnerExe(image);ebd=a.ebd.read_bytes();rows=[];steps=0
    overlay=a.overlay.read_bytes() if a.overlay else b''
    if a.route=='phase5' and not overlay:raise ValueError('Actual phase5 route needs the owner overlay')
    for seed in range(24):
        c=OverlayProbe(image,owner.base,overlay);c.r[4]=WORK;c.r[5]=WORK+840;c.r[29]=0x700000
        before=bytearray(authored(seed));before[0]=1;before[1]=64|seed%4;before[2]=5;before[3]=0;before[4]=1 if a.route=='phase5' else 0;before[6]=0x23;before[8]=0;before[12]=0
        fill(c,WORK,before);fill(c,0x8016c000,ebd)
        c.run(0x8010c930,0x800322a8) if a.route=='phase5' else c.run(0x80014ac0,0x80015198)
        assert not c.return_only_hooks;steps+=c.steps
        rows.append([seed,before.hex(),read(c,WORK,1444).hex()])
    fixture=('seed before after\n'+''.join(' '.join(map(str,r))+'\n' for r in rows)).encode()
    public_rows=[];script_steps=0
    scripts=[]
    for op in range(1,26):
        n=3 if op in (1,19) else (0 if op in (15,16,18) else 1)
        scripts.append([op]+[0xffff-i*0x1234 for i in range(n)]+[0,1+op])
    scripts.extend([[27],[26,2,5,77,0,19],[1,65535,32768,32767,5,255,19,1024,2048,4096,17,3,23,65535,0,0]])
    for seed,words in enumerate(scripts):
        c=OverlayProbe(image,owner.base,overlay);c.r[4]=WORK;c.r[5]=WORK+840;c.r[29]=0x700000
        before=bytearray(authored(seed));struct.pack_into('<IHH',before,840,SCRIPT,0,0)
        fill(c,WORK,before);fill(c,SCRIPT,struct.pack('<'+str(len(words))+'H',*words))
        try:c.run(0x80014bcc)
        except Exception as error:raise ValueError(f"authored case{seed} words{words}: {error}") from error
        assert not c.return_only_hooks;script_steps+=c.steps
        public_rows.append([seed,','.join(map(str,words)),before.hex(),read(c,WORK,1444).hex()])
    public=('seed words before after\n'+''.join(' '.join(map(str,r))+'\n' for r in public_rows)).encode()
    proof={'schema':'source-scene-actor40-prefix/1','status':'SOURCE_BACKED_PREFIX','owner_exe_sha256':OWNER_SHA,
           'owner_initializer_cases':len(rows),'owner_initializer_instructions':steps,'authored_script_cases':len(public_rows),'authored_script_instructions':script_steps,
           'helper_hooks':0,'instruction_patches':0,'retail_parity':False,'hardware_tested':False,'prefix_end':'0x800322A8' if a.route=='phase5' else '0x80015198','route':a.route,
           'private_initializer_fixture_sha256':hashlib.sha256(fixture).hexdigest(),'authored_script_fixture_sha256':hashlib.sha256(public).hexdigest(),
           'limits':['Owner initializer fixtures contain retained owner descriptor bytes and stay private.','Authored script fixture contains no original script words.','Actual phase5 prefix ends before integer/GTE pose consumer, wrapper/global stores and later callbacks; global script oracle is a separate route.']}
    out.mkdir(parents=True);(out/'owner-prefix.tsv').write_bytes(fixture);(out/'authored-script.tsv').write_bytes(public);(out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()
