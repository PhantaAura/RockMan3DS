#!/usr/bin/env python3
"""Private unchanged-instruction oracle for the source scene caller tail.

Only owner-local images and authored state. No hooks or instruction patches.
The type10 fade is stopped at its entry; no completion is fabricated.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE/'tools'), str(SOURCE/'docs/research_01415'),
                str(SOURCE/'docs/research_01414')]
from cook_actor_combat import OwnerExe, OWNER_SHA
from tutorial_actor_identity_probe import OverlayProbe, OVERLAY_SHA, iter_resources

WORK = 0x800c4c48
FLAGS = 0x800be378
INPUT = 0x800c0c2a
STAGE = 0x800c1b6a
POOL = 0x80200000
OT = 0x80201000

class Probe(OverlayProbe):
    def __init__(self, image, base, overlay):
        super().__init__(image, base, overlay)
        self.watch_reads = []
    def get(self, address, size, signed=False):
        if address in (INPUT, STAGE) or FLAGS <= address < FLAGS+144:
            self.watch_reads.append([address, size])
        return super().get(address, size, signed)

def fill(cpu, address, data):
    for i, value in enumerate(data): cpu.put(address+i, 1, value)
def read(cpu, address, n): return bytes(cpu.get(address+i, 1) for i in range(n))
def pattern(seed, n): return bytes((seed*29+i*17+11)&255 for i in range(n))
def setup(cpu, seed, event, stage, flag, inputs):
    cpu.r[29] = 0x700000
    cpu.put(WORK+1, 1, event)
    cpu.put(STAGE, 1, stage)
    fill(cpu, FLAGS, pattern(seed, 144))
    index = 896+event
    address, mask = FLAGS+index//8, 128>>(index&7)
    old = cpu.get(address, 1)
    cpu.put(address, 1, (old|mask) if flag else (old&~mask))
    cpu.put(INPUT, 2, inputs)
    cpu.watch_reads.clear()
def prepare_packet(cpu, seed):
    fill(cpu, POOL, pattern(seed, 80))
    fill(cpu, OT, pattern(seed+1, 128))
    cpu.put(0x1f800070, 4, POOL)
    cpu.put(0x80098934, 4, OT)
    cpu.put(0x80097718, 1, seed%4)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe','st00','out'): parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists() or SOURCE == args.out.resolve() or SOURCE in args.out.resolve().parents:
        raise ValueError('Choose a new evidence directory outside source')
    image = args.exe.read_bytes(); owner = OwnerExe(image)
    overlay = next(r['payload'] for r in iter_resources(args.st00)
                   if r['load']==0x80100000 and 'PROGBIN' in r['internal'].upper())
    assert hashlib.sha256(overlay).hexdigest() == OVERLAY_SHA
    predicate = []; colors = []; callers = []; total = 0
    for seed in range(192):
        event = (0,1,7,8,31,255)[seed%6]
        stage = (0,1,128,255)[seed//6%4]
        flag = seed//24%2
        inputs = (0,8,0xffff,0xfff7)[seed//48%4]
        cpu = Probe(image, owner.base, overlay)
        setup(cpu, seed, event, stage, flag, inputs)
        before = read(cpu, FLAGS, 144); cpu.watch_reads.clear()
        cpu.run(0x8001de84)
        assert cpu.r[2] == int((stage!=0 or flag!=0) and (inputs&8)!=0)
        assert before == read(cpu, FLAGS, 144)
        input_reads = sum(a==INPUT for a,n in cpu.watch_reads)
        assert input_reads == int(stage!=0 or flag!=0)
        assert not cpu.return_only_hooks
        predicate.append([seed,event,stage,flag,inputs,cpu.r[2],input_reads,cpu.steps])
        total += cpu.steps
    for seed in range(48):
        rgb = [(seed*67+0x100*i)&0xffffffff for i in range(3)]
        mode = (0,1,2,3,0xffff,0x10002)[seed%6]
        cpu = Probe(image, owner.base, overlay); cpu.r[29] = 0x700000
        prepare_packet(cpu, seed)
        before = bytearray(read(cpu, POOL, 80)); tag = cpu.get(OT+116,4)
        cpu.r[4:8] = rgb+[mode]
        cpu.run(0x80100614)
        packet = read(cpu,POOL,80)
        expected = before[:]
        expected[0:4] = ((3<<24)|(tag&0xffffff)).to_bytes(4,'little')
        expected[4:8] = bytes([v&255 for v in rgb]+[0x62])
        expected[8:12] = bytes(4)
        expected[12:16] = (320).to_bytes(2,'little')+(240).to_bytes(2,'little')
        expected[40:44] = ((2<<24)|(POOL&0xffffff)).to_bytes(4,'little')
        tpage = (((mode&0xffff)<<5)|15)&0xffff
        mask = 0x27ff if seed%4 in (1,2) else 0x9ff
        gp0 = 0xe1000000|(tpage&mask)
        expected[44:48] = gp0.to_bytes(4,'little')
        expected[48:52] = bytes(4)
        assert packet == expected, (seed,packet.hex(),expected.hex())
        assert cpu.get(0x1f800070,4) == POOL+80
        assert cpu.get(OT+116,4) == (tag&0xff000000)|((POOL+40)&0xffffff)
        assert not cpu.return_only_hooks
        colors.append([seed,*rgb,mode,seed%4,packet.hex(),cpu.get(OT+116,4),cpu.steps])
        total += cpu.steps
    for seed in range(64):
        event = 1; stage = (0,1,255,0)[seed%4]; flag = seed//4%2
        inputs = (0,8)[seed//8%2]; busy = (0,1,0xffff,0)[seed//16%4]
        forced = seed%7 == 0
        enabled = seed%3 != 0
        cpu = Probe(image,owner.base,overlay); setup(cpu,seed,event,stage,flag,inputs)
        fill(cpu,WORK,pattern(seed,168)); cpu.put(WORK+1,1,event)
        cpu.put(WORK+4,1,6); cpu.put(WORK+9,1,19 if forced else 0)
        cpu.put(WORK+16,4,44 if forced else seed)
        cpu.r[16] = WORK; prepare_packet(cpu,seed)
        cpu.put(0x80098a72,2,busy)
        cpu.put(0x80122428,1,int(enabled))
        for i,value in enumerate((64,128,255,2)): cpu.put(0x8012242c+4*i,1,value)
        before = read(cpu,WORK,168)
        enters_fade = busy==0 and (forced or ((stage!=0 or flag!=0) and (inputs&8)!=0))
        cpu.run(0x801011e8,0x80017c30 if enters_fade else None)
        assert read(cpu,WORK,168) == before
        assert not cpu.return_only_hooks
        if enters_fade:
            assert cpu.r[4] == 10
            assert cpu.get(0x1f800070,4) == POOL
        else:
            assert cpu.get(0x1f800070,4) == POOL+(80 if enabled else 0)
            assert sum(call['target']=='0x80100614' for call in cpu.calls) == int(enabled)
        callers.append([seed,busy,stage,flag,inputs,int(forced),int(enabled),int(enters_fade),cpu.steps])
        total += cpu.steps
    args.out.mkdir(parents=True)
    fixtures = {}
    for name,header,rows in (
        ('predicate','seed event stage flag inputs result input_reads steps',predicate),
        ('color','seed r g b mode gpu_variant packet ot116 steps',colors),
        ('caller','seed busy stage flag inputs forced enabled needs_type10 steps',callers)):
        data = ('\t'.join(header.split())+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode()
        (args.out/(name+'.tsv')).write_bytes(data)
        fixtures[name] = {'cases':len(rows),'sha256':hashlib.sha256(data).hexdigest()}
    proof = {'schema':'mml-source-event-caller-tail/1','status':'SOURCE_BACKED_PARTIAL',
             'owner_exe_sha256':OWNER_SHA,'overlay_sha256':OVERLAY_SHA,
             'unchanged_original_instructions':total,'instruction_patches':0,'helper_hooks':0,
             'fixtures':fixtures,'predicate_short_circuit_verified':True,
             'color_tile':{'width':320,'height':240,'opcode':0x62,'allocator_bytes':80,
                           'draw_mode2_word':0xe100004f,'ordering_table_byte_offset':116},
             'type10_source_entry_only':True,'phase_completion_acknowledged':False,
             'live_retail_parity':False,'hardware_tested':False,
             'limits':['Authored inputs and packet buffers are not live retail state.',
                       'Input bit8 meaning is not inferred from its test.',
                       'Type10 fade source entry is reached but not executed by this probe.',
                       'Caller false/ busy branches preserve phase6; no scene completion is acknowledged.']}
    (args.out/'source-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))

if __name__ == '__main__': main()
