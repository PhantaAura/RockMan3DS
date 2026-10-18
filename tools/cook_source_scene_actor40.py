#!/usr/bin/env python3
"""Cook the exact class40/subtype5 first script prefix; owner-local output only."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from cook_actor_combat import OwnerExe,OWNER_SHA
SOURCE=Path(__file__).resolve().parents[1]
SCRIPT_TOKEN=0x800810c8
SCRIPT_SHA='20940c69dab1a7883edb78ca34311a477099a20e0ac26df60900c7e75be91906'
def cook(image):
    owner=OwnerExe(image)
    table=owner.word(0x800820e0);token=owner.word(table)
    if token!=SCRIPT_TOKEN:raise ValueError('Unsupported initial script identity')
    words=[];index=0
    for _ in range(128):
        op=struct.unpack('<H',owner.read(token+index*2,2))[0]
        if op==0:
            words.extend(struct.unpack('<2H',owner.read(token+index*2,4)));break
        if not 1<=op<=25:raise ValueError('Unsupported first script prefix')
        operands=3 if op in (1,19) else (0 if op in (15,16,18) else 1)
        words.extend(struct.unpack('<'+str(operands+1)+'H',owner.read(token+index*2,(operands+1)*2)))
        index+=operands+1
    else:raise ValueError('Script prefix budget exceeded')
    payload=struct.pack('<'+str(len(words))+'H',*words)
    if hashlib.sha256(payload).hexdigest()!=SCRIPT_SHA:raise ValueError('Unsupported first script payload')
    data=struct.pack('<4sHH6I',b'L3A4',1,96,token,96+len(payload),len(words),0,0,0)+bytes.fromhex(OWNER_SHA)+bytes.fromhex(SCRIPT_SHA)+payload
    return data,{'schema':'source-scene-actor40-bootstrap/1','status':'SOURCE_BACKED_PREFIX','owner_exe_sha256':OWNER_SHA,'script_token':hex(token),'script_sha256':SCRIPT_SHA,'script_words':len(words),'package_bytes':len(data),'package_sha256':hashlib.sha256(data).hexdigest(),'original_payload_exported':False,'hardware_tested':False,'retail_parity':False,'next_boundary':'0x80015198 movement, sound, animation/pose update'}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    out=a.out.resolve()
    if out.exists() or out.is_relative_to(SOURCE):raise ValueError('Choose NEW owner-local output outside source')
    data,proof=cook(a.exe.read_bytes());out.mkdir(parents=True)
    (out/'source_scene_actor40.l3a4').write_bytes(data);(out/'bootstrap-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()
