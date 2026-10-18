#include "legends3ds/actor_action9_contact.h"
#include <limits>

namespace l3 {
namespace {
int32_t signed_word(uint32_t value) noexcept {
    return value<=INT32_MAX?static_cast<int32_t>(value)
        :static_cast<int32_t>(int64_t(value)-0x100000000LL);
}
bool all_neighbor_samples_admitted(const RuntimeMap& map,size_t area_index,
                                  int16_t x,int16_t z) noexcept {
    if(area_index>=map.areas.size())return false;
    const auto& area=map.areas[area_index];
    const int32_t nx=(uint16_t(x)&511u)<257u?-512:512;
    const int32_t nz=(uint16_t(z)&511u)<257u?-512:512;
    const std::array<std::array<int32_t,2>,4> delta{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    const auto shift9=[](int32_t v){return v>=0?v/512:-((-v+511)/512);};
    for(const auto& offset:delta){
        const auto gx=shift9(int32_t(x)+offset[0]+32768)-area.start_x;
        const auto gz=shift9(int32_t(z)+offset[1]+32768)-area.start_z;
        if(gx<0 || gz<0 || gx>=area.width || gz>=area.height)return false;
        const uint64_t local=uint64_t(gz)*area.width+uint64_t(gx);
        if(local>=area.cell_count || uint64_t(area.cell_first)+local>=map.cells.size())return false;
    }
    return true;
}
}
ActorAction9ContactPreparation prepare_actor_action9_contact(
    const Class20_03CallbackState& state,const ActorContactWork& motion,
    const RuntimeMap& map,size_t area,const ReferenceTables& refs,
    RetailPlayerBounds bounds,std::optional<uint32_t> work80,
    std::optional<uint16_t> target,std::optional<int8_t> cue) noexcept {
    using Status=ActorAction9ContactStatus;
    ActorAction9ContactPreparation out;
    const auto& b=state.behavior;
    out.before=motion;out.after_wall=motion;
    out.initial_action=b.action_09;out.initial_phase=b.phase_0a;
    out.initial_flags=b.flags_328;out.initial_heading=b.heading_56;
    out.work80=work80;out.turn_target=target;out.source_a6=cue;
    if(b.action_09!=9 || b.phase_0a>1)return out;
    if(!work80){out.status=Status::NeedsWork80;return out;}
    if(*work80>0x00480000u && (b.flags_328&0x10u))return out;
    if(motion.object_class!=0x20 || motion.raw!=state.raw_14 || motion.fixed!=state.fixed_1c ||
       motion.previous_raw!=state.previous_raw_2c || motion.previous_fixed!=state.previous_fixed_34){
        out.status=Status::StaleWork;return out;
    }
    if(!target){out.status=Status::NeedsTurnTarget;return out;}
    if(!cue){out.status=Status::NeedsAnimationCue;return out;}
    out.source_turn=class20_03_turn_step(*target,b.heading_56,32);
    const auto heading=static_cast<uint16_t>((int32_t(b.heading_56)+out.source_turn)&4095);
    const auto d=reference_player_motion_delta(refs,heading,52,0,0);
    const std::array<int32_t,3> delta{{d.dx_fixed,d.dy_fixed,d.dz_fixed}};
    for(size_t axis=0;axis<3;++axis){
        out.after_wall.fixed[axis]=signed_word(uint32_t(out.after_wall.fixed[axis])+uint32_t(delta[axis]));
        out.after_wall.raw[axis]=reference_fixed_to_short_80030150(out.after_wall.fixed[axis]);
    }
    if(!all_neighbor_samples_admitted(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){
        out.status=Status::UnsupportedBoundary;return out;
    }
    out.wall=query_actor_wall_contact_8004b6ac(map,area,out.after_wall,bounds);
    if(!apply_actor_contact_result_8004b414(out.after_wall,out.wall)){
        out.status=Status::UnsupportedWall;return out;
    }
    if(!all_neighbor_samples_admitted(map,area,out.after_wall.raw[0],out.after_wall.raw[2])){
        out.status=Status::UnsupportedBoundary;return out;
    }
    out.floor=query_actor_floor_contact_80048ea4(map,area,out.after_wall,bounds);
    if(out.floor.status!=RuntimeContactStatus::SourceBackedPartial){
        out.status=Status::UnsupportedFloor;return out;
    }
    Class20_03PreparedAction9 prepared;
    prepared.raw_14=out.after_wall.raw;prepared.fixed_1c=out.after_wall.fixed;
    prepared.source_floor_word=out.floor.result;
    prepared.source_contact_word=uint32_t(out.wall.result);
    prepared.heading_after_turn=heading;prepared.source_a6=cue;
    out.inputs=prepared;out.status=Status::SourceBackedPartial;return out;
}
} // namespace l3
