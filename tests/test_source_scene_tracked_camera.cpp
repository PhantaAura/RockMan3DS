// 0x8001E5BC tracked scene camera: camera+8.. = actor+1C.. + camera+A8..
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/source_event_phase6.h"
#include <cstdio>
#include <cstdlib>
namespace {
void put(uint8_t* raw,uint8_t* known,size_t at,uint32_t v){for(size_t i=0;i<4;++i){raw[at+i]=uint8_t(v>>(8*i));known[at+i]=1;}}
uint32_t get(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
}
int main(){
    l3::SourceEventWork work;l3::SourceEventPhase5Context scene;l3::SourceEventPhase6Context context;
    // event+0x28 -> E0 slot 2 (camera[1] = 1), event+0x24 -> class-40 slot 0.
    put(work.raw.data(),work.known.data(),0x24,0x800c3580u);
    put(work.raw.data(),work.known.data(),0x28,0x800bf118u+2*80);
    scene.pool40.used[0]=0x80000000u;scene.pool40.slots[0].raw[1]=0x40;scene.pool40.slots[0].known[1]=1;
    scene.pool_e0.used[0]=0x20000000u;scene.pool_e0.slots[2].raw[1]=0xe2;scene.pool_e0.slots[2].known[1]=1;
    auto& a=scene.pool40.slots[0];auto& e=scene.pool_e0.slots[2];
    for(size_t i=0;i<3;++i){put(a.raw.data(),a.known.data(),0x1c+i*4,0x01000000u*(i+1));
        put(e.raw.data(),e.known.data(),0x1c+i*4,0x00100000u*(i+5));}
    auto& cam=scene.camera_800bc738;
    for(size_t i=0;i<3;++i){put(cam.raw.data(),cam.known.data(),0xa8+i*4,0xffff0000u);put(cam.raw.data(),cam.known.data(),8+i*4,7);}
    cam.raw[1]=0;cam.known[1]=1;
    context.byte_800bc733=0;
    require(!l3::apply_source_scene_tracked_target_8001e5bc(work,scene,context)&&get(cam.raw.data()+8)==7,"untracked stores nothing");
    context.byte_800bc733=1;
    require(l3::apply_source_scene_tracked_target_8001e5bc(work,scene,context),"tracked class-40 commit");
    for(size_t i=0;i<3;++i)require(get(cam.raw.data()+8+i*4)==uint32_t(0x01000000u*(i+1)+0xffff0000u),"class-40 target store");
    cam.raw[1]=1;
    require(l3::apply_source_scene_tracked_target_8001e5bc(work,scene,context),"tracked E0 commit");
    for(size_t i=0;i<3;++i)require(get(cam.raw.data()+8+i*4)==uint32_t(0x00100000u*(i+5)+0xffff0000u),"E0 target store");
    e.known[0x20]=0;const auto before=cam;
    require(!l3::apply_source_scene_tracked_target_8001e5bc(work,scene,context)&&cam.raw==before.raw,"unknown actor field holds");
    cam.raw[1]=9;require(!l3::apply_source_scene_tracked_target_8001e5bc(work,scene,context),"unbound slot holds");
    std::puts("source_scene_tracked_camera: 6 checks OK");
}
