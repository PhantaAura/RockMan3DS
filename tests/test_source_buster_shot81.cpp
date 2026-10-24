// Mega Buster shot subtype 0x81 (0x800437F8 / 0x80043A8C / 0x80043E78 /
// 0x80043E00) and its impact effect 0x82 (0x80044AEC / 0x80044BA0).
// Stats are the tutorial row-1 level-0 values measured in PCSX-Redux
// (damage 24, range 18, radius 24, speed 100); trig is authored.
#include "legends3ds/child_runtime.h"
#include "legends3ds/class20_03_hit.h"
#include "legends3ds/source_buster_profile.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace {
using namespace l3;
uint32_t r32(const SourceBusterWork& w,size_t o){uint32_t v=0;for(size_t i=0;i<4;++i)v|=uint32_t(w[o+i])<<(8*i);return v;}
int16_t s16(const SourceBusterWork& w,size_t o){return int16_t(uint16_t(w[o]|(w[o+1]<<8)));}
ReferenceTables refs(){
    ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096)),int16_t(std::lround(std::cos(a)*4096))};}
    return r;
}
// 64x64 cells of one authored flat contact list (no solid at shot height).
RuntimeMap flat_map(){
    RuntimeMap m;RuntimeMapArea a;a.start_x=32;a.start_z=32;a.width=64;a.height=64;a.cell_count=4096;
    m.areas.push_back(a);m.cells.assign(4096,1);m.idx.resize(2);m.idx[1].descriptor_index=0;
    RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});m.contact_groups.push_back({-8192,8192,-8192,8192,0,1});
    m.contact_primitives.push_back({0,1024,3,0});return m;
}
SourceRngProfile rng_profile(uint32_t seed){
    SourceRngProfile p;
    const auto digest=[](const char* hex){SourceRngDigest d{};for(size_t i=0;i<32;++i){unsigned v=0;std::sscanf(hex+i*2,"%2x",&v);d[i]=uint8_t(v);}return d;};
    p.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    p.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    p.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    p.trace_sha256.fill(23);p.validated_prefix_sha256=p.trace_sha256;
    p.scope=SourceRngCaptureScope::WholeBoundedCapture;p.warnings=3;
    p.origin_seed32=seed;p.origin_game_entry_count=1;p.origin_vsync=1062;
    p.actual_rand_returns=1;p.trace_rows=5;p.validated_prefix_rows=5;
    p.captured_bytes=1200;p.validated_prefix_bytes=1200;p.final_rand_ordinal=1;p.observed_consumer_count=1;
    return p;
}
SourceBusterPlayer player_at(int16_t x,int16_t y,int16_t z,uint16_t heading){
    SourceBusterProfile profile;profile.selection[0].subtype_80089a8b=0x80;profile.selection[0].initial_counter_4fa=32767;
    profile.selection[1].subtype_80089a8b=0x81;profile.selection[1].initial_counter_4fa=3;
    SourceBusterPlayer p;initialize_source_buster_lanes_8003be6c(profile,p,0,1);
    p.class_01=0;p.heading_56=heading;
    p.lanes[1].muzzle_3c8_or_3e0=std::array<uint16_t,3>{{uint16_t(x),uint16_t(y),uint16_t(z)}};
    return p;
}
struct Rig {
    ChildRuntime owner;ReferenceTables r=refs();RuntimeMap map=flat_map();
    SourceAudioQueue audio;std::optional<uint32_t> mask=0xffu;SourceBusterFrameContext frame;
    Rig(){
        initialize_source_audio_queue_80019e14(audio);
        frame.proxy_mask_80098798=&mask;frame.audio=&audio;frame.map=&map;frame.area_index=0;frame.references=&r;
        frame.shot81=SourceBusterShot81Stats{24,18,24,100};
    }
};
}

