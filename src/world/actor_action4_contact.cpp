#include "legends3ds/actor_action4_contact.h"
#include <limits>

namespace l3 {
namespace {
int32_t signed_word(uint32_t value) noexcept {
    return value<=INT32_MAX?static_cast<int32_t>(value)
        :static_cast<int32_t>(int64_t(value)-0x100000000LL);
}
bool admitted_neighbors(const RuntimeMap& map,size_t index,int16_t x,int16_t z) noexcept {
    if(index>=map.areas.size())return false;
    const auto& a=map.areas[index];
    const int32_t nx=(uint16_t(x)&511u)<257u?-512:512;
    const int32_t nz=(uint16_t(z)&511u)<257u?-512:512;
    const std::array<std::array<int32_t,2>,4> offsets{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    const auto shift9=[](int32_t v){return v>=0?v/512:-((-v+511)/512);};
    for(const auto& offset:offsets){
        const auto gx=shift9(int32_t(x)+offset[0]+32768)-a.start_x;
        const auto gz=shift9(int32_t(z)+offset[1]+32768)-a.start_z;
        if(gx<0 || gz<0 || gx>=a.width || gz>=a.height)return false;
        const uint64_t local=uint64_t(gz)*a.width+uint64_t(gx);
        if(local>=a.cell_count || uint64_t(a.cell_first)+local>=map.cells.size())return false;
    }
    return true;
}
}
ActorAction4ContactPreparation prepare_actor_action4_contact(
    const Class20_03CallbackState& state,const ActorContactWork& motion,
    const RuntimeMap& map,size_t area,const ReferenceTables& refs,
    RetailPlayerBounds bounds) noexcept {
    using Status=ActorAction4ContactStatus;
    ActorAction4ContactPreparation out;const auto& b=state.behavior;
    out.before=motion;out.after_wall=motion;out.initial_action=b.action_09;
    out.initial_phase=b.phase_0a;out.initial_request=b.animation_ac;
    out.initial_heading=b.heading_56;out.initial_timer=b.timer_32c;
    out.initial_variant=state.variant_32e;out.plan=class20_03_plan_action4(state);
    if(b.action_09!=4 || !out.plan.forward)return out;
    if(motion.object_class!=0x20 || motion.raw!=state.raw_14 || motion.fixed!=state.fixed_1c ||
       motion.previous_raw!=state.previous_raw_2c || motion.previous_fixed!=state.previous_fixed_34){
        out.status=Status::StaleWork;return out;
    }
    const auto d=reference_player_motion_delta(refs,b.heading_56,*out.plan.forward,0,0);
    const std::array<int32_t,3> delta{{d.dx_fixed,d.dy_fixed,d.dz_fixed}};
    for(size_t axis=0;axis<3;++axis){
        out.after_wall.fixed[axis]=signed_word(uint32_t(out.after_wall.fixed[axis])+uint32_t(delta[axis]));
        out.after_wall.raw[axis]=reference_fixed_to_short_80030150(out.after_wall.fixed[axis]);
    }
    if(!admitted_neighbors(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){
        out.status=Status::UnsupportedBoundary;return out;
    }
    out.wall=query_actor_wall_contact_8004b6ac(map,area,out.after_wall,bounds);
    if(!apply_actor_contact_result_8004b414(out.after_wall,out.wall)){
        out.status=Status::UnsupportedWall;return out;
    }
    if(!admitted_neighbors(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){
        out.status=Status::UnsupportedBoundary;return out;
    }
    out.floor=query_actor_floor_contact_80048ea4(map,area,out.after_wall,bounds);
    if(out.floor.status!=RuntimeContactStatus::SourceBackedPartial){out.status=Status::UnsupportedFloor;return out;}
    Class20_03PreparedAction4 p;p.raw_14=out.after_wall.raw;p.fixed_1c=out.after_wall.fixed;
    p.source_floor_word=out.floor.result;p.source_forward=*out.plan.forward;p.heading=b.heading_56;
    out.inputs=p;out.status=Status::SourceBackedPartial;return out;
}
} // namespace l3
