# 0x8002786C mesh-set choice (no retail data: the owner EXE reader is stubbed).
import struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import source_player_arms as arms
assert [arms.mesh_set_index(*x) for x in ((0,0),(5,0),(1,1),(0,1))]==[0,0,1,2]
sets=[(1,2,3,4,5),(1,2,6,7,5),(1,2,3,7,5)]
class Stub:
    def __init__(self,image):pass
    def read(self,address,size):
        i=(address-arms.MESH_SETS)//20;assert size==20 and address==arms.MESH_SETS+i*20
        return struct.pack('<5I',*sets[i])
arms.OwnerExe=Stub
by={a:'..\\PLBIN\\%s.PBD'%n for a,n in ((1,'B'),(2,'H'),(3,'R00'),(4,'L00'),(5,'F'),(7,'L04'))}
r=arms.select_player_meshes(b'',by,0,1)
assert r['set']==2 and r['resources']['left']=='L04.PBD' and r['resources']['right']=='R00.PBD'
assert arms.select_player_meshes(b'',by,0,0)['resources']['left']=='L00.PBD'
try: arms.select_player_meshes(b'',by,1,1); raise SystemExit('missing bank substituted')
except ValueError: pass
print('PASS 0x8002786C mesh set: tutorial loadout draws the Buster left arm')
