#include "legends3ds/runtime_session.h"
#include "legends3ds/source_scene_view.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace l3 {
namespace {
// RuntimeSession and renderer receipts are serviced on the single engine
// thread. This process-wide nonce is not copied or reset by session assignment,
// profile rebinding, reconstruction at the same address, or save/area loads.
uint64_t scene_overlay_serial=0;
uint64_t mint_scene_overlay_serial() noexcept {
    if(scene_overlay_serial==std::numeric_limits<uint64_t>::max())return 0;
    return ++scene_overlay_serial;
}
uint16_t rd16w(const L3PlayerCompatWork& w,size_t o){return uint16_t(w.bytes[o]|(uint16_t(w.bytes[o+1])<<8));}
void wr16w(L3PlayerCompatWork& w,size_t o,uint16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(v>>8);}
void wr32w(L3PlayerCompatWork& w,size_t o,int32_t v){uint32_t u=static_cast<uint32_t>(v);for(int i=0;i<4;++i)w.bytes[o+i]=uint8_t(u>>(i*8));}
int32_t rd32w(const L3PlayerCompatWork& w,size_t o){uint32_t u=0;for(int i=0;i<4;++i)u|=uint32_t(w.bytes[o+i])<<(i*8);return static_cast<int32_t>(u);}
int16_t rds16w(const L3PlayerCompatWork& w,size_t o){return static_cast<int16_t>(rd16w(w,o));}
int32_t lerp_i32(int32_t a,int32_t b,uint32_t n,uint32_t d){if(d==0)return b;return a+int32_t((int64_t(b-a)*n)/d);}
RuntimeVec3i lerp_vec(const RuntimeVec3i&a,const RuntimeVec3i&b,uint32_t n,uint32_t d){return {lerp_i32(a.x,b.x,n,d),lerp_i32(a.y,b.y,n,d),lerp_i32(a.z,b.z,n,d)};}
int64_t dist2(const RuntimeVec3i&a,const RuntimeVec3i&b){const int64_t dx=int64_t(a.x)-b.x,dy=int64_t(a.y)-b.y,dz=int64_t(a.z)-b.z;return dx*dx+dy*dy+dz*dz;}
int64_t dist2_xz(const RuntimeVec3i&a,const RuntimeVec3i&b){const int64_t dx=int64_t(a.x)-b.x,dz=int64_t(a.z)-b.z;return dx*dx+dz*dz;}
uint32_t args_u32le(const std::vector<uint8_t>&a){return a.size()>=4?uint32_t(a[0])|(uint32_t(a[1])<<8)|(uint32_t(a[2])<<16)|(uint32_t(a[3])<<24):0u;}
}

RuntimeSession::RuntimeSession(RuntimeMap map,TutorialRuntimeArea area,std::vector<MmlMessageFile> banks,L3QolSettings settings)
    :map_(std::make_shared<const RuntimeMap>(std::move(map))),area_(std::move(area)),area_index_(area_.area_index),map_area_index_(area_.area_index),message_banks_(std::make_shared<const std::vector<MmlMessageFile>>(std::move(banks))),settings_(settings){
    l3_qol_settings_sanitize(&settings_);
    area_registry_.emplace(area_index_,area_);
    location_registry_.emplace(area_index_,SharedLocationRegistration{map_,map_area_index_,area_});
    reset_conditional_spawns();
    initialize_source_audio_queue_80019e14(source_audio_queue_);
    reset_player({},0,100,100);
}
RuntimeSession::NativeOverlayGeneration::NativeOverlayGeneration() noexcept:value(mint_scene_overlay_serial()){}
RuntimeSession::NativeOverlayGeneration::NativeOverlayGeneration(const NativeOverlayGeneration&) noexcept:value(mint_scene_overlay_serial()){}
RuntimeSession::NativeOverlayGeneration::NativeOverlayGeneration(NativeOverlayGeneration&&) noexcept:value(mint_scene_overlay_serial()){}
RuntimeSession::NativeOverlayGeneration& RuntimeSession::NativeOverlayGeneration::operator=(const NativeOverlayGeneration&) noexcept {
    value=mint_scene_overlay_serial();return *this;
}
RuntimeSession::NativeOverlayGeneration& RuntimeSession::NativeOverlayGeneration::operator=(NativeOverlayGeneration&&) noexcept {
    value=mint_scene_overlay_serial();return *this;
}

RuntimeSession& RuntimeSession::operator=(const RuntimeSession& other){
    static_assert(std::is_nothrow_move_assignable<RuntimeSession>::value,
                  "prepared session copy publication must not allocate");
    if(this!=&other){RuntimeSession prepared(other);*this=std::move(prepared);}
    return *this;
}

void RuntimeSession::reset_player(RuntimeVec3i p,uint16_t yaw,uint16_t hp,uint16_t hpmax){
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_loop_snapshot_admitted_=false;
    source_loop_snapshot_status_=source_loop_snapshot_?SourceLoopSnapshotStatus::NeedsConstructorReceipt:SourceLoopSnapshotStatus::Disabled;
    source_event_phase6_context_.scratch_half_1f800002.reset();
    source_scene_actor40_initializer_globals_={};
    source_event_player_known_={};source_event_consumers_bound_=false;
    source_event_phase2_bound_=false;source_event_command_bound_=false;
    player_={}; player_position_=p; hp_max_=hpmax?hpmax:1; hp_=std::min(hp,hp_max_);
    wr16w(player_,0x56,yaw&0x0FFFu);
    // Retail USA boots with compatibility control profile 0 (table at 0x8008A04C).
    l3_pl00_install_control_profile_800425fc(&player_,0);
    player_.bytes[0x08]=1; player_.bytes[0x09]=0; player_.bytes[0x0A]=0;
    l3_pl00_shot_enable_on(&player_); sync_primary_position_to_compat();
    reset_player_contact();
    locked_target_index_.reset(); projectiles_.clear(); frame_counter_=0; aux_triggers_.clear();
    source_door_event0_={};source_door_objects_.clear();
    update_gameplay_camera_target(); camera_.current=camera_.target;
}

bool RuntimeSession::recover_player_position_diagnostic(RuntimeVec3i p){
    for(const int32_t coordinate:{p.x,p.y,p.z})
        if(coordinate<std::numeric_limits<int16_t>::min() ||
           coordinate>std::numeric_limits<int16_t>::max())return false;
    if(!map_ || !runtime_map_find_cell(*map_,map_area_index_,p.x,p.z))return false;
    player_position_=p;sync_primary_position_to_compat();
    l3_pl00_snapshot_collision_pose(&player_);
    // The preceding contact observation described the abandoned position.
    // Preserve the retail work state; the next admitted contact query replaces
    // this diagnostic observation without inventing a motion-state reset.
    player_contact_={};
    return true;
}

bool RuntimeSession::seed_player_replay_diagnostic(const RuntimePlayerReplaySeed& s){
    const auto action=s.state[1],phase=s.state[2];
    const bool known_phase=((action==0||action==1||action==4)&&phase<=1) ||
                           (action==2&&phase<=6) || (action==7&&phase<=5) || (action==8&&phase<=7);
    if(s.state[0]!=1 || !known_phase ||
       s.direction<0 || s.direction>3 || s.previous_direction<0 || s.previous_direction>3 ||
       s.yaw>0xFFF)return false;
    const std::array<int32_t,3> short_position{{s.position.x,s.position.y,s.position.z}};
    for(size_t i=0;i<3;++i){
        if(short_position[i]<-32768||short_position[i]>32767||
           reference_fixed_to_short_80030150(s.fixed_position[i])!=short_position[i])return false;
    }
    reset_player(s.position,s.yaw,hp_,hp_max_);
    for(size_t i=0;i<3;++i){
        wr32w(player_,0x1C+i*4,s.fixed_position[i]);
        player_.bytes[0x08+i]=s.state[i];
        wr16w(player_,0x44+i*2,static_cast<uint16_t>(s.velocity_work[i]));
    }
    player_.bytes[0x0B]=s.work_0b;player_.bytes[0x74]=s.contact_state;
    player_.bytes[0xAC]=s.animation_request;player_.bytes[0xAD]=s.animation_request;
    player_.bytes[0x112]=static_cast<uint8_t>(s.direction);
    player_.bytes[0x113]=static_cast<uint8_t>(s.previous_direction);
    wr16w(player_,0x116,static_cast<uint16_t>(s.direction_yaw_offset));
    wr16w(player_,0xB4,static_cast<uint16_t>(s.turn_accumulator));
    submit_player_animation_record(s.animation_request,s.animation_pose,s.animation_countdown,
                                   s.animation_record_byte2,s.animation_signed_control);
    l3_pl00_snapshot_collision_pose(&player_);
    update_gameplay_camera_target();camera_.current=camera_.target;
    return true;
}

void RuntimeSession::set_area(size_t idx,TutorialRuntimeArea area){
    register_area(idx,std::move(area));
    if(!transition_to_area(idx,player_position_,player_yaw())) throw std::runtime_error("registered tutorial area transition failed");
}
void RuntimeSession::register_area(size_t idx,TutorialRuntimeArea area){
    if(idx>=map_->areas.size()) throw std::out_of_range("tutorial area index outside current map");
    area.area_index=idx;
    TutorialRuntimeArea registry_area=area;
    SharedLocationRegistration location{registry_map(),idx,std::move(area)};
    static_assert(std::is_nothrow_move_assignable<TutorialRuntimeArea>::value &&
                  std::is_nothrow_move_assignable<SharedLocationRegistration>::value,
                  "area registration publication must not allocate");
    auto registered=area_registry_.find(idx);
    const bool inserted=registered==area_registry_.end();
    if(inserted)registered=area_registry_.emplace(idx,std::move(registry_area)).first;
    try {
        auto destination=location_registry_.find(idx);
        if(destination==location_registry_.end())location_registry_.emplace(idx,std::move(location));
        else destination->second=std::move(location);
    } catch(...) {
        if(inserted)area_registry_.erase(registered);
        throw;
    }
    if(!inserted)registered->second=std::move(registry_area);
}
bool RuntimeSession::has_registered_area(size_t idx)const{return area_registry_.find(idx)!=area_registry_.end();}
const TutorialRuntimeArea* RuntimeSession::registered_area(size_t idx)const{
    const auto it=location_registry_.find(idx);
    if(it==location_registry_.end() || it->second.map!=registry_map() || it->second.map_area_index!=idx)return nullptr;
    return &it->second.area;
}
bool RuntimeSession::transition_to_area(size_t idx,RuntimeVec3i p,uint16_t yaw){return transition_to_location(idx,p,yaw);}
void RuntimeSession::register_location(size_t id,RuntimeMap map,size_t map_area,TutorialRuntimeArea area){
    if(id>0xFFFFu)throw std::out_of_range("runtime location id exceeds save format");
    if(map_area>=map.areas.size())throw std::out_of_range("runtime location map area out of range");
    area.area_index=map_area;
    location_registry_[id]=SharedLocationRegistration{std::make_shared<const RuntimeMap>(std::move(map)),map_area,std::move(area)};
}
bool RuntimeSession::has_registered_location(size_t id)const{return location_registry_.find(id)!=location_registry_.end();}
bool RuntimeSession::unregister_location(size_t id){
    if(id==area_index_)return false;
    auto it=location_registry_.find(id);if(it==location_registry_.end())return false;
    location_registry_.erase(it);
    area_registry_.erase(id);
    return true;
}
size_t RuntimeSession::release_inactive_locations(){
    size_t removed=0;
    for(auto it=location_registry_.begin();it!=location_registry_.end();){
        if(it->first==area_index_){++it;continue;}
        it=location_registry_.erase(it);++removed;
    }
    for(auto it=area_registry_.begin();it!=area_registry_.end();){
        if(it->first==area_index_){++it;continue;}
        it=area_registry_.erase(it);
    }
    return removed;
}
bool RuntimeSession::transition_to_location(size_t id,RuntimeVec3i p,uint16_t yaw){
    auto it=location_registry_.find(id);if(it==location_registry_.end())return false;
    // The stage/work copy can allocate. Complete it before changing identity,
    // collision ownership or queued event state; moving the prepared area is
    // allocation-free with the containers used by TutorialRuntimeArea.
    static_assert(std::is_nothrow_move_assignable<TutorialRuntimeArea>::value,
                  "area transition commit must not allocate");
    TutorialRuntimeArea next_area=it->second.area;
    area_=std::move(next_area);area_index_=id;map_=it->second.map;map_area_index_=it->second.map_area_index;
    // Door objects belong to the outgoing area; its IDX swaps go with it.
    if(door_map_)++idx_descriptor_generation_;
    source_door_objects_.clear();door_map_.reset();
    player_position_=p;wr16w(player_,0x56,yaw&0x0FFFu);sync_primary_position_to_compat();
    reset_player_contact();
    // Area-local metadata must be rebound to the new source stage. Global
    // event work survives; unknown transition/busy values cannot be inferred
    // from this native diagnostic transition succeeding.
    source_event_profile_.reset();source_event_stage_.reset();
    source_event_transition_.reset();source_event_busy_.reset();source_event_result_={};
    source_door_request_.reset();source_door_result_={};
    source_fade_profile_.reset();source_fade_context_={};source_fade_intents_={};
    source_fade_callback_={};source_fade_endpoint_hold_.reset();
    source_effect_dispatch_lease_.invalidate();source_effect_dispatch_profile_.reset();source_prop_lift_profile_.reset();invalidate_source_prop_foreign_globals(source_prop_foreign_globals_);source_event_phase5_profile_.reset();source_event_phase5_context_={};source_event_phase5_resources_={};source_event_phase6_context_={};source_handoff_display_.reset();source_scene_actor40_initializer_globals_={};source_event_phase6_profile_.reset();
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_loop_snapshot_.reset();source_loop_snapshot_admitted_=false;source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
    // Retained transport receipts and descriptors belong to the outgoing
    // stage. Publish their invalidation only after fallible preparation; a
    // rejected transition must preserve the active owner's pending request.
    source_event_phase2_package_.reset();source_event_command_package_.reset();
    source_event_phase2_bound_=false;source_event_phase2_context_={};
    source_event_commands_={};source_event_phase2_intents_={};
    source_event_command_bound_=false;source_event_command_context_={};
    source_event_cd_inputs_={};source_event_command_result_={};source_event_cd_mix_intents_={};
    reset_conditional_spawns();
    locked_target_index_.reset();targets_.clear();projectiles_.clear();aux_triggers_.clear();stop_dialogue();
    dialogue_={};active_cutscene_id_.reset();cutscene_skip_hold_frames_=0;
    camera_.owner=RuntimeCameraOwner::Gameplay;camera_.transition_frame=camera_.transition_frames=0;
    update_gameplay_camera_target();camera_.current=camera_.target;return true;
}
void RuntimeSession::set_objective(std::optional<MessageRef> o){objective_=o;}
void RuntimeSession::set_reference_tables(ReferenceTables tables){reference_tables_=std::move(tables);}
void RuntimeSession::register_target(RuntimeTarget t){targets_.push_back(t);}
void RuntimeSession::clear_targets(){targets_.clear();locked_target_index_.reset();}
std::optional<uint32_t> RuntimeSession::locked_target_id()const{if(!locked_target_index_||*locked_target_index_>=targets_.size())return std::nullopt;return targets_[*locked_target_index_].id;}
RuntimeVec3i RuntimeSession::player_position()const{return player_position_;}
uint16_t RuntimeSession::player_yaw()const{return rd16w(player_,0x56)&0x0FFFu;}

