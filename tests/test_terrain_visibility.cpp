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
    // 0x1F8001C0 (0x800238BC): the player's view depth. The follow camera sits
    // 512 behind Mega Man, so his depth is about 512 (Area 1 measured 522).
    const auto depth=terrain_view_depth_1f8001c0(start,2822,-150,-4833);
    assert(depth&&*depth>480&&*depth<540);
    // Depth cue: an authored area record with SetFogNear distance 1280 and
    // FarColor (0,16,16), as in the ST00 Area 1-3 records.
    std::array<uint8_t,40> area{};area[32]=0;area[33]=16;area[34]=16;area[0x24]=0x00;area[0x25]=0x05;
    const auto fogged=make_terrain_face_camera(source_gameplay_camera_80037fb4(r,{{2822,-150,-4833}},512,2048,0x30,0,0).camera_1f800008,&area);
    assert(fogged.far_color&&!start.far_color);
    // The start wall at SZ ~285 is nearer than 1280: no fog.
    f=classify_terrain_face(fogged,Xr,Y,Z,0x00808080u,false,2,*depth);
    assert(f.ir0==0&&f.rgb==0x808080u);
    // About 6000 units ahead: IR0 near 4096, colour almost at FarColor.
    const auto distant=make_terrain_face_camera(source_gameplay_camera_80037fb4(r,{{2822,-150,1000}},512,2048,0x30,0,0).camera_1f800008,&area);
    f=classify_terrain_face(distant,X,Y,Z,0x00808080u,false,2,*depth);
    const unsigned red=f.rgb&255u,green=(f.rgb>>8)&255u,blue=(f.rgb>>16)&255u;
    assert(f.ir0>3800&&red<12&&green>=14&&green<26&&green==blue);
    // Positive faces with bit 28 keep their RGB; negative cells always fog.
    assert(classify_terrain_face(distant,X,Y,Z,0x10808080u,false,2,*depth).rgb==0x808080u);
    assert(classify_terrain_face(distant,X,Y,Z,0x10808080u,true,2,*depth).rgb==f.rgb);
    // Without the area record the colour is untouched.
    assert(classify_terrain_face(camera_at(r,1000),X,Y,Z,0x00808080u,false,2,*depth).rgb==0x808080u);
    // See-through (0x80025900 / 0x80024F30): the start wall's front face lies
    // between the camera and Mega Man (max SZ < his depth) across the screen
    // centre, so it gets 0x1F80007C = 0x02000000.
    f=classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,2,*depth);
    assert(f.face==TerrainFace::Draw&&f.see_through);
    assert(classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,1,*depth).see_through);
    // Farther than the player, back faces, bit 31, a zero prefix, modes 0/3
    // and negative cells: opaque.
    assert(!classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,2,200).see_through);
    assert(!classify_terrain_face(start,X,Y,Z,0x00808080u,false,2,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0x80808080u,false,2,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0,false,2,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,0,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0x00808080u,false,3,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0x00808080u,true,2,*depth).see_through);
    // Double-sided back face drawn: also see-through.
    assert(classify_terrain_face(start,X,Y,Z,0x40808080u,false,2,*depth).see_through);
    // 0x1F80007C is 0 while the event byte 0x800C4C48 is set.
    assert(terrain_see_through_code_1f80007c(0)==0x02000000u&&terrain_see_through_code_1f80007c(1)==0);
    auto during_event=start;during_event.see_through_1f80007c=terrain_see_through_code_1f80007c(1);
    assert(!classify_terrain_face(during_event,Xr,Y,Z,0x00808080u,false,2,*depth).see_through);
    // Off the centre region (wall moved 700 units sideways): opaque.
    const std::array<int32_t,4> Xs{{3772,3260,3772,3260}};
    f=classify_terrain_face(start,Xs,Y,Z,0x00808080u,false,2,*depth);
    assert(f.face==TerrainFace::Draw&&!f.see_through);
    // Bit 29: kept below min SZ 0x181 (mode 1 also needs bit 24 clear),
    // excluded beyond it.
    assert(classify_terrain_face(start,Xr,Y,Z,0x21808080u,false,2,*depth).see_through);
    assert(!classify_terrain_face(start,Xr,Y,Z,0x21808080u,false,1,*depth).see_through);
    assert(classify_terrain_face(start,Xr,Y,Z,0x20808080u,false,1,*depth).see_through);
    f=classify_terrain_face(far,Xr,Y,Z,0x00808080u,false,2,2000);
    assert(f.min_sz>=0x181&&f.see_through);
    assert(!classify_terrain_face(far,Xr,Y,Z,0x20808080u,false,2,2000).see_through);
    std::cout<<"PASS: start wall back face skipped (min SZ "<<classify_terrain_face(start,X,Y,Z,0,false,2,522).min_sz
             <<"), far back face black, bits 24/30, modes, negative cells, player depth "<<*depth
             <<", depth cue, see-through rule\n";
}
