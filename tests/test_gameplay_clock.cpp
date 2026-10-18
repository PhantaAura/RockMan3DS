#include "legends3ds/gameplay_clock.h"
#include "legends3ds/runtime_animation.h"
#include <cassert>
int main() {
    l3::GameplayClock clock;
    for (unsigned i=0;i<60;++i) {clock.advance(i%3==0?16666:16667,false); while(clock.consume()) {}}
    assert(clock.updates()==30 && clock.pending_updates()==0);
    clock.advance(500000,false);assert(clock.pending_updates()==15);
    clock.advance(1000000,true);assert(!clock.consume());
    clock.advance(2000000,false);assert(clock.discarded_us()==1000000 && clock.pending_updates()==30);
    l3::PbdAnimation anim;anim.action_slots=2;
    l3::PbdAnimationAction seq;seq.action_index=0;seq.has_s_records=true;seq.s_records={{0x0200,0x0000,0},{0x0301,0xff01,0}};anim.actions.push_back(seq);
    l3::PbdAnimationAction pose;pose.action_index=1;pose.m_is_frame_table=true;pose.angle_frames.resize(2);anim.actions.push_back(pose);
    L3QolSettings q{};l3_qol_settings_defaults(&q);
    l3::RuntimeSession session({}, {}, {},q);session.reset_player({},0,100,100);
    l3::PbdMainBodyTrack track;
    assert(l3::runtime_animation_prepare(anim,track,session));
    assert(session.compat_player().bytes[0xa5]==2);
    assert(l3::runtime_animation_finish(anim,track,session));
    assert(session.compat_player().bytes[0xa5]==1);
    assert(l3::runtime_animation_finish(anim,track,session));
    assert(session.compat_player().bytes[0xa4]==1 && session.compat_player().bytes[0xa6]==1 && session.compat_player().bytes[0xa7]==255);
    l3::RuntimePlayerReplaySeed seed{};seed.animation_pose=1;
    assert(session.seed_player_replay_diagnostic(seed));
    track={};
    assert(l3::runtime_animation_prepare(anim,track,session));
    assert(track.record_index==1 && track.ticks_remaining==3); // source-selected nonzero record retained
    auto invalid=track;invalid.request=127;assert(!l3::runtime_animation_submit(anim,invalid,session));
}
