"""Hash/address and aggregate placement evidence; never emits game byte payloads.

The small CPU evaluator exercises only the already identified preprojection
coordinate blocks using synthetic geometry and registers. It is not a game or
console emulator, and establishes neither runtime fidelity nor retail parity.
"""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
import argparse

from build_stage_catalog import iter_resources

def sha(b):return hashlib.sha256(b).hexdigest()
def signed16(v):return v-65536 if v&32768 else v
def signed32(v):return v-4294967296 if v&2147483648 else v
def signed8(v):return v-256 if v&128 else v

class SyntheticCPU:
    def __init__(self,image):self.image=image;self.reg=[0]*32;self.mem={};self.executed=0;self.gte_skipped=0
    def put(self,a,n,v):
        for i in range(n):self.mem[a+i]=(v>>(i*8))&255
    def get(self,a,n,signed=False):
        v=sum(self.mem.get(a+i,0)<<(i*8) for i in range(n))
        return v-(1<<(8*n)) if signed and v&(1<<(8*n-1)) else v
    def block(self,start,end):
        pc=start; pending=None
        while pc<end:
            if self.executed>1000:raise ValueError('synthetic CPU step bound exceeded')
            previous_target=pending;pending=None
            w=struct.unpack_from('<I',self.image,pc-0x80010000+2048)[0]
            op,rs,rt,rd,sh,fn=w>>26,(w>>21)&31,(w>>16)&31,(w>>11)&31,(w>>6)&31,w&63
            imm=w&65535;si=signed16(imm);a,b=self.reg[rs],self.reg[rt];v=None;dest=rt
            if w==0:pass
            elif op==0:
                dest=rd
                if fn==0:v=b<<sh
                elif fn==2:v=b>>sh
                elif fn==3:v=signed32(b)>>sh
                elif fn==4:v=b<<(a&31)
                elif fn==6:v=b>>(a&31)
                elif fn==7:v=signed32(b)>>(a&31)
                elif fn==33:v=a+b
                elif fn==35:v=a-b
                else:raise ValueError(f'unsupported synthetic CPU operation at {pc:x}')
            elif op==9:v=a+si
            elif op==12:v=a&imm
            elif op==15:v=imm<<16
            elif op in (35,36,37):v=self.get((a+si)&0xffffffff,{35:4,36:1,37:2}[op])
            elif op in (41,43):self.put((a+si)&0xffffffff,2 if op==41 else 4,b)
            elif op in (4,5):
                if (a==b)==(op==4):pending=pc+4+si*4
            elif op==2:pending=((pc+4)&0xf0000000)|((w&0x3ffffff)<<2)
            elif op==50:pass # GTE input is irrelevant to subsequent CPU coordinate gathering.
            elif op==18 and rs>=16:self.gte_skipped+=1
            else:raise ValueError(f'unsupported synthetic CPU operation at {pc:x}')
            if v is not None and dest:self.reg[dest]=v&0xffffffff
            self.reg[0]=0;self.executed+=1
            pc=previous_target if previous_target is not None else pc+4

FAMILIES=[
    {'entry':0x80023ea8,'start':0x80023fa0,'end':0x800240c8,'pointer':19,'x':11,'y':10,'z':30,'shift':18},
    {'entry':0x80024f30,'start':0x80025030,'end':0x80025158,'pointer':20,'x':10,'y':30,'z':23,'shift':18},
    {'entry':0x80025900,'start':0x800259f8,'end':0x80025b20,'pointer':19,'x':11,'y':10,'z':30,'shift':18},
    {'entry':0x80026b04,'start':0x80026bfc,'end':0x80026d24,'pointer':19,'x':11,'y':10,'z':30,'shift':18},
]