void RuntimeSession::sync_primary_position_to_compat(){
    // Initialize both mirrors. During motion, the original-style 16.16 values at
    // +0x1C/+0x20/+0x24 remain authoritative so fractional movement is retained.
    wr16w(player_,0x14,uint16_t(int16_t(player_position_.x)));
    wr16w(player_,0x16,uint16_t(int16_t(player_position_.y)));
    wr16w(player_,0x18,uint16_t(int16_t(player_position_.z)));
    wr32w(player_,0x1C,static_cast<int32_t>(int64_t(player_position_.x)*65536ll));
    wr32w(player_,0x20,static_cast<int32_t>(int64_t(player_position_.y)*65536ll));
    wr32w(player_,0x24,static_cast<int32_t>(int64_t(player_position_.z)*65536ll));
}

void RuntimeSession::sync_primary_position_from_fixed(){
    const int32_t fx=rd32w(player_,0x1C),fy=rd32w(player_,0x20),fz=rd32w(player_,0x24);
    const int16_t x=reference_fixed_to_short_80030150(fx);
    const int16_t y=reference_fixed_to_short_80030150(fy);
    const int16_t z=reference_fixed_to_short_80030150(fz);
    wr16w(player_,0x14,static_cast<uint16_t>(x));
    wr16w(player_,0x16,static_cast<uint16_t>(y));
    wr16w(player_,0x18,static_cast<uint16_t>(z));
    player_position_={x,y,z};
}

void RuntimeSession::update_input_compat(const RuntimeInputFrame& in){
    const uint16_t previous=rd16w(player_,0x11C);
    wr16w(player_,0x11C,in.held_psx_mask);
    wr16w(player_,0x11E,uint16_t((in.held_psx_mask & uint16_t(~previous))|in.pressed_psx_mask));
}

void RuntimeSession::reset_player_contact(){
    player_contact_={};
    player_animation_record_request_.reset();
    wr16w(player_,0x46,0);player_.bytes[0x74]=0;
    if(player_.bytes[0x09]==7 || player_.bytes[0x09]==8){
        player_.bytes[0x09]=0;player_.bytes[0x0A]=0;
        wr16w(player_,0x44,0);wr16w(player_,0x48,0);wr16w(player_,0xB4,0);
        l3_pl00_anim_request_80041ddc(&player_,0,0,1);l3_pl00_shot_enable_on(&player_);
    }
    l3_pl00_snapshot_collision_pose(&player_);
}

bool RuntimeSession::submit_player_animation_record(uint8_t request,uint8_t pose,
                                                    uint8_t countdown,uint8_t byte2,
                                                    int8_t control){
    if(player_.bytes[0xAC]!=request)return false;
    player_.bytes[0xA4]=pose;player_.bytes[0xA5]=countdown;
    player_.bytes[0xA6]=byte2;player_.bytes[0xA7]=static_cast<uint8_t>(control);
    player_animation_record_request_=request;
    return true;
}

void RuntimeSession::integrate_player_motion(bool vertical_motion){
    if(!reference_tables_)return;
    // 0x80041E90 temporarily subtracts +116 from yaw around 0x8002FEA4.
    const uint16_t motion_yaw=uint16_t(player_yaw()-rd16w(player_,0x116))&0xFFFu;
    const auto d=reference_player_motion_delta(*reference_tables_,motion_yaw,
                                               rds16w(player_,0x44),vertical_motion?rds16w(player_,0x46):0,rds16w(player_,0x48));
    // Preserve MIPS 32-bit addition wrap, avoiding signed-overflow UB on host.
    wr32w(player_,0x1C,static_cast<int32_t>(uint32_t(rd32w(player_,0x1C))+uint32_t(d.dx_fixed)));
    wr32w(player_,0x20,static_cast<int32_t>(uint32_t(rd32w(player_,0x20))+uint32_t(d.dy_fixed)));
    wr32w(player_,0x24,static_cast<int32_t>(uint32_t(rd32w(player_,0x24))+uint32_t(d.dz_fixed)));
    sync_primary_position_from_fixed();
}

void RuntimeSession::update_airborne_motion(){
    // Exact normal direction-0 subset of 0x8003E068. Rotation/strafe use the
    // existing installed profile and retail turn helper. Other action owners
    // remain outside this partial jump-state translation.
    const uint16_t held=rd16w(player_,0x11C);
    const uint16_t forward_pair=rd16w(player_,0x144),positive=rd16w(player_,0x146);
    const uint16_t strafe_pair=rd16w(player_,0x148),negative=rd16w(player_,0x14A);
    const uint16_t f_hit=held&forward_pair,s_hit=held&strafe_pair;
    if(f_hit && f_hit!=forward_pair){
        const int value=rds16w(player_,0x44)+((held&positive)?0x28:-0x28);
        wr16w(player_,0x44,static_cast<uint16_t>(static_cast<int16_t>(std::clamp(value,-0x280,0x280))));
    }
    if(s_hit==strafe_pair&&s_hit)wr16w(player_,0x48,0);
    else if(s_hit){
        const int value=rds16w(player_,0x48)+((held&negative)?-0x20:0x20);
        wr16w(player_,0x48,static_cast<uint16_t>(static_cast<int16_t>(std::clamp(value,-0x100,0x100))));
    }
    l3_pl00_turn_pair_80042044(&player_,4,0x20);
    integrate_player_motion();
    // The source tests last-frame collision state here. Current-frame contact
    // is resolved later, preserving its one-update landing handoff.
    if(player_.bytes[0x74]==0){
        l3_pl00_anim_request_80041ddc(&player_,player_.bytes[0x09]==7?19u:22u,0,1);
        player_.bytes[0x09]=8;player_.bytes[0x0A]=6;
    }
}

void RuntimeSession::update_running_player_motion(){
    // USA action2 0x8003CD4C. Ordinary direction0/2 and the four direction
    // initialization rows are scalar translations, not guessed acceleration.
    const auto directional_motion=[this]{
        if(l3_pl00_directional_setup_8003d124(&player_,static_cast<int8_t>(player_.bytes[0x112])))
            integrate_player_motion(false);
    };
    const bool record_current=player_animation_record_request_==player_.bytes[0xAC];
    switch(player_.bytes[0x0A]){
        case 0:{
            const uint8_t direction=player_.bytes[0x112],previous=player_.bytes[0x113];
            if(direction>3||previous>3)return;
            static constexpr uint8_t requests[4][4]={{4,5,9,7},{7,0,5,9},{9,7,0,5},{5,9,7,0}};
            static constexpr int16_t offsets[4]={0,0x400,0x800,-0x400};
            wr16w(player_,0x116,static_cast<uint16_t>(offsets[direction]));
            if(direction!=0||previous!=0)l3_pl00_shot_enable_off(&player_);
            wr16w(player_,0x44,0x200);player_.bytes[0x113]=0;
            player_.bytes[0x0A]=2;player_.bytes[0x174]|=4;
            l3_pl00_anim_request_80041ddc(&player_,requests[direction][previous],0,1);
            directional_motion();return;
        }
        case 1:
            if(record_current&&player_.bytes[0xA4]==1)++player_.bytes[0x0A];
            return;
        case 2:
            if(record_current&&static_cast<int8_t>(player_.bytes[0xA7])==-1){
                player_.bytes[0x44D]=1;player_.bytes[0x0A]=4;l3_pl00_shot_enable_on(&player_);
                l3_pl00_anim_request_80041ddc(&player_,1,player_.bytes[0x112]==1?9:0,0);
            }
            directional_motion();return;
        case 3:
            if(record_current&&player_.bytes[0xA4]==5){player_.bytes[0x44D]=1;player_.bytes[0x0A]=4;}
            [[fallthrough]];
        case 4:
            l3_pl00_turn_pair_80042044(&player_,0x28,0x28);
            directional_motion();return;
        case 5:{
            const auto direction=player_.bytes[0x112];
            if(direction>3)return;
            if(direction==0){player_.bytes[0x09]=0;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;}
            else{
                static constexpr uint8_t stop_requests[4]={0,8,10,6};
                l3_pl00_shot_enable_off(&player_);
                l3_pl00_anim_request_80041ddc(&player_,stop_requests[direction],0,0);
            }
            wr16w(player_,0x4C,0x30);++player_.bytes[0x0A];directional_motion();return;
        }
        case 6:
            if(record_current&&static_cast<int8_t>(player_.bytes[0xA6])==1){
                player_.bytes[0x09]=0;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;
                wr16w(player_,0x116,0);player_.bytes[0x112]=0;wr16w(player_,0x44,0);wr16w(player_,0xB4,0);
                l3_pl00_shot_enable_on(&player_);l3_pl00_anim_request_80041ddc(&player_,0,0,0);
            }
            return;
        default:return;
    }
}

void RuntimeSession::update_ordinary_player_motion(){
    const uint16_t held=rd16w(player_,0x11C);
    const bool forward=(held&rd16w(player_,0x124))!=0,reverse=(held&rd16w(player_,0x126))!=0;
    const uint16_t side_a=rd16w(player_,0x12C),side_b=rd16w(player_,0x12E),side_pair=side_a|side_b;
    const uint16_t side_hit=held&side_pair,turn_pair=rd16w(player_,0x128)|rd16w(player_,0x12A);
    const bool modifier=(held&rd16w(player_,0x134))!=0;
    const auto idle=[this]{player_.bytes[0x09]=0;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;};
    const auto run=[this](uint8_t direction){
        const auto previous=player_.bytes[0x112];player_.bytes[0x09]=2;
        player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;player_.bytes[0x112]=direction;player_.bytes[0x113]=previous;
    };
    // These predicates cover ordinary held movement with the firing, scripted,
    // interaction and jump owners absent. Source selectors 0x80040380..40570
    // preserve their priority and simultaneous-input behavior.
    if(l3_pl00_state_busy_8004324c(&player_))return;
    const auto action=player_.bytes[0x09];
    if(action!=0&&action!=1&&action!=2&&action!=4)return;
    if(action==2){
        const auto direction=player_.bytes[0x112];
        const uint16_t owner_inputs=rd16w(player_,0x144)|side_pair|rd16w(player_,0x138)|
                                    rd16w(player_,0x13A)|rd16w(player_,0x13C);
        if(direction==0){
            if(reverse){
                if(modifier){player_.bytes[0x09]=1;player_.bytes[0x0A]=0;player_.bytes[0x0B]=1;}
                else run(2);
            }else if(!forward)idle();
            else if(modifier){player_.bytes[0x09]=1;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;}
        }else if(direction==2){
            if((held&owner_inputs)==0){if(player_.bytes[0x0A]<5)player_.bytes[0x0A]=5;}
            else if(!reverse&&!modifier){
                if(forward)run(0);
                else if(side_hit!=side_pair&&(held&side_a))run(1);
                else if(side_hit!=side_pair&&(held&side_b))run(3);
                else if(player_.bytes[0x0A]<5)player_.bytes[0x0A]=5;
            }
        }else if(direction==1||direction==3){
            if((held&owner_inputs)==0||side_hit==side_pair){if(player_.bytes[0x0A]<5)player_.bytes[0x0A]=5;}
            else if(!modifier){
                if(forward)run(0);
                else if(direction==1&&reverse)run(2);
                else if(direction==1&&(held&side_b))run(3);
                else if(direction==3&&(held&side_a))run(1);
                else if(direction==3&&reverse)run(2);
                else if(!(held&(direction==1?side_a:side_b))&&player_.bytes[0x0A]<5)player_.bytes[0x0A]=5;
            }
        }
    }else{
        if(!modifier){
            if(action==4&&side_hit!=side_pair&&(held&side_a))run(1);
            else if(action==4&&side_hit!=side_pair&&(held&side_b))run(3);
            else if(forward)run(0);
            else if(side_hit!=side_pair&&(held&side_a))run(1);
            else if(side_hit!=side_pair&&(held&side_b))run(3);
            else if(reverse)run(2);
            else if(held&turn_pair){if(action!=4){player_.bytes[0x09]=4;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;}}
            else if(action!=0)idle();
        }else if(forward||reverse){
            const uint8_t backward=forward?0:1;
            if(action!=1||player_.bytes[0x0B]!=backward){player_.bytes[0x09]=1;player_.bytes[0x0A]=0;player_.bytes[0x0B]=backward;}
        }else if(held&turn_pair){if(action!=4){player_.bytes[0x09]=4;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;}}
        else if(action!=0)idle();
    }
    switch(player_.bytes[0x09]){
        case 0: // 0x8003CB20
            if(player_.bytes[0x0A]==0){
                player_.bytes[0x0A]=1;wr16w(player_,0x44,0);wr16w(player_,0x48,0);
                player_.bytes[0x174]&=0xFC;l3_pl00_shot_enable_on(&player_);
            }else{player_.bytes[0x44D]=0;wr16w(player_,0xB4,0);}
            l3_pl00_anim_request_80041ddc(&player_,0,0,0);return;
        case 1: // 0x8003CBF8: modifier-selected slow forward/reverse.
            if(player_.bytes[0x0A]==0){
                player_.bytes[0x0A]=1;wr16w(player_,0x48,0);
                wr16w(player_,0x44,static_cast<uint16_t>(player_.bytes[0x0B]?-0x50:0x50));player_.bytes[0x174]|=4;
            }
            l3_pl00_anim_request_80041ddc(&player_,player_.bytes[0x0B]?13:12,0,0);
            l3_pl00_turn_pair_80042044(&player_,player_.bytes[0x0B]?0x40:0x20,player_.bytes[0x0B]?0x40:0x20);
            integrate_player_motion(false);return;
        case 2:update_running_player_motion();return;
        case 4: // 0x8003D6DC idle-turn action, not the running turn ramp.
            if(player_.bytes[0x0A]==0){player_.bytes[0x0A]=1;wr16w(player_,0xB6,0);player_.bytes[0x44D]=0;}
            l3_pl00_anim_request_80041ddc(&player_,(held&turn_pair)==turn_pair?0:13,0,0);
            l3_pl00_turn_pair_80042044(&player_,modifier?2:4,modifier?0x20:0x48);return;
        default:return;
    }
}

