// Per-cell terrain visibility (0x80020BB4 / 0x80021258 / 0x80021548 /
// 0x80021834) on an authored 32x32 grid. Trig values are authored, not the
// retail table.
#include "legends3ds/render/terrain_cell_visibility.h"
#include "legends3ds/source_gameplay_camera.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
using namespace l3;
ReferenceTables refs(){
    ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096)),int16_t(std::lround(std::cos(a)*4096))};}
    for(size_t i=0;i<r.scalar_quarter.size();++i)r.scalar_quarter[i]=int16_t(std::lround(std::sin(double(i)*1.5707963267948966/1024.0)*4096));
    r.scalar_quarter_known=true;return r;
}
// Grid 0x30..0x4F on both axes; cell (gx, gz) has its own IDX 1 + local.
RuntimeMap grid(){
    RuntimeMap m;
    RuntimeMapArea a;a.start_x=0x30;a.start_z=0x30;a.width=32;a.height=32;a.cell_first=0;a.cell_count=1024;
    m.areas.push_back(a);
    m.idx.resize(1025);
    for(int i=0;i<1024;++i)m.cells.push_back(int16_t(1+i));
    return m;
}
int16_t idx_at(int gx,int gz){return int16_t(1+(gz-0x30)*32+(gx-0x30));}
TerrainCellViewInput view(const ReferenceTables& r,int32_t yaw,bool wide=true){
    const std::array<int32_t,3> t{{0,-150,0}};
    const auto f=source_gameplay_camera_80037fb4(r,t,512,yaw,0x30,0,0);
    TerrainCellViewInput in;
    in.camera_1f800008=f.camera_1f800008;
    in.translation_1f800030=terrain_cell_translation_1f800030(f.camera_1f800008,f.eye_800a39f0);
    in.eye_x_1f800028=int16_t(f.eye_800a39f0[0]);in.eye_z_1f80002c=int16_t(f.eye_800a39f0[2]);
    in.pitch_800a3a10=f.rotation_800a3a10[0];in.yaw_800a3a12=f.rotation_800a3a10[1];
    in.target_x_800a39a0=t[0];in.target_z_800a39a8=t[2];in.widen_for_400=wide;
    return in;
}
// Grid cell of world point target + k cells forward + s cells to the side.
std::pair<int,int> cell(const ReferenceTables& r,int32_t yaw,double k,double s){
    const double fx=reference_rsin_8007a060(r,yaw)/4096.0,fz=reference_rcos_8007a12c(r,yaw)/4096.0;
    const double x=512*(k*fx+s*fz),z=512*(k*fz-s*fx);
    return {int(std::floor((x+0x8000)/512)),int(std::floor((z+0x8000)/512))};
}
}

int main(){
    const auto r=refs();
    const auto m=grid();
    // The 0x1F800030 translation equals the 0x1F800008 one at shift 0.
    {const auto in=view(r,0x800);for(int i=0;i<3;++i)assert(in.translation_1f800030[size_t(i)]==int32_t(in.camera_1f800008[size_t(5+i)]));}
    assert(terrain_cell_far_1f800076(0,0)==3584&&terrain_cell_far_1f800076(2,4)==704&&terrain_cell_far_1f800076(3,0)==0);
    for(int32_t yaw:{0,0x200,0x300,0x400,0x700,0x800,0xA80,0xC00,0xE40}){
        TerrainCellVisibility v;
        compute_terrain_cell_visibility(m,0,r,view(r,yaw),v);
        assert(v.valid&&v.positive_cells>0&&v.negative_cells==0);
        const auto vis=[&](std::pair<int,int> c){return v.idx_visible[size_t(idx_at(c.first,c.second))]!=0;};
        assert(vis(cell(r,yaw,0,0)));          // near window around the target
        for(double k:{3.0,5.0,8.0})assert(vis(cell(r,yaw,k,0)));   // straight ahead
        assert(!vis(cell(r,yaw,-6,6)));        // behind and to the side
        assert(!vis(cell(r,yaw,6,12)));        // far outside the side cut
        TerrainCellVisibility narrow;
        compute_terrain_cell_visibility(m,0,r,view(r,yaw,false),narrow);
        assert(narrow.positive_cells<=v.positive_cells);
        for(size_t i=0;i<v.idx_visible.size();++i)assert(!narrow.idx_visible[i]||v.idx_visible[i]);
    }
    // Mode 1 (0x80020A4C) walks the whole area with radius 800.
    {auto in=view(r,0x800);in.mode_1f800075=1;TerrainCellVisibility v;
     compute_terrain_cell_visibility(m,0,r,in,v);assert(v.valid&&v.cells_tested==1024);}
    std::cout<<"terrain cell visibility ok\n";
    return 0;
}