int main(){
    // Square lane: stack10 0 selects +171 (subtype 0x81) at muzzle +3E0.
    {
        Rig rig;auto player=player_at(100,-80,-5000,0);
        const auto shot=rig.owner.spawn_source_buster_80035724(player,0,-1,rig.mask);
        assert(shot.status==SourceBusterStatus::SourceBackedPartial&&shot.source_identity==source_buster_base);
        const auto& w=rig.owner.buster_pool().slots[0];
        assert(w[2]==0x81&&s16(w,0x14)==100&&s16(w,0x18)==-5000&&*player.lanes[1].counter_4fa==2);
        // Missing stats hold without changing the work.
        auto held_frame=rig.frame;held_frame.shot81.reset();const auto before=w;
        const auto held=rig.owner.tick_source_busters_80017154(held_frame);
        assert(held.held==1&&held.status==SourceBusterCallbackStatus::NeedsShotProfile&&rig.owner.buster_pool().slots[0]==before);
        // State 0: velocity (0,0,-4096)*100>>4, radius 24, sound 0x9A, register.
        const auto first=rig.owner.tick_source_busters_80017154(rig.frame);
        assert(first.committed==1&&first.registered==1&&rig.audio.count==1&&rig.owner.hit_proxy_bank().count==2);
        assert(w[8]==1&&s16(w,0x44)==0&&s16(w,0x46)==0&&s16(w,0x48)==-25600&&s16(w,0x98)==24&&w[0x0b]==0);
        assert(r32(w,0x78)==0x80097da0u&&(w[0]&0x2a)==0x2a);
        // State 1: 100 units a frame for 18 frames, previous point trails by one.
        int moves=0;
        for(int f=0;f<40&&rig.owner.buster_pool().slots[0][0];++f){
            rig.owner.reset_frame_queues();
            const int16_t z_before=s16(w,0x18);
            const auto t=rig.owner.tick_source_busters_80017154(rig.frame);
            assert(t.held==0&&t.impacts==0);
            if(t.released){assert(moves==18);break;}
            ++moves;assert(s16(w,0x18)==z_before-100&&s16(w,0x30)==z_before&&s16(w,0x14)==100);
        }
        assert(moves==18&&rig.owner.buster_pool().pending_release_80098a60==0x80000000u);
        rig.owner.consume_pending_releases();assert(rig.owner.buster_pool().allocated_800989f0==0);
    }
    // Heading 1024: the shot flies along -x (RotY third column, -sin).
    {
        Rig rig;auto player=player_at(0,-80,-5000,1024);
        (void)rig.owner.spawn_source_buster_80035724(player,0,-1,rig.mask);
        (void)rig.owner.tick_source_busters_80017154(rig.frame);
        const auto& w=rig.owner.buster_pool().slots[0];
        assert(s16(w,0x44)==-25600&&s16(w,0x48)==0);
    }
    // Enemy contact: the pair packet carries damage 24 | 0x08000000; the next
    // frame spawns impact effect 0x82 (+0C 0) and releases the shot.
    {
        Rig rig;auto player=player_at(100,-80,-5000,0);
        (void)rig.owner.spawn_source_buster_80035724(player,0,-1,rig.mask);
        (void)rig.owner.tick_source_busters_80017154(rig.frame);
        SourceHitProxyTarget enemy;enemy.source_address=0x800b103cu;
        SourceHitProxyRegistration reg;reg.pointed_words={{uint32_t(uint16_t(100))|(uint32_t(uint16_t(-80))<<16),uint16_t(-5000)}};
        reg.a1=80;reg.a3=0x8000;reg.stack10=0x20;reg.stack18=0x04880000;reg.stack1c=0x00800000;
        reg.target=&enemy;reg.mask_80098798=0;
        assert(source_hit_proxy_register_800341bc(rig.owner.hit_proxy_bank(),reg).status==SourceHitProxyStatus::SourceBackedPartial);
        SourceHitProxyRules rules;rules.unsigned_80097d00.fill(255);
        const auto pairs=rig.owner.consume_proxy_pairs(rules);
        assert(pairs.overlapping_pairs==1&&enemy.packet_00==0x08000018u);
        Class20_03CallbackState enemy_state;enemy_state.packet_33c=enemy.packet_00;enemy_state.health_6e=64;
        Class20_03HitInputs hit;hit.packet_33c=enemy.packet_00;hit.sound_consumer_ready=true;
        hit.selected_record=Class20_03HitRecordByte09{0,32}; // authored byte09
        const auto damage=class20_03_consume_hit_801041b0(enemy_state,hit);
        assert(damage.status==Class20_03HitStatus::SourceBackedPartial&&*enemy_state.health_6e==40);
        const auto impact=rig.owner.tick_source_busters_80017154(rig.frame);
        assert(impact.impacts==1&&impact.map_impacts==0&&impact.released==1);
        // The next shot reuses slot 0 (whose +0xC4 still holds the hit): it
        // must fly, not republish the old packet and impact on frame 2.
        rig.owner.consume_pending_releases();rig.owner.reset_frame_queues();
        (void)rig.owner.spawn_source_buster_80035724(player,0,-1,rig.mask);
        (void)rig.owner.tick_source_busters_80017154(rig.frame);
        rig.owner.reset_frame_queues();
        const auto second=rig.owner.tick_source_busters_80017154(rig.frame);
        assert(second.impacts==0&&second.released==0);
        const auto& e=rig.owner.effect_pool().slots[0];
        assert(e[0]==9&&e[2]==0x82&&e[3]==1&&e[0x0c]==0);
        // Effect 0x82: state 0 draws one rand (RA 0x80044B50), then 6 steps.
        SourceRngService rng;assert(install_source_rng_service(rng,rng_profile(0x1234u),SourceRngMode::CandidateOpenBios)==SourceRngStatus::Ready);
        SourceEffect128Inputs in;in.audio=&rig.audio;in.rng=&rng;in.rng_frame=1;in.source_generation=1;
        in.impact82_timers_8008a240=std::array<uint8_t,8>{{1,1,1,1,1,1,0,0}};
        auto no_rng=in;no_rng.rng=nullptr;
        assert(rig.owner.tick_effects(no_rng).held==1&&rig.owner.effect_pool().slots[0][8]==0);
        const auto ordinal=rng.native_global_ordinal();
        assert(rig.owner.tick_effects(in).committed==1&&rng.native_global_ordinal()==ordinal+1&&rig.owner.effect_pool().slots[0][8]==1);
        int steps=0;
        while(rig.owner.effect_pool().slots[0][0]&&steps<20){(void)rig.owner.tick_effects(in);++steps;}
        assert(steps==6);
    }
    // Map: a point off the grid is "no cell" (0x40000000), a map impact (+0C 1).
    {
        Rig rig;auto player=player_at(100,-80,-16300,0);
        (void)rig.owner.spawn_source_buster_80035724(player,0,-1,rig.mask);
        (void)rig.owner.tick_source_busters_80017154(rig.frame);
        int frames=0;SourceBusterBatch t;
        do{rig.owner.reset_frame_queues();t=rig.owner.tick_source_busters_80017154(rig.frame);++frames;}while(!t.impacts&&frames<10);
        assert(t.impacts==1&&t.map_impacts==1&&rig.owner.effect_pool().slots[0][0x0c]==1);
    }
    // Lock-on homing (0x80043B54): a shot leased to an enemy proxy turns toward
    // the target point +BC each frame (0x80033F04 with 1060/143/71 by age) and
    // drops the lease on its ninth frame. Authored inverse-sqrt 0x8007A694 table.
    {
        Rig rig;SourceNormalizationLookup lookup{};
        for(size_t i=0;i<lookup.size();++i)lookup[i]=int16_t(std::lround(32768.0/std::sqrt(double(i+64))));
        rig.frame.normalization=&lookup;
        SourceHitProxyTarget enemy;enemy.source_address=0x800b103cu;
        SourceHitProxyRegistration reg;
        reg.pointed_words={{uint32_t(uint16_t(1000))|(uint32_t(uint16_t(-80))<<16),uint32_t(uint16_t(-6000))}};
        reg.a1=80;reg.a3=0x8000;reg.stack10=0x20;reg.stack18=0x04880000;reg.stack1c=0x00800000;reg.target=&enemy;reg.mask_80098798=0;
        assert(source_hit_proxy_register_800341bc(rig.owner.hit_proxy_bank(),reg).status==SourceHitProxyStatus::SourceBackedPartial);
        auto player=player_at(0,-80,-5000,0);
        const auto shot=rig.owner.spawn_source_buster_80035724(player,0,0,rig.mask);
        assert(shot.status==SourceBusterStatus::SourceBackedPartial&&shot.lease_index>=0);
        const auto& w=rig.owner.buster_pool().slots[0];
        assert(w[3]==0x80);
        // Each frame the leased enemy re-registers through the owner table, which
        // publishes its point into the shot's +BC/+C0.
        const auto frame=[&]{
            rig.owner.reset_frame_queues();
            auto again=reg;again.owners_800be478=rig.owner.proxy_owner_pointers();again.mask_80098798=rig.mask;
            (void)source_hit_proxy_register_800341bc(rig.owner.hit_proxy_bank(),again);
            rig.owner.publish_proxy_owner_points();
            return rig.owner.tick_source_busters_80017154(rig.frame);
        };
        assert(frame().committed==1&&s16(w,0x44)==0);               // state 0: straight -z
        assert(s16(w,0xbc)==1000&&s16(w,0xc0)==-6000);
        int16_t vx_prev=0;int turning=0;
        for(int f=1;f<=12&&w[0];++f){
            const auto t=frame();
            assert(t.held==0);
            if(t.impacts||t.released)break;
            if(f<=8){assert(w[3]==0x80&&s16(w,0x44)>=vx_prev);if(s16(w,0x44)>vx_prev)++turning;vx_prev=s16(w,0x44);}
            if(f==9)assert(w[3]==0);                                     // ninth frame: lease dropped
        }
        assert(turning>=3&&vx_prev>0);
        // Without the 0x8007A694 table a leased shot holds instead of flying straight.
        Rig held;held.frame.normalization=nullptr;
        assert(source_hit_proxy_register_800341bc(held.owner.hit_proxy_bank(),reg).status==SourceHitProxyStatus::SourceBackedPartial);
        (void)held.owner.spawn_source_buster_80035724(player,0,0,held.mask);
        const auto held_frame=[&]{
            held.owner.reset_frame_queues();
            auto again=reg;again.owners_800be478=held.owner.proxy_owner_pointers();again.mask_80098798=held.mask;
            (void)source_hit_proxy_register_800341bc(held.owner.hit_proxy_bank(),again);held.owner.publish_proxy_owner_points();
            return held.owner.tick_source_busters_80017154(held.frame);
        };
        (void)held_frame();                                             // state 0 keeps the lease
        assert(held_frame().status==SourceBusterCallbackStatus::NeedsNormalization);
    }
    std::cout<<"PASS: Buster shot 0x81 lane/state0/18-frame flight/enemy 24 damage/map hit/effect 0x82 rand+6 steps/lock-on homing\n";
}