void RuntimeSession::update_player_motion(){
    if(!reference_tables_) return;
    // USA 0x8003C89C saves rollback mirrors even on stationary frames.
    l3_pl00_snapshot_collision_pose(&player_);
    player_.bytes[0xAD]=player_.bytes[0xAC]; // 0x8003C918: last-request mirror.
    player_.bytes[0xBD]=uint8_t(player_.bytes[0xBD]&0xF7u); // 0x8003C90C..C92C clears bit 8.
    const uint16_t held=rd16w(player_,0x11C);
    const uint16_t forward_mask=rd16w(player_,0x124);

    if(player_.bytes[0x09]==7){
        switch(player_.bytes[0x0A]){
            case 0: // 0x8003DA74 initialization; animation owns launch readiness.
                player_.bytes[0x112]=0;wr16w(player_,0x116,0);wr16w(player_,0xB4,0);
                if((held&rd16w(player_,0x148))==0)wr16w(player_,0x48,0);
                player_.bytes[0x0A]=1;l3_pl00_shot_enable_off(&player_);
                l3_pl00_anim_request_80041ddc(&player_,16,0,1);return;
            case 1:
                if(player_animation_record_request_!=player_.bytes[0xAC])return;
                if(!l3_pl00_jump_launch_8003dac0(&player_))return;
                l3_pl00_anim_request_80041ddc(&player_,17,0,1);
                update_airborne_motion();return;
            case 2:{
                update_airborne_motion();
                if(player_.bytes[0x09]!=7)return;
                const int gravity=l3_pl00_jump_hold_gravity_8003db50(&player_);
                if(gravity==0)player_.bytes[0x0A]=3;
                else if(gravity==2){l3_pl00_anim_request_80041ddc(&player_,18,0,1);player_.bytes[0x0A]=4;}
                return;
            }
            case 3:
                update_airborne_motion();
                if(player_.bytes[0x09]!=7)return;
                // Released-jump deceleration at 0x8003DBBC..DBFC.
                if(-int(rds16w(player_,0x46))>0x240)wr16w(player_,0x46,uint16_t(rds16w(player_,0x46)+0x240));
                else{l3_pl00_anim_request_80041ddc(&player_,18,0,1);wr16w(player_,0x46,0);player_.bytes[0x0A]=5;}
                return;
            case 4:
                l3_pl00_anim_request_80041ddc(&player_,18,0,1);player_.bytes[0x0A]=5;
                [[fallthrough]];
            case 5:
                update_airborne_motion();
                if(player_.bytes[0x09]==7)wr16w(player_,0x46,uint16_t(rds16w(player_,0x46)+0x30));
                return;
            default:return;
        }
    }
    if(player_.bytes[0x09]==8){
        // USA 0x8003DC5C, direction-0 running-jump owner. Other directional
        // air-control rows and the modifier landing owner remain unproved.
        if(player_.bytes[0x112]==0){
            switch(player_.bytes[0x0A]){
                case 0:
                    player_.bytes[0x74]=1;player_.bytes[0x44D]=1;
                    player_.bytes[0x0A]=2;player_.bytes[0xAD]=player_.bytes[0xAC];
                    wr16w(player_,0x46,uint16_t(int16_t((player_.bytes[0x16B]&1)?-0x320:-0x1C0)));
                    if((held&rd16w(player_,0x148))==0)wr16w(player_,0x48,0);
                    l3_pl00_shot_enable_on(&player_);
                    l3_pl00_anim_request_80041ddc(&player_,20,0,1);
                    update_airborne_motion();return;
                case 1:
                case 2:
                    update_airborne_motion();
                    if((held&rd16w(player_,0x138))==0)++player_.bytes[0x0A];
                    else{
                        const int16_t next=int16_t(uint16_t(rds16w(player_,0x46)+0x30));
                        wr16w(player_,0x46,uint16_t(next));
                        if(next>=0){
                            l3_pl00_anim_request_80041ddc(&player_,21,0,1);
                            player_.bytes[0x0A]=uint8_t(player_.bytes[0x0A]+3);
                        }
                    }
                    return;
                case 3:
                    update_airborne_motion();
                    if(-int(rds16w(player_,0x46))>0x240)
                        wr16w(player_,0x46,uint16_t(rds16w(player_,0x46)+0x240));
                    else{
                        l3_pl00_anim_request_80041ddc(&player_,21,0,1);
                        wr16w(player_,0x46,0);player_.bytes[0x0A]=uint8_t(player_.bytes[0x0A]+2);
                    }
                    return;
                case 4:
                    l3_pl00_anim_request_80041ddc(&player_,18,0,1);
                    wr16w(player_,0x44,0x200);++player_.bytes[0x0A];
                    [[fallthrough]];
                case 5:
                    update_airborne_motion();
                    wr16w(player_,0x46,uint16_t(rds16w(player_,0x46)+0x30));return;
                default:break;
            }
        }
        // Original animation-gated direction-0 landing branches. The modifier
        // branch invokes action3, which remains outside this proved subset.
        if(player_animation_record_request_!=player_.bytes[0xAC])return;
        if(player_.bytes[0x0A]==6 && player_.bytes[0x112]==0 &&
           static_cast<int8_t>(player_.bytes[0xA7])==-1 && (held&rd16w(player_,0x134))==0){
            l3_pl00_shot_enable_off(&player_);
            if((held&forward_mask)!=0){
                wr16w(player_,0x44,0x200);wr16w(player_,0x46,0);
                player_.bytes[0x09]=2;player_.bytes[0x0A]=2;player_.bytes[0x0B]=0;
                l3_pl00_anim_request_80041ddc(&player_,23,0,0);
            }else{
                l3_pl00_anim_request_80041ddc(&player_,42,0,0);player_.bytes[0x0A]=7;
            }
        }else if(player_.bytes[0x0A]==7 && static_cast<int8_t>(player_.bytes[0xA7])==-1){
            player_.bytes[0x09]=0;player_.bytes[0x0A]=0;
            wr16w(player_,0x44,0);wr16w(player_,0x48,0);wr16w(player_,0xB4,0);
            wr16w(player_,0x116,0);player_.bytes[0x112]=0;player_.bytes[0x44A]=0;
            l3_pl00_shot_enable_on(&player_);l3_pl00_anim_request_80041ddc(&player_,0,0,0);
        }
        return;
    }

    update_ordinary_player_motion();
}

void RuntimeSession::update_player_contact(){
    // Keep unsupported geometry a hold: preparing the earlier adhesion owner
    // must not publish only half of a frame whose later response is unknown.
    static constexpr std::array<size_t,16> adhesion_offsets{{
        0x09,0x0A,0x0B,0x16,0x17,0x20,0x21,0x22,0x23,
        0x46,0x47,0x74,0x168,0x169,0x110,0x111}};
    std::array<uint8_t,adhesion_offsets.size()> before_adhesion{};
    for(size_t i=0;i<adhesion_offsets.size();++i)before_adhesion[i]=player_.bytes[adhesion_offsets[i]];
    const auto requested_position=player_position_;
    // Original 0x8003C610..C6F8 queries/adheres to the floor before calling
    // 0x8004B414 at C6FC. Adhesion can introduce a wall/ceiling overlap: the
    // same frame's geometry solver must see that resulting requested pose.
    player_contact_=runtime_player_floor_query(*map_,map_area_index_,player_);
    // Floor adhesion/fall at 0x8003C4EC..0x8003C6F8 (decompiled 8003C4EC):
    //   if (work+8 in {1,2} && work+74 == 0 && work+76 == 0) {
    //     floor = 80048E6C(work) - bounds+10;
    //     if (y < floor-0x20) { if (work+75 == 0) {fall action 7/4 ...} work+74 = 1; }
    //     else if (y < floor) { y = floor; }
    //   }
    // The no-floor query returns 32767, so leaving the floor (or the grid)
    // selects the fall branch. 0.0.15.0 removes the earlier native-only
    // "already grounded" precondition, which let Mega Man walk on air.
    // Unported: work+9==0x0E calls 0x800394AC; work+9==3 stops cues 0x8E/0x8F.
    const uint8_t work08=player_.bytes[0x08];
    if(work08>=1&&work08<=2&&player_.bytes[0x74]==0&&player_.bytes[0x76]==0){
        const int32_t floor=player_contact_.floor_y?int32_t(*player_contact_.floor_y):32767;
        if(player_position_.y<floor-32){
            if(player_.bytes[0x75]==0){
                player_.bytes[0x09]=7;player_.bytes[0x0A]=4;player_.bytes[0x0B]=0;
                wr16w(player_,0x46,0);player_.bytes[0x168]=0;player_.bytes[0x169]=0;
                player_.bytes[0x110]=1;player_.bytes[0x111]=0;
            }
            player_.bytes[0x74]=1;
            player_contact_.grounded=false;
        }else if(player_position_.y<floor){
            wr16w(player_,0x16,static_cast<uint16_t>(floor));
            wr32w(player_,0x20,floor*65536);sync_primary_position_from_fixed();
            player_contact_.scratch.y=static_cast<int16_t>(floor);player_contact_.grounded=true;
        }
    }
    player_contact_=runtime_player_contact_step(*map_,map_area_index_,player_);
    if(player_contact_.status==RuntimeContactStatus::Unsupported){
        // A primitive response that is not yet translated (remaining diagonal
        // subtypes and slope/step codes via 0x8004BDC8) must not become
        // free motion: through 0.0.14.51 the requested pose was accepted and
        // Mega Man walked through walls and off the map. Apply the original
        // 0x8004B414 failure contract instead (negative result: restore the
        // +2C/+34 pre-move pose), exactly like IterationLimit. CANDIDATE policy
        // until those responses are ported; never a new collision rule.
        for(size_t i=0;i<adhesion_offsets.size();++i)player_.bytes[adhesion_offsets[i]]=before_adhesion[i];
        player_position_=requested_position;
        player_contact_.scratch={rds16w(player_,0x14),rds16w(player_,0x16),rds16w(player_,0x18),player_.bytes[0x74]};
        l3_pl00_apply_contact_result_8004b414(&player_,static_cast<int32_t>(0x80000000u),&player_contact_.scratch);
        sync_primary_position_from_fixed();
        return;
    }
    if(player_contact_.result!=0){
        // The bounded solver's explicit negative result is the original
        // failure contract. Do not turn an iteration failure into free motion
        // by returning before the source rollback wrapper has consumed it.
        l3_pl00_apply_contact_result_8004b414(&player_,player_contact_.result,&player_contact_.scratch);
        sync_primary_position_from_fixed();
    }
    if(player_contact_.status==RuntimeContactStatus::IterationLimit)return;
    if(player_.bytes[0x74]==1)player_contact_.grounded=false;
    const uint16_t pressed=rd16w(player_,0x11E);
    if(player_contact_.grounded.value_or(false)&&(pressed&rd16w(player_,0x138))!=0){
        const auto action=player_.bytes[0x09];
        if(action<=1 || action==4){
            // 0x800406A8 selects action7; 0x800406DC selects running action8.
            player_.bytes[0x09]=7;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;
        }else if(action==2 && player_.bytes[0x112]==0){
            player_.bytes[0x09]=8;player_.bytes[0x0A]=0;player_.bytes[0x0B]=0;
        }
    }
}

std::optional<size_t> RuntimeSession::nearest_target_index()const{
    std::optional<size_t> best; int64_t bestd=std::numeric_limits<int64_t>::max(); const int64_t maxd=int64_t(combat_tuning_.lock_range)*combat_tuning_.lock_range;
    for(size_t i=0;i<targets_.size();++i){const auto&t=targets_[i];if(!t.active||!t.targetable||t.hp<=0)continue;const auto d=dist2_xz(player_position_,t.position);if(d<maxd&&d<bestd){best=i;bestd=d;}}
    return best;
}

void RuntimeSession::update_lock_on(const RuntimeInputFrame& in){
    const bool toggle=settings_.lock_on_mode==L3_LOCK_ON_TOGGLE;
    if(toggle){if(in.lock_pressed){if(locked_target_index_)locked_target_index_.reset();else locked_target_index_=nearest_target_index();}}
    else {locked_target_index_=in.lock_held?nearest_target_index():std::optional<size_t>{};}
    if(locked_target_index_){const size_t i=*locked_target_index_;if(i>=targets_.size()||!targets_[i].active||!targets_[i].targetable||targets_[i].hp<=0||dist2_xz(player_position_,targets_[i].position)>=int64_t(combat_tuning_.lock_range)*combat_tuning_.lock_range)locked_target_index_.reset();}
}

void RuntimeSession::spawn_buster_projectile(){
    if(!l3_pl00_shot_is_enabled(&player_)||l3_pl00_state_busy_8004324c(&player_))return;
    size_t active=0;for(const auto&p:projectiles_)if(p.active)++active;if(active>=combat_tuning_.max_projectiles)return;
    RuntimeProjectile p; p.serial=next_projectile_serial_++;p.position=player_position_;p.position.y+=0x20;p.lifetime=combat_tuning_.projectile_lifetime;p.radius=combat_tuning_.projectile_radius;p.damage=combat_tuning_.projectile_damage;
    RuntimeVec3i target{};bool has=false;if(locked_target_index_&&*locked_target_index_<targets_.size()){target=targets_[*locked_target_index_].position;has=true;}
    if(has){double dx=double(target.x-p.position.x),dy=double(target.y-p.position.y),dz=double(target.z-p.position.z);double len=std::sqrt(dx*dx+dy*dy+dz*dz);if(len<1.0)len=1.0;p.velocity={int32_t(std::lround(dx/len*combat_tuning_.projectile_speed)),int32_t(std::lround(dy/len*combat_tuning_.projectile_speed)),int32_t(std::lround(dz/len*combat_tuning_.projectile_speed))};}
    else if(reference_tables_){const auto&t=reference_tables_->trig[player_yaw()&0x0FFFu];p.velocity={int32_t((-int64_t(t.sin_q12)*combat_tuning_.projectile_speed)/4096),0,int32_t((-int64_t(t.cos_q12)*combat_tuning_.projectile_speed)/4096)};}
    else{const double ang=(double(player_yaw())/4096.0)*6.2831853071795864769;p.velocity={int32_t(std::lround(-std::sin(ang)*combat_tuning_.projectile_speed)),0,int32_t(std::lround(-std::cos(ang)*combat_tuning_.projectile_speed))};}
    projectiles_.push_back(p);
}

