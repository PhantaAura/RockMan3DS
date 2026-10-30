// ST00 class 0x20 subtype 0x1F (0x801062EC): init, the work80 wait, the
// 9-frame lunge with its ScaleMatrix z growth, and the hand-off flag into
// subtype 3 parameter 0x40. Trig values are authored, not the retail table.
#include "legends3ds/actor_runtime.h"
#include "legends3ds/class20_1f_popout.h"
#include "legends3ds/source_rng_service.h"
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
Class20_1FState fresh(int16_t x,int16_t y,int16_t z,uint16_t heading){
    Class20_1FState s;s.raw_14={x,y,z,0};
    for(size_t i=0;i<3;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.heading_56=heading;return s;
}

void reducer(){
    const auto r=refs();
    auto s=fresh(2560,-129,-7040,0);
    Class20_1FInputs in;in.references=&r;in.initial_health=3;
    // The prologue rand() is unconditional: no sample, no commit.
    auto held=s;assert(step_class20_1f_801062ec(held,in).status==Class20_1FStatus::NeedsRandomSample);
    assert(held.state_08==0&&held.flags_348==0);
    in.random_sample=0x1234;
    auto out=step_class20_1f_801062ec(s,in);
    assert(out.status==Class20_1FStatus::SourceBackedPartial);
    assert(s.state_08==1&&s.action_09==0&&s.flags_348==0x2C&&(s.work_00&2)&&s.health_6e==3);
    assert(s.random_34a==0x1234&&s.scale_z_384==0&&out.proxy_800341bc);
    // Action 0 needs work80; at 0x4E201 it keeps waiting.
    held=s;in.work_80.reset();
    assert(step_class20_1f_801062ec(held,in).status==Class20_1FStatus::NeedsWork80);
    in.work_80=0x4E201u;(void)step_class20_1f_801062ec(s,in);
    assert(s.action_09==0&&s.phase_0a==1);
    // Bit 2 is clear (0x2C): a hit packet is never read while hidden.
    in.packet_35c=0x08000040u;(void)step_class20_1f_801062ec(s,in);
    assert(s.health_6e==3);in.packet_35c=0;
    // 0x4E200 < 0x4E201: flag 1 -> action 1 the same frame.
    in.work_80=0x4E200u;(void)step_class20_1f_801062ec(s,in);
    assert(s.action_09==1&&s.phase_0a==0&&!(s.flags_348&1u));
    // Lunge: 8 frames forward 0x40 (+0x100 z scale), then 0x200 (+0x800).
    const auto start=s.fixed_1c;
    const auto slow=reference_player_motion_delta(r,0,0x40,0,0),fast=reference_player_motion_delta(r,0,0x200,0,0);
    for(int frame=0;frame<8;++frame){
        (void)step_class20_1f_801062ec(s,in);
        assert(s.action_09==1&&s.timer_34c==7-frame&&s.scale_z_384==0x100*(frame+1));
    }
    out=step_class20_1f_801062ec(s,in);
    assert(s.scale_z_384==0x1000&&s.morph_to_class20_03&&s.state_08==0&&s.action_09==0&&s.phase_0a==0);
    assert(s.fixed_1c[0]==start[0]+8*slow.dx_fixed+fast.dx_fixed);
    assert(s.fixed_1c[2]==start[2]+8*slow.dz_fixed+fast.dz_fixed);
    assert(s.raw_14[2]==reference_fixed_to_short_80030150(s.fixed_1c[2]));
    assert(s.previous_raw_2c!=s.raw_14);
    // 0x80106800 with bit 2: sound 0xA3 always, HP only for 0x18000000.
    auto h=fresh(0,0,0,0);h.state_08=1;h.flags_348=2;h.health_6e=5;h.phase_0a=1;
    in.work_80=0x100000u;in.packet_35c=0x00000002u;
    held=h;assert(step_class20_1f_801062ec(held,in).status==Class20_1FStatus::NeedsSoundConsumer);
    in.sound_consumer_ready=true;
    out=step_class20_1f_801062ec(h,in);
    assert(out.sound_8001a050==0xA3&&h.health_6e==5);
    in.packet_35c=0x10000005u;out=step_class20_1f_801062ec(h,in);
    assert(h.health_6e==0&&h.action_09==1&&h.phase_0a==1&&(h.flags_348&4u)&&!(h.flags_348&2u));
    // State 2 is the 0x80031824 release; unknown states hold.
    auto rel=fresh(0,0,0,0);rel.state_08=2;assert(step_class20_1f_801062ec(rel,in).status==Class20_1FStatus::Released);
    rel.state_08=3;assert(step_class20_1f_801062ec(rel,in).status==Class20_1FStatus::UnknownState);
}

void actor_bridge(){
    const auto r=refs();
    ImportedActorInstance a;a.source_class=0x20;a.subtype=0x1F;a.initializer=0x8010648C;
    a.source_dispatch_flag0=1;a.source_dispatch_flag6=1;a.raw_scale={{512,512,512}};
    a.source_popout=fresh(2560,-129,-7040,0);
    assert(sample_imported_actor_target(a,2560,-7040+100,nullptr).has_value());
    auto t=tick_imported_actor_popout_1f(a,r,uint16_t(1),std::nullopt);
    assert(t.committed&&a.source_popout->state_08==1&&a.raw_scale[2]==0); // flat while hidden
    // Dispatcher bit 0 gates the callback.
    a.source_dispatch_flag6=0;assert(!tick_imported_actor_popout_1f(a,r,uint16_t(1),0u).committed);
    a.source_dispatch_flag6=1;
    (void)tick_imported_actor_popout_1f(a,r,uint16_t(1),0u);   // wait -> action 1
    for(int i=0;i<4;++i)(void)tick_imported_actor_popout_1f(a,r,uint16_t(1),0u);
    assert(a.raw_scale[2]==512*0x400/4096&&a.source_origin[2]==a.source_popout->raw_14[2]);
    for(int i=0;i<4;++i)(void)tick_imported_actor_popout_1f(a,r,uint16_t(1),0u);
    t=tick_imported_actor_popout_1f(a,r,uint16_t(1),0u);
    assert(t.morph_pending&&a.raw_scale[2]==512);
    // Next frame belongs to the subtype-3 init, not to 0x801062EC.
    t=tick_imported_actor_popout_1f(a,r,uint16_t(1),0u);
    assert(!t.committed&&t.morph_pending);
    // The new caller RA is admitted by the class03 RNG stage.
    static_assert(kSourceClass1FRngCallerRa==0x80106378u,"0x80106370 jal + 8");
}
// The hidden 0x1F registers 0x800341BC every frame (flags 0x2C has bit 8):
// 0x80106410..0x80106470 with radius 0x50, 0x8000, stack 0x04880000|+350,
// 0x00800000, 0x80, then clears +0x35C. Bit 2 is clear, so a hit packet only
// gets cleared: no 0xA3 sound and no damage.
void hidden_proxy(){
    const auto r=refs();
    ImportedActorInstance a;a.source_class=0x20;a.subtype=0x1F;a.initializer=0x8010648C;
    a.source_dispatch_flag0=1;a.source_dispatch_flag6=1;a.raw_scale={{512,512,512}};a.original_pool_slot=2;
    a.pose=ActorPoseResource{};a.pose->root_translation={{0,0,0}};
    a.source_popout=fresh(2560,-129,-7040,0);
    SourceHitProxyBank bank;
    // Not bound yet: nothing is registered.
    auto t=tick_imported_actor_popout_1f(a,r,uint16_t(1),std::nullopt,&bank,0xffu);
    assert(t.committed&&!t.proxy_registered&&bank.count==0&&a.source_popout->flags_348==0x2C);
    assert(initialize_imported_actor_fresh_pose_proxy_subset(a));
    assert(a.source_proxy_target.source_address==0x8009f108u+2u*0x3c0u+0x35cu);
    assert(!initialize_imported_actor_fresh_pose_proxy_subset(a));
    a.source_proxy_target.packet_00=0x18000040u;   // a Buster hit from the previous frame
    t=tick_imported_actor_popout_1f(a,r,uint16_t(1),0u,&bank,0xffu);
    assert(t.committed&&t.proxy_registered&&bank.count==1);
    const auto& row=bank.records[0];
    assert(row.target==&a.source_proxy_target&&row.encoded_extent==((0x50u>>2)|0x8000u));
    assert(row.parallel_word==(0x04880000u|a.source_popout->work_350));
    assert((row.encoded_identity>>24)==(0x20u|2u)&&(row.encoded_identity&0x00ffffffu)==(((0x8009f108u+2u*0x3c0u+0x35cu)&0x7fffffu)|0x00800000u));
    assert(row.point[0]==a.source_point_180[0]&&row.point[2]==a.source_point_180[2]);
    assert((a.source_proxy_target.flags_04&0x80u)!=0);
    assert(a.source_proxy_target.packet_00==0&&!t.result.sound_8001a050&&a.source_popout->health_6e==std::nullopt);
    // A missing bank or mask registers nothing but still runs the callback.
    t=tick_imported_actor_popout_1f(a,r,uint16_t(1),0u,nullptr);
    assert(t.committed&&!t.proxy_registered);
}
} // namespace

int main(){
    reducer();
    actor_bridge();
    hidden_proxy();
    std::cout<<"PASS: class20/1F init, work80 wait, lunge + z scale, 0x80106800, morph flag, actor bridge, hidden proxy\n";
}
