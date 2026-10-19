// Owner-local regression for the 0.0.15.4 Azahar playtest (Oct 19 2026).
// Loads the generalized boot package exactly like the 3DS build, binds the
// tutorial sidecars, and replays the recorded input from trace frame 2132:
// idle at (-334,-1,-5750) heading 2620, then Up+L1 (psx 0x0410) pressed and
// held toward the first Area 1 door. Without the sidecar map initialization
// the floor vanishes at trace frame 2141 (-141,-1,-5582) and Mega Man sinks
// through the door slab; with it he stays grounded and the slab stops him.
//   l3door_replay BOOT.mmlpkg TUTORIAL_DIR [--no-init]
// Output contains only counts and coordinates; no retail payload.
#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_animation.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
using namespace l3;
int main(int argc,char** argv){
    try{
        if(argc<3){std::fprintf(stderr,"usage: l3door_replay BOOT.mmlpkg TUTORIAL_DIR [--no-init]\n");return 2;}
        const std::filesystem::path root=argv[2];
        const bool apply_init=!(argc>3&&std::strcmp(argv[3],"--no-init")==0);
        AreaPackageLimits limits;limits.max_load_peak_bytes=8u*1024u*1024u;limits.max_map_areas=256;
        auto package=load_area_package(argv[1],limits);
        if(package.map.areas.size()<2)
            throw std::runtime_error("boot package lacks Area 1");
        bool applied=false;
        if(apply_init)applied=apply_sidecar_map_initialization(package,
            load_map_initialization(read_file(root/"st00.l3init"),true));
        const auto writes=package.diagnostics.map_initialization_writes;
        const auto anim=load_l3animation(read_file(root/"player/main.l3anim"),true);
        const auto refs=load_l3reference(read_file(root/"reference.l3ref"),true);
        std::vector<MmlMessageFile> banks;
        for(const char* n:{"st00_01.l3msg","st00_02.l3msg"})banks.push_back(load_l3message(read_file(root/"messages"/n),true));
        L3QolSettings q{};l3_qol_settings_defaults(&q);
        auto area=load_tutorial_runtime_area(read_file(root/"st00.l3flow"),read_file(root/"ebd/area_01.l3ebd"),1,true);
        RuntimeSession s(std::move(package.map),std::move(area),banks,q);s.set_reference_tables(refs);
        s.reset_player({-334,-1,-5750},2620,100,100);PbdMainBodyTrack track{};
        int airborne=0,floorless=0,below_floor=0,door_contacts=0,first_fail=-1;RuntimeVec3i last{};
        for(int f=0;f<40;++f){
            RuntimeInputFrame in{};in.held_psx_mask=f<2?0:0x0410;in.pressed_psx_mask=f==2?0x0410:0;
            (void)runtime_animation_prepare(anim,track,s);s.tick(in);(void)runtime_animation_finish(anim,track,s);
            const auto p=s.player_position();const auto& c=s.player_contact();last=p;
            const bool fail=!c.floor_y||!c.grounded.value_or(false)||p.y>-1;
            if(!c.floor_y)++floorless;
            if(!c.grounded.value_or(false))++airborne;
            if(p.y>-1)++below_floor;
            if(c.wall_contact&&p.z<=-5409)++door_contacts;
            if(fail&&first_fail<0)first_fail=f;
        }
        const bool pass=first_fail<0&&door_contacts>0&&last.z<=-5409;
        std::printf("{\"map_initialization_applied\":%s,\"map_initialization_writes\":%u,\"frames\":40,"
                    "\"floorless_frames\":%d,\"airborne_frames\":%d,\"below_floor_frames\":%d,"
                    "\"door_slab_contacts\":%d,\"first_failure_frame\":%d,\"final\":[%d,%d,%d],\"result\":\"%s\"}\n",
            applied?"true":"false",writes,floorless,airborne,below_floor,door_contacts,first_fail,
            last.x,last.y,last.z,pass?"PASS":"FAIL");
        return pass?0:1;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 2;}
}
