#!/usr/bin/env python3
"""Source-backed scalar player-contact probe, using only owner-local executable.

Runs bounded integer response blocks on synthetic inputs, writes hashes,
addresses, field values and aggregate checks only. It does not emit game bytes
or original instruction listings, and does not establish full-game parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
SUPPORTED_EXE="10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0"
def s16(v): return v-65536 if v&32768 else v
def s32(v): return v-4294967296 if v&2147483648 else v
class CPU:
    def __init__(self,image,base):
        self.image=image; self.base=base; self.r=[0]*32; self.mem={}; self.lo=0; self.hi=0; self.steps=0
    def put(self,a,n,v):
        for i in range(n): self.mem[a+i]=(v>>(8*i))&255
    def get(self,a,n,signed=False):
        def byte(p):
            if p in self.mem:return self.mem[p]
            q=p-self.base+2048
            if self.base<=p<self.base+len(self.image)-2048:return self.image[q]
            return 0
        v=sum(byte(a+i)<<(8*i) for i in range(n))
        return v-(1<<(8*n)) if signed and v&(1<<(8*n-1)) else v
    def run(self,start,end=None):
        pc=start;pending=None
        while pc and pc!=end:
            if self.steps>=10000:raise ValueError('CPU probe step bound exceeded')
            w=self.get(pc,4); op=w>>26; rs=w>>21&31; rt=w>>16&31; rd=w>>11&31; sh=w>>6&31;fn=w&63
            imm=w&65535;si=s16(imm);a,b=self.r[rs],self.r[rt];v=None;dest=rt;old=pending;pending=None
            if not w:pass
            elif op==0:
                dest=rd
                if fn==0:v=b<<sh
                elif fn==2:v=b>>sh
                elif fn==3:v=s32(b)>>sh
                elif fn==4:v=b<<(a&31)
                elif fn==6:v=b>>(a&31)
                elif fn==7:v=s32(b)>>(a&31)
                elif fn==8:pending=a
                elif fn==16:v=self.hi
                elif fn==18:v=self.lo
                elif fn==24:self.lo=(s32(a)*s32(b))&0xffffffff
                elif fn==25:self.lo=(a*b)&0xffffffff
                elif fn==26:
                    aa,bb=s32(a),s32(b)
                    if not bb:raise ValueError("source division trap")
                    q=abs(aa)//abs(bb);q=-q if (aa<0)!=(bb<0) else q
                    self.lo=q&0xffffffff;self.hi=(aa-q*bb)&0xffffffff
                elif fn==27:
                    if not b:raise ValueError("source division trap")
                    self.lo=a//b;self.hi=a%b
                elif fn==33:v=a+b
                elif fn==35:v=a-b
                elif fn==36:v=a&b
                elif fn==37:v=a|b
                elif fn==38:v=a^b
                elif fn==39:v=~(a|b)
                elif fn==42:v=int(s32(a)<s32(b))
                elif fn==43:v=int(a<b)
                else:raise ValueError('unsupported integer operation at '+hex(pc))
            elif op==9:v=a+si
            elif op==10:v=int(s32(a)<si)
            elif op==11:v=int(a<(si&0xffffffff))
            elif op==12:v=a&imm
            elif op==13:v=a|imm
            elif op==14:v=a^imm
            elif op==15:v=imm<<16
            elif op in (32,33,35,36,37):v=self.get((a+si)&0xffffffff,{32:1,33:2,35:4,36:1,37:2}[op],op in (32,33))
            elif op in (40,41,43):self.put((a+si)&0xffffffff,{40:1,41:2,43:4}[op],b)
            elif op in (4,5):
                if (a==b)==(op==4):pending=pc+4+si*4
            elif op in (6,7):
                if (s32(a)<=0)==(op==6):pending=pc+4+si*4
            elif op==1 and rt in (0,1):
                if (s32(a)<0)==(rt==0):pending=pc+4+si*4
            elif op==2:pending=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2)
            elif op==3:
                self.r[31]=pc+8;pending=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2)
            else:raise ValueError('unsupported opcode at '+hex(pc))
            if v is not None and dest:self.r[dest]=v&0xffffffff
            self.r[0]=0;self.steps+=1;pc=old if old is not None else pc+4


def scalar_checks(image,base):
    def cpu():
        c=CPU(image,base);c.r[29]=0x4000;return c
    cases=0;deep_unknown=0
    for x in (-20,0,32,68,120):
      for y in (-80,0,80):
       for z in (-20,0,32,68,120):
        for delta in ((50,50,50),(-50,50,50),(50,-50,50),(50,50,-50),(150,0,0),(0,150,0),(0,0,150),(-150,-150,-150)):
         for blocked in (0,1,2,4,8,16,32):
          lo=(-32,-160,-32);hi=(132,160,132);p=[x,y,z]
          correction=[(hi[i] if delta[i]<0 else lo[i])-p[i] for i in range(3)]
          def eligible(i):
            bits=((2,1),(32,16),(8,4))[i]
            return delta[i]!=0 and abs(correction[i])<=abs(delta[i]) and not(blocked&(bits[0] if correction[i]>0 else bits[1]))
          ex,ey,ez=[eligible(i) for i in range(3)]
          if not(ex or ey or ez):deep_unknown+=1;continue
          c=cpu();c.r[4]=0x1000;c.r[5]=0x2000;c.r[6]=1
          for i,v in enumerate(p):c.put(0x1000+2*i,2,v);c.put(0x1f8000c0+2*i,2,v)
          for i,v in enumerate(delta):c.put(0x1f8000d0+2*i,2,v);c.put(0x2000+2*i,2,p[i]-v)
          for i in range(3):c.put(0x1f8000f0+2*i,2,lo[i]);c.put(0x1f8000f8+2*i,2,hi[i])
          c.put(0x1f8000ff,1,blocked)
          c.run(0x8004f44c)
          axis=0 if ex else (2 if ez else 1)
          if ex and ez:axis=0 if abs(correction[0])<abs(correction[2]) else 2
          horizontal=abs(correction[axis])
          if ey and ((not ex and not ez) or (abs(correction[1])<horizontal if ex and ez else abs(correction[1])<=horizontal)):axis=1
          expected=p[:];expected[axis]+=correction[axis]
          actual=[c.get(0x1000+2*i,2,True) for i in range(3)]
          if actual!=expected:raise ValueError('rectangle response mismatch '+str((p,delta,blocked,actual,expected)))
          expected_result=1 if axis==1 and correction[1]<=0 else 0
          if c.r[2]!=expected_result:raise ValueError('rectangle repeat branch mismatch')
          cases+=1
    floor_cases=0
    for contact in (0,1,2):
      for y in range(-31,32):
       for before in (y-20,y,y+20):
        # code03 downward/grounded branch and code0F ceiling are exactly the
        # selected supported response paths; airborne upward code03 is UNKNOWN.
        for code in (3,15):
         if code==3 and y<before and contact!=0:continue
         c=cpu();c.r[4]=0x1000;c.r[5]=code;c.r[6]=1
         for off,v in enumerate((-32,132,-32,132,-32,32)):c.put(0x1000+2*off,2,v)
         c.put(0x1f8000c2,2,y);c.put(0x1f8000d2,2,y-before);c.put(0x1f8000f6,1,contact)
         c.put(0x1f80012c,2,-160);c.put(0x1f80012e,2,0)
         c.run(0x8004bdc8)
         expected=(-32 if code==3 else 192)
         if not(-32<y<192):expected=y
         if c.get(0x1f8000c2,2,True)!=expected:raise ValueError('floor/ceiling response mismatch')
         if code==3 and expected!=y and c.get(0x1f8000f6,1)!=0:raise ValueError('floor state mismatch')
         floor_cases+=1
    negative_cases=0
    for level in range(32):
      for bands in range(4):
       packed=0x8000|(level<<8)|(bands<<13)
       floor=-level*128-1;ceiling=-level*128-bands*384+161
       for y in (floor-32,floor,floor+32,ceiling-32,ceiling,ceiling+32):
        for dy in (-8,0,8):
         c=cpu();c.r[16]=packed;c.r[23]=0
         c.put(0x1f80012c,2,-160);c.put(0x1f80012e,2,0)
         c.put(0x1f8000c2,2,y);c.put(0x1f8000d2,2,dy);c.put(0x1f8000f6,1,1)
         c.run(0x8004b8b8,0x8004bb1c)
         expected=y
         if floor<y:expected=floor
         elif bands and y<ceiling:expected=ceiling
         if c.get(0x1f8000c2,2,True)!=expected:raise ValueError('packed negative-cell Y response mismatch')
         expected_state=0 if floor<y and dy>0 else 1
         if c.get(0x1f8000f6,1)!=expected_state:raise ValueError('packed negative-cell state mismatch')
         negative_cases+=1
    launch_cases=0
    for gate in (-1,0,1):
      for high in (0,1):
       c=cpu();c.r[16]=0x1000;c.put(0x1000+0xa7,1,gate);c.put(0x1000+0x16b,1,high)
       c.run(0x8003dac0,0x8003dc44 if gate!=-1 else 0x8003db0c)
       v=c.get(0x1000+0x46,2,True)
       if v!=((-800 if high else -448) if gate==-1 else 0):raise ValueError('jump gate/velocity mismatch')
       launch_cases+=1
    return {'rectangle_response_cases_passed':cases,'untranslated_deep_overlap_cases_excluded':deep_unknown,
            'floor_ceiling_response_cases_passed':floor_cases,'negative_packed_cell_cases_passed':negative_cases,'jump_launch_cases_passed':launch_cases,
            'limits':['synthetic scalar blocks, no GTE or complete player tick','diagonal walls/slope response/deep-overlap remain UNKNOWN','no measured retail-vs-native parity']}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    image=a.exe.read_bytes();digest=hashlib.sha256(image).hexdigest()
    if digest!=SUPPORTED_EXE:raise ValueError('unsupported executable identity')
    base=struct.unpack_from('<I',image,0x18)[0]
    bounds=list(struct.unpack_from('<6h',image,0x80097d90-base+2048))
    if bounds!=[-32,32,-32,32,-160,0]:raise ValueError('PL00 bounds source mismatch')
    regions=[('player_bounds_initializer',0x8003bed4,0x8003bee0),('player_contact_wrapper',0x8004b414,0x8004b5cc),
             ('negative_cell_response',0x8004b8b8,0x8004b984),('response_dispatch',0x8004bdc8,0x8004c170),
             ('rectangle_response',0x8004f44c,0x8004f8bc),('axis_eligibility',0x80050808,0x80050c2c),
             ('floor_query',0x80048e6c,0x8004920c),('floor_adhesion',0x8003c610,0x8003c6fc),
             ('jump_action',0x8003da34,0x8003dc5c),('air_control_landing',0x8003e068,0x8003e224),
             ('animation_record_seed',0x8003218c,0x800321ac),('animation_record_tick',0x8003234c,0x800323e8)]
    result={'format':'player-contact-source-evidence/1','status':'SOURCE_BACKED_PARTIAL','PHYSICS_1TO1':False,
            'exe_sha256':digest,'PL00_bounds_address':'0x80097D90','bounds_signed_world_units':bounds,
            'coordinate_convention':'retail signed16 Y-down, fixed16.16 integration; velocity in sixteenth world units/update',
            'supported_geometry':['rectangle code01 motion-eligible response','code03 downward/grounded floor','code0F ceiling','negative packed-cell Y response'],
            'unknown_geometry':['diagonal classifiers2..7','slope response code08','deep-overlap fallback0x80050400','airborne-upward code03 horizontal/ceiling branch'],
            'source_scalar_checks':scalar_checks(image,base),
            'source_blocks':[{'id':name,'start':hex(start),'end_exclusive':hex(end),'sha256':hashlib.sha256(image[start-base+2048:end-base+2048]).hexdigest()}for name,start,end in regions],
            'payload_policy':'hashes, addresses, behavioral constants and aggregate counts only; no owner executable bytes/instruction listings'}
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['source_scalar_checks']))
if __name__=='__main__':main()