void RuntimeSession::update_projectiles(){
    for(auto&p:projectiles_){if(!p.active)continue;p.position.x+=p.velocity.x;p.position.y+=p.velocity.y;p.position.z+=p.velocity.z;if(++p.age>=p.lifetime){p.active=false;continue;}for(auto&t:targets_){if(!t.active||t.hp<=0)continue;const int64_t rr=int64_t(p.radius+t.radius)*(p.radius+t.radius);if(dist2(p.position,t.position)<=rr){t.hp-=p.damage;if(t.hp<=0){t.hp=0;t.active=false;if(locked_target_index_&&*locked_target_index_==size_t(&t-targets_.data()))locked_target_index_.reset();}p.active=false;break;}}}
    projectiles_.erase(std::remove_if(projectiles_.begin(),projectiles_.end(),[](const auto&p){return !p.active;}),projectiles_.end());
}

void RuntimeSession::start_dialogue(MessageRef ref){
    if(!message_banks_ || ref.bank>=message_banks_->size()||ref.block>=(*message_banks_)[ref.bank].blocks.size())throw std::out_of_range("dialogue reference out of range");
    dialogue_={};dialogue_.active=true;dialogue_.ref=ref;dialogue_cursor_.emplace((*message_banks_)[ref.bank]);dialogue_cursor_->start_block(ref.block);history_.push(ref);
}
void RuntimeSession::stop_dialogue(){dialogue_.active=false;dialogue_.waiting_for_advance=false;dialogue_.waiting_for_script=false;dialogue_.waiting_for_audio=false;dialogue_.waiting_audio_id=0;dialogue_.wait_frames=0;dialogue_cursor_.reset();}

void RuntimeSession::update_dialogue(const RuntimeInputFrame& in){
    if(!dialogue_.active||!dialogue_cursor_)return;
    if(dialogue_.waiting_for_audio)return;
    if(dialogue_.wait_frames){--dialogue_.wait_frames;return;}
    if(dialogue_.waiting_for_advance){if(!in.advance_dialogue)return;dialogue_.waiting_for_advance=false;}
    if(dialogue_.waiting_for_script)return;
    unsigned budget=settings_.text_speed==L3_TEXT_SPEED_INSTANT?64u:(settings_.text_speed==L3_TEXT_SPEED_FAST?4u:1u);
    MmlRuntimeEvent ev;
    while(budget--&&dialogue_cursor_->next(ev)){
        switch(ev.kind){
            case MmlRuntimeEventKind::Glyph: dialogue_.visible_glyphs.push_back(ev.opcode); break;
            case MmlRuntimeEventKind::Newline: dialogue_.visible_glyphs.push_back(0x86); break;
            case MmlRuntimeEventKind::Audio:{const uint32_t id=args_u32le(ev.args);dialogue_.audio_requests.push_back({dialogue_.ref,ev.args,id});dialogue_.waiting_for_audio=true;dialogue_.waiting_audio_id=id;return;}
            case MmlRuntimeEventKind::Wait: dialogue_.wait_frames=ev.args.size()>=2?uint16_t(ev.args[0]|(uint16_t(ev.args[1])<<8)):0;if(dialogue_.wait_frames)return;break;
            case MmlRuntimeEventKind::Next: dialogue_.waiting_for_advance=true; return;
            case MmlRuntimeEventKind::Selection:
            case MmlRuntimeEventKind::Multi:
            case MmlRuntimeEventKind::Control:
            case MmlRuntimeEventKind::UnknownControl:
                dialogue_.script_requests.push_back({dialogue_.ref,ev.opcode,ev.kind,ev.args});dialogue_.waiting_for_script=true; return;
            case MmlRuntimeEventKind::End:
            case MmlRuntimeEventKind::BlockEnd: stop_dialogue(); return;
        }
    }
}

void RuntimeSession::submit_contact_sample(const St00ContactSample& sample,const St00Type3ObjectBounds* bounds){
    const auto matches=match_st00_aux_records(area_.flow.aux,sample,bounds);
    for(const auto&m:matches)aux_triggers_.push_back({m.record_index,m.kind,sample});
}
std::vector<RuntimeAuxTrigger> RuntimeSession::take_aux_triggers(){auto out=std::move(aux_triggers_);aux_triggers_.clear();return out;}
RuntimeMapProbe RuntimeSession::probe_player_map()const{
    RuntimeMapProbe p; p.cell=runtime_map_find_cell(*map_,map_area_index_,player_position_.x,player_position_.z); p.in_area=p.cell.has_value();
    if(p.cell){
        p.idx=runtime_map_cell_idx(*map_,*p.cell);
        p.descriptor=runtime_map_cell_descriptor(*map_,*p.cell);
        p.contact_list_index=runtime_map_cell_contact_list_index(*map_,*p.cell);
        p.contact_groups=runtime_map_cell_contact_groups(*map_,*p.cell);
    }
    return p;
}
int32_t RuntimeSession::apply_retail_contact_resolution(const RuntimeRetailContactResolution& resolution){
    const int32_t result=l3_pl00_apply_contact_result_8004b414(&player_,resolution.result,&resolution.scratch);
    sync_primary_position_from_fixed();
    return result;
}
std::vector<RuntimeAudioRequest> RuntimeSession::take_audio_requests(){auto out=std::move(dialogue_.audio_requests);dialogue_.audio_requests.clear();return out;}
std::vector<RuntimeScriptRequest> RuntimeSession::take_script_requests(){auto out=std::move(dialogue_.script_requests);dialogue_.script_requests.clear();return out;}
void RuntimeSession::complete_audio(uint32_t id){if(dialogue_.active&&dialogue_.waiting_for_audio&&dialogue_.waiting_audio_id==id){dialogue_.waiting_for_audio=false;dialogue_.waiting_audio_id=0;}}
void RuntimeSession::resume_dialogue_script_wait(){if(dialogue_.active)dialogue_.waiting_for_script=false;}

void RuntimeSession::register_cutscene_completion(RuntimeCutsceneCompletionContract c){
    if(c.id==0)throw std::invalid_argument("cutscene contract id 0 is reserved");
    for(const auto* flags:{&c.flags_on,&c.flags_off})
        for(uint16_t flag:*flags)
            if(flag>=kL3StoryFlagBytes*8u)throw std::out_of_range("cutscene completion flag out of range");
    cutscene_contracts_[c.id]=std::move(c);
}
void RuntimeSession::begin_cutscene(uint32_t id){active_cutscene_id_=id;cutscene_skip_hold_frames_=0;camera_.owner=RuntimeCameraOwner::Script;invalidate_source_fade_scheduler_budget();}
void RuntimeSession::end_cutscene(){active_cutscene_id_.reset();cutscene_skip_hold_frames_=0;camera_.owner=RuntimeCameraOwner::Gameplay;camera_.transition_frame=camera_.transition_frames=0;update_gameplay_camera_target();camera_.current=camera_.target;}
bool RuntimeSession::cutscene_skippable()const{return active_cutscene_id_&&cutscene_contracts_.find(*active_cutscene_id_)!=cutscene_contracts_.end();}
bool RuntimeSession::apply_active_cutscene_completion(){
    if(!cutscene_skippable())return false;
    const auto c=cutscene_contracts_.at(*active_cutscene_id_);
    // Destination admission and fallible stage/work preparation precede every
    // completion side effect. Registered flag domains are already validated.
    if(c.destination_location){if(!transition_to_location(*c.destination_location,c.destination_position,c.destination_yaw))throw std::runtime_error("cutscene completion destination is not registered");}
    for(uint16_t f:c.flags_on) set_story_flag(f,true);
    for(uint16_t f:c.flags_off) set_story_flag(f,false);
    if(c.buster_enabled)l3_pl00_shot_enable_on(&player_);else l3_pl00_shot_enable_off(&player_);
    if(c.stop_dialogue)stop_dialogue();
    end_cutscene();return true;
}

void RuntimeSession::start_script_camera(const RuntimeCameraPose&t,uint16_t frames){camera_.owner=RuntimeCameraOwner::Script;camera_.start=camera_.current;camera_.target=t;camera_.transition_frame=0;camera_.transition_frames=frames;}
void RuntimeSession::release_script_camera(uint16_t frames){camera_.owner=RuntimeCameraOwner::Gameplay;camera_.start=camera_.current;update_gameplay_camera_target();camera_.transition_frame=0;camera_.transition_frames=frames;}
void RuntimeSession::update_gameplay_camera_target(){if(camera_.owner!=RuntimeCameraOwner::Gameplay)return;camera_.target.look_at=player_position_;camera_.target.look_at.y+=0x50;camera_.target.eye=player_position_;camera_.target.eye.y+=0x140;const uint16_t yaw=player_yaw();if(reference_tables_){const auto&t=reference_tables_->trig[yaw&0x0FFFu];camera_.target.eye.x+=int32_t((int64_t(t.sin_q12)*0x300)/4096);camera_.target.eye.z+=int32_t((int64_t(t.cos_q12)*0x300)/4096);}else{const double a=(double(yaw)/4096.0)*6.2831853071795864769;camera_.target.eye.x+=int32_t(std::lround(std::sin(a)*0x300));camera_.target.eye.z+=int32_t(std::lround(std::cos(a)*0x300));}camera_.target.yaw=yaw;}
void RuntimeSession::update_camera(){
    if(camera_.owner==RuntimeCameraOwner::Gameplay)update_gameplay_camera_target();
    if(camera_.transition_frames&&camera_.transition_frame<camera_.transition_frames){++camera_.transition_frame;const uint32_t n=camera_.transition_frame,d=camera_.transition_frames;camera_.current.eye=lerp_vec(camera_.start.eye,camera_.target.eye,n,d);camera_.current.look_at=lerp_vec(camera_.start.look_at,camera_.target.look_at,n,d);camera_.current.yaw=uint16_t(lerp_i32(camera_.start.yaw,camera_.target.yaw,n,d));}
    else camera_.current=camera_.target;
}

void RuntimeSession::reset_conditional_spawns(){
    conditional_spawn_objects_={};conditional_spawn_pending_.reset();
    conditional_spawn_qualifier_=-1;conditional_spawn_active_=false;
    ++conditional_spawn_revision_;
    if(!area_.conditional_spawns)return;
    // The proved C0/21 initializer allocates E0/00. Do not enable it merely
    // because an unrelated room has rectangular metadata.
    for(const auto& instance:area_.stage.instances){
        const auto& o=instance.object;
        if(o.source_class==0xC0 && o.subtype==0x21){conditional_spawn_active_=true;break;}
    }
}
void RuntimeSession::update_conditional_spawns(){
    if(!conditional_spawn_active_ || !area_.conditional_spawns)return;
    const auto& area=*area_.conditional_spawns;
    if(conditional_spawn_pending_){
        const auto index=*conditional_spawn_pending_;
        const auto& region=area.regions.at(index);
        StageObjectList admitted;
        admitted.records.reserve(region.records.size());
        for(const auto& record:region.records){
            if(record.predicate!=0 && story_flag(static_cast<uint16_t>(record.predicate)))continue;
            admitted.records.push_back(record.object);
        }
        conditional_spawn_objects_=std::move(admitted);
        conditional_spawn_qualifier_=static_cast<int8_t>(region.qualifier);
        conditional_spawn_pending_.reset();++conditional_spawn_revision_;
        return; // source state2 cannot scan a second gate on this tick.
    }
    const auto selected=st00_spawn_selection(area,conditional_spawn_qualifier_,
        static_cast<int16_t>(player_position_.x),static_cast<int16_t>(player_position_.z));
    if(selected){
        conditional_spawn_objects_.records.clear();++conditional_spawn_revision_;
        conditional_spawn_pending_=selected; // source state1 -> state2 next tick.
    }
}

void RuntimeSession::bind_source_event_profile(SourceEventPollProfile profile,int8_t stage){
    // Validate before publishing. The loader validates the owner's serialized
    // package; this API also rejects incompatible manually constructed profiles.
    if(profile.decoder!=1||profile.source_stage!=0||stage!=profile.source_stage||
       profile.records.size()>kSourceEventPollMaximumRecords)
        throw std::invalid_argument("unsupported source event profile");
    for(const auto& record:profile.records)if(record.flag>=2048)
        throw std::invalid_argument("source event flag outside bank");
    auto prepared=std::make_shared<const SourceEventPollProfile>(std::move(profile));
    source_event_profile_=std::move(prepared);source_event_stage_=stage;
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_loop_snapshot_.reset();source_loop_snapshot_admitted_=false;source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
    source_event_phase6_context_.scratch_half_1f800002.reset();
}
void RuntimeSession::bind_source_event_state(SourceEventWork work,SourceEventFlagSnapshot flags,
    std::optional<int16_t> hp,std::optional<int8_t> transition,std::optional<int16_t> busy)noexcept{
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_event_work_=work;source_event_flags_=flags;
    source_loop_snapshot_admitted_=false;source_loop_snapshot_status_=SourceLoopSnapshotStatus::NeedsConstructorReceipt;
    source_fade_context_={};source_fade_intents_={};
    source_fade_callback_={};source_fade_endpoint_hold_.reset();
    source_event_phase5_context_={};
    source_event_phase6_context_={};source_handoff_display_.reset();source_scene_actor40_initializer_globals_={};
    source_fade_intents_.initialized=source_fade_profile_.has_value();
    // Copy only independently known semantic IDs. Original MSB storage must
    // first be converted with source_event_flags_from_original().
    for(size_t i=0;i<source_event_flags_.values.size();++i){
        const auto known=source_event_flags_.known[i];
        story_flags_[i]=uint8_t((story_flags_[i]&uint8_t(~known))|(flags.values[i]&known));
    }
    update_source_event_globals(hp,transition,busy);source_event_result_={};
}
void RuntimeSession::update_source_event_globals(std::optional<int16_t> hp,
    std::optional<int8_t> transition,std::optional<int16_t> busy)noexcept{
    source_event_hp_=hp;source_event_transition_=transition;source_event_busy_=busy;
    auto& fade=source_fade_context_.work_80098a68;
    fade.known[10]=fade.known[11]=busy.has_value();
    if(busy) {
        const uint16_t bits=static_cast<uint16_t>(*busy);
        fade.raw[10]=uint8_t(bits);fade.raw[11]=uint8_t(bits>>8);
    }
}
bool RuntimeSession::bind_source_fade_profile(SourceFadeStartupProfile profile) noexcept {
    // Rebinding may not erase a pending callback owner's work or receipts.
    if(source_fade_intents_.count||source_fade_callback_.running)return false;
    source_fade_profile_=profile;source_fade_intents_.initialized=true;
    return true;
}

