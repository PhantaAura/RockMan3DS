#!/usr/bin/env python3
"""Bounded owner-executable evidence for the player pose/fire bridge."""
import argparse,hashlib,importlib.util,json,struct
from pathlib import Path

SUPPORTED='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
EXPORT2='41e27c71cbb928a3143dcd77d6e8377663aac1a37a02f203d14b8c22c1c9afbc'

def load_cpu(root:Path):
    path=root/'docs/research_01414/player_contact_source_probe.py'
    spec=importlib.util.spec_from_file_location('l3_source_cpu_01414',path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module.CPU

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--decomp-export',required=True,type=Path);parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    image=args.exe.read_bytes();export=args.decomp_export.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SUPPORTED:raise ValueError('unsupported owner executable')
    if hashlib.sha256(export).hexdigest()!=EXPORT2:raise ValueError('unsupported decompiler export identity')
    base=struct.unpack_from('<I',image,24)[0]
    def region(start,end):
        a=2048+start-base;b=2048+end-base
        if a<2048 or b>len(image) or a>=b:raise ValueError('source block outside load image')
        return {'start':hex(start),'end_exclusive':hex(end),'bytes':b-a,'sha256':hashlib.sha256(image[a:b]).hexdigest()}
    CPU=load_cpu(root);fire=[]
    for action,flag in enumerate((0x86,0x82,0x45,0x41)):
        cpu=CPU(image,base);cpu.r[29]=0x7000;cpu.r[31]=0x12345678;cpu.r[4]=0x1000;cpu.r[5]=action
        before=bytes((i*17+3)&255 for i in range(0x5f4))
        for i,value in enumerate(before):cpu.put(0x1000+i,1,value)
        cpu.run(0x80040e9c,0x12345678)
        changed=[i for i,value in enumerate(before) if cpu.get(0x1000+i,1)!=value]
        if changed!=[0x174] or cpu.get(0x1174,1)!=flag:raise ValueError('fire action scalar mismatch')
        fire.append({'action':action,'flag_174':flag,'changed_offsets':['0x174'],'instruction_steps':cpu.steps})
    pointer_payload=bytearray()
    for selection in range(16):
        row=2048+0x80089a84+selection*12-base
        pointer_at=2048+0x80089eac+selection*4-base
        pointer=struct.unpack_from('<I',image,pointer_at)[0];counter_at=2048+pointer+2-base
        pointer_payload+=struct.pack('<Bh',image[row+7],struct.unpack_from('<h',image,counter_at)[0])
    result={
        'format':'source-player-pose-fire-evidence/1','status':'SOURCE_BACKED_PARTIAL',
        'exe_sha256':SUPPORTED,'decompiler_export_sha256':EXPORT2,
        'source_blocks':{
            'pose_driver_800325d4':region(0x800325d4,0x800329d4),
            'pose_part_800331a0':region(0x800331a0,0x800333a0),
            'buster_spawn_80035724':region(0x80035724,0x800358f0),
            'buster_counter_init_80037534':region(0x80037534,0x80037594),
            'fire_state_80040e9c':region(0x80040e9c,0x80040fa8)},
        'pose_position_layout':{'base_offset':'0x3A8','stride_bytes':8,'part_count':15,
            'buster_muzzle_part4':'0x3C8','buster_muzzle_part7':'0x3E0'},
        'fire_scalar_cases':fire,
        'selection_profile':{'selection_count':16,'row_address':'0x80089A84','row_stride':12,
            'subtype_byte_address':'row+7 / 0x80089A8B+selection*12','counter_pointer_table':'0x80089EAC',
            'tuple_sha256':hashlib.sha256(pointer_payload).hexdigest()},
        'limitations':['alternate eight-part limb pose bank selected by work+0x106 remains undecoded',
            'fire helper returns 0x800423DC/0x80042458 remain explicit receipts',
            'no projectile callback, camera projection, damage, or hardware parity claim'],
        'payload_policy':'hashes, addresses, scalar results and aggregate identities only; no executable or decompiler text copied'}
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'fire_cases':len(fire),'pose_parts':15,'profile_selections':16}))

if __name__=='__main__':main()
