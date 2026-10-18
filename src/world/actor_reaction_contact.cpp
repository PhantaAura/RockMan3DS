#include "legends3ds/actor_reaction_contact.h"
#include <climits>
namespace l3 {
namespace {
int32_t signed_word(uint32_t v)noexcept{return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-0x100000000LL);}
bool neighbors(const RuntimeMap& map,size_t index,int16_t x,int16_t z)noexcept{
    if(index>=map.areas.size())return false;
    const auto& a=map.areas[index];
    const int32_t nx=(uint16_t(x)&511u)<257u?-512:512,nz=(uint16_t(z)&511u)<257u?-512:512;
    const std::array<std::array<int32_t,2>,4> offsets{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    const auto shift9=[](int32_t v){return v>=0?v/512:-((-v+511)/512);};
    for(const auto& d:offsets){
        const int32_t gx=shift9(int32_t(x)+d[0]+32768)-a.start_x,gz=shift9(int32_t(z)+d[1]+32768)-a.start_z;
        if(gx<0||gz<0||gx>=a.width||gz>=a.height)return false;
        const uint64_t local=uint64_t(gz)*a.width+uint64_t(gx);
        if(local>=a.cell_count||uint64_t(a.cell_first)+local>=map.cells.size())return false;
    }
    return true;
}
}
ActorReactionContactPreparation prepare_actor_reaction_contact(
    const Class20_03CallbackState& s,const Class20_03ReactionInputs& in,
    const ActorContactWork& w,const RuntimeMap& map,size_t area,const ReferenceTables& refs,
    RetailPlayerBounds bounds)noexcept{
    using Status=ActorReactionContactStatus;ActorReactionContactPreparation out;
    out.before=w;out.after_wall=w;out.plan=class20_03_plan_reaction(s,in);
    out.initial_action=s.behavior.action_09;out.initial_phase=s.behavior.phase_0a;
    out.initial_aux=s.aux_0b;out.initial_parameter=s.behavior.parameter_03;
    out.initial_timer=s.behavior.timer_32c;out.initial_heading=s.behavior.heading_56;
    out.initial_table=s.reaction_table_364;
    if(out.plan.status!=Class20_03ReactionStatus::SourceBackedPartial){out.status=Status::PlanHeld;return out;}
    if(!out.plan.moves)return out;
    if(w.object_class!=0x20||w.raw!=s.raw_14||w.fixed!=s.fixed_1c||
       w.previous_raw!=s.previous_raw_2c||w.previous_fixed!=s.previous_fixed_34||w.velocity!=s.velocity_44){
        out.status=Status::StaleWork;return out;
    }
    out.after_wall.velocity=out.plan.velocity_before_wall;
    const auto delta=reference_player_motion_delta(refs,s.behavior.heading_56,out.plan.forward,out.plan.vertical,0);
    const std::array<int32_t,3> d{{delta.dx_fixed,delta.dy_fixed,delta.dz_fixed}};
    for(size_t i=0;i<3;++i){
        out.after_wall.fixed[i]=signed_word(uint32_t(w.fixed[i])+uint32_t(d[i]));
        out.after_wall.raw[i]=reference_fixed_to_short_80030150(out.after_wall.fixed[i]);
    }
    if(!neighbors(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){out.status=Status::UnsupportedBoundary;return out;}
    out.wall=query_actor_wall_contact_8004b6ac(map,area,out.after_wall,bounds);
    if(!apply_actor_contact_result_8004b414(out.after_wall,out.wall)){out.status=Status::UnsupportedWall;return out;}
    if(!neighbors(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){out.status=Status::UnsupportedBoundary;return out;}
    Class20_03PreparedReaction m;m.entry_raw_14=s.raw_14;m.entry_fixed_1c=s.fixed_1c;
    m.raw_14=out.after_wall.raw;m.fixed_1c=out.after_wall.fixed;
    m.heading=s.behavior.heading_56;m.forward=out.plan.forward;m.vertical=out.plan.vertical;
    m.velocity_before_wall=out.plan.velocity_before_wall;m.velocity_after_wall=out.after_wall.velocity;
    m.contact_word=uint32_t(out.wall.result);out.inputs=m;out.status=Status::SourceBackedPartial;return out;
}
} // namespace l3
