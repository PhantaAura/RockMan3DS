#!/usr/bin/env python3
"""Execute owner proxy registration/pair instructions on authored kind0 states.

The original paired dispatcher body executes. Its earlier player/GTE consumers
and timing hook are explicit return boundaries, not declared implemented.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'tools'),str(SOURCE/'docs/research_01415'),
               str(SOURCE/'docs/research_01414')]
from tutorial_actor_identity_probe import OverlayProbe
from player_contact_source_probe import CPU
from cook_actor_combat import OwnerExe, OWNER_SHA
from cook_hit_proxy import build_metadata

BANK = 0x800A2F48
COUNT = 0x800988E0
TARGETS = (0x1000, 0x1100)


class Probe(OverlayProbe):
    def __init__(self, image, base):
        CPU.__init__(self, image, base)
        self.calls, self.indirect, self.return_only_hooks = [], [], []


def register(cpu, spec):
    w0,w1,a1,a2,a3,source_class,target,word,identity_flags,extra,mask,flags,packet = spec
    cpu.r = [0]*32
    cpu.r[4:8] = [0x2000,a1,a2,a3]
    cpu.r[28],cpu.r[29],cpu.r[31] = 0x80097864,0x7000,0
    cpu.put(0x2000,4,w0);cpu.put(0x2004,4,w1)
    for offset,value in ((16,source_class),(20,target),(24,word),(28,identity_flags),(32,extra)):
        cpu.put(0x7000+offset,4,value)
    cpu.put(0x80098798,4,mask)
    cpu.put(target,4,packet);cpu.put(target+4,4,flags)
    cpu.run(0x800341BC)
    assert cpu.r[2] == 1
    return {'flags_04':cpu.get(target+4,4),
            'record':[cpu.get(BANK+(cpu.get(COUNT,4)-1)*12+off,width)
                      for off,width in ((0,4),(4,4),(8,4))]}


def consume(cpu):
    cpu.r = [0]*32
    cpu.r[28],cpu.r[29],cpu.r[31] = 0x80097864,0x7000,0
    for entry in (0x80076A84,0x80034EB8,0x800358F0): cpu.hook(entry)
    cpu.run(0x8003450C)


def point(x,y,z,fourth=1234):
    return (x&65535)|((y&65535)<<16),(z&65535)|((fourth&65535)<<16)


def spec(p,a1,mode,cl,target,word,packet=0,flags=0,extra=128,mask=0xffffffff,identity=0x00800000):
    return (*point(*p),a1,0,mode<<13,cl,target,word,identity,extra,mask,flags,packet)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose NEW evidence outside source')
    image=args.exe.read_bytes();owner=OwnerExe(image)
    cases=[]
    for modea,modeb,rule in itertools.product(range(8),range(8),(0,255)):
        cases.append((spec((0,0,0),32,modea,32,TARGETS[0],0x08ff0001),
                      spec((0,0,0),32,modeb,96,TARGETS[1],0x10ff0002),rule))
    for delta,radius,axis in itertools.product((-32768,-129,-65,-64,-63,-33,-32,-31,-1,0,1,31,32,33,63,64,65,129,32767),
                                               (0,1,3,4,31,32,33,2044),(0,1,2)):
        p=[0,0,0];p[axis]=delta
        cases.append((spec((0,0,0),radius,4,32,TARGETS[0],0x08ff0001),
                      spec(p,radius,5,96,TARGETS[1],0x10ff0002),255))
    for worda,wordb,packet,identity,flags,mask in itertools.product(
            (0,0x08800001,0x10010080,0x38ffFFFF),(0,0x08010001,0x2080FFFF),
            (0,0x00ffffff,0xf8ffffff),(0,0x00800000),(0,0x800000ff),(0,0xffffffff)):
        cases.append((spec((7,-8,9),32,0,32,TARGETS[0],worda,packet,flags,mask=mask,identity=identity),
                      spec((7,-8,9),32,7,96,TARGETS[1],wordb,packet,flags,mask=mask,identity=identity),255))
    fixture=[];details=[];steps=0
    for a,b,rule in cases:
        cpu=Probe(image,owner.base)
        cpu.put(COUNT,4,0)
        for i in range(24):
            cpu.put(0x800BE478+i*4,4,0x3000+i*0x100)
        # Authored rule table, not exported retail bytes.
        for i in range(8): cpu.put(0x80097D00+i,1,rule)
        rows=[register(cpu,a),register(cpu,b)]
        owners_before=[[cpu.get(0x3000+i*0x100+0xBC+j*4,4) for j in range(2)] for i in range(24)]
        consume(cpu)
        after=[]
        for target in TARGETS:
            after.extend([cpu.get(target,4),cpu.get(target+4,4),
                          cpu.get(target+8,2,True),cpu.get(target+10,2,True),
                          cpu.get(target+12,2,True),cpu.get(target+14,2)])
        encoded=[]
        for row in rows: encoded.extend(row['record'])
        # Original registration copies second point to each high-bit-selected owner+BC.
        owner_hash=hashlib.sha256(json.dumps(owners_before).encode()).hexdigest()
        fixture.append(' '.join(map(str,(*a,*b,rule,*encoded,*after,cpu.get(COUNT,4),
                                         *owners_before[0]))))
        details.append({'authored_inputs':[*a,*b,rule],'registration':rows,
                        'target_outputs':after,'final_count':cpu.get(COUNT,4),
                        'owner_bc_points_sha256':owner_hash,'instructions':cpu.steps})
        steps+=cpu.steps
    # Separate source checks for aliasing/order and the pre-argument cap guard.
    extra_checks=[]
    cpu=Probe(image,owner.base);cpu.put(COUNT,4,0)
    for i in range(8): cpu.put(0x80097D00+i,1,255)
    register(cpu,spec((0,0,0),32,0,32,0x1000,0x08000001))
    register(cpu,spec((0,0,0),32,0,96,0x1000,0x10000002))
    consume(cpu)
    assert cpu.get(0x1000,4)==0x18000003
    extra_checks.append({'case':'shared_target_pair_order','packet':cpu.get(0x1000,4),
                         'instructions':cpu.steps})
    steps+=cpu.steps
    cpu=Probe(image,owner.base);cpu.put(COUNT,4,64)
    cpu.r[29],cpu.r[31]=0x7000,0
    before_count=cpu.get(COUNT,4);cpu.run(0x800341BC)
    assert cpu.r[2]==0 and cpu.get(COUNT,4)==before_count
    extra_checks.append({'case':'full_bank_no_argument_side_effect','return':cpu.r[2],
                         'instructions':cpu.steps})
    steps+=cpu.steps
    cpu=Probe(image,owner.base);cpu.put(COUNT,4,0)
    for i in range(8):cpu.put(0x80097D00+i,1,255)
    for i,(cl,word) in enumerate(((32,0x08000001),(96,0x10000002),(64,0x20000003))):
        register(cpu,spec((i*32,0,0),32,0,cl,0x1000+i*0x100,word))
    consume(cpu)
    packets=[cpu.get(0x1000+i*0x100,4) for i in range(3)]
    assert packets==[0x30000005,0x28000004,0x18000003]
    extra_checks.append({'case':'three_target_ordered_all_pairs','packets':packets,
                         'instructions':cpu.steps})
    steps+=cpu.steps
    data=('\n'.join(fixture)+'\n').encode()
    package=build_metadata(image)
    assert package[24:]==owner.read(0x80097D00,8)
    result={'schema':'mml-hit-proxy-source/1','status':'SOURCE_BACKED_PARTIAL',
            'exe_sha256':OWNER_SHA,'registration':'0x800341BC',
            'dispatcher':'0x8003450C','pair':'0x800346AC',
            'registration_code_sha256':hashlib.sha256(owner.read(0x800341BC,0x150)).hexdigest(),
            'pair_code_sha256':hashlib.sha256(owner.read(0x8003450C,0x9AC)).hexdigest(),
            'cases':details,'case_count':len(cases),'extra_original_checks':extra_checks,'instructions_executed':steps,
            'fixture_sha256':hashlib.sha256(data).hexdigest(),
            'owner_mode_table_package_matches':True,'retail_payload_exported':False,
            'retail_parity':False,'limits':['Authored states, not a live Buster or attack encounter.',
            'Kind0 single-record shape only; composite kinds1..3 remain unsupported.',
            'Original registration and complete pair traversal/packet/point writes execute.',
            'Timing80076A84 and earlier player/GTE consumers80034EB8/800358F0 are explicit return hooks.',
            'Mode rule table supplied owner-locally; no retail constants compiled into source.']}
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
    (args.out/'authored.tsv').write_bytes(data)
    print(json.dumps({'cases':len(cases),'instructions':steps,'fixture_sha256':result['fixture_sha256']}))


if __name__=='__main__':main()