def probe(image):
    count=0;details=[]
    for f in FAMILIES:
        instructions=0
        for seed in range(8):
            raw=bytes((seed*31+i*47+128)&255 for i in range(12))
            for shift in (0,1,3,5):
                c=SyntheticCPU(image);c.reg[16]=0x1f800100;c.reg[29]=0x1f800800
                c.reg[f['pointer']]=0x100004;c.reg[f['x']]=1000;c.reg[f['y']]=2000;c.reg[f['z']]=3000;c.reg[f['shift']]=shift
                for i,b in enumerate(raw):c.put(0x100004+i,1,b)
                c.block(f['start'],f['end']);instructions=c.executed
                for vertex in range(4):
                    expected=[1000+(signed8(raw[3*vertex])<<shift),2000-(raw[3*vertex+1]<<shift),3000+(signed8(raw[3*vertex+2])<<shift)]
                    actual=[c.get(0x1f800100+152+8*vertex+2*axis,2,True) for axis in range(3)]
                    expected=[signed16(x&65535) for x in expected]
                    if actual!=expected:raise ValueError(f'coordinate mismatch family={f["entry"]:x} seed={seed} shift={shift}')
                count+=1
        details.append({'entry':hex(f['entry']),'block_start':hex(f['start']),'block_end_exclusive':hex(f['end']),
                        'block_sha256':sha(image[f['start']-0x80010000+2048:f['end']-0x80010000+2048]),'synthetic_cases':32,'instructions_per_case':instructions})
    return {'status':'SOURCE_BACKED_PARTIAL','method':'Execute four bounded original CPU gather blocks on synthetic geometry; GTE projections ignored. Halfword stores compared exactly.',
            'synthetic_cases':count,'cases_passed':count,'families':details,
            'limitation':'A coordinate decode proof over synthetic inputs, not retail gameplay or full renderer execution.'}

def negative_probe(image):
    families=[{'entry':0x80021b14,'start':0x80021c3c,'end':0x80021e6c,'base':7,'pointer':25,'x':13,'y':24,'z':12,'shift':11,'flags':6},
              {'entry':0x8002235c,'start':0x8002246c,'end':0x8002269c,'base':16,'pointer':22,'x':20,'y':30,'z':19,'shift':18,'flags':23}]
    count=0;details=[]
    for f in families:
        for seed in range(8):
            raw=bytes((seed*31+i*47+128)&255 for i in range(12))
            for shift in (0,1,3,5):
                for rotation in range(4):
                    c=SyntheticCPU(image);c.reg[f['base']]=0x1f800130;c.reg[29]=0x1f800800
                    c.reg[f['pointer']]=0x100004;c.reg[f['x']]=1000;c.reg[f['y']]=2000;c.reg[f['z']]=3000;c.reg[f['shift']]=shift;c.reg[f['flags']]=0x8000|rotation
                    if f['entry']==0x80021b14:c.reg[15]=0x100000 # t7 points four bytes before t9.
                    for i,b in enumerate(raw):c.put(0x100004+i,1,b)
                    c.block(f['start'],f['end'])
                    for vertex in range(4):
                        x,z=signed8(raw[3*vertex])<<shift,signed8(raw[3*vertex+2])<<shift
                        if rotation==1:x,z=z,-x
                        elif rotation==2:x,z=-x,-z
                        elif rotation==3:x,z=-z,x
                        expected=[signed16((1000+x)&65535),2000,signed16((3000+z)&65535)]
                        actual=[c.get(0x1f800130+112+8*vertex+2*axis,2,True) for axis in range(3)]
                        if actual!=expected:raise ValueError(f'negative coordinate mismatch family={f["entry"]:x} seed={seed} shift={shift} rot={rotation} actual={actual} expected={expected}')
                    count+=1
        details.append({'entry':hex(f['entry']),'block_start':hex(f['start']),'block_end_exclusive':hex(f['end']),
                        'block_sha256':sha(image[f['start']-0x80010000+2048:f['end']-0x80010000+2048]),'synthetic_cases':128})
    return {'status':'SOURCE_BACKED_PARTIAL','method':'Execute two bounded original negative flat-coordinate CPU gather blocks with four quarter turns and synthetic geometry; branch delay slots honored, GTE projection ignored.',
            'synthetic_cases':count,'cases_passed':count,'families':details,'limitation':'Source instruction block probes only; no retail parity or renderer execution.'}

