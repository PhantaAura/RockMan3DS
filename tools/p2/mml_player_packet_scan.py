#!/usr/bin/env python3
"""Conservative player-material metadata scanner.

It never resolves texture pixels. It records only packet-like TPAGE/CLUT pairs and
keeps them as stateful VRAM addresses. Use on owner files locally; do not distribute
input bytes.
"""
from pathlib import Path
import argparse, json, hashlib, struct

def tpage_addr(t):
    mode=(t>>7)&3
    return {"x_word":(t&0xF)*64,"y":((t>>4)&1)*256,"bpp":4 if mode==0 else 8 if mode==1 else 16}
def clut_addr(c): return {"x_word":(c&0x3F)*16,"y":(c>>6)&0x1FF}

def scan(data):
    # Recorded H1 evidence established textured primitive streams, but the original
    # exact parser bytes were not retained. This replacement deliberately emits only
    # conservative aligned candidates whose TPAGE/CLUT decode into PS1 VRAM bounds.
    out=[]
    for off in range(0,max(0,len(data)-8),2):
        clut,tpage=struct.unpack_from('<HH',data,off)
        ta=tpage_addr(tpage); ca=clut_addr(clut)
        if ta['x_word']<1024 and ca['x_word']<1024 and ca['y']<512 and ta['bpp'] in (4,8,16):
            # Avoid drowning output: require the known player packet texture-page high
            # bits to be sparse. This is metadata discovery, not semantic proof.
            if (tpage & 0xFC60)==0:
                out.append({"offset":off,"tpage":tpage,"clut":clut,"tpage_addr":ta,"clut_addr":ca})
    return out

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('file'); ap.add_argument('-o','--output')
    a=ap.parse_args(); p=Path(a.file); b=p.read_bytes(); r={"file":p.name,"sha256":hashlib.sha256(b).hexdigest(),"candidates":scan(b),"resolution":"address_only"}
    s=json.dumps(r,indent=2); Path(a.output).write_text(s+'\n') if a.output else print(s)
if __name__=='__main__': main()
