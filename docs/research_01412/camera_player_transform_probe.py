#!/usr/bin/env python3
"""Bounded source-instruction probes; emits semantics, addresses and hashes only.

No owner byte payloads, disassembly, textures or models are written. This tiny
CPU evaluator only runs identified integer transform blocks with synthetic data.
It does not execute the game or claim GTE/gameplay/Old3DS equivalence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SUPPORTED_EXE = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'

def s16(v): return v-65536 if v&32768 else v
def s32(v): return v-4294967296 if v&2147483648 else v

class CPU:
    def __init__(self,image,base):
        self.image=image; self.base=base; self.r=[0]*32; self.mem={}; self.lo=0; self.steps=0
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
            if self.steps>=1000:raise ValueError('CPU probe step bound exceeded')
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
                elif fn==18:v=self.lo
                elif fn==24:self.lo=(s32(a)*s32(b))&0xffffffff
                elif fn==25:self.lo=(a*b)&0xffffffff
                elif fn==33:v=a+b
                elif fn==35:v=a-b
                elif fn==36:v=a&b
                elif fn==37:v=a|b
                elif fn==42:v=int(s32(a)<s32(b))
                elif fn==43:v=int(a<b)
                else:raise ValueError('unsupported integer operation at '+hex(pc))
            elif op==9:v=a+si
            elif op==10:v=int(s32(a)<si)
            elif op==11:v=int(a<(si&0xffffffff))
            elif op==12:v=a&imm
            elif op==13:v=a|imm
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

def probes(image,base):
    translation_cases=0
    for index in range(128):
        actor=[(index*971+axis*319)%60001-30000 for axis in range(3)]
        offset=[(index*313+axis*157)%8192-4096 for axis in range(3)]
        world=[actor[i]+offset[i] for i in range(3)]
        for part in (0,14):
            c=CPU(image,base); c.r[29]=0x4000;c.r[18]=0x1000;c.r[17]=part*32
            c.r[6]=0x1000+part*8;c.r[3]=actor[0]&0xffffffff;c.r[5]=world[0]&0xffffffff
            c.put(0x4000+58,2,actor[1]);c.put(0x4000+60,2,actor[2])
            c.put(0x1f8000d8,4,world[1]);c.put(0x1f8000dc,4,world[2])
            c.run(0x800332ac,0x80033318)
            actual=[c.get(0x1000+part*8+936+axis*2,2,True) for axis in range(3)]
            expected=[s16(((world[i]+7*actor[i]+4)>>3)&65535) for i in range(3)]
            if actual!=expected:raise ValueError('render translation reduction mismatch')
            translation_cases+=1
    rotation_cases=0;scale_cases=0
    for yaw in (0,512,1024,1536,2048,2560,3072,3584,-1024):
        c=CPU(image,base);c.r[4]=0x1000;c.r[5]=0x2000;c.put(0x1000+2,2,yaw)
        c.run(0x8007b5a8)
        got=[c.get(0x2000+i*2,2,True) for i in range(9)]
        packed=struct.unpack_from('<I',image,0x80092ec4+((yaw&4095)*4)-base+2048)[0]
        sine=s16(packed&65535);cosine=s16(packed>>16)
        expected=[cosine,0,sine,0,4096,0,-sine,0,cosine]
        if got!=expected:raise ValueError('root Y matrix sign mismatch')
        rotation_cases+=1
        for scale in (512,1024,4096):
            d=CPU(image,base);d.r[4]=0x2000;d.r[5]=0x3000
            for i,v in enumerate(got):d.put(0x2000+i*2,2,v)
            for i in range(3):d.put(0x3000+i*4,4,scale)
            d.run(0x8007ade8)
            scaled=[d.get(0x2000+i*2,2,True) for i in range(9)]
            if scaled!=[s16(((v*scale)>>12)&65535) for v in got]:raise ValueError('Q12 matrix scale mismatch')
            scale_cases+=1
    camera_cases=0
    def trig(angle):
        # Camera scalar utilities use a separate quarter-wave table; the packed
        # matrix table is not bit-identical at every angle and negative branch.
        out=[]
        for function in (0x8007a060,0x8007a12c):
            c=CPU(image,base);c.r[29]=0x4000;c.r[4]=angle&0xffffffff;c.run(function)
            out.append(s32(c.r[2]))
        return out
    for yaw in (0,512,1024,1536,2048,2560,3072,3584,4095):
        for pitch in (0,48,-48):
            for distance in (512,1200):
                target=[3125,-151,-5073]
                c=CPU(image,base);c.r[29]=0x4000;c.r[22]=distance;c.r[21]=yaw;c.r[18]=pitch;c.r[19]=0x1000
                for i,v in enumerate(target):c.put(0x1000+i*4,4,v)
                c.run(0x8003819c,0x800382cc)
                actual=[c.get(0x800a39f0+i*4,4,True) for i in range(3)]
                sy,cy=trig(yaw);sp,cp=trig(pitch)
                expected=[target[0]-((((distance*sy)>>12)*cp)>>12),
                          target[1]-((distance*sp)>>12),
                          target[2]-((((distance*cy)>>12)*cp)>>12)]
                expected=[s16(v&65535) for v in expected]
                if actual!=expected:raise ValueError('camera eye formula mismatch: '+str((yaw,pitch,distance,actual,expected)))
                camera_cases+=1
    return {'status':'SOURCE_BACKED_PARTIAL','translation_cases_passed':translation_cases,
            'Y_rotation_cases_passed':rotation_cases,'Q12_scale_cases_passed':scale_cases,
            'camera_eye_cases_passed':camera_cases,
            'limits':['bounded integer blocks with synthetic inputs','no original GTE or complete pose-chain execution',
                      'native float rounding remains distinct from source fixed point']}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('exe',type=Path);p.add_argument('output',type=Path);args=p.parse_args()
    image=args.exe.read_bytes();digest=hashlib.sha256(image).hexdigest()
    if digest!=SUPPORTED_EXE:raise ValueError('unsupported executable identity')
    base=struct.unpack_from('<I',image,0x18)[0]
    gp=0x80097864;scale_addr=gp+0x498;scale=struct.unpack_from('<I',image,scale_addr-base+2048)[0]
    if scale!=512:raise ValueError('source initial scale mismatch')
    blocks=[('render_translation',0x800332ac,0x80033318),('Q12_scale',0x8007ade8,0x8007af0c),
            ('Y_rotation',0x8007b5a8,0x8007b834),('camera_core',0x80037fb4,0x80038324),
            ('camera_init',0x80038648,0x80038894),('actor_root_yaw',0x80032648,0x80032664),
            ('source_GP_init',0x80068080,0x80068088)]
    j={'format':'camera-player-transform-source-probe/1','status':'SOURCE_BACKED_PARTIAL','executable_sha256':digest,
       'GP':hex(gp),'player_scale_address':hex(scale_addr),'initial_player_scale_Q12':scale,'local_player_scale':scale/4096,
       'source_integer_checks':probes(image,base),
       'camera_fields':{'base':'0x800a3958','radial_distance_s32':'0x800a3964','yaw_s32':'0x800a3968','pitch_s32':'0x800a396c',
                        'target_s32x3':'0x800a39a0','computed_eye_s32x3':'0x800a39f0','render_rotation_s16x3':'0x800a3a10',
                        'previous_computed_eye_s32x3':'0x800a3a00','global_shift_u8':'0x1f800074',
                        'scratch_eye_s16x3':'0x1f800028','render_camera_matrix':'0x1f800008'},
       'camera_initialization':{'status':'SOURCE_BACKED_PARTIAL','radial_distance':512,'yaw':0,'pitch':48,'target_y_offset':150,
                                'limits':'these are source initialization values, not every scripted/state-dependent camera'},
       'baseline_trig_and_rounding':{'status':'SOURCE_BACKED_PARTIAL','packed_matrix_trig48':[301,4084],
                                    'scalar_camera_sin48':301,'scalar_camera_cos48':4085,
                                    'camera_yaw2048_sin_cos':[0,-4096],
                                    'source_eye_relative_to_actor_yaw0':[0,-187,511],
                                    'source_target_relative_to_actor_yaw0':[0,-150,0],
                                    'native_baseline_horizontal_distance':511/512,
                                    'native_baseline_eye_height':187/512,'native_baseline_target_height':150/512,
                                    'rounding_note':'arithmetic (-512*4085)>>12 is -511; negated eye component is +511. Positive component truncates to510.'},
       'ordinary_follow_camera':{'status':'SOURCE_BACKED_PARTIAL','update_anchor':'0x80038fa4',
                                'player_work_base':'0x800b51b0','target':'actor XYZ with source Y reduced by state+0x5c',
                                'camera_yaw':'raw actor work+0x56 plus 2048 plus optional state yaw term',
                                'eye_formula':'target - distance * [sin(camera_yaw)*cos(pitch), sin(pitch), cos(camera_yaw)*cos(pitch)]',
                                'fixed_point':'Q12 intermediate arithmetic shifts; stored unscaled eye narrowed to signed16',
                                'native_Y_inversion':'behind +sin(actor yaw)/+cos(actor yaw); eye height=(target offset + distance*sin(pitch))/512'},
       'source_blocks':[{'id':name,'start':hex(start),'end_exclusive':hex(end),'sha256':hashlib.sha256(image[start-base+2048:end-base+2048]).hexdigest()} for name,start,end in blocks],
       'findings':[{'id':'PLAYER_RENDER_TRANSLATION_REDUCTION','status':'SOURCE_BACKED_PARTIAL','formula':'(unscaled_world_translation + 7*scaled_actor_position + 4) arithmetic_shift_right 3'},
                   {'id':'PLAYER_RENDER_MATRIX_SCALE','status':'SOURCE_BACKED_PARTIAL','formula':'coefficient * (512 << global_shift) arithmetic_shift_right 12'},
                   {'id':'PLAYER_ROOT_YAW','status':'SOURCE_BACKED_PARTIAL','formula':'raw actor work+0x56 minus raw work+0x116; m02=+sinY, m20=-sinY'},
                   {'id':'EXTRA_YAW_FIELD_SEMANTICS','status':'UNKNOWN','field':'raw actor work+0x116'},
                   {'id':'CAMERA_COMPLETE_BEHAVIOR','status':'UNKNOWN','missing':'live state-dependent camera ownership, interpolation, obstruction and scripted transitions'}],
       'payload_policy':'no owner payloads or instruction listings emitted'}
    args.output.write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(j['source_integer_checks']))

if __name__=='__main__':main()