SourceFadeCallbackResult RuntimeSession::service_source_fade_scheduler() noexcept {
    auto result=source_fade_callback_.running
        ?advance_source_fade_callback(source_fade_context_,source_fade_callback_,source_fade_endpoint_hold_)
        :acknowledge_source_fade_scheduler_intent(source_fade_context_,source_fade_intents_,source_fade_callback_,source_fade_endpoint_hold_);
    if(result.committed) {
        const auto& work=source_fade_context_.work_80098a68;
        if(work.known[10]&&work.known[11]) {
            const uint16_t bits=uint16_t(work.raw[10])|(uint16_t(work.raw[11])<<8);
            source_event_busy_=static_cast<int16_t>(bits<=32767?int32_t(bits):int32_t(bits)-65536);
        }
    }
    return result;
}
void RuntimeSession::bind_source_event_phase5_profile(SourceEventPhase5Profile profile) noexcept {
    source_effect_dispatch_lease_.invalidate();source_effect_dispatch_profile_.reset();source_prop_lift_profile_.reset();invalidate_source_prop_foreign_globals(source_prop_foreign_globals_);
    source_event_phase5_profile_=profile;source_event_phase5_resources_={};
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_loop_snapshot_.reset();source_loop_snapshot_admitted_=false;source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
    source_event_phase5_context_={};source_event_phase6_context_={};source_handoff_display_.reset();source_scene_actor40_initializer_globals_={};source_event_phase6_profile_.reset();
}
SourceEventPhase5ResourceStatus RuntimeSession::bind_source_event_phase5_resources() noexcept {
    source_effect_dispatch_lease_.invalidate();
    if(!source_event_phase5_profile_){source_event_phase5_resources_={};return SourceEventPhase5ResourceStatus::NeedsProfile;}
    source_event_phase5_resources_=l3::bind_source_event_phase5_resources(*source_event_phase5_profile_,area_.ebd);
    return source_event_phase5_resources_.status();
}
bool RuntimeSession::bind_source_event_phase6_profile(SourceEventPhase6Profile profile) noexcept {
    if(source_event_phase6_context_.awaiting_camera_8001ec0c||source_event_phase6_context_.awaiting_timeline_8001f070||
        source_event_phase6_context_.awaiting_caller_801011e8||!profile.initialized||profile.camera_program_token!=0x8011af4cu||
        profile.callback_table_token!=0x8011b2e8u)return false;
    source_event_phase6_profile_=profile;return true;
}
std::optional<SourceEventSceneOverlayPresentation> RuntimeSession::source_event_scene_overlay() const noexcept {
    if(!source_event_work_||source_event_work_->active00()==0||source_event_work_->event01()!=1||
        source_event_work_->phase04()!=6||!source_event_phase5_profile_||!source_event_phase6_profile_||
        !source_event_phase5_resources_.matches(*source_event_phase5_profile_)||
        !source_event_phase5_resources_.resident_in(area_.ebd)||!source_event_phase6_context_.scene_overlay_pending)return std::nullopt;
    if(source_event_scene_overlay_generation_.value==0)return std::nullopt;
    auto out=source_event_phase6_context_.scene_overlay_pending;out->lease.native_owner=this;
    out->lease.generation=source_event_scene_overlay_generation_.value;return out;
}
bool RuntimeSession::acknowledge_source_event_scene_overlay(SourceEventSceneOverlayLease lease) noexcept {
    if(lease.native_owner!=this||lease.generation==0||lease.generation!=source_event_scene_overlay_generation_.value)return false;
    const auto pending=source_event_scene_overlay();
    if(!pending||pending->lease.serial!=lease.serial)return false;
    return acknowledge_source_event_scene_overlay_80100614(source_event_phase6_context_,lease.serial);
}
SceneE0FirstUpdatePass RuntimeSession::service_diagnostic_scene_e0_first_updates(const SceneE0Resources& resources) noexcept {
    SceneE0FirstUpdatePass out;
    if(!source_event_phase5_profile_||!source_event_phase5_resources_.matches(*source_event_phase5_profile_)||
        !source_event_phase5_resources_.resident_in(area_.ebd)||!resources.resident_in(area_.ebd)){
        out.status=SceneE0Status::NeedsResourceOwnership;return out;
    }
    // Validate the live roster on every sweep, including sweeps without an
    // auxiliary release. An unsupported unchecked producer revokes the bound.
    (void)prepare_source_prop_foreign_release(source_prop_foreign_globals_,0x800bf578u,
        source_event_scene_overlay_generation_.value,source_event_phase5_context_.pool_e0);
    const auto& occupied=source_event_phase5_context_.pool_e0.used[0];
    if(!occupied)return out;
    out.status=SceneE0Status::NotScheduled;
    for(size_t i=0;i<out.slots.size();++i){
        auto& result=out.slots[i];
        if((*occupied&(0x80000000u>>i))==0){result.status=SceneE0Status::NotScheduled;continue;}
        result=initialize_scene_e0_first_update_80112bdc(source_event_phase5_context_.pool_e0.slots[i],resources,area_.ebd);
        if(result.committed){++out.initialized_count;out.status=SceneE0Status::Initialized;}
        else if(result.status!=SceneE0Status::NotScheduled){++out.held_count;if(out.initialized_count==0)out.status=result.status;}
    }
    return out;
}

