// Original map face test (NCLIP + family rule) on the Area 1 start wall:
// quad (2560..3072, y 0..-192, z -4608) seen from the start camera
// (0x80037FB4, target 2822/-150/-4833, distance 512, yaw 2048, pitch 0x30).
// Trig values are authored, not the retail table.
#include "legends3ds/render/terrain_visibility.h"
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
TerrainFaceCamera camera_at(const ReferenceTables& r,int32_t target_z){
    return make_terrain_face_camera(source_gameplay_camera_80037fb4(r,{{2822,-150,target_z}},512,2048,0x30,0,0).camera_1f800008);
}
const std::array<int32_t,4> X{{2560,3072,2560,3072}},Y{{-192,-192,0,0}},Z{{-4608,-4608,-4608,-4608}};
const std::array<int32_t,4> Xr{{3072,2560,3072,2560}}; // v0/v1 swapped: the opposite face
}

int main(){
    const auto r=refs();
    const auto start=camera_at(r,-4833);
    // Back face 285 units ahead: skipped in mode 2 (threshold 522) -> Mega Man shows.
    auto f=classify_terrain_face(start,X,Y,Z,0x00808080u,false,2,kObservedArea1BackFaceThreshold);
    assert(f.mac0<0&&f.min_sz>250&&f.min_sz<300&&f.face==TerrainFace::Skip);
    // Same face from farther away (eye about 900 units off): drawn black.
    const auto far=camera_at(r,-4000);
    f=classify_terrain_face(far,X,Y,Z,0x00808080u,false,2,kObservedArea1BackFaceThreshold);
    assert(f.mac0<0&&f.min_sz>522&&f.face==TerrainFace::Black);
    // Bit 24 skips the black fill; bit 30 makes the quad double-sided.
    assert(classify_terrain_face(far,X,Y,Z,0x01808080u,false,2,kObservedArea1BackFaceThreshold).face==TerrainFace::Skip);
    assert(classify_terrain_face(start,X,Y,Z,0x40808080u,false,2,kObservedArea1BackFaceThreshold).face==TerrainFace::Draw);
    // The opposite winding faces the camera: drawn.
    f=classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,2,kObservedArea1BackFaceThreshold);
    assert(f.mac0>0&&f.face==TerrainFace::Draw);
    // Other positive modes and negative cells never black-fill.
    assert(classify_terrain_face(far,X,Y,Z,0x00808080u,false,0,kObservedArea1BackFaceThreshold).face==TerrainFace::Skip);
    assert(classify_terrain_face(far,X,Y,Z,0x00808080u,true,2,kObservedArea1BackFaceThreshold).face==TerrainFace::Skip);
    assert(classify_terrain_face(far,X,Y,Z,0x40808080u,true,2,kObservedArea1BackFaceThreshold).face==TerrainFace::Draw);
    // Behind the camera or outside signed16: unsupported, caller keeps drawing.
    const std::array<int32_t,4> Zb{{-3000,-3000,-3000,-3000}};
    assert(classify_terrain_face(start,X,Y,Zb,0,false,2,kObservedArea1BackFaceThreshold).face==TerrainFace::Unsupported);
    std::cout<<"PASS: start wall back face skipped (min SZ "<<classify_terrain_face(start,X,Y,Z,0,false,2,522).min_sz
             <<"), far back face black, bits 24/30, modes, negative cells\n";
}
