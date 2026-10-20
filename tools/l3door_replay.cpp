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
//  3. Terrain patch (0.0.15.8): the 3DS renderer's IDX swap bookkeeping is
//     run beside phase 2 over the package's render source. Every descriptor
//     generation change re-plans; both doors must be hidden and restored, and
//     the open descriptor's primitive count is reported (not a pass gate).
//  4. Buster (0.0.15.11): the Azahar diagnostic spawn (-334,-1,-5750) heading
//     0. Three Y presses run the 3DS main.cpp chain: fire admission, integer
//     pose 0x800325D4, muzzles 0x800331A0, callbacks 0x80017154, then lock-on
//     0x80034EB8 with the 0x80037FB4 camera and 0x80035724. Each press must
//     spawn one shot whose registered segment (owner point -> +400/+418
//     endpoint) is non-zero on all 10 state-1 frames, then release. The
//     endpoint is still the CANDIDATE forward ray used by main.cpp.
//   l3door_replay BOOT.mmlpkg TUTORIAL_DIR [--no-init] [--trace]
// Output contains only counts and coordinates; no retail payload.
#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_animation.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/positive_terrain.h"
#include "legends3ds/render/terrain_descriptor_patch.h"
#include "legends3ds/child_runtime.h"
#include "legends3ds/gameplay_gates.h"
#include "legends3ds/source_buster_profile.h"
#include "legends3ds/source_camera.h"
#include "legends3ds/source_player_buster.h"
#include "legends3ds/source_player_pose.h"
#include "legends3ds/source_scene_view.h"
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
    St00RenderSource render;
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
    b.render=std::move(package.render);
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
// Mirrors platform/3ds main.cpp: bake the area's positive instances (Mid LOD),
// then re-plan on each descriptor generation change. No GPU buffers here, so
// only the hide/restore bookkeeping and overlay primitive counts are checked.
struct PatchObserver {
    TerrainDescriptorPatch patch;
    size_t area=SIZE_MAX;
    uint32_t generation=0;
    int changes=0,hides=0,restores=0,max_overlay_idx=0;
    size_t max_baked=0,max_spans=0;
    uint16_t door_idx=0,closed_descriptor=0xFFFFu,open_descriptor=0xFFFFu;
    size_t closed_prims=0,open_prims=0;
    static size_t prims(const RuntimeMap& m,const St00RenderSource& r,size_t area,uint16_t idx,uint16_t* descriptor){
        for(const auto& i:collect_area_positive_placement(m,area).instances){
            if(i.idx_index!=idx)continue;
            if(descriptor)*descriptor=i.descriptor_index;
            if(i.descriptor_index==0xFFFFu)return 0;
            return expand_positive_terrain_descriptor_lod(r,i.descriptor_index,i.bank,St00Lod::Mid,i.origin,0).size();
        }
        return 0;
    }
    void observe(const RuntimeSession& s,const St00RenderSource& r){
        if(s.map_area_index()!=area){
            patch.reset();area=s.map_area_index();generation=s.idx_descriptor_generation();
            uint32_t first=0;
            const auto& m=s.active_map();
            for(const auto& i:collect_area_positive_placement(m,area).instances){
                // As on the 3DS: only door descriptors (word0 bit 0x1000) are tracked.
                if(i.descriptor_index>=m.descriptors.size()||!(m.descriptors[i.descriptor_index].word0&0x1000u))continue;
                patch.bake(i.idx_index,i.descriptor_index);
                const auto n=uint32_t(expand_positive_terrain_descriptor_lod(r,i.descriptor_index,i.bank,St00Lod::Mid,i.origin,0).size()*6u);
                patch.add_span(i.idx_index,0,first,n);first+=n;
            }
            max_baked=std::max(max_baked,patch.baked_count());max_spans=std::max(max_spans,patch.span_count());
        }
        if(s.idx_descriptor_generation()==generation)return;
        generation=s.idx_descriptor_generation();++changes;
        const auto plan=patch.plan(s.active_map());
        hides+=int(plan.hide.size());restores+=int(plan.restore.size());
        max_overlay_idx=std::max(max_overlay_idx,int(plan.overlay.size()));
        if(!plan.hide.empty()){
            door_idx=plan.hide.front();
            for(const auto& o:s.source_door_objects())
                if(o.idx_index==door_idx){closed_descriptor=o.closed_descriptor;}
            open_prims=prims(s.active_map(),r,area,door_idx,&open_descriptor);
            // The closed count comes from the bake: the descriptor the door object saved.
            if(closed_descriptor!=0xFFFFu&&closed_descriptor<r.descriptors.size()){
                for(const auto& i:collect_area_positive_placement(s.active_map(),area).instances)
                    if(i.idx_index==door_idx)
                        closed_prims=expand_positive_terrain_descriptor_lod(r,closed_descriptor,i.bank,St00Lod::Mid,i.origin,0).size();
            }
        }
        (void)patch.apply(plan,1,[](uint64_t)->uint8_t*{return nullptr;},[](uint8_t*,size_t){});
    }
};
int16_t lane16(const SourceEventHandoffTransitionIntent& t,size_t at){return int16_t(uint16_t(t.raw[at]|(uint16_t(t.raw[at+1])<<8)));}
// Phase 4. Mirrors the platform/3ds main.cpp source-actor block for the player
// only (no imported actors, so the proxy bank stays empty and no target can be
// selected, as in the Azahar run where broad/window were -1).
struct BusterReplay {
    int presses=0,admitted=0,spawned=0,released=0,state1_frames=0,zero_segments=0,drawn_segments=0,final_live=-1;
    long max_length=0;
    bool pass=false;
};
BusterReplay run_buster(const Inputs& in,Built& b){
    BusterReplay r;auto& s=*b.session;
    const auto refs=load_l3reference(read_file(in.root/"reference.l3ref"),true);
    const auto skeleton=load_l3skeleton(read_file(in.root/"player/main.l3skel"),true);
    SourceBusterPlayer player;
    initialize_source_buster_fresh_lanes_80037534(load_source_buster_profile(read_file(in.root/"player/buster.l3bp"),true),player);
    SourcePlayerTargetProfile target;
    if(!decode_source_player_target_profile(read_file(in.root/"player/target.l3tg"),target)||!target.image_target_mode_80097d02)
        throw std::runtime_error("L3TG v2 required");
    SourceGteRtpsState gte;
    {SourceProjectionControls pc;source_projection_init_geom_8007a4f0(pc);
     (void)initialize_source_scene_projection_80011f48(pc,load_source_scene_view_profile(read_file(in.root/"source_scene_view.l3sv")));
     (void)bind_source_projection_controls(gte,pc);}
    auto children=std::make_unique<ChildRuntime>();
    std::optional<uint32_t> mask=0xffu;
    s.reset_player({-334,-1,-5750},0,100,100);
    auto track=pbd_main_body_track_begin(b.anim,s.compat_player().bytes[0xAC],s.compat_player().bytes[0xA4]);
    const auto pack=[](int32_t x,int32_t y,int32_t z){
        const auto v=[](int32_t n){return uint16_t(int16_t(std::clamp(n,int32_t(INT16_MIN),int32_t(INT16_MAX))));};
        return std::array<uint32_t,2>{{uint32_t(v(x))|(uint32_t(v(y))<<16),uint32_t(v(z))}};};
    for(int f=0;f<200;++f){
        RuntimeInputFrame input{};
        if(f==60||f==100||f==140){input.fire_pressed=true;input.held_psx_mask=input.pressed_psx_mask=0x0800;++r.presses;}
        const bool fire=source_buster_fire_admitted(true,input.fire_pressed,s.player_shot_enabled(),s.player_busy_8004324c());
        r.admitted+=fire;
        (void)runtime_animation_prepare(b.anim,track,s);s.tick(input);
        children->reset_frame_queues();
        const auto& pw=s.compat_player().bytes;
        SourcePlayerPoseInputs pose;pose.skeleton=&skeleton;pose.reference=&refs;pose.scratch_shift_1f800074=0;
        pose.limb_pose_selector_106=pw[0x106];
        std::optional<PbdMainPoseBank> bank;
        if(const auto* frame=pbd_main_body_track_frame(b.anim,track)){bank=decode_pbd_main_pose_bank(*frame);pose.primary_pose=&*bank;}
        const auto p=s.player_position();
        pose.player_position_14={{int16_t(p.x),int16_t(p.y),int16_t(p.z)}};
        pose.heading_56=uint16_t(pw[0x56]|(uint16_t(pw[0x57])<<8));
        pose.heading_offset_116=uint16_t(pw[0x116]|(uint16_t(pw[0x117])<<8));
        const auto muzzle=source_player_muzzles_800331a0(produce_source_player_integer_pose_800325d4(pose));
        const bool muzzle_bound=bind_source_player_muzzles_80035724(muzzle,player);
        player.class_01=pw[1];player.heading_56=pose.heading_56;
        SourceBusterFrameContext ctx;ctx.owner_position_14=pack(p.x,p.y,p.z);
        if(muzzle_bound){
            // main.cpp CANDIDATE forward ray (0x800) for +400/+418.
            const auto& d=refs.trig[pose.heading_56&0x0fffu];
            const int32_t dx=-int32_t((int64_t(d.sin_q12)*0x800)/4096),dz=-int32_t((int64_t(d.cos_q12)*0x800)/4096);
            ctx.owner_endpoint_400=pack(int16_t(muzzle.muzzle_3c8_part4[0])+dx,int16_t(muzzle.muzzle_3c8_part4[1]),int16_t(muzzle.muzzle_3c8_part4[2])+dz);
            ctx.owner_endpoint_418=pack(int16_t(muzzle.muzzle_3e0_part7[0])+dx,int16_t(muzzle.muzzle_3e0_part7[1]),int16_t(muzzle.muzzle_3e0_part7[2])+dz);
        }
        std::optional<uint32_t> busy=s.source_buster_busy_198();
        ctx.owner_busy_198=&busy;ctx.proxy_mask_80098798=&mask;ctx.map_collision_80049c24=false;ctx.audio=&s.source_audio_queue();
        const auto tick=children->tick_source_busters_80017154(ctx);
        if(busy)s.set_source_buster_busy_198(*busy);
        r.released+=tick.released;
        const auto& camera=s.source_gameplay_camera();
        if(camera.initialized&&s.camera().owner==RuntimeCameraOwner::Gameplay&&muzzle_bound){
            SourcePlayerBusterInputs select;select.active=true;select.target_mode_49e=*target.image_target_mode_80097d02;
            select.fire_flags_174=uint8_t(fire?0x10u:0u);select.player_point_14=pose.player_position_14;
            select.target_profile_8008994c=target;select.camera_1f800008=camera.camera_1f800008;
            const auto lock=source_player_select_and_spawn_80034eb8(*children,gte,player,mask,select);
            if(lock.spawn_attempted[1]&&lock.spawns[1].status==SourceBusterStatus::SourceBackedPartial&&lock.spawns[1].source_identity)++r.spawned;
        }
        children->consume_pending_releases();
        (void)runtime_animation_finish(b.anim,track,s);
        const auto& pool=children->buster_pool();
        for(size_t slot=0;slot<source_buster_capacity;++slot){
            if(!(pool.allocated_800989f0&(0x80000000u>>slot)))continue;
            const auto& w=pool.slots[slot];
            if(!(int8_t(w[0])&w[6]&1)||w[2]!=0x80||w[8]!=1)continue;
            const auto& seg=children->buster_registered_segments()[slot];
            long length=0;for(size_t a=0;a<3;++a)length+=std::labs(long(seg.first_14[a])-long(seg.second_2c[a]));
            // The state-0 registration is endpoint to endpoint by design
            // (one frame per shot); the ten state-1 frames must not be.
            if(!length)++r.zero_segments;else ++r.drawn_segments;
            ++r.state1_frames;r.max_length=std::max(r.max_length,length);
        }
    }
    r.final_live=__builtin_popcount(children->buster_pool().allocated_800989f0);
    r.pass=r.presses==3&&r.admitted==3&&r.spawned==3&&r.released==3&&r.state1_frames==33&&
        r.zero_segments==3&&r.drawn_segments==30&&r.max_length>0&&r.final_live==0;
    return r;
}
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
        PatchObserver patch;patch.observe(t,d.render);
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
            step(d,door_track,{});patch.observe(t,d.render);log("pre",f);
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
            committed=t.commit_source_door_transition();patch.observe(t,d.render);
            arrival=t.player_position();arrival_heading=t.player_yaw();
            counter_at_arrival=t.source_door_event0().counter;
            log("commit",0);
            for(int f=0;f<400&&t.source_door_event0().active;++f){
                step(d,door_track,{});patch.observe(t,d.render);log("post",f);++event_frames;
                for(const auto& o:t.source_door_objects()){if(o.state>=3)area2_door_opened=1;if(area2_door_opened&&o.state==1)area2_door_closed=1;}
            }
            for(int f=0;f<10;++f){step(d,door_track,{});patch.observe(t,d.render);if(!t.player_contact().floor_y)++door_floorless;log("idle",f);}
            final_pos=t.player_position();
        }
        const bool door_pass=intent&&offered&&committed&&t.map_area_index()==2&&offered->destination_area==2&&
            lane16(*offered,0)==-256&&lane16(*offered,2)==-1&&lane16(*offered,4)==6272&&
            area1_door_opened&&arrival.x==expected.x&&arrival.y==expected.y&&arrival.z==expected.z&&arrival_heading==2048&&
            flag_7b0&&!t.source_door_event0().active&&area2_door_opened&&area2_door_closed&&
            !t.story_flag(0x7B0)&&!t.story_flag(0x7B1)&&!t.story_flag(0x7B2)&&!t.story_flag(0x7B3)&&t.story_flag(0x380)&&
            final_pos.z>6656&&door_floorless==0;
        // Both doors (Area 1, then Area 2 after the rebake) collapse and come back.
        const bool patch_pass=patch.hides>=1&&patch.restores>=1&&patch.changes>=2&&patch.max_overlay_idx==1&&
            !patch.patch.hidden(patch.door_idx);
        // Phase 4: Buster fire from the Azahar diagnostic spawn.
        auto g=build(inputs);
        const auto buster=run_buster(inputs,g);
        const bool pass=fall_pass&&door_pass&&patch_pass&&buster.pass;
        std::printf("{\"map_initialization_applied\":%s,\"map_initialization_writes\":%u,\"frames\":40,"
                    "\"floorless_frames\":%d,\"airborne_frames\":%d,\"below_floor_frames\":%d,"
                    "\"door_slab_contacts\":%d,\"first_failure_frame\":%d,\"final\":[%d,%d,%d],\"fall_result\":\"%s\",\n"
                    " \"door\":{\"press_status\":\"%s\",\"request_frame\":%d,\"at\":[%d,%d,%d],"
                    "\"request_destination\":[%d,%d,%d],\"request_area\":%d,\"request_stage\":%d,\"flag_7b0_set\":%s,"
                    "\"area1_door_opened\":%s,\"area1_max_z\":%d,\"load_offered_frame\":%d,"
                    "\"committed\":%s,\"area\":%zu,\"arrival\":[%d,%d,%d],\"expected_arrival\":[%d,%d,%d],"
                    "\"heading\":%u,\"counter_at_arrival\":%d,\"area2_door_opened\":%s,\"area2_door_closed\":%s,"
                    "\"event_frames_after_commit\":%d,\"final\":[%d,%d,%d],\"floorless_frames\":%d,\"result\":\"%s\"},\n"
                    " \"terrain_patch\":{\"generation_changes\":%d,\"hides\":%d,\"restores\":%d,\"max_overlay_idx\":%d,\"max_tracked_idx\":%zu,\"max_spans\":%zu,"
                    "\"last_door_idx\":%u,\"closed_descriptor\":%u,\"open_descriptor\":%u,\"closed_mid_prims\":%zu,\"open_mid_prims\":%zu,\"result\":\"%s\"},\n"
                    " \"buster\":{\"presses\":%d,\"admitted\":%d,\"spawned\":%d,\"released\":%d,\"state1_frames\":%d,"
                    "\"zero_length_frames\":%d,\"drawn_segment_frames\":%d,\"max_segment_length\":%ld,\"live_after\":%d,"
                    "\"endpoint\":\"CANDIDATE_FORWARD_RAY\",\"result\":\"%s\"},\n"
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
            patch.changes,patch.hides,patch.restores,patch.max_overlay_idx,patch.max_baked,patch.max_spans,unsigned(patch.door_idx),
            unsigned(patch.closed_descriptor),unsigned(patch.open_descriptor),patch.closed_prims,patch.open_prims,
            patch_pass?"PASS":"FAIL",
            buster.presses,buster.admitted,buster.spawned,buster.released,buster.state1_frames,
            buster.zero_segments,buster.drawn_segments,buster.max_length,buster.final_live,buster.pass?"PASS":"FAIL",
            pass?"PASS":"FAIL");
        return pass?0:1;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 2;}
}
