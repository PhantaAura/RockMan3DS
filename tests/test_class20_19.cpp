// ST00 class 0x20 subtype 0x19 (0x80104864) on an authored flat map:
// dormant drop-in on the wake flag, idle/selector, hit/stagger/defeat.
// Trig and angle values are authored, not the retail tables.
#include "legends3ds/class20_19.h"
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
Class20_19State spawned(uint8_t parameter){
    Class20_19State s;s.work_00=1;s.parameter_03=parameter;s.raw_14={0,-200,0,0x607};
    for(size_t i=0;i<3;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;return s;
}
SourceAngleSteps steps(){SourceAngleSteps t{};for(size_t i=0;i<t.size();++i)t[i]=int32_t(i*40);return t;}
}

int main(){
    const auto r=refs();const auto map=floor_map();const auto st=steps();
    Class20_03MotionWorld world;world.map=&map;world.references=&r;world.bounds={-32,32,-32,32,-32,0};world.source_a6=0;
    Class20_19Inputs in;in.world=&world;in.random_sample=0;in.sound_consumer_ready=true;in.angle_steps=&st;
    in.record=Class20_19Record{32,16,16,6,0x103};in.work_80=0x100000u;in.player_raw={0,0,3000};
    in.player_fixed={0,0,3000*65536};in.floor_sample_80048e6c=uint16_t(0);

    // Trap-room variant: hidden and dormant until 0x607.
    auto d=spawned(1);
    auto out=step_class20_19_80104864(d,in);
    assert(out.status==Class20_19Status::SourceBackedPartial);
    assert(d.state_08==1&&d.action_09==4&&d.phase_0a==0xFF&&!(d.work_00&2)&&d.flags_348==0x48&&!out.register_proxy);
    assert(d.health_6e==32&&d.raw_14[1]==-200&&d.wake_flag_37c==0x607&&d.raw_14[3]==0); // no floor sample while hidden
    out=step_class20_19_80104864(d,in);assert(d.phase_0a==0xFF);
    in.wake_flag_set=true;in.random_sample=0x10;
    const auto before=d.raw_14[1];
    out=step_class20_19_80104864(d,in);
    assert((d.work_00&2)&&(d.flags_348&6)&&!(d.flags_348&0x40)&&d.phase_0a==1&&d.timer_34c==15);
    assert(d.velocity_44[0]==0&&d.accel_4e==0x30&&d.raw_14[1]<before&&out.register_proxy);  // upward hop
    // Drops until two floor contacts, then the action ends (flags 9) and becomes 3.
    bool ended=false;
    for(int i=0;i<200&&!ended;++i){(void)step_class20_19_80104864(d,in);ended=d.action_09==3;}
    assert(ended&&(d.flags_348&8));

    // Normal variant: floor sample at init, then 60 idle frames.
    in.wake_flag_set=false;
    auto n=spawned(0);
    out=step_class20_19_80104864(n,in);
    assert(n.action_09==0&&n.raw_14[1]==0&&(n.work_00&2)&&out.register_proxy&&out.proxy_word_18==0x04880000u);
    for(int i=0;i<61;++i)(void)step_class20_19_80104864(n,in);
    assert(n.action_09==3);                                      // rand&15 == 0 -> action 3
    // Selector picks: r 5 -> wander +0x10, r 15 -> wander -0x10.
    auto w=spawned(0);(void)step_class20_19_80104864(w,in);in.random_sample=5;
    for(int i=0;i<61;++i)(void)step_class20_19_80104864(w,in);
    assert(w.action_09==1&&w.turn_66==0x10);
    auto v=spawned(0);in.random_sample=0;(void)step_class20_19_80104864(v,in);in.random_sample=15;
    for(int i=0;i<61;++i)(void)step_class20_19_80104864(v,in);
    assert(v.action_09==1&&v.turn_66==-0x10);

    // Hits: non-damaging 0xA3; damage 0xA0 below the stagger limit.
    auto h=spawned(0);in.random_sample=0;(void)step_class20_19_80104864(h,in);
    h.packet_35c=0x00000004u;out=step_class20_19_80104864(h,in);
    assert(out.sound_count==1&&out.sounds[0]==0xA3&&h.health_6e==32);
    h.packet_35c=0x08000004u;out=step_class20_19_80104864(h,in);
    assert(out.sounds[0]==0xA0&&h.health_6e==28&&h.stagger_352==3&&h.packet_35c==0);
    // A strong hit staggers into the knockback hop.
    h.packet_35c=0x28000004u;h.hit_point_364={0,0,-500};out=step_class20_19_80104864(h,in);
    assert(out.sounds[0]==0xA2&&h.action_09==4&&h.stagger_352==0&&h.shade_15e==0x7fff);
    // Lethal: flags 0x88, then the 0x37E countdown releases with 0xA1.
    auto k=spawned(0);(void)step_class20_19_80104864(k,in);
    k.packet_35c=0x08000040u;out=step_class20_19_80104864(k,in);
    assert((k.flags_348&0x80)&&!(k.flags_348&2)&&k.action_09==4); // the hop start clears 0x28
    bool released=false;
    for(int i=0;i<6&&!released;++i){out=step_class20_19_80104864(k,in);released=out.released_80031824;}
    assert(released&&out.defeat_reward_80047148&&out.defeat_effect_128_65&&out.sounds[out.sound_count-1]==0xA1);
    assert(step_class20_19_80104864(k,in).status==Class20_19Status::Released);
    // No random sample: no commit.
    auto z=spawned(0);in.random_sample.reset();
    assert(step_class20_19_80104864(z,in).status==Class20_19Status::NeedsRandomSample&&z.state_08==0);
    std::cout<<"PASS: class20/19 dormant drop-in, idle/selector, hit 0xA3/0xA0/0xA2 stagger hop, defeat 0xA1\n";
}
