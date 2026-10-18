#include "legends3ds/class20_03_action9.h"

namespace l3 {
namespace {
int16_t signed16(uint16_t word) noexcept {
    return word < 0x8000u ? static_cast<int16_t>(word)
        : static_cast<int16_t>(static_cast<int32_t>(word)-65536);
}
int32_t signed32(uint32_t word) noexcept {
    return word <= 0x7fffffffu ? static_cast<int32_t>(word)
        : static_cast<int32_t>(static_cast<int64_t>(word)-0x100000000LL);
}
Class20_03Action9Result hold(Class20_03Action9Status status) noexcept {
    Class20_03Action9Result result;result.status=status;return result;
}
}

Class20_03Action9Result class20_03_step_action9(
    Class20_03CallbackState& state,const Class20_03Action9Inputs& input) noexcept {
    using Status=Class20_03Action9Status;
    if(state.behavior.action_09!=9)return hold(Status::UnknownAction);
    if(!input.target_angle_player_to_actor_80031198)return hold(Status::NeedsPlayerToActorAngle);
    if(!input.player_heading_800b5206)return hold(Status::NeedsPlayerHeading);
    auto next=state;
    auto& b=next.behavior;
    next.work_36e=static_cast<uint16_t>(*input.target_angle_player_to_actor_80031198-
                                      *input.player_heading_800b5206);
    Class20_03Action9Result result;
    if(b.phase_0a>1){state=next;return result;}
    if(!input.work_80)return hold(Status::NeedsWork80);
    if(b.phase_0a==0){
        b.phase_0a=1;b.animation_ac=1;b.timer_32c=90;b.work_330=0;
    }
    if(*input.work_80>0x00480000u && (b.flags_328&0x10u)){
        b.action_09=3;b.phase_0a=0;next.aux_0b=0;
        state=next;return result;
    }
    if(!input.turn_target_actor_to_player_80031198)return hold(Status::NeedsTurnTarget);
    if(!input.prepared_motion)return hold(Status::NeedsMotion);
    const auto& motion=*input.prepared_motion;
    const auto turn=class20_03_turn_step(*input.turn_target_actor_to_player_80031198,
                                        b.heading_56,32);
    const auto heading=static_cast<uint16_t>((static_cast<int32_t>(b.heading_56)+turn)&4095);
    if(motion.heading_after_turn!=heading)return hold(Status::StaleMotion);
    if(!motion.source_a6)return hold(Status::NeedsAnimationCue);
    if(*motion.source_a6==1 && !input.sound_consumer_ready)return hold(Status::NeedsSoundConsumer);
    b.work_36c=turn;b.heading_56=heading;next.angle_54[1]=heading;
    next.raw_14=motion.raw_14;next.fixed_1c=motion.fixed_1c;
    next.work_370=motion.source_contact_word;
    if(*motion.source_a6==1){
        result.sound_8001a050=176;
        result.sound_pointed_words=std::array<uint32_t,2>{{
            uint32_t(uint16_t(motion.raw_14[0]))|(uint32_t(uint16_t(motion.raw_14[1]))<<16),
            uint32_t(uint16_t(motion.raw_14[2]))|(uint32_t(uint16_t(motion.raw_14[3]))<<16)}};
    }
    next.raw_14[1]=signed16(static_cast<uint16_t>(motion.source_floor_word));
    next.fixed_1c[1]=signed32(static_cast<uint32_t>(motion.source_floor_word)<<16);
    const auto delta=static_cast<int32_t>(next.raw_14[1])-next.previous_raw_2c[1];
    if(delta>=33 || delta<=-33){
        next.raw_14=next.previous_raw_2c;next.fixed_1c=next.previous_fixed_34;
        b.action_09=1;b.phase_0a=0;next.aux_0b=0;next.restore_action_0e=4;
    }else if(motion.source_contact_word&0xf00u){
        b.action_09=1;b.phase_0a=0;next.aux_0b=0;next.restore_action_0e=4;
    }else if(next.work_35e==0 && *input.work_80<=0x00011fffu){
        b.action_09=10;b.phase_0a=0;next.aux_0b=0;b.flags_328&=0xfffeu;
    }else if(next.work_35c==0 && *input.work_80<=0x0011ffffu &&
             turn>-16 && turn<16 && ((uint32_t(next.work_36e)+1024u)&4095u)<2048u){
        b.action_09=4;b.phase_0a=0;next.aux_0b=0;
    }else if(!(b.flags_328&0x10u)){
        b.flags_328|=1u;
    }
    state=next;return result;
}
} // namespace l3
