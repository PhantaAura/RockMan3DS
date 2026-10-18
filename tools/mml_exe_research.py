#!/usr/bin/env python3
"""Conservative MIPS-I control-flow/xref index. Outputs metadata, never instruction bytes.

A reached address is a candidate code location, not a proven function/meaning.
No disassembler guesses convert candidate structures or parameters to gameplay rules.
"""
from __future__ import annotations
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import struct

ANCHORS = {0x800980ac:'save_filename_pointer_table', 0x80098110:'save_filename_DASH00'}
LOAD_OPS = {0x20:'lb',0x21:'lh',0x22:'lwl',0x23:'lw',0x24:'lbu',0x25:'lhu',0x26:'lwr',
            0x28:'sb',0x29:'sh',0x2a:'swl',0x2b:'sw',0x2e:'swr'}


def signed(v):
    return v-65536 if v&0x8000 else v


class Image:
    def __init__(self,data):
        if len(data)<2048 or data[:8]!=b'PS-X EXE':
            raise ValueError('not a PS-X EXE')
        self.data=data
        self.entry, self.gp, self.base, self.size=struct.unpack_from('<4I',data,0x10)
        if self.size>len(data)-2048 or self.base+self.size>0x100000000 or self.base&3:
            raise ValueError('invalid text range')
        self.end=self.base+self.size

    def valid(self,addr):
        return self.base<=addr<=self.end-4 and addr%4==0

    def offset(self,addr):
        if not self.valid(addr):raise ValueError('address outside text')
        return addr-self.base+2048

    def word(self,addr):
        return struct.unpack_from('<I',self.data,self.offset(addr))[0]


def index_function(image,start,limit=4096):
    pending=deque([start]); seen=set(); calls=[]; memories=[]; indirect=[]; constants=[]; exits=[]
    while pending and len(seen)<limit:
        pc=pending.popleft()
        if pc in seen or not image.valid(pc):continue
        seen.add(pc)
        w=image.word(pc);op=w>>26;rs=(w>>21)&31;rt=(w>>16)&31;imm=w&0xffff
        if op in LOAD_OPS:
            memories.append({'site':hex(pc),'operation':LOAD_OPS[op],'base_register':rs,
                             'candidate_structure_offset':signed(imm),'status':'CANDIDATE'})
        # Adjacent LUI + ADDIU/ORI address constructions only, with no intervening definition.
        if op==15 and image.valid(pc+4):
            w2=image.word(pc+4);op2=w2>>26;rs2=(w2>>21)&31
            if rs2==rt and op2 in (9,13):
                val=((imm<<16)+(signed(w2&65535) if op2==9 else w2&65535))&0xffffffff
                if 0x80000000<=val<0x80200000 or 0x1f800000<=val<0x1f800400:
                    constants.append({'site':hex(pc),'value':hex(val),'anchor':ANCHORS.get(val),
                                      'status':'SOURCE_BACKED_PARTIAL'})
        if op in (2,3):
            target=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2)
            seen.add(pc+4) if image.valid(pc+4) else None
            if op==3:
                calls.append({'site':hex(pc),'target':hex(target),'kind':'jal'})
                pending.append(pc+8)
            else:
                # Preserve tail jumps as edges; don't consume another function body.
                exits.append({'site':hex(pc),'target':hex(target),'kind':'jump_or_tail_call'})
            continue
        if op==0 and (w&63) in (8,9):
            seen.add(pc+4) if image.valid(pc+4) else None
            if (w&63)==9:
                indirect.append({'site':hex(pc),'register':rs,'kind':'jalr'})
                pending.append(pc+8)
            elif rs!=31:
                indirect.append({'site':hex(pc),'register':rs,'kind':'jr'})
            continue
        branch = op in (4,5,6,7) or (op==1 and rt in (0,1,16,17)) or (op in (16,17,18,19) and rs==8)
        if branch:
            seen.add(pc+4) if image.valid(pc+4) else None
            target=pc+4+signed(imm)*4
            if op==1 and rt in (16,17):
                calls.append({'site':hex(pc),'target':hex(target),'kind':'branch_and_link'})
            else:
                pending.append(target)
            pending.append(pc+8)
            continue
        if op==0 and (w&63) in (12,13):
            continue
        pending.append(pc+4)
    # Delay slots contribute raw memory access evidence too, but not a second control transfer.
    known={m['site'] for m in memories}
    for pc in sorted(seen):
        w=image.word(pc);op=w>>26
        if op in LOAD_OPS and hex(pc) not in known:
            memories.append({'site':hex(pc),'operation':LOAD_OPS[op],'base_register':(w>>21)&31,
                             'candidate_structure_offset':signed(w&65535),'status':'CANDIDATE'})
    return {'address':hex(start),'file_offset':image.offset(start),'status':'CANDIDATE',
            'semantic_label':None,'callers':[],'callees':calls,'tail_edges':exits,
            'referenced_global_candidates':constants,'memory_operands':memories,
            'indirect_control_flow':indirect,'visited_instruction_count':len(seen),
            'bounded_scan_truncated':bool(pending),'confidence':'control-flow candidate; semantics unassigned'}


