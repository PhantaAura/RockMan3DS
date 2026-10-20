// ST00 actions 5 (0x8010324C), 6 (0x80103554) and 10 (0x80103DFC) on an
// authored flat map. Trig values are authored, not the retail table.
#include "legends3ds/class20_03_late_actions.h"
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
RuntimeMap floor_map(){
    RuntimeMap m;RuntimeMapArea a;a.start_x=32;a.start_z=32;a.width=64;a.height=64;a.cell_count=4096;
    m.areas.push_back(a);m.cells.assign(4096,1);m.idx.resize(2);m.idx[1].descriptor_index=0;
    RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});m.contact_groups.push_back({-8192,8192,-8192,8192,0,1});
    m.contact_primitives.push_back({0,1024,3,0});return m;
}
Class20_03CallbackState actor(int16_t x,int16_t y,int16_t z,uint16_t heading){
    Class20_03CallbackState s;
    s.raw_14={x,y,z,0};
    for(size_t i=0;i<3;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;
    s.behavior.heading_56=heading;s.health_6e=40;
    return s;
}
// The bridge copies the previous snapshot before each body.
void snapshot(Class20_03CallbackState& s){s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;}
}

int main(){
    const auto r=refs();const auto map=floor_map();
    Class20_03MotionWorld world;world.map=&map;world.references=&r;world.source_a6=0;
    Class20_03LateActionInputs in;in.world=&world;in.combat_record_byte_a=7;
    in.target_heading_80031198=1024;in.sound_consumer_ready=true;

    // Holds leave the state untouched.
    auto s=actor(0,-1,0,0);s.behavior.action_09=10;const auto before=s;
    auto no_world=in;no_world.world=nullptr;
    assert(class20_03_step_late_action(s,no_world).status==Class20_03LateActionStatus::NeedsWorld);
    auto no_profile=in;no_profile.combat_record_byte_a.reset();
    assert(class20_03_step_late_action(s,no_profile).status==Class20_03LateActionStatus::NeedsCombatProfile);
    assert(s.raw_14==before.raw_14&&s.behavior.phase_0a==0);

    // Action 10: animation 4, turn32 toward the target, forward 52, 45 frames,
    // then cooldown 0x35E=45 and the common transition bit.
    int ended=-1;
    for(int f=0;f<60&&ended<0;++f){
        snapshot(s);const uint16_t h=s.behavior.heading_56;
        const auto t=class20_03_step_late_action(s,in);
        assert(t.status==Class20_03LateActionStatus::SourceBackedPartial);
        assert(s.behavior.animation_ac==4&&s.behavior.work_330==7&&s.behavior.phase_0a==1);
        assert(s.behavior.heading_56==uint16_t((h+class20_03_turn_step(1024,h,32))&4095));
        const int dx=s.raw_14[0]-s.previous_raw_2c[0],dz=s.raw_14[2]-s.previous_raw_2c[2];
        // forward 52 is q12*52 in 16.16 fixed: about 3.25 raw units a frame.
        assert(dx*dx+dz*dz>=4&&dx*dx+dz*dz<=17);
        if(s.behavior.flags_328&1u)ended=f;
    }
    assert(ended==44&&s.work_35e==45&&s.behavior.timer_32c==0);
    // Cue 1 queues sound 181 at the pre-motion position.
    auto cue=actor(0,-1,0,0);cue.behavior.action_09=10;auto cue_world=world;cue_world.source_a6=1;
    auto cue_in=in;cue_in.world=&cue_world;
    const auto cued=class20_03_step_late_action(cue,cue_in);
    assert(cued.sound_count==1&&cued.sounds[0]==181&&cued.sound_words[0][0]==0xffff0000u);
    cue_in.sound_consumer_ready=false;auto cue2=actor(0,-1,0,0);cue2.behavior.action_09=10;
    assert(class20_03_step_late_action(cue2,cue_in).status==Class20_03LateActionStatus::NeedsSoundConsumer);

    // Action 6: animation 6, timer 10, vertical 0x46 = 1..10, ends on 0x1000.
    auto k=actor(0,-1,0,0);k.behavior.action_09=6;
    for(int f=0;f<10;++f){
        snapshot(k);assert(class20_03_step_late_action(k,in).status==Class20_03LateActionStatus::SourceBackedPartial);
        assert(k.behavior.animation_ac==6&&k.behavior.phase_0a==1&&k.velocity_44[1]==f+1);
    }
    snapshot(k);assert(class20_03_step_late_action(k,in).status==Class20_03LateActionStatus::SourceBackedPartial);
    assert(k.behavior.phase_0a==2&&k.behavior.timer_32c<0&&k.velocity_44[1]==10);

    // Action 5: launch from rsin/rcos, 115 frames, then two frames to HP 0
    // and flags 0x41.
    auto d=actor(0,-1,0,1024);d.behavior.action_09=5;d.behavior.flags_328=8;
    assert(class20_03_step_late_action(d,in).status==Class20_03LateActionStatus::SourceBackedPartial);
    assert(d.behavior.animation_ac==4&&d.behavior.work_330==7&&!(d.behavior.flags_328&8u));
    assert(d.velocity_44[0]==-32&&d.velocity_44[2]==0&&d.work_4e==16&&d.behavior.timer_32c==114);
    int frames=1;
    while(d.behavior.phase_0a!=2&&frames<200){snapshot(d);
        const auto st=class20_03_step_late_action(d,in);
        assert(st.status==Class20_03LateActionStatus::SourceBackedPartial);++frames;}
    // 32 raw units a frame from rsin(1024)=4096 >> 7, for timers 99..0.
    assert(d.raw_14[0]==-32*100&&d.raw_14[2]==0);
    assert(frames==116&&d.behavior.timer_32c==1&&d.behavior.work_330==0);
    snapshot(d);(void)class20_03_step_late_action(d,in);
    assert(*d.health_6e==40&&!(d.behavior.flags_328&0x40u));
    snapshot(d);(void)class20_03_step_late_action(d,in);
    assert(*d.health_6e==0&&d.defeat_timer_360==0&&(d.behavior.flags_328&0x41u)==0x41u);
    // The packet bit 0x02000000 cuts the bounce short.
    auto p=actor(0,-1,0,0);p.behavior.action_09=5;
    (void)class20_03_step_late_action(p,in);p.packet_33c=0x02000001u;
    (void)class20_03_step_late_action(p,in);
    assert(p.behavior.phase_0a==2&&p.behavior.timer_32c==1&&p.behavior.work_330==0);
    std::cout<<"PASS: late actions 5/6/10 timers, motion, sounds, holds and the action5 defeat stores\n";
}
