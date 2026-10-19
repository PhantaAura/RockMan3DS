// Owner-local regression for the first Area 1 door (Oct 19 2026 playtest).
// Loads the generalized boot package exactly like the 3DS build, binds the
// tutorial sidecars, and runs two phases:
//  1. Fall regression: replays the recorded input from trace frame 2132:
//     idle at (-334,-1,-5750) heading 2620, then Up+L1 (psx 0x0410) pressed and
//     held toward the door. Without the sidecar map initialization the floor
//     vanishes at trace frame 2141 (-141,-1,-5582) and Mega Man sinks through
//     the door slab; with it he stays grounded and the slab stops him.
//  2. Door event: from the same idle spot facing the door (heading 2048), Up
//     walks him into the slab, then Circle (psx 0x2000, profile-0 +0x136) is
//     pressed. 0x80050C2C case 3 must request Area 2 from aux record 2.
//     Event 0 (0x800485E4) then runs with no pad input: the Area 1 door opens
//     (0x7B1/0x7B2, IDX descriptor swap), the forced walk 0x80048A9C walks him
//     in, the door closes and the load is offered (356C = 1). The commit must
//     place him where substate 8 (0x80048898) puts him; the Area 2 door then
//     opens, the second walk takes him through it, the door closes and phase
//     2 returns control with 0x7B0 clear and 0x380 set. He must end in Area 2
//     beyond the door cell (z > 6656) on a floor.
//   l3door_replay BOOT.mmlpkg TUTORIAL_DIR [--no-init] [--trace]
// Output contains only counts and coordinates; no retail payload.
#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_animation.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
using namespace l3;
namespace {
struct Inputs {
    std::filesystem::path root;
    std::string package;
    bool apply_init=true;
};
struct Built {
    std::unique_ptr<RuntimeSession> session;
    PbdAnimation anim;
    bool applied=false;
    uint32_t writes=0;
};
Built build(const Inputs& in){
    AreaPackageLimits limits;limits.max_load_peak_bytes=8u*1024u*1024u;limits.max_map_areas=256;
    auto package=load_area_package(in.package,limits);
    if(package.map.areas.size()<3)
        throw std::runtime_error("boot package lacks Areas 1 and 2");
    Built b;
    if(in.apply_init)b.applied=apply_sidecar_map_initialization(package,
        load_map_initialization(read_file(in.root/"st00.l3init"),true));
    b.writes=package.diagnostics.map_initialization_writes;
    b.anim=load_l3animation(read_file(in.root/"player/main.l3anim"),true);
    const auto refs=load_l3reference(read_file(in.root/"reference.l3ref"),true);
    std::vector<MmlMessageFile> banks;
    for(const char* n:{"st00_01.l3msg","st00_02.l3msg"})banks.push_back(load_l3message(read_file(in.root/"messages"/n),true));
    L3QolSettings q{};l3_qol_settings_defaults(&q);
    const auto flow=read_file(in.root/"st00.l3flow");
    auto area=load_tutorial_runtime_area(flow,read_file(in.root/"ebd/area_01.l3ebd"),1,true);
    b.session=std::make_unique<RuntimeSession>(std::move(package.map),std::move(area),banks,q);
    b.session->set_reference_tables(refs);
    // As the 3DS boot does, every other tutorial area is registered up front.
    b.session->register_area(2,load_tutorial_runtime_area(flow,read_file(in.root/"ebd/area_02.l3ebd"),2,true));
    return b;
}
void step(Built& b,PbdMainBodyTrack& track,const RuntimeInputFrame& in){
    (void)runtime_animation_prepare(b.anim,track,*b.session);b.session->tick(in);
    (void)runtime_animation_finish(b.anim,track,*b.session);
}
int16_t lane16(const SourceEventHandoffTransitionIntent& t,size_t at){return int16_t(uint16_t(t.raw[at]|(uint16_t(t.raw[at+1])<<8)));}
} // namespace