std::optional<SourceEventHandoffTransitionIntent> RuntimeSession::source_area_handoff() const noexcept {
    const auto& pending=source_event_phase6_context_.transition_pending;
    if(!pending || pending->source_generation!=source_event_scene_overlay_generation_.value ||
       !source_event_phase5_profile_ || !source_event_phase5_resources_.resident_in(area_.ebd))return std::nullopt;
    return pending;
}
std::optional<uint64_t> RuntimeSession::source_handoff_display_generation() const noexcept {
    if (!source_event_consumers_bound_ || !source_event_work_ || source_handoff_display_ ||
        !source_event_phase5_profile_ ||
        !source_event_phase5_resources_.matches(*source_event_phase5_profile_) ||
        !source_event_phase5_resources_.resident_in(area_.ebd) ||
        source_event_result_.phase6_consumer_status != SourceEventPhase6Status::NeedsHandoffDisplayOwner)
        return std::nullopt;
    const auto& work = *source_event_work_;
    if (!work.known[0] || !work.known[1] || !work.known[4] ||
        !work.active00() || work.event01() != 1 || work.phase04() != 7)
        return std::nullopt;
    return source_event_scene_overlay_generation_.value;
}
bool RuntimeSession::can_commit_source_area_handoff() const noexcept {
    const auto intent = source_area_handoff();
    if (!intent || intent->source_function != 0x80101274u || intent->source_stage != 0 ||
        !intent->known[13] || intent->raw[13] != intent->source_stage ||
        !intent->known[15] || intent->raw[15] != intent->destination_area ||
        intent->destination_area == map_area_index_ || !registered_area(intent->destination_area))
        return false;
    // The existing area adapter invalidates outgoing transport owners. Do not
    // discard the source type-2 audio callback by treating a successful load
    // as its completion. Native load/audio overlap remains unimplemented.
    if (!source_event_commands_.initialized || !source_event_command_result_.committed ||
        source_event_commands_.count > source_event_commands_.records.size())
        return false;
    for (size_t i = 0; i < source_event_commands_.count; ++i) {
        const auto& record = source_event_commands_.records[i];
        for (size_t lane = 0; lane < 12; ++lane)
            if (!record.known[lane]) return false;
        const auto word = [&record](size_t at) noexcept {
            uint32_t value = 0;
            for (size_t byte = 0; byte < 4; ++byte)
                value |= uint32_t(record.raw[at + byte]) << (8 * byte);
            return value;
        };
        if (word(0) == 2 && word(8) == 1) return false;
    }
    auto prepared_player = player_;
    auto prepared_known = source_event_player_known_;
    return apply_source_handoff_spawn_80015ee8(*intent,
        source_event_scene_overlay_generation_.value, prepared_player, prepared_known, true);
}
bool RuntimeSession::commit_source_area_handoff() {
    if (!can_commit_source_area_handoff()) return false;
    const auto intent = *source_area_handoff();
    auto prepared_player = player_;
    auto prepared_known = source_event_player_known_;
    const auto prepared_animation_record = player_animation_record_request_;
    const uint64_t source_generation=source_event_scene_overlay_generation_.value;
    if (!apply_source_handoff_spawn_80015ee8(intent,
            source_event_scene_overlay_generation_.value, prepared_player, prepared_known, true))
        return false;
    auto prepared_flags=source_event_flags_;
    // The admitted ST00 loader prefix has an exact destination-area branch.
    // Area1 writes player+44E=1 and sets the low two player-state bits; it does
    // not manufacture geometry, actor, camera or audio initialization.
    if(!apply_source_st00_area_event_prefix(0,static_cast<int8_t>(intent.destination_area),
            prepared_flags,prepared_player).committed)return false;
    prepared_known[0]=1;prepared_known[0x44e]=1;
    const RuntimeVec3i spawn{rds16w(prepared_player, 0x14), rds16w(prepared_player, 0x16),
                            rds16w(prepared_player, 0x18)};
    // 800158A8 passes argument6=1 for this new-area branch. Its heading is
    // source transition+18 (intent lanes10/11), not an inspection-camera yaw.
    if (!transition_to_area(intent.destination_area, spawn, rd16w(prepared_player, 0x56)))
        return false;
    player_ = prepared_player;
    source_event_player_known_ = prepared_known;
    source_event_flags_=prepared_flags;
    player_animation_record_request_ = prepared_animation_record;
    sync_primary_position_from_fixed();
    // Discard only the old area's observation. The diagnostic reset also
    // clears velocity/actions, which are not stores of original 80015EE8.
    player_contact_ = {};
    update_gameplay_camera_target();
    camera_.current = camera_.target;
    // The source handoff wrote status2 at 800C356C. Original callback80015840
    // clears it at800158CC only after transport, spawn and gameplay-camera
    // initialization. transition_to_area invalidates the outgoing global owner,
    // so publish a new known zero strictly from this successful native receipt.
    std::optional<int8_t> completed_transition_status=2;
    const SourceAreaLoadCompletionReceipt load_receipt{
        this,source_generation,intent.destination_area,0x80015840u,0x80038648u,
        true,true,true};
    if(!acknowledge_source_area_load_80015840(intent,load_receipt,this,completed_transition_status))
        throw std::logic_error("source area load completion receipt rejected after commit");
    source_event_transition_=completed_transition_status;
    return true;
}
void RuntimeSession::update_source_door_event(){
    // 0x8003C4EC calls 0x80050C2C after the player update when flag 0x200 is
    // clear. While a request waits, the source player is owned by event 0.
    // Event 0 keeps +0xBD nonzero while it owns the player, so 0x80050C2C
    // returns before its switch; the explicit check keeps a stale request out.
    if(source_door_request_||source_door_event0_.active||story_flag(0x200))return;
    const auto query=runtime_player_event_primitive_800514fc(*map_,map_area_index_,player_,
        reference_tables_?&*reference_tables_:nullptr);
    source_door_result_=evaluate_source_door_event_80050c2c(query,player_,area_.flow.aux,
        story_flags_.data(),story_flags_.size());
    if(source_door_result_.flag_set)set_story_flag(*source_door_result_.flag_set,true);
    if(source_door_result_.request){
        source_door_request_=source_door_result_.request;++source_door_generation_;
        // 0x7B0 is polled by ST00 0x80100598; 0x8001DDE4 starts event 0 with
        // its work zeroed (CANDIDATE: started in the requesting frame).
        source_door_event0_={};source_door_event0_.active=true;
        source_door_event0_.lanes=source_door_request_->raw;
    }
}
std::optional<SourceEventHandoffTransitionIntent> RuntimeSession::source_door_transition() const noexcept {
    // Offered while event 0 substate 6 waits for the loader (356C == 1).
    const auto& e=source_door_event0_;
    if(!source_door_request_||!e.active||e.phase!=1||e.substate!=6||e.status_356c!=1)return std::nullopt;
    return source_door_transition_intent(*source_door_request_,source_door_generation_);
}
bool RuntimeSession::can_commit_source_door_transition() const noexcept {
    const auto intent=source_door_transition();
    if(!intent||!reference_tables_||!intent->known[15]||intent->destination_area==map_area_index_||
       !registered_area(intent->destination_area))return false;
    auto player=player_;SourceEventPlayerKnown known{};known.fill(1);
    return apply_source_handoff_spawn_80015ee8(*intent,intent->source_generation,player,known,true)&&
           apply_source_door_arrival_80048898(*intent,*reference_tables_,player);
}
bool RuntimeSession::commit_source_door_transition() {
    if(!can_commit_source_door_transition())return false;
    const auto intent=*source_door_transition();
    auto prepared=player_;auto known=source_event_player_known_;
    if(!source_event_consumers_bound_)known.fill(1);
    // Loader spawn: 800158A8 passes argument6=1 for a new area.
    if(!apply_source_handoff_spawn_80015ee8(intent,intent.source_generation,prepared,known,true))return false;
    // Event 0 substate 8 targets; the state-3 copy runs here too so the
    // spawn and previous pose are the arrival (it repeats every frame).
    if(!apply_source_door_arrival_80048898(intent,*reference_tables_,prepared))return false;
    prepared.bytes[8]=3;prepared.bytes[9]=1;
    const RuntimeVec3i spawn{rds16w(prepared,0x14),rds16w(prepared,0x16),rds16w(prepared,0x18)};
    auto event=source_door_event0_;
    if(!transition_to_area(intent.destination_area,spawn,rd16w(prepared,0x56)))return false;
    player_=prepared;
    if(source_event_consumers_bound_){
        for(const size_t at:{size_t(8),size_t(9)})known[at]=1;
        source_event_player_known_=known;
    }
    sync_primary_position_from_fixed();
    player_contact_={};
    // Loader: 356C 1 -> 2 (substate 6), 3 (substate 7), 0, then substate 8.
    // Each is one counted event frame here; the PS1 CD load spends more
    // (CANDIDATE), which shortens the second walk there.
    event.status_356c=2;++event.substate;++event.counter;
    event.status_356c=3;++event.substate;++event.counter;
    event.status_356c=0;++event.camera_requests;++event.substate;++event.counter;
    source_door_event0_=event;
    update_gameplay_camera_target();camera_.current=camera_.target;
    return true;
}
void RuntimeSession::update_source_door_event0(){
    auto& e=source_door_event0_;
    if(!e.active||!reference_tables_)return;
    ++e.frames;
    const auto lane16=[&](size_t at){return int16_t(uint16_t(e.lanes[at]|(uint16_t(e.lanes[at+1])<<8)));};
    // ST00 poll: stage 0x800C1B60 is 0 while event 0 runs from it.
    constexpr uint8_t stage=0;
    if(e.phase==0){
        // 0x80048620: player +6 |= 3, 0x8002EE90(8,128), then 0x80042238.
        if(e.substate==0){player_.bytes[6]=uint8_t(player_.bytes[6]|3u);e.last_sound=8;e.substate=1;}
        const auto ready=source_player_event_ready_80042238(player_,source_event_byte_800c1b93_);
        if(!ready||!*ready)return;
        // 0x800BC7F0 camera script by door heading and event +0xA4 = 0x8008A9F4
        // are camera/command owners (0x8001EC0C), not ported.
        e.substate=0;e.phase=1;return;
    }
    if(e.phase==2){
        // 0x80048A40.
        source_player_event_release_80042314(player_);
        player_.bytes[8]=0;player_.bytes[9]=0;player_.bytes[10]=0;player_.bytes[11]=0;
        set_story_flag(0x7B3,false);e.active=false;
        source_door_request_.reset();
        // 0x8001D990 after the event ends: flag 0x380 + event id.
        set_story_flag(0x380,true);
        return;
    }
    const auto targets=[&](int16_t x,int16_t y,int16_t z,uint16_t heading){
        // 0x80048754 / 0x80048898: player state 3 substate 1 toward the target.
        const auto& trig=reference_tables_->trig[heading&0x0FFFu];
        const int16_t tx=int16_t(uint16_t(int32_t(x)+((int32_t(trig.sin_q12)*64)>>12)));
        const int16_t tz=int16_t(uint16_t(int32_t(z)+((int32_t(trig.cos_q12)*64)>>12)));
        player_.bytes[8]=3;player_.bytes[9]=1;
        wr16w(player_,0x438,uint16_t(tx));wr16w(player_,0x43A,uint16_t(y));wr16w(player_,0x43C,uint16_t(tz));
        wr16w(player_,0x440,0);wr16w(player_,0x442,heading);wr16w(player_,0x444,0);player_.bytes[0x448]=48;
        wr32w(player_,0x428,int32_t(uint32_t(int32_t(tx))<<16));wr32w(player_,0x42C,int32_t(uint32_t(int32_t(y))<<16));
        wr32w(player_,0x430,int32_t(uint32_t(int32_t(tz))<<16));
        ++e.camera_requests;
    };
    switch(e.substate){
        case 0:targets(lane16(0),rds16w(player_,0x16),lane16(4),uint16_t(lane16(16)));++e.substate;break;
        case 1:case 9:++e.substate;set_story_flag(0x7B1,true);break;
        case 2:case 10:if(!story_flag(0x7B2)){++player_.bytes[9];if(e.substate==2)e.walk_state=0;++e.substate;}break;
        case 3:case 11:source_event0_forced_walk_80048a9c(e,player_,stage);break;
        case 4:case 12:set_story_flag(0x7B1,false);++e.substate;break;
        case 5:if(!story_flag(0x7B2)){e.status_356c=1;e.lanes[25]=10;e.lanes[24]=2;++e.substate;}break;
        case 6:if(e.status_356c==2)++e.substate;break;
        case 7:if(!story_flag(0x7B2)){e.status_356c=3;++e.substate;}break;
        case 8:if(e.status_356c==0){targets(lane16(8),int16_t(lane16(10)-1),lane16(12),uint16_t(lane16(18)));++e.substate;}break;
        case 13:if(!story_flag(0x7B2)){set_story_flag(0x7B0,false);set_story_flag(0x7B3,true);e.phase=2;e.substate=0;}break;
        default:break;
    }
    // 0x8001EC0C (command interpreter) is not ported; 0x8001F070 counts.
    ++e.counter;
}
std::optional<int16_t> RuntimeSession::player_packed_cell() const {
    // 0x80048C60 over the player position +0x14/+0x18.
    const auto cell=runtime_map_find_cell(*map_,map_area_index_,rds16w(player_,0x14),rds16w(player_,0x18));
    if(!cell)return int16_t(0);
    return cell->packed_cell;
}
void RuntimeSession::write_idx_descriptor(uint16_t idx,uint16_t descriptor){
    if(idx>=map_->idx.size()||map_->idx[idx].descriptor_index==descriptor)return;
    if(!door_map_||door_map_.get()!=map_.get()){
        // The shared original stays alive for references taken before the swap.
        door_map_source_=map_;door_map_=std::make_shared<RuntimeMap>(*map_);map_=door_map_;
    }
    door_map_->idx[idx].descriptor_index=descriptor;
    ++idx_descriptor_generation_;
}
void RuntimeSession::update_source_door_objects(){
    const bool f7b1=story_flag(0x7B1);
    // CANDIDATE: the area object list that spawns 0x801124B0 is not cooked.
    // A door object is assumed for the player's cell when 0x7B1 is raised
    // there, one of the area's door aux records (0x80050C2C hw4/hw6) lies in
    // that cell, and its descriptor has a 0x1000 closed/open pair.
    const auto cell=player_packed_cell();
    if(f7b1&&cell&&*cell>0){
        bool known=false;for(const auto& o:source_door_objects_)known|=o.idx_index==uint16_t(*cell);
        bool door_record=false;
        for(const auto& r:area_.flow.aux.records){
            if(r.halfwords[0]==-1)break;
            const auto at=runtime_map_find_cell(*map_,map_area_index_,r.halfwords[4],r.halfwords[6]);
            door_record|=at&&at->packed_cell==*cell;
        }
        if(!known&&door_record)
            if(const auto o=source_door_object_init_801124b0(*map_,uint16_t(*cell)))source_door_objects_.push_back(*o);
    }
    for(auto& o:source_door_objects_){
        const auto step=source_door_object_step(o,story_flag(0x7B0),story_flag(0x7B1),cell,*map_);
        if(step.flag_7b1)set_story_flag(0x7B1,*step.flag_7b1);
        if(step.flag_7b2)set_story_flag(0x7B2,*step.flag_7b2);
        if(step.descriptor_write)write_idx_descriptor(o.idx_index,*step.descriptor_write);
    }
}
bool RuntimeSession::bind_source_handoff_display_receipt(const SourceEventHandoffDisplayReceipt& receipt) noexcept {
    if(!receipt.admitted || receipt.native_owner!=this ||
       receipt.source_generation!=source_event_scene_overlay_generation_.value ||
       receipt.completed_function!=0x8002d004u || receipt.first_header!=0x800f7820u ||
       receipt.second_header!=0x800f7830u || receipt.argument5 || receipt.argument6 ||
       !source_event_phase5_resources_.resident_in(area_.ebd))return false;
    source_handoff_display_=receipt;return true;
}
bool RuntimeSession::bind_source_prop_lift_profile(SourcePropLiftProfile profile) noexcept {
    if(!profile.admitted()||!source_event_phase5_profile_||
       !source_event_phase5_resources_.matches(*source_event_phase5_profile_)||
       !source_event_phase5_resources_.resident_in(area_.ebd)||
       profile.ebd_digest()!=source_event_phase5_profile_->ebd_source_digest)return false;
    source_prop_lift_profile_=std::move(profile);return true;
}
SceneE0FirstUpdatePass RuntimeSession::service_diagnostic_scene_e0_updates(const SceneE0Resources& resources,SourceRngService* candidate_rng) noexcept {
    SceneE0FirstUpdatePass out;
    if(!source_event_phase5_profile_||!source_event_phase5_resources_.matches(*source_event_phase5_profile_)||
        !source_event_phase5_resources_.resident_in(area_.ebd)||!resources.resident_in(area_.ebd)){
        out.status=SceneE0Status::NeedsResourceOwnership;return out;
    }
    // Validate the live roster on every sweep, including sweeps without an
    // auxiliary release. An unsupported unchecked producer revokes the bound.
    (void)prepare_source_prop_foreign_release(source_prop_foreign_globals_,0x800bf578u,
        source_event_scene_overlay_generation_.value,source_event_phase5_context_.pool_e0);
    const auto& occupied=source_event_phase5_context_.pool_e0.used[0];
    if(!occupied)return out;
    out.status=SceneE0Status::NotScheduled;
    SceneE0UpdateInputs inputs;inputs.references=reference_tables_?&*reference_tables_:nullptr;
    SourcePropLiftCandidateRng candidate;
    if(candidate_rng&&candidate_rng->admitted()&&!source_scene_rng_tape_.admitted()){
        candidate.service=candidate_rng;candidate.update={frame_counter_,frame_counter_,SourceRngUpdatePhase::PropLiftCallback};
        inputs.lift_candidate=&candidate;
    }
    inputs.event_work=source_event_work_?&*source_event_work_:nullptr;inputs.pool=&source_event_phase5_context_.pool_e0;
    inputs.scratch_half_1f800002=source_event_phase6_context_.scratch_half_1f800002;
    inputs.scene_context=&source_event_phase5_context_;
    SourcePropLiftActorFrameReceipt frame;
    if(inputs.event_work){
        const auto& event=*inputs.event_work;bool pointer_known=true;uint32_t actor_id=0;
        for(size_t k=0;k<4;++k){pointer_known=pointer_known&&event.known[0x24+k];
            actor_id|=uint32_t(event.raw[0x24+k])<<(8*k);}
        if(pointer_known&&actor_id>=0x800c3580u&&(actor_id-0x800c3580u)%1444u==0){
            const auto slot=size_t((actor_id-0x800c3580u)/1444u);
            const auto& actors=source_event_phase5_context_.pool40;
            if(slot<4&&actors.used[0]&&(*actors.used[0]&(0x80000000u>>slot))){
                const auto& actor=actors.slots[slot];uint32_t sequence=0;bool sequence_known=true;
                for(size_t k=0;k<4;++k){sequence_known=sequence_known&&actor.known[152+k];
                    sequence|=uint32_t(actor.raw[152+k])<<(8*k);}
                const auto& payload=area_.ebd.source_payload;
                if(sequence_known&&sequence>=area_.ebd.load_base){
                    const size_t offset=sequence-area_.ebd.load_base;
                    if(offset<=payload.size()&&8<=payload.size()-offset){
                        frame.source_actor=actor_id;frame.sequence_identity=sequence;
                        frame.source_generation=source_event_scene_overlay_generation_.value;
                        frame.frame=payload.data()[offset];frame.admitted=true;
                    }
                }
            }
        }
    }
    inputs.lift.frame=&frame;inputs.lift.source_generation=source_event_scene_overlay_generation_.value;
    if(source_prop_lift_profile_)inputs.lift.curves=&source_prop_lift_profile_->curves();
    for(size_t i=0;i<out.slots.size();++i){auto& result=out.slots[i];
        if((*occupied&(0x80000000u>>i))==0){result.status=SceneE0Status::NotScheduled;continue;}
        inputs.prop_identity=0x800bf118u+uint32_t(i)*80u;
        auto release=prepare_source_prop_foreign_release(source_prop_foreign_globals_,*inputs.prop_identity,
            source_event_scene_overlay_generation_.value,source_event_phase5_context_.pool_e0);
        inputs.prop_release_owner=&release.owner;
        auto& prop=source_event_phase5_context_.pool_e0.slots[i];
        const auto before=prop;
        result=update_scene_e0_80112b9c(prop,resources,area_.ebd,inputs);
        if(result.foreign_release_committed &&
            !commit_source_prop_foreign_release(source_prop_foreign_globals_,release,true)){
            // Synchronous one-use publication: preserve the entire record if
            // its external owner rejected the transaction.
            prop=before;result.committed=false;result.foreign_release_committed=false;
            result.status=SceneE0Status::NeedsForeignReleaseOwner;
        }
        if(result.committed){if(result.status==SceneE0Status::Initialized)++out.initialized_count;else ++out.updated_count;out.status=result.status;}
        else if(result.status!=SceneE0Status::NotScheduled){++out.held_count;if(out.initialized_count==0&&out.updated_count==0)out.status=result.status;}
    }
    return out;
}
bool RuntimeSession::initialize_source_scene_projection_from_executable_boot(const SourceSceneViewProfile& profile) noexcept {
    return initialize_source_scene_projection_80011f48(source_event_phase6_context_.projection,profile);
}
void RuntimeSession::bind_source_main_loop_half_snapshot_1f800002(uint16_t raw) noexcept {
    source_event_phase6_context_.scratch_half_1f800002=raw;
}
bool RuntimeSession::bind_source_loop_snapshot_replay(const SourceLoopSnapshot& snapshot) noexcept {
    // Installing a measured profile supplies no live boundary or counter.
    if(source_scene_rng_tape_.admitted()||!is_supported_source_loop_snapshot(snapshot)||source_loop_snapshot_admitted_)return false;
    source_loop_snapshot_=snapshot;source_loop_snapshot_status_=SourceLoopSnapshotStatus::NeedsConstructorReceipt;return true;
}
bool RuntimeSession::bind_source_scene_rng_replay(const SourceEventRandomTapeProfile& profile) noexcept {
    if(source_recorded_scene_admitted())return false;
    const auto status=install_source_event_random_tape(source_scene_rng_tape_,profile,SourceEventRandomTapeMode::RecordedOpenBiosScene);
    if(status!=SourceEventRandomTapeStatus::Ready)return false;
    // These checkpoints are from one paired capture. Never combine them with
    // the historical L3LS capture or initialize from a native frame counter.
    source_loop_snapshot_.reset();source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
    source_event_phase6_context_.scratch_half_1f800002.reset();
    source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=status;return true;
}
SourceMainLoopPhaseResult RuntimeSession::source_main_loop_recorded_phase_gate() const noexcept {
    if(source_scene_rng_constructor_admitted_&&source_scene_rng_tape_.admitted()){
        const auto& ctor=source_scene_rng_tape_.profile().constructor;
        const std::optional<uint8_t> active=source_event_work_&&source_event_work_->known[0]
            ?std::optional<uint8_t>(source_event_work_->raw[0]):std::nullopt;
        return evaluate_source_main_loop_phase_800155d4(int8_t(ctor.main_phase),int8_t(ctor.subphase),source_event_transition_,active);
    }
    if(!source_loop_snapshot_||!source_loop_snapshot_admitted_)return {};
    const std::optional<uint8_t> active=source_event_work_&&source_event_work_->known[0]
        ?std::optional<uint8_t>(source_event_work_->raw[0]):std::nullopt;
    return evaluate_source_main_loop_phase_800155d4(int8_t(source_loop_snapshot_->main_phase),
        int8_t(source_loop_snapshot_->subphase),source_event_transition_,active);
}
bool RuntimeSession::service_source_main_loop_tail_80011ecc() noexcept {
    return consume_source_main_loop_tail_80011ecc(source_event_phase6_context_.scratch_half_1f800002);
}

