"""Regenerate docs/validation_01415/floor08-source-cases.tsv by executing the
original 0x8004A060 (ROCK_NEO.EXE, read only) in the research_01414 CPU.
Bounds go to 0x1F800124..0x12E in table order (x0 x1 z0 z1 y0 y1), as
0x8004B3C0 copies them from the 0x80097D90 table. Only synthetic inputs and
scalar outputs are written. Usage: floor08_source_regen.py EXE OUT [IN]
"""
import sys,struct
import os
sys.path.insert(0,os.path.join(os.path.dirname(__file__),'..','research_01414'))
from player_contact_source_probe import CPU
image=open(sys.argv[1],'rb').read()
base=struct.unpack_from('<I',image,24)[0]
RET=0x80000100
ok=bad=0; rows=[]
for line in open(sys.argv[3] if len(sys.argv)>3 else sys.argv[2]):
    if not line.strip() or line[0]=='#': continue
    v=list(map(int,line.split()))
    code,x0,x1,z0,z1,y0,y1,qx,qz,bx0,bx1,bz0,bz1,by0,by1,ef,ec=v
    c=CPU(image,base); c.r[29]=0x801FFF00; c.r[31]=RET
    R=0x80100000
    # classifier-expanded rectangle (family-0/1 0x8004E304 contract), raw Y
    for i,val in enumerate((x0-bx1,x1-bx0,z0-bz1,z1-bz0,y0,y1)): c.put(R+2*i,2,val&0xffff)
    for i,val in enumerate((bx0,bx1,bz0,bz1,by0,by1)): c.put(0x1F800124+2*i,2,val&0xffff)
    c.put(0x1F8000C0,2,qx&0xffff); c.put(0x1F8000C4,2,qz&0xffff)
    c.r[4]=R; c.r[5]=code
    c.run(0x8004A060,RET)
    f=c.get(R+8,2,True); g=c.get(R+10,2,True)
    rows.append((v,f,g))
    if (f,g)==(ef,ec): ok+=1
    else:
        bad+=1
        if bad<=4: print('fixture',ef,ec,'original',f,g,'row',v[:9])
print('fixture matches original:',ok,'differs:',bad)
with open(sys.argv[2],'w') as o:
    o.write('# Source 0x8004A060 executed by docs/research_01414 CPU (bounds at 0x1F800124 in table order x0 x1 z0 z1 y0 y1); expanded group/classifier contract; no movement-response parity\n')
    o.write('# code x0 x1 z0 z1 y0 y1 query_x query_z bounds_x0 bounds_x1 bounds_z0 bounds_z1 bounds_y0 bounds_y1 floor_y ceiling_y\n')
    for v,f,g in rows: o.write('\t'.join(map(str,v[:15]+[f,g]))+'\n')
