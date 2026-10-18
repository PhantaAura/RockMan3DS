#!/usr/bin/env python3
"""Original hitconsumer801041B0 on authored fields, no asset/instruction export.

The original sound producer executes. The source angle getter is an explicit
authored return boundary, not a reconstructed Buster/collision producer.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SOURCE / 'tools'), str(SOURCE / 'docs/research_01415'),
               str(SOURCE / 'docs/research_01414')]
from tutorial_actor_identity_probe import OverlayProbe, SUPPORTED_EXE, OVERLAY_SHA, iter_resources
from player_contact_source_probe import CPU
from cook_actor_combat import OwnerExe, build_metadata

WORK = 0x1000
FIELDS = [(0x6E, 2, True), (0xB2, 2, True), (0x328, 2, False),
          (0x332, 2, True), (3, 1, False), (9, 1, False), (10, 1, False),
          (11, 1, False), (0x56, 2, False), (0x36C, 2, True), (0x15E, 2, False)]


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Probe(OverlayProbe):
    def __init__(self, image, base, overlay):
        # Keep immutable source bytes separate from sparse authored memory.
        # The inherited instruction executor itself is unchanged.
        CPU.__init__(self, image, base)
        self.overlay = overlay
        self.calls, self.indirect, self.return_only_hooks = [], [], []
        self.sound_arguments, self.angle_arguments = [], []

    def get(self, address, size, signed=False):
        if size == 4 and address == 0x8001A050:
            self.sound_arguments.append(self.r[4:6])
        if size == 4 and address == 0x80031198:
            self.angle_arguments.append(self.r[4:6])
        if 0x80100000 <= address and address + size <= 0x80100000 + len(self.overlay):
            return int.from_bytes(self.overlay[address - 0x80100000:address - 0x80100000 + size],
                                  'little', signed=signed)
        return CPU.get(self, address, size, signed)


def seed(cpu, spec):
    packet, hp, flags, counter, variant, action, phase, aux, heading, angle, word34a, byte09 = spec
    cpu.r[4], cpu.r[29], cpu.r[31] = WORK, 0x7000, 0
    for offset, width, value in ((1,1,0x20),(2,1,3),(3,1,0xFF),(9,1,action),
            (10,1,phase),(11,1,aux),(0x56,2,heading),(0x6E,2,hp),
            (0xB2,2,-73),(0x15E,2,16912),(0x328,2,flags),(0x32E,2,variant),
            (0x332,2,counter),(0x33C,4,packet),(0x34A,2,word34a),(0x36C,2,-117)):
        cpu.put(WORK + offset, width, value)
    for i, value in enumerate((101,-7,-202,1234)):
        cpu.put(WORK + 0x14 + i * 2, 2, value)
    for i, value in enumerate((-109,3,211)):
        cpu.put(WORK + 0x344 + i * 2, 2, value)
    cpu.put(0x80098844,4,0x3000); cpu.put(0x300C,4,0x4000)
    cpu.put(0x4000 + variant * 16 + 9,1,byte09)
    cpu.put(0x80098938,4,0x9000)
    cpu.hook(0x80031198, angle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--dat',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args = parser.parse_args()
    if args.out.exists() or args.out.resolve().is_relative_to(SOURCE):
        raise ValueError('Choose NEW owner-local evidence outside source')
    image = args.exe.read_bytes()
    if sha(image) != SUPPORTED_EXE: raise ValueError('Unsupported owner executable')
    owner = OwnerExe(image)
    overlay = next(r['payload'] for r in iter_resources(args.dat / 'ST00.BIN')
                   if r['load'] == 0x80100000 and 'PROGBIN' in r['internal'].upper())
    if sha(overlay) != OVERLAY_SHA: raise ValueError('Unsupported tutorial overlay')
    specs = []
    for hp, flags, packet in itertools.product((-32768,-1,0,1,50,128,32767),
            (0,8,0xA,0x48), (0,1,0x08000001,0x10000010,0x20000010,0x38000080,0x08FFFFFF)):
        specs.append((packet,hp,flags,-7,0,0,23,29,4095,1024,1,127))
    for counter, variant, action, word, angle in itertools.product((-32768,-1,0,126,127,32767),
            (0,1),(0,5,6),(0,1),(0,1023,1024,2047,2048,4095,32768,65535)):
        specs.append((0x08000001,128,8,counter,variant,action,23,29,0,angle,word,127))
    for byte09 in (0,1,127,128,255):
        for hp in (1,2,128):
            specs.append((0x08000010,hp,0,-1,0,0,23,29,1024,4095,1,byte09))
    fixture = []
    cases = []
    instructions = 0
    for spec in specs:
        cpu = Probe(image,owner.base,overlay); seed(cpu,spec)
        before = [cpu.get(WORK + off,width,signed) for off,width,signed in FIELDS]
        cpu.run(0x801041B0)
        after = [cpu.get(WORK + off,width,signed) for off,width,signed in FIELDS]
        cues = [a[0] for a in cpu.sound_arguments]
        assert len(cues) <= 1
        assert cpu.get(WORK + 0x33C,4) == spec[0]  # Consumer does not clear packet.
        if cues:
            assert cpu.sound_arguments[0][1] == WORK + 0x14
            assert cpu.get(0x9002,2) == cues[0]
            assert cpu.get(0x9004,4) == ((101 & 65535) | ((-7 & 65535) << 16))
            assert cpu.get(0x9008,4) == ((-202 & 65535) | (1234 << 16))
        if cpu.angle_arguments:
            assert cpu.angle_arguments == [[WORK + 0x14 if spec[10] & 1 else 0x8011BF08, WORK + 0x344]]
        fixture.append(' '.join(map(str, list(spec) + after + [cues[0] if cues else -1])))
        cases.append({'authored_inputs': list(spec), 'before': before, 'after': after,
                      'sound_requests': cues, 'angle_arguments': cpu.angle_arguments,
                      'instructions': cpu.steps, 'original_calls': cpu.calls})
        instructions += cpu.steps
    # Verify owner selection and retained byte09 without publishing record values.
    packages,_ = build_metadata(owner); combat = packages['actor_combat.l3cb']
    assert struct.unpack_from('<H',combat,4)[0] == 3
    owner_matches = []
    for selector in (1,2):
        cpu = Probe(image,owner.base,overlay)
        cpu.r[28],cpu.r[29],cpu.r[31] = 0x80097864,0x7000,0
        cpu.put(0x800C1B6B,1,selector); cpu.run(0x80064718)
        table = cpu.get(0x80098844,4); record = cpu.get(table + 12,4)
        row = combat[32 + (selector-1)*12:44 + (selector-1)*12]
        assert row[10] == 3 and row[11] == owner.read(record+9,1)[0]
        owner_matches.append({'raw_selector':selector,'source_record':hex(record),
                              'retained_offset09_matches':True,'actor_variant32E':0})
    fixture_data = ('\n'.join(fixture)+'\n').encode()
    result = {'schema':'mml-class03-hit-source/1','status':'SOURCE_BACKED_PARTIAL',
              'exe_sha256':sha(image),'overlay_sha256':sha(overlay),
              'entry':'0x801041B0','end_exclusive':'0x80104470',
              'code_region_sha256':sha(overlay[0x41B0:0x4470]),
              'field_layout':FIELDS,'case_count':len(cases),'instructions_executed':instructions,
              'fixture_sha256':sha(fixture_data),'cases':cases,'owner_record_selection':owner_matches,
              'retail_parity':False,'retail_payload_exported':False,
              'limits':['Packet/HP/flags/counters/angles are authored states, not a live Buster encounter.',
                        'Original hitconsumer and original positional sound queue writer execute.',
                        'Angle getter80031198 is an explicit authored return boundary.',
                        'Outer hit dispatch, actual collision packet production, motion/reaction actions7/8 and final packet clear are not covered.',
                        'Only base records actorvariant32E0 are retained in native owner metadata.']}
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
    (args.out/'authored.tsv').write_bytes(fixture_data)
    print(json.dumps({'cases':len(cases),'instructions':instructions,
                      'fixture_sha256':sha(fixture_data),'owner_byte09_matches':True}))


if __name__ == '__main__': main()