void RuntimeSession::bind_source_event_consumers(SourceEventPlayerKnown known,
    SourceEventDriverContext driver,SourceEventDriverIntentQueue intents,
    std::optional<uint8_t> byte) noexcept {
    source_event_player_known_=known;source_event_driver_=driver;
    source_event_driver_intents_=intents;source_event_byte_800c1b93_=byte;
    source_event_consumers_bound_=true;
}
SourceSceneActor40PrefixResult RuntimeSession::service_diagnostic_scene_actor40_prefix(const SourceSceneActor40Resources& resources) noexcept {
    if(!source_event_phase5_resources_.resident_in(area_.ebd)||
       !source_event_phase5_context_.pool40.used[0]||
       !(*source_event_phase5_context_.pool40.used[0]&0x80000000u))return {};
    return initialize_source_scene_actor40_prefix_8010c930(source_event_phase5_context_.pool40.slots[0],resources,area_.ebd);
}
SourceSceneActor40PoseCommit RuntimeSession::service_diagnostic_scene_actor40_initial_pose(const SourceSceneActor40Resources& resources) noexcept {
    if(!source_event_phase5_resources_.resident_in(area_.ebd)||
       !source_event_phase5_context_.pool40.used[0]||
       !(*source_event_phase5_context_.pool40.used[0]&0x80000000u))return {};
    return commit_source_scene_actor40_initial_pose_800322a8(source_event_phase5_context_.pool40.slots[0],resources,area_.ebd,
        reference_tables_?&*reference_tables_:nullptr,source_event_phase6_context_.byte_1f800074);
}
SourceSceneActor40InitializerResult RuntimeSession::service_diagnostic_scene_actor40_initializer(const SourceSceneActor40Resources& resources) noexcept {
    SourceSceneActor40InitializerResult out;
    if(!source_event_consumers_bound_||!source_event_phase5_resources_.resident_in(area_.ebd)||
       !source_event_phase5_context_.pool40.used[0]||
       !(*source_event_phase5_context_.pool40.used[0]&0x80000000u)){
        out.status=SourceSceneActor40InitializerStatus::NeedsResourceOwnership;return out;
    }
    out=initialize_source_scene_actor40_wrapper_8010d824(source_event_phase5_context_.pool40.slots[0],resources,area_.ebd,
        reference_tables_?&*reference_tables_:nullptr,source_event_phase6_context_.byte_1f800074,source_scene_actor40_initializer_globals_);
    if(out.committed&&out.player_mirrors){
        (void)apply_source_scene_actor40_player_mirrors_8010d824(*out.player_mirrors,player_,source_event_player_known_);
        // Exact primary halfword mirrors feed presentation only. Do not synthesize
        // the separately owned fixed16.16 accumulator or player action/yaw fields.
        player_position_={rds16w(player_,0x14),rds16w(player_,0x16),rds16w(player_,0x18)};
    }
    return out;
}
SourceSceneActor40SequenceTick RuntimeSession::service_diagnostic_scene_actor40_sequence(
    const SourceSceneActor40Resources& resources) noexcept {
    SourceSceneActor40SequenceTick out;
    if(!source_event_phase5_resources_.resident_in(area_.ebd)||
       !source_event_phase5_context_.pool40.used[0]||
       !(*source_event_phase5_context_.pool40.used[0]&0x80000000u)){
        out.status=SourceSceneActor40PoseStatus::NeedsResourceOwnership;return out;
    }
    return advance_source_scene_actor40_pose_80032488(source_event_phase5_context_.pool40.slots[0],
        resources,area_.ebd,reference_tables_?&*reference_tables_:nullptr,
        source_event_phase6_context_.byte_1f800074);
}
void RuntimeSession::initialize_source_main_loop_pads_from_executable_boot_800177cc() noexcept {
    l3::initialize_source_main_loop_pads_from_executable_boot(source_main_loop_pads_);
    source_main_loop_pads_bound_=true;source_main_loop_pad_result_={};
}
SourceSceneActor40LaterResult RuntimeSession::service_diagnostic_scene_actor40_update(
    const SourceSceneActor40Resources& resources) noexcept {
    SourceSceneActor40LaterResult out;
    if(!source_event_phase5_resources_.resident_in(area_.ebd)||
       !source_event_phase5_context_.pool40.used[0]||
       !(*source_event_phase5_context_.pool40.used[0]&0x80000000u)){
        out.status=SourceSceneActor40LaterStatus::NeedsResourceOwnership;return out;
    }
    out=update_source_scene_actor40_prefix_8010d824(source_event_phase5_context_.pool40.slots[0],
        resources,area_.ebd,reference_tables_?&*reference_tables_:nullptr,
        source_event_phase6_context_.byte_1f800074,source_event_work_?&*source_event_work_:nullptr,
        &source_event_phase5_context_);
    if(out.committed&&out.audio_cue)
        (void)enqueue_source_audio_8001a008(source_audio_queue_,out.audio_cue->id,
            out.audio_cue->raw_a1,out.audio_cue->raw_a2);
    if(out.committed&&out.pre_presentation_mirrors){
        const auto tail=admit_source_scene_actor40_final_mirrors_8010d824(
            source_event_phase5_context_.pool40.slots[0],resources,area_.ebd,*out.pre_presentation_mirrors);
        out.tail_status=tail.status;
        if(tail.admitted&&tail.player_mirrors){
            out.final_player_mirrors_applied=apply_source_scene_actor40_player_mirrors_8010d824(
                *tail.player_mirrors,player_,source_event_player_known_);
            if(out.final_player_mirrors_applied)
                player_position_={rds16w(player_,0x14),rds16w(player_,0x16),rds16w(player_,0x18)};
        }
    }
    return out;
}
bool RuntimeSession::bind_source_effect_dispatch_profile(SourceEffectDispatchProfile profile) noexcept {
    source_effect_dispatch_lease_.invalidate();
    source_effect_dispatch_profile_=std::move(profile);
    if(!source_event_phase5_profile_)return false;
    return source_effect_dispatch_lease_.bind_overlay_load_80100354(
        *source_effect_dispatch_profile_,*source_event_phase5_profile_,
        source_event_phase5_resources_,area_.ebd,this,source_event_scene_overlay_generation_.value);
}
SourceEffectDispatchBatch RuntimeSession::service_diagnostic_scene_lower_effects(
    const SourceEffectDispatchInputProvider& provider) noexcept {
    if(!source_effect_dispatch_profile_||!source_event_phase5_profile_)return {};
    return tick_source_effect_dispatch_800172f4(source_event_phase5_context_,
        *source_effect_dispatch_profile_,source_effect_dispatch_lease_,
        *source_event_phase5_profile_,source_event_phase5_resources_,area_.ebd,
        this,source_event_scene_overlay_generation_.value,provider);
}
SourceEffectPhase5Batch RuntimeSession::service_diagnostic_scene_effects() noexcept {
    if(!source_event_phase5_resources_.resident_in(area_.ebd)){
        SourceEffectPhase5Batch out;out.status=SourceEffectStatus::InvalidPool;return out;
    }
    return tick_source_phase5_effects_800173b4(source_event_phase5_context_);
}
SourceEffectPhase5Batch RuntimeSession::service_diagnostic_scene_effect_releases() noexcept {
    if(!source_event_phase5_resources_.resident_in(area_.ebd)){
        SourceEffectPhase5Batch out;out.status=SourceEffectStatus::InvalidPool;return out;
    }
    return consume_source_phase5_effect_releases_80031aa4(source_event_phase5_context_);
}
SourceMainLoopPadResult RuntimeSession::service_source_main_loop_digital_pad_adapter(uint16_t held,uint16_t pressed) noexcept {
    if(!source_main_loop_pads_bound_){source_main_loop_pad_result_={};return source_main_loop_pad_result_;}
    auto next=source_main_loop_pads_;
    const uint16_t active_low=uint16_t(~uint16_t(held|pressed));
    bind_source_main_loop_pad_packet(next[0],{{0,0x41,uint8_t(active_low),uint8_t(active_low>>8),0,0,0,0}});
    bind_source_main_loop_pad_packet(next[1],{{255,0,255,255,0,0,0,0}});
    source_main_loop_pad_result_=consume_source_main_loop_pads_80012298(next);
    if(source_main_loop_pad_result_.committed){source_main_loop_pads_=next;
        source_event_phase6_context_.input_800c0c2a=source_main_loop_pad_pressed_800c0c2a(next[0]);}
    return source_main_loop_pad_result_;
}

bool RuntimeSession::initialize_fresh_source_event_subset_st00() noexcept {
    if(!source_event_profile_ || source_event_stage_!=0 || map_area_index_!=0)return false;
    auto flags=initialize_source_story_flags_8001d8c0();
    SourceEventWork work;reset_source_event_work_8001d928(work);
    SourceEventPlayerKnown known{};
    initialize_source_event_fresh_player_header_8003be6c(player_,known);
    initialize_source_event_player_fields_8003bee8(player_,known);
    if(!apply_source_st00_area_event_prefix(0,0,flags,player_).committed)return false;
    // Area0 stores all four state bytes and preserves known upper header bits.
    for(size_t i=8;i<12;++i)known[i]=1;
    SourceEventDriverContext driver;SourceEventDriverIntentQueue intents;
    initialize_source_event_driver_fields(driver,intents);
    bind_source_event_consumers(known,driver,intents);
    bind_source_event_state(work,flags,static_cast<int16_t>(hp_),0,
        initialize_source_event_busy_from_executable_boot());
    if(source_event_phase2_package_) {
        const auto& package=*source_event_phase2_package_;
        initialize_source_event_phase2_from_executable_boot(source_event_phase2_context_,source_event_commands_,
            package.boot_descriptors,package.boot_command_records);
        initialize_source_event_phase2_descriptors_80019f0c(source_event_phase2_context_);
        initialize_source_event_command_queue_8001b44c(source_event_commands_);
        source_event_phase2_intents_={};source_event_phase2_intents_.initialized=true;
        source_event_phase2_bound_=true;
        if(source_event_command_package_) {
            source_event_command_context_=source_event_command_package_->cold_context;
            source_event_cd_inputs_={};source_event_command_result_={};
            source_event_cd_mix_intents_={};source_event_cd_mix_intents_.initialized=true;
            source_event_command_bound_=true;
        }
    }
    // Original fresh/stage initialization stores2 at scratch byte1F800001
    // (8001201C,80013440). This is admitted only by this explicit fresh ST00
    // operation, never by area loading or save restore.
    source_fade_endpoint_hold_=2;
    // Source pool resets are explicit fresh lifecycle operations. Native
    // player owns its six source-layout position bytes; this initialization
    // does not establish retail position or camera-update cadence parity.
    initialize_source_event_scene_from_executable_boot_80068000(source_event_phase5_context_);
    initialize_source_event_phase5_pools_8002e88c(source_event_phase5_context_);
    auto camera_known=source_event_player_known_;
    for(size_t i=0x14;i<0x1a;++i)camera_known[i]=1;
    (void)initialize_source_event_phase5_camera_80038674(source_event_phase5_context_,player_,camera_known);
    if(source_event_phase5_profile_)
        (void)initialize_source_event_phase6_overlay_load(source_event_phase6_context_,*source_event_phase5_profile_);
    initialize_source_event_phase6_executable_boot_80068000(source_event_phase6_context_);
    initialize_source_prop_foreign_globals_from_executable_load(source_prop_foreign_globals_,
        source_event_scene_overlay_generation_.value,true);
    return true;
}
void RuntimeSession::bind_source_event_phase2_package(SourceEventPhase2Package package) noexcept {
    source_event_phase2_package_=package;
    // Installing metadata alone never supplies live source initialization.
    source_event_phase2_bound_=false;
    source_event_command_bound_=false;
}
void RuntimeSession::bind_source_event_command_package(SourceEventCommandPackage package) noexcept {
    source_event_command_package_=package;source_event_command_bound_=false;
}
bool RuntimeSession::deliver_source_event_cd_receipts(const SourceEventCommandInputs& inputs) noexcept {
    if(!source_event_command_bound_||inputs.receipt_count==0||
       inputs.receipt_count>inputs.receipts.size()||
       source_event_cd_inputs_.receipt_count+inputs.receipt_count>source_event_cd_inputs_.receipts.size())return false;
    for(size_t i=0;i<inputs.receipt_count;++i)
        source_event_cd_inputs_.receipts[source_event_cd_inputs_.receipt_count+i]=inputs.receipts[i];
    source_event_cd_inputs_.receipt_count+=inputs.receipt_count;return true;
}