int main(int argc,char** argv){
    try{
        if(argc<3){std::fprintf(stderr,"usage: l3door_replay BOOT.mmlpkg TUTORIAL_DIR [--no-init]\n");return 2;}
        bool trace=false,no_init=false;
        for(int i=3;i<argc;++i){trace|=std::strcmp(argv[i],"--trace")==0;no_init|=std::strcmp(argv[i],"--no-init")==0;}
        Inputs inputs{argv[2],argv[1],!no_init};

        // Phase 1: Oct 19 fall regression.
        auto b=build(inputs);auto& s=*b.session;
        s.reset_player({-334,-1,-5750},2620,100,100);PbdMainBodyTrack track{};
        int airborne=0,floorless=0,below_floor=0,door_contacts=0,first_fail=-1;RuntimeVec3i last{};
        for(int f=0;f<40;++f){
            RuntimeInputFrame in{};in.held_psx_mask=f<2?0:0x0410;in.pressed_psx_mask=f==2?0x0410:0;
            step(b,track,in);
            const auto p=s.player_position();const auto& c=s.player_contact();last=p;
            const bool fail=!c.floor_y||!c.grounded.value_or(false)||p.y>-1;
            if(!c.floor_y)++floorless;
            if(!c.grounded.value_or(false))++airborne;
            if(p.y>-1)++below_floor;
            if(c.wall_contact&&p.z<=-5409)++door_contacts;
            if(fail&&first_fail<0)first_fail=f;
        }
        const bool fall_pass=first_fail<0&&door_contacts>0&&last.z<=-5409;

        // Phase 2: Up into the door, then Circle with Up held.
        auto d=build(inputs);auto& t=*d.session;
        t.reset_player({-334,-1,-5750},2048,100,100);PbdMainBodyTrack door_track{};
        int request_frame=-1;SourceDoorStatus press_status=SourceDoorStatus::NoEvent;
        for(int f=0;f<40;++f){
            RuntimeInputFrame in{};in.held_psx_mask=f<2?0:0x0010;in.pressed_psx_mask=f==2?0x0010:0;
            if(f==30){in.held_psx_mask|=0x2000;in.pressed_psx_mask|=0x2000;}
            step(d,door_track,in);
            if(f==30)press_status=t.source_door_result().status;
            if(t.source_door_event0().active){request_frame=f;break;}
        }
        const auto before=t.player_position();
        const bool flag_7b0=t.story_flag(0x7B0);
        const auto log=[&](const char* tag,int f){
            if(!trace)return;const auto p=t.player_position();const auto& e=t.source_door_event0();
            std::fprintf(stderr,"%s f=%d area=%zu pos=(%d,%d,%d) yaw=%u ph=%u sub=%u ctr=%d bd=%02x st=%u/%u 7b1=%d 7b2=%d doors=%zu\n",
                tag,f,t.map_area_index(),p.x,p.y,p.z,unsigned(t.player_yaw()),e.phase,e.substate,e.counter,
                t.compat_player().bytes[0xBD],t.compat_player().bytes[8],t.compat_player().bytes[9],
                t.story_flag(0x7B1),t.story_flag(0x7B2),t.source_door_objects().size());
        };
        // Event 0 before the load: no pad input, the event owns the player.
        int offer_frame=-1,area1_door_opened=0,max_area1_z=before.z;
        std::optional<SourceEventHandoffTransitionIntent> offered;
        for(int f=0;f<400&&!offered;++f){
            step(d,door_track,{});log("pre",f);
            for(const auto& o:t.source_door_objects())if(o.state>=3)area1_door_opened=1;
            max_area1_z=std::max(max_area1_z,t.player_position().z);
            if(t.source_door_transition()){offered=t.source_door_transition();offer_frame=f;}
        }
        const auto& intent=offered;
        bool committed=false;int door_floorless=0,event_frames=0,area2_door_opened=0,area2_door_closed=0;
        RuntimeVec3i arrival{},expected{},final_pos{};
        uint16_t arrival_heading=0;int counter_at_arrival=-1;
        if(offered){
            // Source arithmetic from the request lanes: 3560 + rsin(356A)*64>>12,
            // 3562 - 1, 3564 + rcos(356A)*64>>12 (reference table = original trig).
            const auto refs=load_l3reference(read_file(inputs.root/"reference.l3ref"),true);
            const auto& trig=refs.trig[uint16_t(lane16(*offered,10))&0x0FFFu];
            expected={lane16(*offered,0)+((int32_t(trig.sin_q12)*64)>>12),lane16(*offered,2)-1,
                      lane16(*offered,4)+((int32_t(trig.cos_q12)*64)>>12)};
            committed=t.commit_source_door_transition();
            arrival=t.player_position();arrival_heading=t.player_yaw();
            counter_at_arrival=t.source_door_event0().counter;
            log("commit",0);
            for(int f=0;f<400&&t.source_door_event0().active;++f){
                step(d,door_track,{});log("post",f);++event_frames;
                for(const auto& o:t.source_door_objects()){if(o.state>=3)area2_door_opened=1;if(area2_door_opened&&o.state==1)area2_door_closed=1;}
            }
            for(int f=0;f<10;++f){step(d,door_track,{});if(!t.player_contact().floor_y)++door_floorless;log("idle",f);}
            final_pos=t.player_position();
        }
        const bool door_pass=intent&&offered&&committed&&t.map_area_index()==2&&offered->destination_area==2&&
            lane16(*offered,0)==-256&&lane16(*offered,2)==-1&&lane16(*offered,4)==6272&&
            area1_door_opened&&arrival.x==expected.x&&arrival.y==expected.y&&arrival.z==expected.z&&arrival_heading==2048&&
            flag_7b0&&!t.source_door_event0().active&&area2_door_opened&&area2_door_closed&&
            !t.story_flag(0x7B0)&&!t.story_flag(0x7B1)&&!t.story_flag(0x7B2)&&!t.story_flag(0x7B3)&&t.story_flag(0x380)&&
            final_pos.z>6656&&door_floorless==0;
        const bool pass=fall_pass&&door_pass;
        std::printf("{\"map_initialization_applied\":%s,\"map_initialization_writes\":%u,\"frames\":40,"
                    "\"floorless_frames\":%d,\"airborne_frames\":%d,\"below_floor_frames\":%d,"
                    "\"door_slab_contacts\":%d,\"first_failure_frame\":%d,\"final\":[%d,%d,%d],\"fall_result\":\"%s\",\n"
                    " \"door\":{\"press_status\":\"%s\",\"request_frame\":%d,\"at\":[%d,%d,%d],"
                    "\"request_destination\":[%d,%d,%d],\"request_area\":%d,\"request_stage\":%d,\"flag_7b0_set\":%s,"
                    "\"area1_door_opened\":%s,\"area1_max_z\":%d,\"load_offered_frame\":%d,"
                    "\"committed\":%s,\"area\":%zu,\"arrival\":[%d,%d,%d],\"expected_arrival\":[%d,%d,%d],"
                    "\"heading\":%u,\"counter_at_arrival\":%d,\"area2_door_opened\":%s,\"area2_door_closed\":%s,"
                    "\"event_frames_after_commit\":%d,\"final\":[%d,%d,%d],\"floorless_frames\":%d,\"result\":\"%s\"},\n"
                    " \"result\":\"%s\"}\n",
            b.applied?"true":"false",b.writes,floorless,airborne,below_floor,door_contacts,first_fail,
            last.x,last.y,last.z,fall_pass?"PASS":"FAIL",
            source_door_status_name(press_status),request_frame,before.x,before.y,before.z,
            intent?lane16(*intent,0):0,intent?lane16(*intent,2):0,intent?lane16(*intent,4):0,
            intent?int(intent->destination_area):-1,intent?int(intent->source_stage):-1,flag_7b0?"true":"false",
            area1_door_opened?"true":"false",max_area1_z,offer_frame,
            committed?"true":"false",t.map_area_index(),arrival.x,arrival.y,arrival.z,expected.x,expected.y,expected.z,
            unsigned(arrival_heading),counter_at_arrival,area2_door_opened?"true":"false",area2_door_closed?"true":"false",
            event_frames,final_pos.x,final_pos.y,final_pos.z,door_floorless,door_pass?"PASS":"FAIL",
            pass?"PASS":"FAIL");
        return pass?0:1;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 2;}
}
