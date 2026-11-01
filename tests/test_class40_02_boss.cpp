// ST00 class 0x40 subtype 0x02 (0x801076E8), the Area 3 tutorial boss, on an
// authored flat map: init, wait/turn/walk/charge/slam, hits, stagger, defeat
// (flag 0x606) and release. Trig and angle values are authored, not retail.
#include "legends3ds/class40_02_boss.h"
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
// CORDIC steps atan(2^-i) in 4096ths of a turn (authored from the formula).
SourceAngleSteps steps(){SourceAngleSteps t{};for(size_t i=0;i<t.size();++i)t[i]=int32_t(std::lround(std::atan(std::ldexp(1.0,-int(i)))*4096.0/6.283185307179586));return t;}
Class40_02State spawned(){
    Class40_02State s;s.work_00=1;s.parameter_03=1;s.raw_14={-256,-385,1001,0};
    for(size_t i=0;i<3;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;return s;
}
int32_t dist2(const Class40_02State& s,const Class40_02Inputs& in){
    const int32_t dx=s.raw_14[0]-in.player_raw[0],dz=s.raw_14[2]-in.player_raw[2];return dx*dx+dz*dz;
}
}

int main(){
    const auto r=refs();const auto map=floor_map();const auto st=steps();
    Class20_03MotionWorld world;world.map=&map;world.references=&r;world.bounds={-192,192,-192,192,-300,0};
    world.object_class=0x40;world.source_a6=0;
    Class40_02Inputs in;in.world=&world;in.random_sample=0;in.sound_consumer_ready=true;in.angle_steps=&st;
    in.record=Class40_02Record{0x20,0x50,5};in.work_80=0x1000000u;in.player_raw={-256,0,-1632};
    in.player_fixed={-256*65536,0,-1632*65536};in.player_heading=0;
    in.detected_8003648c=false;in.player_hp_800b521e=80;in.frame_a4=1;in.arms_allocated=true;

    // Pure chooser 0x80107B48 (ported from the earlier intro slice).
    auto c=choose_class40_02_80107b48(1024,1,5,0,0);
    assert(!c.reset_phase&&c.substate==1);
    c=choose_class40_02_80107b48(1024,0,5,1,0);assert(c.substate==3&&c.saved_0d==1&&c.flags_4e0==0);
    c=choose_class40_02_80107b48(1024,4,5,0x21,0);assert(c.substate==3&&c.saved_0d==1&&c.flags_4e0==0x20);
    c=choose_class40_02_80107b48(1024,3,4,1,0);assert(c.substate==4);
    c=choose_class40_02_80107b48(1024,2,5,1,0);assert(c.substate==1);
    c=choose_class40_02_80107b48(1024,1,5,0x21,0);assert(c.substate==1);
    c=choose_class40_02_80107b48(1024,1,5,1,0);assert(c.substate==0);
    c=choose_class40_02_80107b48(0,2,5,1,0);assert(c.substate==0&&c.defeat_started&&c.flags_4e0==0x40);
    c=choose_class40_02_80107b48(0,2,5,0x41,0);assert(c.substate==0&&!c.defeat_started);
    c=choose_class40_02_80107b48(0,2,5,1,3);assert(c.substate==0&&!c.defeat_started&&c.flags_4e0==0);

    // 0x800398F8 distance-scaled shake.
    assert(!source_camera_shake_800398f8(0x1e,2000,5000,5001));
    assert(*source_camera_shake_800398f8(0x1e,2000,5000,5000)==0);
    assert(*source_camera_shake_800398f8(0x1e,2000,5000,3500)==15);
    assert(*source_camera_shake_800398f8(0x1e,2000,5000,0)==50);

    // Init 0x801078B8: HP byte08<<5, flags 0x1E, gauge, floor, arms on limbs 2/4.
    auto b=spawned();
    auto held=b;in.arms_allocated=false;
    assert(step_class40_02_801076e8(held,in).status==Class40_02Status::NeedsArms&&held.state_08==0);
    in.arms_allocated=true;
    auto out=step_class40_02_801076e8(b,in);
    assert(out.status==Class40_02Status::SourceBackedPartial&&out.animation==Class40_02Animation::Restart);
    assert(b.state_08==1&&b.substate_09==0&&b.health_6e==1024&&b.health_max_b0==1024&&b.flags_4e0==0x1e);
    assert(b.raw_14[1]==0&&b.work_00==3&&b.gauge_registered&&out.register_gauge_8005d620);
    assert(out.register_proxy&&out.proxy_word_18==0x04880008u);
    assert(b.arms[0].part_0d==2&&b.arms[1].part_0d==4);

    // State 1 steps the detection every update; without it the slot holds.
    held=b;in.detected_8003648c.reset();
    assert(step_class40_02_801076e8(held,in).status==Class40_02Status::NeedsDetection);
    in.detected_8003648c=false;
    for(int i=0;i<5;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==0&&b.phase_0a==1);
    // Detection: substate 0 -> 3 (turn, then +0x0D = 1).
    in.detected_8003648c=true;
    out=step_class40_02_801076e8(b,in);
    assert(b.substate_09==3&&b.saved_0d==1&&b.phase_0a==0&&(b.flags_4e0&0x20));
    out=step_class40_02_801076e8(b,in);
    assert(b.animation_ac==3&&b.timer_4e4==11&&out.animation==Class40_02Animation::Restart);
    assert(out.sound_count==1&&out.sounds[0]==200);                  // +0xA4 bit 0
    for(int i=0;i<11;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==1&&b.phase_0a==0);
    // Substate 1 walks toward the player while far (+0x80 >= 0xC8000).
    const auto d0=dist2(b,in);
    out=step_class40_02_801076e8(b,in);
    assert(b.animation_ac==1&&b.timer_4e4==0x4f);
    for(int i=0;i<10;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.raw_14[2]==1001-16*11&&b.heading_56==0);                  // 16 units per update, as observed
    assert(dist2(b,in)<d0&&b.substate_09==1);
    // The 80-tick decision timer selects the charge (facing) with +0x4E6 = 60.
    for(int i=0;i<200&&b.substate_09==1;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==2||b.substate_09==3);
    if(b.substate_09==3){for(int i=0;i<40&&b.substate_09!=2;++i)(void)step_class40_02_801076e8(b,in);}
    assert(b.substate_09==2&&b.timer_4e6==0x3c);
    out=step_class40_02_801076e8(b,in);
    assert(b.animation_ac==2&&b.timer_4e4==0x3b);
    // Footstep shockwaves on cue +0xA6 == 1: three effect128/0x42 points on
    // the floor, the position restored from +0x2C (no step on that frame).
    world.source_a6=1;
    const auto before_cue=b.raw_14;
    out=step_class40_02_801076e8(b,in);
    if(b.substate_09==2){
        assert(out.shockwave_count==3&&out.camera_shake_800398f8&&out.sounds[out.sound_count-1]==0xc2);
        assert(b.raw_14==before_cue&&out.shockwaves[0].point_38[1]==0);
    }
    world.source_a6=0;
    // Close to the player: the charge ends, 2 -> 1, then the slam.
    in.work_80=0x40000u;
    for(int i=0;i<40&&b.substate_09!=6;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==6||b.substate_09==3);
    for(int i=0;i<40&&b.substate_09!=6;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==6);
    // Slam 0x80109638: requests 11/12/13, countdowns 12/9/19/9/14 as observed.
    out=step_class40_02_801076e8(b,in);
    assert(b.animation_ac==0xb&&b.timer_4e4==12&&out.sounds[0]==0xbf);
    for(int i=0;i<13;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.phase_0a==2&&b.timer_4e4==9&&b.arms[0].substate_09==3&&b.arms[1].substate_09==3);
    for(int i=0;i<9;++i)(void)step_class40_02_801076e8(b,in);
    out=step_class40_02_801076e8(b,in);
    assert(b.phase_0a==3&&b.animation_ac==0xc&&b.timer_4e4==19&&out.sounds[0]==0xc0);
    bool shocked=false;
    for(int i=0;i<19;++i){out=step_class40_02_801076e8(b,in);
        if(out.shockwave_count){shocked=true;assert(b.timer_4e4==3&&out.shockwave_count==3&&out.sounds[0]==0xc1);}}
    assert(shocked);
    out=step_class40_02_801076e8(b,in);
    assert(b.phase_0a==4&&b.timer_4e4==9);
    for(int i=0;i<10;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.phase_0a==5&&b.animation_ac==0xd&&b.timer_4e4==14);
    for(int i=0;i<15;++i)(void)step_class40_02_801076e8(b,in);
    assert(b.substate_09==1&&b.phase_0a==0);                          // detected -> 1
    assert(b.arms[0].substate_09==0&&b.arms[0].scale_y_f8==0x1000);  // pulse finished

    // Hits 0x8010A448: no-damage type 0xA3; Buster damage 48 -> 0xA0;
    // stagger mode (bit 4) accumulates +0x4EA to 0x50<<2 then substate 4.
    auto h=b;
    h.packet_4f4=0x00000030u;out=step_class40_02_801076e8(h,in);
    assert(out.sounds[0]==0xa3&&h.health_6e==1024);
    h.packet_4f4=0x08000030u;out=step_class40_02_801076e8(h,in);
    assert(out.sounds[0]==0xa0&&h.health_6e==976&&h.previous_health_b2==1024&&h.shade_15e==0x7fff&&h.packet_4f4==0);
    assert(h.stagger_4ea==47);
    for(int i=0;i<6&&h.substate_09!=4;++i){h.packet_4f4=0x08000030u;out=step_class40_02_801076e8(h,in);}
    assert(h.substate_09==4&&h.stagger_4ea==0&&out.sounds[0]==0xa2);
    out=step_class40_02_801076e8(h,in);
    assert(h.animation_ac==4&&!(h.flags_4e0&4));
    for(int i=0;i<27;++i)(void)step_class40_02_801076e8(h,in);
    assert(h.substate_09==3&&(h.flags_4e0&4));                         // 4 -> 3 -> +0x0D

    // Defeat in the tutorial (stage 0): HP 0 only with a living player, then
    // the next choice sets flag 0x606 once and drops the gauge.
    auto k=b;k.health_6e=24;k.packet_4f4=0x08000030u;
    out=step_class40_02_801076e8(k,in);
    assert(k.health_6e==0&&!(k.flags_4e0&2)&&out.player_lock_800b55f9&&!out.set_flag_606);
    bool flagged=false;int flags=0;
    for(int i=0;i<200;++i){out=step_class40_02_801076e8(k,in);if(out.set_flag_606){flagged=true;++flags;
        assert(out.unregister_gauge_8005d6e8&&!k.gauge_registered&&k.substate_09==0&&(k.flags_4e0&0x40));}}
    assert(flagged&&flags==1&&k.substate_09==0);
    k.packet_4f4=0x08000030u;out=step_class40_02_801076e8(k,in);assert(k.health_6e==0&&!out.sound_count);
    auto dead=b;dead.health_6e=24;dead.packet_4f4=0x08000030u;in.player_hp_800b521e=-1;
    (void)step_class40_02_801076e8(dead,in);assert(dead.health_6e==1&&(dead.flags_4e0&2));
    in.player_hp_800b521e=80;

    // Event 3's constructor writes state 2: release with both arms.
    k.state_08=2;out=step_class40_02_801076e8(k,in);
    assert(out.released&&out.release_arms&&k.released);
    assert(step_class40_02_801076e8(k,in).status==Class40_02Status::Released);

    // Parameter-0 substates are held, never invented.
    auto u=b;u.substate_09=5;const auto u0=u;
    assert(step_class40_02_801076e8(u,in).status==Class40_02Status::UnsupportedSubstate&&u.substate_09==u0.substate_09&&u.health_6e==u0.health_6e);
    u.state_08=4;assert(step_class40_02_801076e8(u,in).status==Class40_02Status::UnsupportedState);
    std::cout<<"class40_02 boss: ok\n";
    return 0;
}