void RuntimeSession::tick(const RuntimeInputFrame& in,SourceRngService* candidate_rng){
    ++frame_counter_;
    if(source_main_loop_pads_bound_)(void)service_source_main_loop_digital_pad_adapter(in.held_psx_mask,in.pressed_psx_mask);
    tick_actor_detection_globals_8005c2b8(actor_detection_globals_);
    if(active_cutscene_id_){
        if(in.skip_held&&cutscene_skippable()){if(cutscene_skip_hold_frames_<60)++cutscene_skip_hold_frames_;if(cutscene_skip_hold_frames_>=60){apply_active_cutscene_completion();return;}}else cutscene_skip_hold_frames_=0;
    }
    const auto source_player_gate=source_event_consumers_bound_
        ?source_player_update_gate_8003c07c(player_,source_event_player_known_):std::optional<bool>{};
    const bool gameplay_owned=!dialogue_.active&&camera_.owner==RuntimeCameraOwner::Gameplay&&!active_cutscene_id_
        &&(!source_player_gate||*source_player_gate);
    if(gameplay_owned){
        update_input_compat(in);
        // 0x8003C07C: +0xBD 0x40 zeroes the pad, 0x20 takes event 0's 0x800C0F96.
        (void)source_player_input_select_8003c07c(player_,source_door_event0_.forced_pad);
        if(player_.bytes[8]==3&&source_door_event0_.active){source_player_state3_80041b8c(player_);sync_primary_position_from_fixed();}
        else if(player_.bytes[8]==0)source_player_state0_8003c7b0(player_);
        else{update_player_motion();update_player_contact();}
        update_source_door_event();
        // CANDIDATE order: event 0 after the player, then the door objects.
        update_source_door_event0();update_source_door_objects();
        if(candidate_combat_enabled_){update_lock_on(in);if(in.fire_pressed)spawn_buster_projectile();}
        else locked_target_index_.reset();
    }
    else{update_input_compat({});locked_target_index_.reset();}
    update_conditional_spawns();
    SourceEventInputs source_event_input;
    source_event_input.flags=&source_event_flags_;
    source_event_input.stage_800c1b60=source_event_stage_;
    source_event_input.transition_status_800c356c=source_event_transition_;
    source_event_input.player_hp_800b521e=source_event_hp_;
    source_event_input.busy_80098a72=source_event_busy_;
    if(source_event_consumers_bound_) {
        source_event_input.player_work=&player_;
        source_event_input.player_known=&source_event_player_known_;
        source_event_input.driver=&source_event_driver_;
        source_event_input.driver_intents=&source_event_driver_intents_;
        source_event_input.byte_800c1b93=source_event_byte_800c1b93_;
    }
    if(source_event_phase2_bound_&&source_event_phase2_package_) {
        source_event_input.phase2_context=&source_event_phase2_context_;
        source_event_input.phase2_profile=&source_event_phase2_package_->profile;
        source_event_input.command_queue=&source_event_commands_;
        source_event_input.phase2_intents=&source_event_phase2_intents_;
    }

    // The command service has independent source ownership. Calling its exact
    // state machine supplies no transport success. The native scheduling order
    // remains a diagnostic integration until original frame chronology is proved.
    if(source_event_command_bound_&&source_event_command_package_) {
        source_event_command_result_=consume_source_event_command_dispatch_8001c95c(
            source_event_commands_,source_event_command_context_,source_event_command_package_->profile,
            source_event_cd_inputs_,&source_event_cd_mix_intents_);
        if(source_event_command_result_.committed&&source_event_command_result_.transport_receipts_consumed) {
            const size_t consumed=source_event_command_result_.transport_receipts_consumed;
            for(size_t i=consumed;i<source_event_cd_inputs_.receipt_count;++i)
                source_event_cd_inputs_.receipts[i-consumed]=source_event_cd_inputs_.receipts[i];
            source_event_cd_inputs_.receipt_count-=consumed;
        }
        source_event_input.byte_80098ab8=source_event_command_context_.byte_80098ab8;
    }
    source_event_input.fade_context=&source_fade_context_;
    source_event_input.fade_profile=source_fade_profile_?&*source_fade_profile_:nullptr;
    source_event_input.fade_intents=&source_fade_intents_;
    source_event_input.phase5_context=&source_event_phase5_context_;
    source_event_input.phase5_profile=source_event_phase5_profile_?&*source_event_phase5_profile_:nullptr;
    source_event_input.phase5_resources=source_event_phase5_resources_.resident_in(area_.ebd)
        ?&source_event_phase5_resources_:nullptr;
    source_event_input.phase6_context=&source_event_phase6_context_;
    source_event_input.phase6_profile=source_event_phase6_profile_?&*source_event_phase6_profile_:nullptr;
    source_event_input.scene_overlay_serial=mint_scene_overlay_serial();
    SourceEventHandoffInputs handoff;
    if(source_event_consumers_bound_){
        handoff.player=&player_;handoff.player_known=&source_event_player_known_;
        handoff.flags=&source_event_flags_;handoff.commands=&source_event_commands_;
        handoff.source_generation=source_event_scene_overlay_generation_.value;
        handoff.display=source_handoff_display_?&*source_handoff_display_:nullptr;
        source_event_input.handoff=&handoff;
    }
    SourceLoopSnapshotBoundary recorded_boundary;
    if(source_loop_snapshot_&&!source_loop_snapshot_admitted_) {
        recorded_boundary.recorded_replay_selected=true;
        if(source_event_work_) {
            const auto& work=*source_event_work_;
            if(work.known[0])recorded_boundary.event_active=work.raw[0];
            if(work.known[1])recorded_boundary.event_id=work.raw[1];
            if(work.known[4])recorded_boundary.event_phase=work.raw[4];
        }
        // These named replay owners come from the explicitly selected capture;
        // they are not recovered native boot globals or a universal retail seed.
        recorded_boundary.main_phase=source_loop_snapshot_->main_phase;
        recorded_boundary.subphase=source_loop_snapshot_->subphase;
        recorded_boundary.cadence=source_loop_snapshot_->cadence;
        recorded_boundary.scratch0=source_loop_snapshot_->scratch0;
        if(source_event_phase5_profile_&&source_event_phase5_resources_.matches(*source_event_phase5_profile_)&&
           source_event_phase5_resources_.resident_in(area_.ebd)) {
            recorded_boundary.owner=recorded_boundary.current_owner=this;
            recorded_boundary.generation=recorded_boundary.current_generation=source_event_scene_overlay_generation_.value;
            recorded_boundary.exe_sha256=source_loop_snapshot_->exe_sha256;
            recorded_boundary.overlay_sha256=source_loop_snapshot_->overlay_sha256;
            recorded_boundary.bios_sha256=source_loop_snapshot_->bios_sha256;
            recorded_boundary.ebd_sha256=source_event_phase5_profile_->ebd_source_digest;
        }
    }
    SourceEventRandomTapePrepared rng_prepared;
    const bool paired_resources=source_scene_rng_tape_.admitted()&&source_event_phase5_profile_&&
        source_event_phase5_resources_.matches(*source_event_phase5_profile_)&&source_event_phase5_resources_.resident_in(area_.ebd)&&
        source_event_phase5_profile_->ebd_source_digest==source_scene_rng_tape_.profile().ebd_sha256;
    const bool paired_constructor_boundary=paired_resources&&!source_scene_rng_constructor_admitted_&&source_event_work_&&
        source_event_work_->known[0]&&source_event_work_->known[1]&&source_event_work_->known[4]&&
        source_event_work_->active00()==1&&source_event_work_->event01()==1&&source_event_work_->phase04()==5;
    if(paired_resources&&source_scene_rng_constructor_admitted_&&source_event_work_&&source_event_phase6_profile_&&
       !source_event_phase6_context_.awaiting_camera_8001ec0c&&!source_event_phase6_context_.awaiting_timeline_8001f070&&
       !source_event_phase6_context_.awaiting_caller_801011e8&&
       source_event_phase6_selected_callback_8010102c(*source_event_work_,*source_event_phase6_profile_)==kSourceEventRandomEffectCallback){
        SourceEventRandomTapeContext context;context.work=&*source_event_work_;context.scene=&source_event_phase5_context_;
        context.callback=kSourceEventRandomEffectCallback;context.scratch_half=source_event_phase6_context_.scratch_half_1f800002;
        context.resource_owner_admitted=true;context.resource_owner=context.current_resource_owner=this;
        context.resource_generation=context.current_resource_generation=source_event_scene_overlay_generation_.value;
        const auto& profile=source_scene_rng_tape_.profile();context.exe_sha256=profile.exe_sha256;context.overlay_sha256=profile.overlay_sha256;
        context.bios_sha256=profile.bios_sha256;context.ebd_sha256=source_event_phase5_profile_->ebd_source_digest;
        rng_prepared=prepare_source_event_random_tape(source_scene_rng_tape_,context);
        source_scene_rng_tape_status_=rng_prepared.status;
        if(rng_prepared.status==SourceEventRandomTapeStatus::Ready){source_event_input.random_callback_receipts=&rng_prepared.receipts;
            source_event_input.random_callback_owner=&rng_prepared.receipt_owner;}
    }
    SourceEventCandidateRng candidate;
    if(candidate_rng&&candidate_rng->admitted()&&!source_scene_rng_tape_.admitted()){
        candidate.service=candidate_rng;candidate.source_generation=source_event_scene_overlay_generation_.value;
        candidate.update={frame_counter_,frame_counter_,SourceRngUpdatePhase::SceneEventCallback};
        source_event_input.candidate_rng=&candidate;
    }
    source_event_result_=step_source_event_pipeline(source_event_work_,source_event_profile_.get(),source_event_input);
    if(paired_constructor_boundary&&source_event_result_.committed&&source_event_result_.phase5_consumer_status==SourceEventPhase5Status::Consumed){
        source_event_phase6_context_.scratch_half_1f800002=source_scene_rng_tape_.profile().constructor.scratch_half;
        source_scene_rng_constructor_admitted_=true;
    }
    if(rng_prepared.status==SourceEventRandomTapeStatus::Ready){
        const bool completed=source_event_result_.random_callback_status==SourceEventRandomEffectStatus::Consumed;
        source_scene_rng_tape_status_=commit_source_event_random_tape(source_scene_rng_tape_,rng_prepared,completed,
            source_event_result_.random_callback_consumed_count);
    }
    if(source_loop_snapshot_&&!source_loop_snapshot_admitted_) {
        recorded_boundary.constructor_consumed=source_event_result_.committed&&
            source_event_result_.phase5_consumer_status==SourceEventPhase5Status::Consumed;
        if(recorded_boundary.constructor_consumed)recorded_boundary.source_pc=kSourceLoopSnapshotConstructorPc;
        source_loop_snapshot_status_=evaluate_source_loop_snapshot_admission(&*source_loop_snapshot_,recorded_boundary);
        if(source_loop_snapshot_status_==SourceLoopSnapshotStatus::Ready) {
            source_event_phase6_context_.scratch_half_1f800002=source_loop_snapshot_->scratch_half;
            source_loop_snapshot_admitted_=true;
        }
    }

    if(source_event_result_.committed && source_event_result_.fade_consumer_status) {
        const auto& fade=source_fade_context_.work_80098a68;
        if(fade.known[10]&&fade.known[11]) {
            const uint16_t bits=uint16_t(fade.raw[10])|(uint16_t(fade.raw[11])<<8);
            source_event_busy_=static_cast<int16_t>(bits<=32767?int32_t(bits):int32_t(bits)-65536);
        }
    }
    update_projectiles();update_dialogue(in);update_camera();
}

BottomScreenModel RuntimeSession::bottom_screen_model()const{
    BottomScreenModel m; m.context=dialogue_.active?BottomScreenContext::Dialogue:(camera_.owner==RuntimeCameraOwner::Script?BottomScreenContext::Cinematic:BottomScreenContext::Gameplay);m.area_name_id=static_cast<uint16_t>(area_index_);m.hp=hp_;m.hp_max=hp_max_;m.buster_enabled=l3_pl00_shot_is_enabled(&player_);m.lock_on_active=locked_target_index_.has_value();m.objective_available=objective_.has_value();if(objective_)m.objective=*objective_;m.dialogue_history_count=history_.size();m.save_available=!dialogue_.active&&camera_.owner==RuntimeCameraOwner::Gameplay;m.cutscene_skip_available=cutscene_skippable();m.cutscene_skip_hold=cutscene_skip_hold_frames_;return m;
}
BottomScreenLayout RuntimeSession::bottom_screen_layout()const{return build_bottom_screen_layout(bottom_screen_model(),settings_);}

void RuntimeSession::set_story_flag(uint16_t bit,bool value){
    if(bit>=kL3StoryFlagBytes*8u)throw std::out_of_range("story flag out of range");
    const uint8_t mask=uint8_t(1u<<(bit&7u));auto& v=story_flags_[bit>>3u];if(value)v=uint8_t(v|mask);else v=uint8_t(v&uint8_t(~mask));
    if(bit<2048){auto& known=source_event_flags_.known[bit>>3u];known=uint8_t(known|mask);
        auto& source=source_event_flags_.values[bit>>3u];if(value)source=uint8_t(source|mask);else source=uint8_t(source&uint8_t(~mask));}
}
bool RuntimeSession::story_flag(uint16_t bit)const{return bit<kL3StoryFlagBytes*8u && (story_flags_[bit>>3u]&(1u<<(bit&7u)))!=0;}
RuntimeSaveState RuntimeSession::make_save_state(uint32_t checkpoint)const{
    RuntimeSaveState s;s.checkpoint_id=checkpoint;s.area_id=static_cast<uint16_t>(area_index_);s.player_yaw=player_yaw();s.player_x=player_position_.x;s.player_y=player_position_.y;s.player_z=player_position_.z;s.hp=hp_;s.hp_max=hp_max_;s.play_frames=frame_counter_;s.qol=settings_;s.story_flags=story_flags_;return s;
}
void RuntimeSession::apply_save_state(const RuntimeSaveState&s){
    // This transaction prepares the fallible stage/work copy before publishing
    // location/collision identity and clears the same outgoing scene state as
    // an ordinary accepted transition. Saved scalar assignments cannot allocate.
    if(!transition_to_location(s.area_id,{s.player_x,s.player_y,s.player_z},s.player_yaw))
        throw std::runtime_error("save references an unregistered runtime location");
    hp_max_=s.hp_max?s.hp_max:1;hp_=std::min(s.hp,hp_max_);frame_counter_=s.play_frames;
    settings_=s.qol;story_flags_=s.story_flags;l3_qol_settings_sanitize(&settings_);
    // Native saves do not retain the original raw selector; a successful load
    // therefore invalidates fresh-game admission. Failed loads preserve it.
    source_stats_selector_.reset();
    source_event_work_.reset();source_event_flags_={};source_event_hp_.reset();
    source_event_player_known_={};source_event_driver_={};source_event_driver_intents_={};source_event_consumers_bound_=false;source_event_byte_800c1b93_.reset();
    source_event_phase2_bound_=false;source_event_phase2_context_={};source_event_commands_={};source_event_phase2_intents_={};
    source_event_command_bound_=false;source_event_command_context_={};source_event_cd_inputs_={};
    source_event_command_result_={};source_event_cd_mix_intents_={};
    source_event_transition_.reset();source_event_busy_.reset();source_event_result_={};
    source_fade_profile_.reset();source_fade_context_={};source_fade_intents_={};
    source_fade_callback_={};source_fade_endpoint_hold_.reset();
    source_effect_dispatch_lease_.invalidate();source_effect_dispatch_profile_.reset();source_prop_lift_profile_.reset();invalidate_source_prop_foreign_globals(source_prop_foreign_globals_);source_event_phase5_profile_.reset();source_event_phase5_context_={};source_event_phase5_resources_={};source_event_phase6_context_={};source_handoff_display_.reset();source_scene_actor40_initializer_globals_={};source_event_phase6_profile_.reset();
    invalidate_source_event_random_tape(source_scene_rng_tape_);source_scene_rng_constructor_admitted_=false;source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    source_loop_snapshot_.reset();source_loop_snapshot_admitted_=false;source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
}
RuntimeCheckpoint RuntimeSession::make_checkpoint(uint32_t id)const{return {make_save_state(id)};}
void RuntimeSession::restore_checkpoint(const RuntimeCheckpoint&cp){apply_save_state(cp.save);}

} // namespace l3

namespace l3 {
bool RuntimeSession::acknowledge_source_event_cd_mix(size_t count) noexcept {
    if(!source_event_command_bound_||!source_event_cd_mix_intents_.initialized||
       count>source_event_cd_mix_intents_.count)return false;
    for(size_t i=count;i<source_event_cd_mix_intents_.count;++i)
        source_event_cd_mix_intents_.records[i-count]=source_event_cd_mix_intents_.records[i];
    source_event_cd_mix_intents_.count-=count;return true;
}
} // namespace l3