def research(exe,source):
    data=Path(exe).read_bytes();image=Image(data)
    evidence={}
    for p in sorted(Path(source).rglob('*')):
        if p.suffix not in ('.h','.c','.cpp') or not p.is_file():continue
        for lineno,line in enumerate(p.read_text(errors='replace').splitlines(),1):
            for word in re.findall(r'0x800[0-9a-fA-F]{5}',line):
                a=int(word,16)
                if image.valid(a):
                    evidence.setdefault(a,[]).append({'file':str(p.relative_to(source)),'line':lineno})
    # Source addresses can name data/instruction interiors, so retain separately from entry-root scan.
    starts=deque([image.entry]);functions={}
    while starts and len(functions)<12000:
        start=starts.popleft()
        if start in functions or not image.valid(start):continue
        f=index_function(image,start);functions[start]=f
        for c in f['callees']+f['tail_edges']:
            target=int(c['target'],16)
            if image.valid(target):starts.append(target)
    for f in functions.values():
        f['source_references']=evidence.get(int(f['address'],16),[])
        for c in f['callees']:
            t=functions.get(int(c['target'],16))
            if t is not None:t['callers'].append({'function':f['address'],'site':c['site']})
    # Add explicitly source-referenced candidates without pretending those are entry-reachable.
    isolated=[]
    for addr,refs in sorted(evidence.items()):
        if addr not in functions:
            f=index_function(image,addr,1024);f['source_references']=refs
            f['warning']='Source address may be data or an interior block, not a function start'
            isolated.append(f)
    ptr_refs=[]
    for addr in range(image.base,image.end-3,4):
        value=image.word(addr)
        if value in ANCHORS:
            ptr_refs.append({'address':hex(addr),'file_offset':image.offset(addr),
                             'target':hex(value),'label':ANCHORS[value], 'status':'MATCHES_MEASURED_FIELDS'})
    anchors=[]
    for addr,label in ANCHORS.items():
        item={'address':hex(addr),'symbol':label,'status':'SOURCE_BACKED_PARTIAL'}
        if image.valid(addr):
            item['file_offset']=image.offset(addr)
            if addr==0x80098110:
                b=data[image.offset(addr):image.offset(addr)+64].split(b'\0')[0]
                item['expected_filename_match']=b.endswith(b'DASH00')
        anchors.append(item)
    return {'format':'mml-executable-evidence/1','source_sha256':hashlib.sha256(data).hexdigest(),
            'entry':hex(image.entry),'text_load':hex(image.base),'text_bytes':image.size,
            'method':'MIPS-I bounded CFG from entry, direct calls and tail edges; separate source anchors',
            'limitations':['indirect targets unresolved','source anchors may be interior blocks or data',
                          'no semantic or 1:1 claims','no liveness or alias proof for global candidates'],
            'functions':list(functions.values()),'source_candidates':isolated,
            'anchors':anchors,'anchor_pointer_references':ptr_refs}


def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('source',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();result=research(a.exe,a.source)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'function_candidates':len(result['functions']),
                      'source_candidates':len(result['source_candidates']),
                      'anchor_pointer_references':len(result['anchor_pointer_references'])}))
if __name__=='__main__':main()