def measurements(resources):
    payload={r['internal'].upper().rsplit('.',1)[-1]:r for r in resources if r['type']==0 and r['internal'].upper().endswith(('.MDT','.IDX','.STG','.HED'))}
    mdt,idx,stg,hed=[payload[k]['payload'] for k in ('MDT','IDX','STG','HED')]
    idxs=[struct.unpack_from('<hhhHI',idx,o) for o in range(0,len(idx),12)]
    descriptor_ptrs={p for _,_,_,_,p in idxs if p}
    descriptor_ptrs.update(struct.unpack_from('<II',hed,o)[1] for o in range(0,len(hed),8) if struct.unpack_from('<II',hed,o)[1])
    areas=[];total=Counter()
    for a in range(struct.unpack_from('<I',mdt)[0]):
        sx,sz,w,h,p=struct.unpack_from('<4BI',mdt,4+a*8)
        cells=struct.unpack_from('<'+str(w*h)+'h',mdt,p-0x80164000)
        pos=Counter(c for c in cells if c>0);neg=[c&65535 for c in cells if c<0]
        b=Counter('secondary' if idxs[i][3]==0 else 'primary' for i in pos)
        rotations=Counter(str(c&3) for c in neg);special=Counter('table_bit7' if c&128 else 'default_stg' for c in neg)
        total.update(special)
        areas.append({'area':a,'positive_cell_occurrences':sum(pos.values()),'positive_unique_idx':len(pos),
          'repeated_positive_occurrences_avoided':sum(pos.values())-len(pos),'positive_bank_unique_idx_counts':dict(b),
          'negative_cells':len(neg),'negative_quarter_turn_counts':dict(sorted(rotations.items())),
          'negative_descriptor_branch_counts':dict(special),'negative_special_table_slot_counts':dict(sorted(Counter(str((c&0x7c)>>2) for c in neg if c&128).items()))})
    bankrecords=Counter();shifts=Counter();y=Counter();records_seen=set();streams_seen=set()
    for p in descriptor_ptrs:
        q=p-0x80194000
        if not 0<=q<=len(stg)-48:continue
        for slot in range(6):
            rp=struct.unpack_from('<I',stg,q+24+slot*4)[0]
            bankrecords[f'{"primary" if slot<3 else "secondary"}_{slot%3}_present']+=bool(rp)
            if not rp or rp in records_seen:continue
            records_seen.add(rp);ro=rp-0x80194000
            mc,flags=stg[ro],stg[ro+1];pc=flags&63
            for s in range(pc):
                uw,gw=struct.unpack_from('<II',stg,ro+4+mc*4+s*8)
                if not uw or not gw or (uw,gw) in streams_seen:continue
                streams_seen.add((uw,gw));shift=uw>>28;count=gw>>24;shifts[str(shift)]+=count
                go=(gw&0xffffff)|0x80000000;go-=0x80194000
                for v in range(count):
                    raw=stg[go+v*16+4:go+v*16+16]
                    y['positive_y_high_bit_set_bytes']+=sum(bool(raw[i]&128) for i in (1,4,7,10))
                    y['positive_y_nonzero_bytes']+=sum(raw[i]!=0 for i in (1,4,7,10))
    return {'resource_hashes':{k:{'sha256':sha(r['payload']),'bytes':len(r['payload']),'load_address':hex(r['load'])} for k,r in payload.items()},
            'idx_rows':len(idxs),'idx_raw6_zero_rows':sum(row[3]==0 for row in idxs),
            'idx_raw6_nonzero_rows':sum(row[3]!=0 for row in idxs),'descriptor_count':len(descriptor_ptrs),
            'descriptor_slot_presence':dict(sorted(bankrecords.items())),'stored_primitive_shift_counts':dict(sorted(shifts.items())),
            'stored_y_byte_diagnostics':dict(y),'areas':areas,'negative_descriptor_branch_totals':dict(total)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--container',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists() or args.output.is_symlink():raise ValueError('evidence output already exists')
    exe=args.exe;image=exe.read_bytes()
    if sha(image)!='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0':
        raise ValueError('unsupported executable identity; the bounded address probes require the supplied USA profile')
    if image[:8]!=b'PS-X EXE':raise ValueError('not PS-X EXE')
    cp=args.container
    if sha(cp.read_bytes())!='3f46c6bac9261ee1def37bddd8a91377343e5ee1235391b2ed3a18f2cbcc8dc0':raise ValueError('unsupported tutorial container identity')
    resources=list(iter_resources(cp))
    result={'format':'terrain-placement-source-evidence/1','status':'SOURCE_BACKED_PARTIAL',
      'executable':{'sha256':sha(image),'bytes':len(image),'load_address':'0x80010000','entry':'0x80068000'},
      'container':{'sha256':sha(cp.read_bytes()),'bytes':cp.stat().st_size},
      'coordinate_block_probe':probe(image),'negative_coordinate_block_probe':negative_probe(image),'data_measurements':measurements(resources),
      'findings':[
        {'id':'POSITIVE_VISIBILITY_IDENTITY','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80021914','0x80021938','0x80021944','0x80021958','0x80024c1c','0x80024c34','0x80024cec','0x80024cf0'],
         'meaning':'Positive full IDX index sets a visibility bit. Drawing iterates bits once and advances the IDX pointer by12. Repeated cell occurrences reference one placed IDX object. Positive low bits are part of identity, not quarter-turn flags.'},
        {'id':'POSITIVE_IDX_ORIGIN','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80024c3c','0x80024c40','0x80024c44','0x80024c48','0x80024c50','0x80023f4c','0x80023f60','0x80023f74'],
         'meaning':'Positive draw copies first8 IDX bytes to scratch, then reads signed halfwords X,Y,Z at scratch320,322,324. Grid centers are visibility-scan locations only.'},
        {'id':'POSITIVE_BANK','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80024c90','0x80024cb0','0x80024cb4','0x80024cbc','0x80024cc4','0x80024cc8','0x800268b4'],
         'meaning':'LOD uses positive transformed depth thresholds2801/7201. raw6 is loaded as unsigned16 at scratch70 and tested as whole16bits: nonzero descriptor+24/28/32, zero adds12 and selects+36/40/44. Current ST00 has zero in all995 IDX raw6 fields.'},
        {'id':'POSITIVE_COORDINATE_GATHER','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80023fa0','0x80023fc0','0x80023fcc','0x80024030','0x80024038','0x800240a4','0x800240b0','0x80025030','0x800259f8','0x80026bfc'],
         'meaning':'Consecutive four triplets across12 compressed bytes: X/Z signed8; Y unsigned8. X/Z add to IDX origin, Y subtracts from IDX origin. No quarter-turn in any of four inspected positive primitive families.'},
        {'id':'POSITIVE_SCALE','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80023f08','0x80023f20','0x80023f50','0x80023f6c','0x80023f78','0x80023f80','0x80013344','0x8001f4c0'],
         'meaning':'Local coordinate shift is UVword high nibble plus unsigned scratch byte0x1f800074. Origin X/Y/Z additionally shift by scratch byte. Startup and a conditional stage path explicitly reset scratch byte to0.'},
        {'id':'NEGATIVE_FLAT_GEOMETRY','status':'SOURCE_BACKED_PARTIAL','anchors':['0x800218fc','0x80021904','0x80021978','0x80021990','0x80021c3c','0x80021c58','0x80021d70','0x8002246c'],
         'meaning':'Negative cells use gridcenter=(grid<<9)-0x7f00, flatY=-((packed&0x1f00)>>1). Distinct primitive path consumes X={b0,b3,b6,b9},Z={b2,b5,b8,b11} relative to12 primitive bytes and ignores Y={b1,b4,b7,b10}, with packed low2quarter-turns. The inherited cooked cross-gather is historically incorrect; retainedbytes permit exact inversion.'},
        {'id':'NEGATIVE_DESCRIPTOR','status':'SOURCE_BACKED_PARTIAL','anchors':['0x800219bc','0x800219cc','0x800219d8','0x800219e4','0x80022270','0x8002228c'],
         'meaning':'Negative packed bit7 selects pointer table0x80139f04+((packed<<1)&0xf8); otherwise descriptorbase0x80194000. Table population/original source identity unresolved. Selected negative LOD is primary slots0..2.'},
        {'id':'POSITIVE_MATERIAL_SLOT','status':'SOURCE_BACKED_PARTIAL','anchors':['0x80023f00','0x80023f04','0x80023f18','0x80024f88','0x80024f8c'],
         'meaning':'Stream UVword material index=(UVword>>24)&0xf; read copied materialword at scratch456+4*index. Source pointer words and primitive prefixes stay retained metadata.'},
      ],
      'unknowns':['Negative special pointer-table population','Negative material override tables and upload lifetime','Prefix-controlled alpha,culling,subdivision beyond preserved flag bits','Dynamic globalshift mutations and ownership beyond inspected reset sites','Old3DS hardware behavior and full original-game rendering parity'],
      'payload_policy':'Hashes, addresses, semantics and aggregate counts only. No gamebytes, disassembly, textures, models or owner saves emitted.'}
    out=args.output
    if out.exists():raise ValueError('evidence output already exists')
    with out.open('x') as stream:stream.write(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'output':str(out),'status':result['status'],'positive_cases':result['coordinate_block_probe']['cases_passed'],'negative_cases':result['negative_coordinate_block_probe']['cases_passed']},indent=2))

if __name__=='__main__':main()
