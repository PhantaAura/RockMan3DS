// Original gameplay follow camera (0x80038648 / 0x80038894). Expected values
// follow the R3000 branch structure of 0x8003A218/0x8003A154 and the Q12
// eye arithmetic of 0x80037FB4; geometry is synthetic, not a retail trace.
#include "legends3ds/source_gameplay_camera.h"
#include "legends3ds/player_contact_runtime.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
l3::ReferenceTables trig() {
    l3::ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){
        const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096.0)),int16_t(std::lround(std::cos(a)*4096.0))};
    }
    return r;
}
void put(L3PlayerCompatWork& w,size_t o,int16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(uint16_t(v)>>8);}
// 4x4 cells of packed cell 1: one floor slab y 0..1024 under the whole area,
// optionally a tall wall block at z <= -200, where the camera eye sits.
l3::RuntimeMap room(bool wall){
    l3::RuntimeMap m;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    m.areas.push_back(a);m.cells.assign(16,1);m.idx.resize(2);m.idx[1].descriptor_index=0;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,uint32_t(wall?2:1)});
    m.contact_groups.push_back({-4096,4096,-4096,4096,0,1});
    m.contact_primitives.push_back({0,1024,1,0});
    if(wall){m.contact_groups.push_back({-4096,4096,-4096,-200,1,1});m.contact_primitives.push_back({-2048,1024,1,0});}
    return m;
}
int32_t expected_eye(const l3::ReferenceTables& r,int32_t t,int32_t d,int32_t s,int32_t c){
    (void)r;return int16_t(-(((d*s>>12)*c>>12)-t));
}
}

int main(){
    const auto refs=trig();
    l3::SourceGameplayCamera cam;
    L3PlayerCompatWork w{};put(w,0x14,100);put(w,0x18,-200);put(w,0x56,0);
    auto map=room(false);
    l3::source_gameplay_camera_init_80038648(cam,refs,100,-200);
    assert(cam.initialized&&cam.distance_800a3964==0x200&&cam.pitch_800a396c==0x30&&cam.yaw_800a3968==0);
    assert(cam.target_800a39a0[0]==100&&cam.target_800a39a0[1]==0&&cam.target_800a39a0[2]==-200);
    // Eye formula: yaw 0 -> straight along +z by d*cos(pitch), up by d*sin(pitch).
    const auto sp=refs.trig[0x30].sin_q12,cp=refs.trig[0x30].cos_q12;
    assert(cam.eye_800a39f0[0]==100);
    assert(cam.eye_800a39f0[1]==int16_t(-(((0x200*sp)>>12)-0)));
    assert(cam.eye_800a39f0[2]==expected_eye(refs,-200,0x200,refs.trig[0].cos_q12,cp));

    // First follow frame snaps behind the player (heading + 0x800).
    put(w,0x56,0x100);
    l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert(cam.follow_frames_80098668==1&&cam.yaw_800a3968==0x900);
    assert(cam.target_800a39a0[1]==-0x96&&cam.target_800a39a0[0]==100&&cam.target_800a39a0[2]==-200);
    const auto e=l3::source_camera_eye_80037fb4(refs,cam.target_800a39a0,0x200,0x900,0x30);
    assert(cam.eye_800a39f0==e&&e[1]==int16_t(-((((0x200*sp)>>12))+0x96)));
    // Small turn: gap 0x100 -> rate 0x40 per frame until the gap is < 0x60
    // (rate floor 0x18), never overshooting.
    put(w,0x56,0x200);
    l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert(cam.yaw_rate_80097d28==0x40&&cam.yaw_800a3968==0x940);
    l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert(cam.yaw_rate_80097d28==0x30&&cam.yaw_800a3968==0x970);
    for(int i=0;i<8;++i)l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert(cam.yaw_800a3968==0xA00);
    // Half turn: gap 0x800 -> rate 0x80, and the lag is clamped to 0x200 at once.
    put(w,0x56,0xA00);  // goal 0x1200 & 0xFFF = 0x200
    l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert(cam.yaw_rate_80097d28==0x80&&(cam.yaw_800a3968&0xfff)==0x400);
    // Wrap across zero picks the short way.
    put(w,0x56,0x7F0);  // goal 0xFF0
    l3::source_gameplay_camera_update_80038894(cam,refs,w,map,0);
    assert((cam.yaw_800a3968&0xfff)==0x1F0&&cam.yaw_rate_80097d28==0x80);

    // Distance: player +0x44D clear -> 60-frame delay, then pull toward 0x200.
    l3::SourceGameplayCamera free_cam;
    l3::source_gameplay_camera_init_80038648(free_cam,refs,0,0);
    put(w,0x14,0);put(w,0x18,0);put(w,0x56,0x800);w.bytes[0x44D]=0;
    for(int i=0;i<61;++i)l3::source_gameplay_camera_update_80038894(free_cam,refs,w,map,0);
    assert(free_cam.pull_in_delay_80097d48==0&&free_cam.distance_800a3964==0x200);
    // +0x44D set and the eye in open space: the camera backs out 10 per frame
    // to 0x300, resetting the delay each frame.
    w.bytes[0x44D]=1;
    for(int i=0;i<20;++i)l3::source_gameplay_camera_update_80038894(free_cam,refs,w,map,0);
    assert(free_cam.distance_800a3964==0x200+20*10&&free_cam.pull_in_delay_80097d48==0x3c);
    for(int i=0;i<40;++i)l3::source_gameplay_camera_update_80038894(free_cam,refs,w,map,0);
    assert(free_cam.distance_800a3964==0x300&&free_cam.unsupported_eye_tests==0);
    // Eye now inside a wall block (yaw 0 puts the eye at -z): hold
    // for 60 frames, then pull in by 10 per frame.
    auto walled=room(true);
    const auto inside=l3::runtime_point_embedded_80049c54(walled,0,int16_t(free_cam.eye_800a39f0[0]),
        int16_t(free_cam.eye_800a39f0[1]),int16_t(free_cam.eye_800a39f0[2]),{-32,32,-32,32,-32,32},true);
    assert(inside.status==l3::RuntimePointEmbedding::Embedded&&inside.raw==1);
    for(int i=0;i<60;++i)l3::source_gameplay_camera_update_80038894(free_cam,refs,w,walled,0);
    assert(free_cam.distance_800a3964==0x300&&free_cam.pull_in_delay_80097d48==0);
    for(int i=0;i<5;++i)l3::source_gameplay_camera_update_80038894(free_cam,refs,w,walled,0);
    assert(free_cam.distance_800a3964==0x300-50);
    // Open space again from a free eye point.
    const auto open=l3::runtime_point_embedded_80049c54(map,0,0,-187,300,{-32,32,-32,32,-32,32},true);
    assert(open.status==l3::RuntimePointEmbedding::Free&&open.raw==0);
    // No cell under the point: the original's 0x40000000 counts as blocked.
    const auto off=l3::runtime_point_embedded_80049c54(map,0,0,-187,9000,{-32,32,-32,32,-32,32},true);
    assert(off.status==l3::RuntimePointEmbedding::Embedded&&off.raw==0x40000000u);
    std::cout<<"PASS: source gameplay camera init/follow/yaw rate/lag clamp/distance and 0x80049C54 eye test\n";
}
