#include "legends3ds/class20_03_reaction.h"
namespace l3 {
namespace {
using Status=Class20_03ReactionStatus;
int16_t s16(uint16_t v)noexcept{return v<32768u?int16_t(v):int16_t(int32_t(v)-65536);}
constexpr std::array<uint32_t,3> tables{{0x8011bee0u,0x8011beecu,0x8011bef8u}};
std::optional<size_t> table(const Class20_03CallbackState& s)noexcept{
    if(s.behavior.action_09==7)return 0;
    if(s.reaction_table_364==tables[1])return 1;
    if(s.reaction_table_364==tables[2])return 2;
    return std::nullopt;
}
Class20_03ReactionPlan prefix(Class20_03CallbackState& s,const Class20_03ReactionInputs& in)noexcept{
    Class20_03ReactionPlan p;auto& b=s.behavior;
    if(b.action_09!=7&&b.action_09!=8)return p;
    p.status=Status::SourceBackedPartial;
    if(b.phase_0a>3)return p;
    if(b.phase_0a==3){
        b.timer_32c=s16(uint16_t(uint16_t(b.timer_32c)-1u));
        if(b.timer_32c<0)b.flags_328|=9u;
        else {
            if(!in.target_heading_80031198){p.status=Status::NeedsTargetHeading;return p;}
            b.work_36c=class20_03_turn_step(*in.target_heading_80031198,b.heading_56,128);
            b.heading_56=uint16_t((int32_t(b.heading_56)+b.work_36c)&4095);
        }
        return p;
    }
    if(!in.profile){p.status=Status::NeedsProfile;return p;}
    if(b.phase_0a==0){
        if(!s.health_6e){p.status=Status::NeedsHealth;return p;}
        const bool eight=b.action_09==8,air=eight&&(b.parameter_03&0x40u);
        if(air&&(!in.combat_byte_b||!in.combat_record_variant||*in.combat_record_variant!=s.variant_32e)){
            p.status=Status::NeedsCombatByteB;return p;
        }
        b.phase_0a=1;b.animation_ac=eight?12:7;b.work_330=0;
        b.flags_328&=0xfff7u;if(*s.health_6e<=0)b.flags_328&=0xfffbu;
        s.velocity_44[0]=eight?(air?8:16):-16;
        const size_t selected=eight?(air?2:1):0;
        if(eight)s.reaction_table_364=tables[selected];
        b.timer_32c=in.profile->timer_pairs[selected][0];
        if(air){s.random_32a=7;b.work_330=*in.combat_byte_b;}
    }
    if(b.phase_0a==1&&b.timer_32c<0)b.phase_0a=2;
    const auto selected=table(s);
    if(!selected){p.status=Status::NeedsTableOwner;return p;}
    if(s.aux_0b>=10){p.status=Status::UnsupportedTableIndex;return p;}
    // LHU decrement supplies the untruncated32bit subtraction to both shifts.
    const uint32_t decremented=uint32_t(uint16_t(b.timer_32c))-1u;
    b.timer_32c=s16(uint16_t(decremented));
    const uint32_t difference=uint32_t(in.profile->timer_pairs[*selected][s.aux_0b+1])-decremented;
    s.velocity_44[1]=s16(uint16_t(difference<<3));
    p.forward=s16(uint16_t(uint32_t(uint16_t(s.velocity_44[0]))<<4));
    p.vertical=s16(uint16_t(difference<<7));
    p.velocity_before_wall=s.velocity_44;p.moves=true;return p;
}
Class20_03ReactionResult hold(Status s)noexcept{Class20_03ReactionResult r;r.status=s;return r;}
}
Class20_03ReactionPlan class20_03_plan_reaction(
    const Class20_03CallbackState& state,const Class20_03ReactionInputs& in)noexcept{
    auto next=state;return prefix(next,in);
}
Class20_03ReactionResult class20_03_step_reaction(
    Class20_03CallbackState& state,const Class20_03ReactionInputs& in)noexcept{
    auto next=state;const auto plan=prefix(next,in);
    if(plan.status!=Status::SourceBackedPartial)return hold(plan.status);
    Class20_03ReactionResult r;r.status=Status::SourceBackedPartial;
    if(!plan.moves){state=next;return r;}
    if(!in.motion)return hold(Status::NeedsMotion);
    const auto& m=*in.motion;
    if(m.entry_raw_14!=state.raw_14||m.entry_fixed_1c!=state.fixed_1c||
       m.heading!=state.behavior.heading_56||m.forward!=plan.forward||m.vertical!=plan.vertical||
       m.velocity_before_wall!=plan.velocity_before_wall)return hold(Status::StaleMotion);
    next.raw_14=m.raw_14;next.fixed_1c=m.fixed_1c;next.velocity_44=m.velocity_after_wall;
    next.work_370=m.contact_word;
    if(m.contact_word&0xf00u)next.velocity_44[0]=0;
    if(m.contact_word&0x1000u){
        if(!in.sound_consumer_ready)return hold(Status::NeedsSoundConsumer);
        r.sound_8001a050=222;
        r.sound_pointed_words=std::array<uint32_t,2>{{
            uint32_t(uint16_t(next.raw_14[0]))|(uint32_t(uint16_t(next.raw_14[1]))<<16),
            uint32_t(uint16_t(next.raw_14[2]))|(uint32_t(uint16_t(next.raw_14[3]))<<16)}};
        next.aux_0b=uint8_t(next.aux_0b+2u);auto& b=next.behavior;
        if(next.aux_0b<10){
            b.phase_0a=1;b.animation_ac=uint8_t(b.animation_ac+1u);
            next.velocity_44[0]=b.action_09==7?-16:((b.parameter_03&0x40u)?8:16);
            b.timer_32c=in.profile->timer_pairs[*table(next)][next.aux_0b];b.flags_328|=8u;
        }else {
            if(!next.health_6e)return hold(Status::NeedsHealth);
            if(*next.health_6e<=0)b.flags_328|=1u;
            else {b.phase_0a=3;b.animation_ac=17;b.timer_32c=18;}
        }
    }
    state=next;return r;
}
} // namespace l3
