from pathlib import Path
import sys,struct,hashlib,json,argparse
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'research_01414'))
from player_contact_source_probe import CPU,s16,s32,SUPPORTED_EXE
parser=argparse.ArgumentParser(description='Hash-gated source floor-transform scalar probe; no complete slope movement parity')
parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args()
if args.out.exists() or args.out.with_suffix('.json').exists():raise ValueError('preserve earlier evidence; choose new output')
image=args.exe.read_bytes()
assert hashlib.sha256(image).hexdigest()==SUPPORTED_EXE
base=struct.unpack_from('<I',image,24)[0]
def div(a,b):
 if b==0 or(a==-2147483648 and b==-1):raise ValueError('source arithmetic trap')
 return (-1 if (a<0)!=(b<0) else 1)*(abs(a)//abs(b))
def mul(a,b):return s32((a*b)&0xffffffff)
def wrap(a):return s32(a&0xffffffff)
def expected(code,x0,x1,z0,z1,y0,y1,x,z,b):
 # Group enters after original classifier expands halfwords.
 x0,x1,z0,z1=[s16(v&65535) for v in (x0-b[1],x1-b[0],z0-b[3],z1-b[2])]
 if code in (8,9):span=z1-z0;delta=b[2]-b[3];distance=z1-z if code==8 else z-z0
 else:span=x1-x0;delta=b[0]-b[1];distance=x-x0 if code==10 else x1-x
 adjusted=wrap(y0-div(mul(y1-y0,delta),span))
 slope=wrap(adjusted+div(mul(wrap(y1-adjusted),distance),span))
 slope=max(y0,slope)
 return(s16((slope-b[5])&65535),s16((y1-b[4])&65535))
rows=[];cases=0
for code in range(8,12):
 for b in ((-32,32,-32,32,-160,0),(-60,60,-60,60,-128,0),(0,0,0,0,0,0),(-15,17,-31,7,-99,2)):
  for rect in ((-256,256,-512,0,-129,383),(-8000,-7424,-8192,-7680,-1,255),(-32700,-32200,-18000,-17400,-16380,-15900)):
   x0,x1,z0,z1,y0,y1=rect
   for xn,zn in ((1,1),(1,3),(2,2),(3,1),(3,3)):
    x=x0+(x1-x0)*xn//4;z=z0+(z1-z0)*zn//4
    c=CPU(image,base);c.r[29]=0x4000;c.r[4]=0x1000;c.r[5]=code
    ext=[x0-b[1],x1-b[0],z0-b[3],z1-b[2],y0,y1]
    for i,v in enumerate(ext):c.put(0x1000+2*i,2,v)
    for o,v in zip((0x126,0x124,0x12a,0x128,0x12c,0x12e),b):c.put(0x1f800000+o,2,v)
    c.put(0x1f8000c0,2,x);c.put(0x1f8000c4,2,z)
    c.run(0x8004a060)
    actual=(c.get(0x1008,2,True),c.get(0x100a,2,True))
    want=expected(code,*rect,x,z,b)
    if actual!=want:raise ValueError(str((code,b,rect,x,z,actual,want)))
    rows.append((code,*rect,x,z,*b,*actual));cases+=1
print('MATCHED',cases)
out=args.out
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text('# Source 0x8004A060; expanded group/classifier contract; no movement-response parity\n# code x0 x1 z0 z1 y0 y1 query_x query_z bounds_x0 bounds_x1 bounds_z0 bounds_z1 bounds_y0 bounds_y1 floor_y ceiling_y\n'+''.join('\t'.join(map(str,r))+'\n' for r in rows))
out.with_suffix('.json').write_text(json.dumps({'source_exe_sha256':SUPPORTED_EXE,'routine':'0x8004A060','source_range_sha256':hashlib.sha256(image[0x8004a060-base+2048:0x8004ac8c-base+2048]).hexdigest(),'cases':cases,'matched_fields':['transformed_floor_y','transformed_ceiling_y'],'fixture_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'limits':['synthetic bounded scalar transform; not movement solver or retail trajectories','requires classifier-expanded X/Z extents','unsupported degenerate/overflow-trap cases rejected','movement codes08..0B not claimed implemented']},indent=2)+'\n')
