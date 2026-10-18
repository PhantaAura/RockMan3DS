#include "legends3ds/actor_runtime.h"
#include "legends3ds/child_runtime.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/class20_03_hit.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace l3 {
namespace {
ImportedActorPacketStatus packet_alias_status(const ImportedActorInstance& actor) noexcept {
    if(!actor.source_proxy_bound||!actor.source_behavior||actor.source_class!=0x20||actor.subtype!=3||
       actor.initializer!=0x80102410)return ImportedActorPacketStatus::NeedsOwner;
    if(actor.original_pool_slot>=16||actor.source_proxy_target.source_address!=
       0x8009f108u+uint32_t(actor.original_pool_slot)*0x3c0u+0x33cu)
        return ImportedActorPacketStatus::InvalidIdentity;
    if(actor.source_behavior->packet_33c&&actor.source_behavior->packet_33c!=actor.source_proxy_target.packet_00)
        return ImportedActorPacketStatus::PendingPacketConflict;
    return ImportedActorPacketStatus::SourceBackedPartial;
}
void copy_packet_alias(Class20_03CallbackState& state,const SourceHitProxyTarget& target) noexcept {
    state.packet_33c=target.packet_00;state.hit_flags_340=target.flags_04;
    state.hit_point_344=target.point_08;state.hit_word_34a=target.word_0e;
}
}
ImportedActorPacketStatus stage_imported_actor_proxy_packet(ImportedActorInstance& actor) noexcept {
    const auto status=packet_alias_status(actor);
    if(status!=ImportedActorPacketStatus::SourceBackedPartial)return status;
    copy_packet_alias(*actor.source_behavior,actor.source_proxy_target);return status;
}
std::optional<ImportedActorTargetSample> sample_imported_actor_target(
    const ImportedActorInstance& actor, int32_t player_x, int32_t player_z,
    const SourceAngleSteps* steps) noexcept {
    if (actor.source_class != 0x20 || actor.subtype != 3 ||
        player_x < INT16_MIN || player_x > INT16_MAX ||
        player_z < INT16_MIN || player_z > INT16_MAX) return std::nullopt;
    ImportedActorTargetSample sample;
    const auto x = static_cast<int16_t>(player_x), z = static_cast<int16_t>(player_z);
    sample.source_work_80 = class20_03_distance_work80(
        actor.source_origin[0], actor.source_origin[2], x, z);
    sample.source_heading_80031198 = source_heading_80031198(
        actor.source_origin[0], actor.source_origin[2], x, z, steps);
    sample.source_player_to_actor_angle_80031198 = source_heading_80031198(
        x, z, actor.source_origin[0], actor.source_origin[2], steps);
    return sample;
}

std::optional<ActorInitializerProfile> actor_initializer_profile_for(const StageRuntimeObject& o) {
    // 0x801068a8 indexes work+3, then 0x80106b10 indexes work+8. For the
    // fresh parameter0/dispatch0 state, its direct initializer is 0x80106974.
    // Other selectors are intentionally unsupported until separately traced.
    if (o.source_class != 0x20 || o.raw[4] != 0)
        return std::nullopt;
    ActorInitializerProfile p;
    if (o.subtype == 3 && (o.source_param3 == 0 || o.source_param3 == 0x80 || o.source_param3 == 0xA0)) {
        // Active subtype table ->0x8010226C; fresh state0 ->0x80102410.
        // Calls0x8002F9C4 with null variant and requests pose0 at0x801025D8.
        p.source_initializer = 0x80102410;
        p.raw_scale = {{512, 512, 512}};
        p.selects_first_class_subtype_row = true;
        p.floor_query_required = true;
        // Six signed bounds at0x8011BED4 are installed before0x801025A8
        // calls0x80048E6C. The fresh current/previous positions are equal.
        p.floor_query_uses_zero_bounds = false;
        p.floor_bounds = RetailPlayerBounds{-60, 60, -60, 60, -128, 0};
        p.initial_pose_request = 0;
        return p;
    }
    if (o.subtype != 0x35 || o.source_param3 != 0) return std::nullopt;
    p.source_initializer = 0x80106974;
    p.raw_scale = {{512, 512, 512}};
    p.raw_shade = 0x4210;
    p.raw_shade_verified = true;
    p.selects_first_class_subtype_row = true; // wrapper receives a null variant pointer.
    p.floor_query_required = true;
    p.floor_bounds = RetailPlayerBounds{0, 0, 0, 0, 0, 0};
    return p;
}

std::optional<int16_t> class20_03_initial_hp(uint8_t raw_stats_selector,
                                          uint8_t raw_context_high_byte) {
    if ((raw_context_high_byte & 0x40u) != 0u) return std::nullopt;
    return raw_stats_selector == 2 ? int16_t(128) : int16_t(64);
}

ActorAreaRuntime spawn_imported_actors(const StageRuntimeArea& stage, const EbdDirectory& ebd, size_t cap) {
    ActorAreaRuntime out;
    std::vector<std::pair<size_t, std::shared_ptr<const ActorMeshResource>>> resources;
    size_t class20_count = 0;
    for (const auto& input : stage.instances) {
        const auto& o = input.object;
        if (o.source_class != 0x20) continue;
        if (++class20_count > 16) throw std::runtime_error("actor class20 original pool capacity exceeded");
        ActorSpawnDiagnostic d;
        d.source_index = o.source_index;
        const auto profile = actor_initializer_profile_for(o);
        if (!profile) {
            d.reason = "no source-backed initializer profile";
            out.diagnostics.push_back(std::move(d));
            continue;
        }
        d.initializer = profile->source_initializer;
        const auto rows = find_ebd_entries(ebd, o.source_class, o.subtype);
        if (rows.empty() || ebd.source_payload.empty()) {
            d.state = ActorSpawnState::RESOURCE_UNAVAILABLE;
            d.reason = rows.empty() ? "initializer has no matching EBD row" : "retained source payload unavailable";
            out.diagnostics.push_back(std::move(d));
            continue;
        }
        const auto row = rows.front(); // proved null-variant first-match semantics.
        d.row = static_cast<uint32_t>(row);
        std::shared_ptr<const ActorMeshResource> resource;
        for (const auto& shared : resources) if (shared.first == row) resource = shared.second;
        try {
            if (!resource) {
                if (out.decoded_resource_bytes > cap) throw std::runtime_error("actor resource budget inconsistent");
                auto decoded = decode_ebd_actor_mesh(ebd, row, ActorMeshTier::NEAR,
                                                    cap - out.decoded_resource_bytes);
                out.decoded_resource_bytes += decoded.footprint.decoded_bytes;
                resource = std::make_shared<const ActorMeshResource>(std::move(decoded));
                resources.emplace_back(row, resource);
                ++out.unique_resource_count;
            }
            if (!input.allocation.pool_slot) throw std::runtime_error("actor lacks original pool allocation");
            ImportedActorInstance actor;
            actor.source_index = o.source_index;
            actor.original_pool_slot = *input.allocation.pool_slot;
            actor.source_class = o.source_class; actor.subtype = o.subtype;
            actor.initializer = profile->source_initializer;
            actor.row = static_cast<uint32_t>(row);
            actor.source_origin = o.source_position;
            actor.heading = static_cast<uint16_t>(uint16_t(o.raw[7]) << 4);
            actor.raw_scale = profile->raw_scale; actor.raw_shade = profile->raw_shade;
            actor.raw_shade_verified = profile->raw_shade_verified;
            actor.floor_query_required = profile->floor_query_required;
            actor.floor_query_uses_zero_bounds = profile->floor_query_uses_zero_bounds;
            actor.floor_bounds = profile->floor_bounds;
            if (!input.work || input.work->bytes.size() < 7)
                throw std::runtime_error("actor lacks fresh source dispatcher work bytes");
            actor.source_dispatch_flag0 = input.work->bytes[0];
            actor.source_dispatch_flag6 = input.work->bytes[6];
            if (input.work->bytes.size() > 0x0f) actor.source_context_high_byte = input.work->bytes[0x0f];
            if (o.subtype == 3) {
                const auto& w = input.work->bytes;
                if (w.size() < 0x64) throw std::runtime_error("actor behavior work seed truncated");
                Class20_03CallbackState state;
                state.work_00 = w[0];
                state.behavior.parameter_03 = w[3];
                state.flags_0c = w[0x0c];
                state.source_delay_0d = w[0x0d] < 128 ? static_cast<int8_t>(w[0x0d])
                    : static_cast<int8_t>(int(w[0x0d])-256);
                state.restore_action_0e = w[0x0e];
                state.behavior.heading_56 = actor.heading;
                for (size_t lane = 0; lane < 4; ++lane) {
                    state.raw_14[lane] = rd_s16(w.data() + 0x14 + lane * 2);
                    state.previous_raw_2c[lane] = rd_s16(w.data() + 0x2c + lane * 2);
                    const auto fixed = rd_u32(w.data() + 0x1c + lane * 4);
                    const auto prior = rd_u32(w.data() + 0x34 + lane * 4);
                    state.fixed_1c[lane] = fixed <= INT32_MAX ? static_cast<int32_t>(fixed)
                        : static_cast<int32_t>(int64_t(fixed) - 0x100000000LL);
                    state.previous_fixed_34[lane] = prior <= INT32_MAX ? static_cast<int32_t>(prior)
                        : static_cast<int32_t>(int64_t(prior) - 0x100000000LL);
                    state.angle_54[lane] = rd_u16(w.data() + 0x54 + lane * 2);
                    state.previous_angle_5c[lane] = rd_u16(w.data() + 0x5c + lane * 2);
                }
                if(w.size()<=0x74)throw std::runtime_error("actor contact work seed truncated");
                state.work_64 = rd_s16(w.data()+0x64);
                for(size_t axis=0;axis<3;++axis)actor.source_contact_velocity[axis]=rd_s16(w.data()+0x44+axis*2);
                actor.source_contact_state74=w[0x74];
                actor.source_behavior = state;
            }
            actor.resource = std::move(resource);
            if (profile->initial_pose_request)
                bind_imported_actor_pose(actor, ebd, *profile->initial_pose_request,
                                         profile->initial_pose_record, true);
            out.actors.push_back(std::move(actor));
            d.state = ActorSpawnState::MESH_BOUND;
            d.reason = profile->initial_pose_request
                ? "mesh and initializer-selected pose bound; floor remains explicit"
                : "mesh bound; floor and retail-selected pose remain explicit";
        } catch (const std::exception& e) {
            d.state = ActorSpawnState::RESOURCE_REJECTED;
            d.reason = e.what();
        }
        out.diagnostics.push_back(std::move(d));
    }
    return out;
}

void commit_imported_actor_reload_candidate(ActorAreaRuntime& active,
                                           ActorAreaRuntime&& candidate, size_t cap) {
    if (active.decoded_resource_bytes > cap || candidate.decoded_resource_bytes > cap)
        throw std::runtime_error("actor reload decoded-element budget exceeded");
    if (candidate.actors.size() > 16)
        throw std::runtime_error("actor reload original class20 capacity exceeded");
    for (const auto& diagnostic : candidate.diagnostics) {
        if (diagnostic.state == ActorSpawnState::RESOURCE_UNAVAILABLE ||
            diagnostic.state == ActorSpawnState::RESOURCE_REJECTED)
            throw std::runtime_error("actor reload candidate resource failure: " + diagnostic.reason);
    }
    for (const auto& actor : candidate.actors)
        if (!actor.resource || !actor.pose)
            throw std::runtime_error("actor reload candidate lacks mesh or presentation pose");
    static_assert(std::is_nothrow_move_assignable<ActorAreaRuntime>::value,
                  "validated actor reload commit must not allocate or throw");
    active = std::move(candidate);
}

void bind_imported_actor_pose(ImportedActorInstance& actor, const EbdDirectory& ebd,
                             uint8_t request, uint8_t record, bool selected_by_retail_state) {
    if (!actor.resource) throw std::runtime_error("actor pose requires bound mesh");
    auto pose = decode_ebd_actor_pose(ebd, actor.row, *actor.resource, request, record);
    actor.pose = std::move(pose);
    actor.pose_selected_by_retail_state = selected_by_retail_state;
    actor.request0_pose_cache.reset();
    actor.request0_sequence.reset();
    actor.animation_bank.reset();
    actor.animation_cursor.reset();
}

void bind_imported_actor_animation_banks(ActorAreaRuntime& area, const EbdDirectory& ebd,
    const uint8_t* requests, size_t request_count, size_t cap) {
    if (area.decoded_resource_bytes > cap || area.actors.size() > 16)
        throw std::runtime_error("actor animation bank budget/capacity inconsistent");
    using Bank = std::shared_ptr<const ActorAnimationBank>;
    std::vector<std::pair<uint32_t, Bank>> shared;
    std::array<Bank, 16> pending{};
    size_t added = 0;
    for (size_t i = 0; i < area.actors.size(); ++i) {
        const auto& actor = area.actors[i];
        if (!actor.source_behavior || !actor.pose_selected_by_retail_state) continue;
        if (!actor.resource || !actor.pose || actor.pose->record_index != 0 ||
            actor.animation_bank || actor.request0_pose_cache)
            throw std::runtime_error("actor bank requires fresh initializer presentation");
        Bank bank;
        for (const auto& item : shared) if (item.first == actor.row) bank = item.second;
        if (!bank) {
            auto decoded = decode_ebd_actor_animation_bank(ebd, actor.row, *actor.resource,
                requests, request_count, cap - area.decoded_resource_bytes - added);
            if (decoded.decoded_bytes > cap - area.decoded_resource_bytes - added)
                throw std::runtime_error("actor animation bank decoded budget exceeded");
            added += decoded.decoded_bytes;
            bank = std::make_shared<const ActorAnimationBank>(std::move(decoded));
            shared.emplace_back(actor.row, bank);
        }
        if (actor_animation_request_ready(*bank, actor.pose->request, *actor.pose) !=
            ActorAnimationStatus::SourceBackedPartial)
            throw std::runtime_error("actor bank lacks current initializer request");
        pending[i] = bank;
    }
    for (size_t i = 0; i < area.actors.size(); ++i) if (pending[i]) {
        auto& actor = area.actors[i];
        const auto* cache = find_actor_animation_request(*pending[i], actor.pose->request);
        actor.animation_cursor = actor_sequence_begin(0, cache->sequence.records[0]);
        actor.animation_bank = std::move(pending[i]);
    }
    area.decoded_resource_bytes += added;
    area.decoded_pose_cache_bytes += added;
    area.unique_pose_cache_count += shared.size();
}

ImportedActorAction2Preparation prepare_imported_actor_action2_contact(
    const ImportedActorInstance& actor,const RuntimeMap& map,size_t area,
    const ReferenceTables& refs) noexcept {
    ImportedActorAction2Preparation out;
    if(!actor.source_behavior || !actor.source_behavior_initialized || !actor.floor_bounds ||
        !actor.animation_cursor || actor.source_class!=0x20 || actor.subtype!=3 ||
        actor.source_behavior->behavior.action_09!=2 || actor.source_behavior->behavior.phase_0a>1)
        return out;
    const auto& state=*actor.source_behavior;
    out.initial_heading=state.behavior.heading_56;out.initial_turn=state.behavior.turn_66;
    out.before.raw=state.raw_14;out.before.fixed=state.fixed_1c;
    out.before.previous_raw=state.raw_14;out.before.previous_fixed=state.fixed_1c;
    out.before.velocity=actor.source_contact_velocity;out.before.state74=actor.source_contact_state74;
    out.before.object_class=actor.source_class;
    out.after_wall=out.before;
    const uint16_t heading=static_cast<uint16_t>((state.behavior.heading_56+state.behavior.turn_66)&4095u);
    const auto delta=reference_player_motion_delta(refs,heading,52,0,0);
    const std::array<int32_t,3> offsets{{delta.dx_fixed,delta.dy_fixed,delta.dz_fixed}};
    for(size_t axis=0;axis<3;++axis) {
        const uint32_t value=static_cast<uint32_t>(out.after_wall.fixed[axis])+static_cast<uint32_t>(offsets[axis]);
        out.after_wall.fixed[axis]=value<=INT32_MAX?static_cast<int32_t>(value)
            :static_cast<int32_t>(int64_t(value)-0x100000000LL);
        out.after_wall.raw[axis]=reference_fixed_to_short_80030150(out.after_wall.fixed[axis]);
    }
    out.wall=query_actor_wall_contact_8004b6ac(map,area,out.after_wall,*actor.floor_bounds);
    out.status=out.wall.status;
    if(!apply_actor_contact_result_8004b414(out.after_wall,out.wall))return out;
    out.floor=query_actor_floor_contact_80048ea4(map,area,out.after_wall,*actor.floor_bounds);
    out.status=out.floor.status;
    if(out.floor.status!=RuntimeContactStatus::SourceBackedPartial)return out;
    Class20_03PreparedAction2 input;
    input.raw_14=out.after_wall.raw;input.fixed_1c=out.after_wall.fixed;
    input.source_floor_word=out.floor.result;
    input.source_contact_word=static_cast<uint32_t>(out.wall.result);
    const uint8_t cue=actor.animation_cursor->opaque_a6;
    input.source_a6=cue<128?static_cast<int8_t>(cue):static_cast<int8_t>(int(cue)-256);
    out.inputs=input;
    return out;
}

namespace {
ActorDetectionWork detector_work(const Class20_03CallbackState& state) noexcept {
    return {state.detection_origin_34c,state.detection_profile_352.value_or(0),
        state.detection_phase_353,state.detection_vector_354,
        state.detection_countdown_35a,state.detection_offset_35b};
}
bool same_detector_work(const ActorDetectionWork& a,const ActorDetectionWork& b) noexcept {
    return a.origin_34c==b.origin_34c && a.profile_352==b.profile_352 &&
        a.phase_353==b.phase_353 && a.vector_354==b.vector_354 &&
        a.countdown_35a==b.countdown_35a && a.offset_35b==b.offset_35b;
}
}
ImportedActorDetectionPreparation prepare_imported_actor_detection(
    const ImportedActorInstance& actor,const ActorDetectionInputs& input) noexcept {
    ImportedActorDetectionPreparation out;
    if(!actor.source_behavior || !actor.source_behavior_initialized ||
        !actor.source_behavior->detection_profile_352 || actor.source_class!=0x20 || actor.subtype!=3)
        return out;
    const auto& state=*actor.source_behavior;
    out.before=detector_work(state);out.after=out.before;
    out.actor_raw={state.raw_14[0],state.raw_14[1],state.raw_14[2]};
    out.heading=state.behavior.heading_56;out.work80=input.distance_work80;
    if(input.actor_raw!=out.actor_raw || input.heading56!=out.heading)return out;
    out.result=step_actor_detection_800364b0(out.after,input);
    return out;
}

ActorAction3ContactPreparation prepare_imported_actor_action3_contact(
    const ImportedActorInstance& actor,const RuntimeMap& map,size_t area,
    const ReferenceTables& refs,std::optional<uint16_t> target) noexcept {
    if(!actor.source_behavior || !actor.source_behavior_initialized || !actor.floor_bounds ||
       actor.source_class!=0x20 || actor.subtype!=3)return {};
    auto state=*actor.source_behavior;
    state.previous_raw_2c=state.raw_14;state.previous_fixed_34=state.fixed_1c;
    ActorContactWork work;
    work.raw=state.raw_14;work.fixed=state.fixed_1c;
    work.previous_raw=state.previous_raw_2c;work.previous_fixed=state.previous_fixed_34;
    work.velocity=actor.source_contact_velocity;work.state74=actor.source_contact_state74;
    work.object_class=actor.source_class;
    return prepare_actor_action3_contact(state,work,map,area,refs,*actor.floor_bounds,target);
}

bool bind_imported_actor_combat_profile(ImportedActorInstance& actor,
    const SourceCombatProfiles& profiles,std::optional<uint8_t> selector) noexcept {
    if(!selector || !actor.source_behavior || !actor.source_behavior_initialized ||
       actor.initializer!=0x80102410)return false;
    auto& state=*actor.source_behavior;
    // Binding belongs to fresh construction. A repeated bind must never heal
    // a live actor or replace an already selected source profile.
    if(state.health_6e || actor.source_combat_byte_a)return false;
    const auto* profile=find_source_combat_profile(profiles,*selector,actor.source_class,
        actor.subtype,state.flags_0c,actor.source_context_high_byte);
    if(!profile)return false;
    // The supported original initializer installs these three copies together.
    state.health_6e=profile->initial_hp();state.initial_health_b0=state.health_6e;
    state.previous_health_b2=state.health_6e;
    state.detection_profile_352=profile->detection_profile();
    actor.source_combat_byte_a=profile->unsigned_byte0a;
    // The retained byteC belongs to the base selected record. Original action4
    // adds signed variant32E*16; other variants need their own retained record.
    if(state.variant_32e==0){actor.source_combat_byte_c=profile->unsigned_byte0c;
        actor.source_combat_byte09=profile->unsigned_byte09;
        actor.source_reward_word00=profile->reward_word00;}
    return true;
}

ActorAction4ContactPreparation prepare_imported_actor_action4_contact(
    const ImportedActorInstance& actor,const RuntimeMap& map,size_t area,
    const ReferenceTables& refs) noexcept {
    if(!actor.source_behavior || !actor.source_behavior_initialized || !actor.floor_bounds ||
       actor.source_class!=0x20 || actor.subtype!=3)return {};
    auto state=*actor.source_behavior;
    state.previous_raw_2c=state.raw_14;state.previous_fixed_34=state.fixed_1c;
    ActorContactWork work;
    work.raw=state.raw_14;work.fixed=state.fixed_1c;
    work.previous_raw=state.raw_14;work.previous_fixed=state.fixed_1c;
    work.velocity=actor.source_contact_velocity;work.state74=actor.source_contact_state74;
    work.object_class=actor.source_class;
    return prepare_actor_action4_contact(state,work,map,area,refs,*actor.floor_bounds);
}

ActorAction9ContactPreparation prepare_imported_actor_action9_contact(
    const ImportedActorInstance& actor,const RuntimeMap& map,size_t area,
    const ReferenceTables& refs,std::optional<uint32_t> work80,
    std::optional<uint16_t> target,std::optional<bool> detected) noexcept {
    if(!actor.source_behavior || !actor.source_behavior_initialized || !actor.floor_bounds ||
       actor.source_class!=0x20 || actor.subtype!=3)return {};
    auto state=*actor.source_behavior;
    if(!(state.behavior.parameter_03&0x20u) && !detected)return {};
    const bool active=!(state.behavior.parameter_03&0x20u) && *detected;
    state.behavior.flags_328=static_cast<uint16_t>((state.behavior.flags_328&0xffefu)|(active?0x10u:0u));
    state.previous_raw_2c=state.raw_14;state.previous_fixed_34=state.fixed_1c;
    ActorContactWork work;
    work.raw=state.raw_14;work.fixed=state.fixed_1c;
    work.previous_raw=state.raw_14;work.previous_fixed=state.fixed_1c;
    work.velocity=actor.source_contact_velocity;work.state74=actor.source_contact_state74;
    work.object_class=actor.source_class;
    std::optional<int8_t> cue;
    if(actor.animation_cursor) {
        const auto raw=actor.animation_cursor->opaque_a6;
        cue=raw<128?static_cast<int8_t>(raw):static_cast<int8_t>(int(raw)-256);
    }
    return prepare_actor_action9_contact(state,work,map,area,refs,*actor.floor_bounds,work80,target,cue);
}

ActorReactionContactPreparation prepare_imported_actor_reaction_contact(
    const ImportedActorInstance& actor,const RuntimeMap& map,size_t area,const ReferenceTables& refs,
    const SourceReactionProfile* profile,const SourceAngleSteps* angle_steps,std::optional<uint16_t> target,
    const SourceAudioQueue* audio,std::optional<std::array<int16_t,3>> relative_origin) noexcept {
    ActorReactionContactPreparation held;
    if(!actor.source_behavior||!actor.source_behavior_initialized||!actor.floor_bounds||
       actor.source_class!=0x20||actor.subtype!=3)return held;
    auto state=*actor.source_behavior;
    if(actor.source_proxy_bound){
        if(packet_alias_status(actor)!=ImportedActorPacketStatus::SourceBackedPartial){
            held.status=ActorReactionContactStatus::PlanHeld;return held;
        }
        copy_packet_alias(state,actor.source_proxy_target);
    }
    state.previous_raw_2c=state.raw_14;state.previous_fixed_34=state.fixed_1c;
    state.velocity_44=actor.source_contact_velocity;
    held.incoming_packet=state.packet_33c;
    std::optional<Class20_03HitStatus> hit_status;
    if(state.packet_33c&&(state.behavior.flags_328&2u)){
        Class20_03HitInputs hit;hit.packet_33c=state.packet_33c;hit.hit_word_34a=state.hit_word_34a;
        if(state.variant_32e==0&&actor.source_combat_byte09)
            hit.selected_record=Class20_03HitRecordByte09{0,*actor.source_combat_byte09};
        const auto origin=(state.hit_word_34a&1u)
            ?std::optional<std::array<int16_t,3>>({{state.raw_14[0],state.raw_14[1],state.raw_14[2]}}):relative_origin;
        if(origin)hit.angle_80031198=source_heading_80031198((*origin)[0],(*origin)[2],
            state.hit_point_344[0],state.hit_point_344[2],angle_steps);
        hit.sound_consumer_ready=audio&&audio->initialized&&audio->count<=audio->records.size();
        const auto consumed=class20_03_consume_hit_801041b0(state,hit);hit_status=consumed.status;
        if(consumed.status!=Class20_03HitStatus::SourceBackedPartial){
            held.status=ActorReactionContactStatus::PlanHeld;held.staged_hit_status=hit_status;return held;
        }
    }
    Class20_03ReactionInputs in;in.profile=profile;in.target_heading_80031198=target;
    if(state.variant_32e==0&&actor.source_combat_byte_b){in.combat_byte_b=actor.source_combat_byte_b;in.combat_record_variant=0;}
    ActorContactWork work;work.raw=state.raw_14;work.fixed=state.fixed_1c;
    work.previous_raw=state.previous_raw_2c;work.previous_fixed=state.previous_fixed_34;
    work.velocity=state.velocity_44;work.state74=actor.source_contact_state74;work.object_class=actor.source_class;
    auto out=prepare_actor_reaction_contact(state,in,work,map,area,refs,*actor.floor_bounds);
    out.incoming_packet=state.packet_33c;out.staged_hit_status=hit_status;return out;
}

ImportedActorBehaviorTick tick_imported_actor_state1(ImportedActorInstance& actor,
    const Class20_03CallbackInputs& inputs,
    const ImportedActorAction2Preparation* movement,
    const ImportedActorDetectionPreparation* detection,SourceAudioQueue* audio,
    const ActorAction3ContactPreparation* action3,
    const ActorAction9ContactPreparation* action9,
    const ActorAction4ContactPreparation* action4,
    ChildRuntime* children,const ReferenceTables* child_references,
    const ActorReactionContactPreparation* reaction,
    const ImportedActorProxyTailContext* proxy_tail) noexcept {
    ImportedActorBehaviorTick result;
    actor.last_source_audio_queued=0;actor.last_source_audio_dropped=0;
    if(actor.source_released)return result;
    if (actor.source_class != 0x20 || actor.subtype != 3 || actor.initializer != 0x80102410)
        return result;
    if (!actor.source_behavior || !actor.source_behavior_initialized) {
        result.status = Class20_03CallbackStatus::NeedsFloor;
        return result;
    }
    if ((actor.source_dispatch_flag0 & actor.source_dispatch_flag6 & 1u) == 0) return result;
    if(actor.source_proxy_bound){
        const auto packet_status=proxy_tail?packet_alias_status(actor):stage_imported_actor_proxy_packet(actor);
        if(packet_status!=ImportedActorPacketStatus::SourceBackedPartial){
            result.status=Class20_03CallbackStatus::NeedsPacket;actor.last_behavior_status=result.status;return result;
        }
    }
    auto next = *actor.source_behavior;
    if(proxy_tail&&actor.source_proxy_bound)copy_packet_alias(next,actor.source_proxy_target);
    next.velocity_44=actor.source_contact_velocity;
    // Supported outer snapshot stores. The shared BIOS random draw/effect/proxy
    // owners remain excluded; these copies make the contact rollback current.
    next.previous_animation_ad=next.behavior.animation_ac;
    next.previous_raw_2c=next.raw_14;next.previous_fixed_34=next.fixed_1c;
    next.angle_54[1]=next.behavior.heading_56;next.previous_angle_5c=next.angle_54;
    next.work_15e=16912;
    auto source=inputs;
    // Runtime detection must come from its staged source preparation. Authored
    // scalar premises belong to the reducer tests, not the live actor bridge.
    source.detection_8003648c.reset();source.detection_range_byte2.reset();
    source.prepared_action3.reset();
    source.prepared_action9.reset();
    source.prepared_action4.reset();
    source.prepared_reaction.reset();
    source.produced_child_374.reset();
    source.defeat_consumer_ready=false;source.defeat_effect_identity=0;
    source.combat_record_byte_c=next.variant_32e==0?actor.source_combat_byte_c:std::nullopt;
    source.combat_record_byte09=next.variant_32e==0?actor.source_combat_byte09:std::nullopt;
    source.hit_record_variant_32e=source.combat_record_byte09?std::optional<int16_t>(0):std::nullopt;
    source.reaction_combat_byte_b=next.variant_32e==0?actor.source_combat_byte_b:std::nullopt;
    source.reaction_combat_record_variant=source.reaction_combat_byte_b?std::optional<int16_t>(0):std::nullopt;
    if(actor.source_proxy_bound){source.action.packet_33c=next.packet_33c;source.hit_word_34a=next.hit_word_34a;}
    std::optional<SourceAudioQueue> staged_audio;
    if(audio && audio->initialized && audio->count<=kSourceAudioQueueCapacity)staged_audio=*audio;
    source.sound_consumer_ready=staged_audio.has_value();
    uint8_t queued=0,dropped=0;
    const auto produced=[&](SourceAudioQueueStatus status) noexcept {
        if(status==SourceAudioQueueStatus::Queued){++queued;return true;}
        if(status==SourceAudioQueueStatus::DroppedFull){++dropped;return true;}
        return false;
    };
    if(detection && !(next.behavior.parameter_03&0x20u)) {
        actor.last_detection_status=detection->result.status;
        const std::array<int16_t,3> raw{{next.raw_14[0],next.raw_14[1],next.raw_14[2]}};
        if(!next.detection_profile_352 || !source.action.work_80 ||
            !same_detector_work(detection->before,detector_work(next)) ||
            detection->actor_raw!=raw || detection->heading!=next.behavior.heading_56 ||
            detection->work80!=*source.action.work_80 ||
            detection->result.status!=ActorDetectionStatus::SourceBackedPartial) {
            result.status=Class20_03CallbackStatus::NeedsDetection;
            actor.last_behavior_status=result.status;return result;
        }
        if(detection->result.sound156_intent) {
            if(!staged_audio || !produced(enqueue_source_audio_8001a008(*staged_audio,156,0,0))) {
                result.status=Class20_03CallbackStatus::NeedsSoundConsumer;
                actor.last_behavior_status=result.status;return result;
            }
        }
        next.detection_origin_34c=detection->after.origin_34c;
        next.detection_profile_352=detection->after.profile_352;
        next.detection_phase_353=detection->after.phase_353;
        next.detection_vector_354=detection->after.vector_354;
        next.detection_countdown_35a=detection->after.countdown_35a;
        next.detection_offset_35b=detection->after.offset_35b;
        source.detection_8003648c=detection->result.detected;
    }
    if(next.behavior.action_09==2 && next.behavior.phase_0a<=1) {
        if(!movement||!movement->inputs||movement->status!=RuntimeContactStatus::SourceBackedPartial||
            movement->before.raw!=next.raw_14||movement->before.fixed!=next.fixed_1c||
            movement->before.previous_raw!=next.previous_raw_2c||movement->before.previous_fixed!=next.previous_fixed_34||
            movement->before.object_class!=actor.source_class||!actor.animation_cursor||
            !movement->inputs->source_a6||static_cast<uint8_t>(*movement->inputs->source_a6)!=actor.animation_cursor->opaque_a6||
            movement->initial_heading!=next.behavior.heading_56||movement->initial_turn!=next.behavior.turn_66||
            movement->before.velocity!=actor.source_contact_velocity||movement->before.state74!=actor.source_contact_state74) {
            result.status=Class20_03CallbackStatus::NeedsAction2Motion;
            actor.last_behavior_status=result.status;return result;
        }
        source.prepared_action2=movement->inputs;
    }
    if(next.behavior.action_09==3 && action3 && action3->inputs) {
        if(action3->status!=ActorAction3ContactStatus::SourceBackedPartial ||
           action3->before.raw!=next.raw_14 || action3->before.fixed!=next.fixed_1c ||
           action3->before.previous_raw!=next.previous_raw_2c || action3->before.previous_fixed!=next.previous_fixed_34 ||
           action3->before.object_class!=actor.source_class ||
           action3->before.velocity!=actor.source_contact_velocity ||
           action3->before.state74!=actor.source_contact_state74 ||
           action3->initial_action!=next.behavior.action_09 ||
           action3->initial_phase!=next.behavior.phase_0a ||
           action3->initial_timer!=next.behavior.timer_32c ||
           action3->initial_work64!=next.work_64 || action3->initial_packet!=next.packet_33c ||
           action3->initial_heading!=next.behavior.heading_56 ||
           action3->target_heading!=source.action.target_heading_80031198) {
            result.status=Class20_03CallbackStatus::NeedsAction3Motion;
            actor.last_behavior_status=result.status;return result;
        }
        const auto& p=*action3->inputs;
        source.prepared_action3=Class20_03CallbackInputs::Action3Motion{
            p.raw_14,p.fixed_1c,p.source_floor_word,p.source_contact_word,p.heading_after_turn};
    }
    if(next.behavior.action_09==9 && action9 && action9->inputs) {
        const uint16_t effective_flags=static_cast<uint16_t>((next.behavior.flags_328&0xffefu)|
            (!(next.behavior.parameter_03&0x20u) && source.detection_8003648c.value_or(false)?0x10u:0u));
        if(action9->status!=ActorAction9ContactStatus::SourceBackedPartial ||
           action9->before.raw!=next.raw_14 || action9->before.fixed!=next.fixed_1c ||
           action9->before.previous_raw!=next.previous_raw_2c || action9->before.previous_fixed!=next.previous_fixed_34 ||
           action9->before.object_class!=actor.source_class ||
           action9->before.velocity!=actor.source_contact_velocity || action9->before.state74!=actor.source_contact_state74 ||
           action9->initial_action!=next.behavior.action_09 || action9->initial_phase!=next.behavior.phase_0a ||
           action9->initial_flags!=effective_flags || action9->initial_heading!=next.behavior.heading_56 ||
           action9->work80!=source.action.work_80 || action9->turn_target!=source.action.target_heading_80031198 ||
           action9->inputs->raw_14!=action9->after_wall.raw || action9->inputs->fixed_1c!=action9->after_wall.fixed ||
           action9->inputs->source_floor_word!=action9->floor.result ||
           action9->inputs->source_contact_word!=static_cast<uint32_t>(action9->wall.result) ||
           !actor.animation_cursor || !action9->source_a6 ||
           static_cast<uint8_t>(*action9->source_a6)!=actor.animation_cursor->opaque_a6 ||
           action9->inputs->source_a6!=action9->source_a6) {
            result.status=Class20_03CallbackStatus::NeedsAction9Motion;
            actor.last_behavior_status=result.status;return result;
        }
        const auto& p=*action9->inputs;
        source.prepared_action9=Class20_03CallbackInputs::Action9Motion{
            p.raw_14,p.fixed_1c,p.source_floor_word,p.source_contact_word,p.heading_after_turn,p.source_a6};
    }
    if(next.behavior.action_09==4 && action4 && action4->inputs) {
        const auto plan=class20_03_plan_action4(next);
        const auto& p=*action4->inputs;
        if(action4->status!=ActorAction4ContactStatus::SourceBackedPartial ||
           action4->wall.status!=RuntimeContactStatus::SourceBackedPartial ||
           action4->floor.status!=RuntimeContactStatus::SourceBackedPartial ||
           action4->before.raw!=next.raw_14 || action4->before.fixed!=next.fixed_1c ||
           action4->before.previous_raw!=next.previous_raw_2c || action4->before.previous_fixed!=next.previous_fixed_34 ||
           action4->before.object_class!=actor.source_class ||
           action4->before.velocity!=actor.source_contact_velocity || action4->before.state74!=actor.source_contact_state74 ||
           action4->initial_action!=next.behavior.action_09 || action4->initial_phase!=next.behavior.phase_0a ||
           action4->initial_request!=next.behavior.animation_ac || action4->initial_heading!=next.behavior.heading_56 ||
           action4->initial_timer!=next.behavior.timer_32c || action4->initial_variant!=next.variant_32e ||
           !plan.forward || p.source_forward!=*plan.forward || p.heading!=next.behavior.heading_56 ||
           p.raw_14!=action4->after_wall.raw || p.fixed_1c!=action4->after_wall.fixed ||
           action4->after_wall.previous_raw!=action4->before.previous_raw ||
           action4->after_wall.previous_fixed!=action4->before.previous_fixed ||
           action4->after_wall.object_class!=action4->before.object_class ||
           p.source_floor_word!=action4->floor.result) {
            result.status=Class20_03CallbackStatus::NeedsAction4Motion;
            actor.last_behavior_status=result.status;return result;
        }
        source.prepared_action4=Class20_03CallbackInputs::Action4Motion{
            p.raw_14,p.fixed_1c,p.source_floor_word,p.source_forward,p.heading};
    }
    // A packet may enter the body only when the exact later packet-clear
    // consumer is part of this same transaction. The independent diagnostic
    // tail must never clear a body that held (or repeat already-applied HP).
    if(!source.action.packet_33c || ((*source.action.packet_33c||next.packet_33c)&&!proxy_tail)){
        result.status=Class20_03CallbackStatus::NeedsPacket;
        actor.last_behavior_status=result.status;return result;
    }
    const bool pending_packet=*source.action.packet_33c||next.packet_33c;
    if(pending_packet&&(!actor.source_proxy_bound||!proxy_tail->bank||!proxy_tail->references)){
        result.status=Class20_03CallbackStatus::NeedsProxyConsumer;
        actor.last_behavior_status=result.status;return result;
    }
    if(pending_packet&&(next.work_00&2u)&&!proxy_tail->shadows){
        result.status=Class20_03CallbackStatus::NeedsEffectConsumer;
        actor.last_behavior_status=result.status;return result;
    }
    if((next.behavior.action_09==7||next.behavior.action_09==8)&&reaction&&reaction->inputs){
        const auto& r=*reaction;const auto& p=*r.inputs;
        if(r.status!=ActorReactionContactStatus::SourceBackedPartial||r.incoming_packet!=next.packet_33c||
           r.before.raw!=next.raw_14||r.before.fixed!=next.fixed_1c||r.before.previous_raw!=next.previous_raw_2c||
           r.before.previous_fixed!=next.previous_fixed_34||r.before.velocity!=next.velocity_44||
           r.before.state74!=actor.source_contact_state74||r.before.object_class!=actor.source_class||
           r.initial_action!=next.behavior.action_09||r.initial_phase!=next.behavior.phase_0a||
           r.initial_aux!=next.aux_0b||r.initial_parameter!=next.behavior.parameter_03||
           r.initial_timer!=next.behavior.timer_32c||r.initial_heading!=next.behavior.heading_56||
           r.initial_table!=next.reaction_table_364||p.raw_14!=r.after_wall.raw||p.fixed_1c!=r.after_wall.fixed||
           p.velocity_after_wall!=r.after_wall.velocity||p.contact_word!=uint32_t(r.wall.result)){
            result.status=Class20_03CallbackStatus::NeedsReactionMotion;actor.last_behavior_status=result.status;return result;
        }
        source.prepared_reaction=r.inputs;
    }
    ParentChildDelivery child_delivery;
    if(children && child_references && staged_audio) {
        const auto status=children->prepare_parent_delivery(next,*child_references,*staged_audio,child_delivery);
        if(status==ChildRuntimeStatus::SourceBackedPartial) {
            if(!children->validate_parent_delivery(child_delivery,next,*staged_audio)) {
                result.status=Class20_03CallbackStatus::NeedsChildConsumer;
                actor.last_behavior_status=result.status;return result;
            }
            source.produced_child_374=child_delivery.source_identity();
            *staged_audio=child_delivery.staged_audio();
            queued=uint8_t(queued+child_delivery.audio_queued());
            dropped=uint8_t(dropped+child_delivery.audio_dropped());
        } else if(status!=ChildRuntimeStatus::NotSpawnBranch) {
            result.status=Class20_03CallbackStatus::NeedsChildConsumer;
            actor.last_behavior_status=result.status;return result;
        }
    }
    EnemyDefeatDelivery defeat_delivery;
    const bool defeat_due=(next.behavior.flags_328&0x40u)&&next.defeat_timer_360<=0;
    if(defeat_due){
        if(!children||!staged_audio){
            result.status=Class20_03CallbackStatus::NeedsDefeatLifecycle;
            actor.last_behavior_status=result.status;return result;
        }
        const auto status=children->prepare_enemy_defeat_delivery(next.proxy_position_180,
            actor.source_reward_word00,*staged_audio,defeat_delivery);
        if(status!=ChildRuntimeStatus::SourceBackedPartial||
           !children->validate_enemy_defeat_delivery(defeat_delivery,*staged_audio)){
            result.status=status==ChildRuntimeStatus::NeedsSoundConsumer?
                Class20_03CallbackStatus::NeedsSoundConsumer:
                status==ChildRuntimeStatus::EffectHeld?Class20_03CallbackStatus::NeedsEffectConsumer:
                Class20_03CallbackStatus::NeedsDefeatLifecycle;
            actor.last_behavior_status=result.status;return result;
        }
        source.defeat_consumer_ready=true;
        source.defeat_effect_identity=defeat_delivery.effect_identity();
        *staged_audio=defeat_delivery.staged_audio();
        queued=uint8_t(queued+defeat_delivery.audio_queued());
        dropped=uint8_t(dropped+defeat_delivery.audio_dropped());
    }
    const auto body = class20_03_step_state1(next, source);
    result.status = body.status;
    if (body.status != Class20_03CallbackStatus::SourceBackedPartial) {
        actor.last_behavior_status = result.status;
        return result;
    }
    if(body.defeat_801027cc!=defeat_delivery.prepared()){
        result.status=Class20_03CallbackStatus::NeedsDefeatLifecycle;
        actor.last_behavior_status=result.status;return result;
    }
    if (body.sound_8001a050 || body.secondary_sound_8001a050) {
        const auto words=actor_source_sound_position_words(actor.source_behavior->raw_14);
        const auto emit=[&](std::optional<uint16_t> id,
                           const std::optional<std::array<uint32_t,2>>& pointed) noexcept {
            return !id || (staged_audio && produced(enqueue_source_audio_8001a050(*staged_audio,*id,pointed.value_or(words))));
        };
        if(!emit(body.sound_8001a050,body.sound_pointed_words) ||
           !emit(body.secondary_sound_8001a050,body.secondary_sound_pointed_words)) {
            result.status=Class20_03CallbackStatus::NeedsSoundConsumer;
            actor.last_behavior_status=result.status;return result;
        }
    }
    if (!actor.animation_bank || !actor.animation_cursor || !actor.pose) {
        result.status = Class20_03CallbackStatus::NeedsAnimationConsumer;
        actor.last_behavior_status = result.status;
        return result;
    }
    result.animation_status = actor_animation_request_ready(*actor.animation_bank,
        next.behavior.animation_ac, *actor.pose);
    if (result.animation_status != ActorAnimationStatus::SourceBackedPartial) {
        result.status = Class20_03CallbackStatus::NeedsAnimationConsumer;
        actor.last_behavior_status = result.status;
        return result;
    }
    // Preserve legacy zero-packet callers without a bound shadow owner. A
    // nonzero packet cannot use that partial path or clear before this tail.
    const bool atomic_tail=proxy_tail&&(!(next.work_00&2u)||proxy_tail->shadows);
    Class20_03ProxyInputs proxy_input;
    SourceShadowPlan shadow_plan;
    if(atomic_tail){
        if(!actor.source_proxy_bound||!proxy_tail->bank||!proxy_tail->references){
            result.status=Class20_03CallbackStatus::NeedsProxyConsumer;
            actor.last_behavior_status=result.status;return result;
        }
        if(next.work_00&2u){
            // Exact class03 call80102368 supplies48,512,0. Full source queue
            // returns before reading work76 or asking80048E6C for a floor.
            SourceShadowWork shadow_work;
            if(proxy_tail->shadows->count<proxy_tail->shadows->records.size()){
                if(!actor.source_shadow_work76){
                    result.status=Class20_03CallbackStatus::NeedsEffectConsumer;
                    actor.last_behavior_status=result.status;return result;
                }
                for(size_t axis=0;axis<3;++axis)shadow_work.raw[axis]=next.raw_14[axis];
                shadow_work.heading56=next.behavior.heading_56;shadow_work.work76=*actor.source_shadow_work76;
            }
            std::optional<int32_t> shadow_floor;
            if(proxy_tail->shadows->count<proxy_tail->shadows->records.size()&&shadow_work.work76==0){
                if(!proxy_tail->map||!actor.floor_bounds){
                    result.status=Class20_03CallbackStatus::NeedsEffectConsumer;
                    actor.last_behavior_status=result.status;return result;
                }
                ActorContactWork floor_work;floor_work.raw=next.raw_14;floor_work.previous_raw=next.previous_raw_2c;
                floor_work.fixed=next.fixed_1c;floor_work.previous_fixed=next.previous_fixed_34;
                floor_work.velocity=next.velocity_44;floor_work.state74=actor.source_contact_state74;
                floor_work.object_class=uint8_t(0x20u|uint8_t(actor.original_pool_slot));
                const auto floor=query_actor_floor_contact_80048ea4(*proxy_tail->map,proxy_tail->area_index,
                    floor_work,*actor.floor_bounds);
                if(floor.status==RuntimeContactStatus::SourceBackedPartial)shadow_floor=floor.result;
            }
            shadow_plan=plan_source_shadow_8002c8cc(proxy_tail->shadows->count,shadow_work,48,512,0,shadow_floor);
            result.shadow_status=shadow_plan.status;
            if(shadow_plan.status!=SourceConsumerStatus::SourceBackedPartial){
                result.status=Class20_03CallbackStatus::NeedsEffectConsumer;
                actor.last_behavior_status=result.status;return result;
            }
        }
        const auto preview=actor_animation_preview(*actor.animation_bank,next.behavior.animation_ac,
            *actor.pose,*actor.animation_cursor);
        if(preview.result.status!=ActorAnimationStatus::SourceBackedPartial||!preview.pose){
            result.animation_status=preview.result.status;
            result.status=Class20_03CallbackStatus::NeedsAnimationConsumer;
            actor.last_behavior_status=result.status;return result;
        }
        if((next.work_00&1u)&&(next.behavior.flags_328&4u)){
            ActorPoseCollisionInputs pose_input;pose_input.reference=proxy_tail->references;
            pose_input.heading_56=next.behavior.heading_56;
            for(size_t axis=0;axis<3;++axis)pose_input.raw_14[axis]=next.raw_14[axis];
            pose_input.scale_158=actor.raw_scale;pose_input.root_translation=preview.pose->root_translation;
            pose_input.flags_06=actor.source_dispatch_flag6;
            pose_input.scratch_shift_1f800074=actor.source_pose_scratch_shift;
            pose_input.preserved_186=actor.source_pose_preserved_186;
            proxy_input.pose_point_180=produce_actor_pose_collision_point_8003346c(pose_input).point_180;
        }
        proxy_input.source_work_01=uint8_t(0x20u|uint8_t(actor.original_pool_slot));
        proxy_input.producer_330=uint16_t(next.behavior.work_330);
        proxy_input.target_33c=&actor.source_proxy_target;
        proxy_input.mask_80098798=proxy_tail->mask_80098798;
        proxy_input.owners_800be478=proxy_tail->owners_800be478;
        const auto checked=class20_03_check_proxy_80102398(next,*proxy_tail->bank,proxy_input);
        result.proxy_status=checked.status;
        if(checked.status!=Class20_03ProxyStatus::SourceBackedPartial){
            result.status=Class20_03CallbackStatus::NeedsProxyConsumer;
            actor.last_behavior_status=result.status;return result;
        }
    }
    auto cursor = *actor.animation_cursor;
    const auto animation = actor.pose->request == next.behavior.animation_ac
        ? actor_animation_step(*actor.animation_bank, *actor.pose, cursor)
        : actor_animation_reset(*actor.animation_bank, next.behavior.animation_ac, *actor.pose, cursor);
    result.animation_status = animation.status;
    if (animation.status != ActorAnimationStatus::SourceBackedPartial) {
        result.status = Class20_03CallbackStatus::NeedsAnimationConsumer;
        actor.last_behavior_status = result.status;
        return result;
    }
    // The fixed pool cannot change inside the synchronous body/pose consumers.
    // Publish the checked pool receipt only after every parent consumer passed.
    if(child_delivery.prepared() && !children->commit_parent_delivery(child_delivery)) {
        result.status=Class20_03CallbackStatus::NeedsChildConsumer;
        actor.last_behavior_status=result.status;return result;
    }
    if(defeat_delivery.prepared()&&!children->commit_enemy_defeat_delivery(defeat_delivery)){
        result.status=Class20_03CallbackStatus::NeedsDefeatLifecycle;
        actor.last_behavior_status=result.status;return result;
    }
    if(atomic_tail){
        // No owner changes between the read-only tail check and publication.
        // The existing exact finalizer publishes the point, leased BC/C0 and
        // packet clear only after body, audio and selected pose are admitted.
        if(shadow_plan.append){
            proxy_tail->shadows->records[proxy_tail->shadows->count++]=*shadow_plan.append;
            result.shadow_appended=true;
        }
        const auto finalized=class20_03_finalize_proxy_80102398(next,*proxy_tail->bank,proxy_input);
        result.proxy_finalized=true;result.proxy_registered=finalized.registered;
        result.proxy_bank_full=finalized.bank_full;result.proxy_status=finalized.status;
        copy_packet_alias(next,actor.source_proxy_target);
    }
    if(movement && source.prepared_action2 && actor.source_behavior->behavior.action_09==2 && actor.source_behavior->behavior.phase_0a<=1) {
        actor.source_contact_velocity=movement->after_wall.velocity;
        actor.source_contact_state74=movement->after_wall.state74;
    }
    if(action3 && source.prepared_action3) {
        actor.source_contact_velocity=action3->after_wall.velocity;
        actor.source_contact_state74=action3->after_wall.state74;
    }
    if(action9 && source.prepared_action9) {
        actor.source_contact_velocity=action9->after_wall.velocity;
        actor.source_contact_state74=action9->after_wall.state74;
    }
    if(action4 && source.prepared_action4) {
        actor.source_contact_velocity=action4->after_wall.velocity;
        actor.source_contact_state74=action4->after_wall.state74;
    }
    if(source.prepared_reaction&&reaction){
        actor.source_contact_velocity=next.velocity_44;
        actor.source_contact_state74=reaction->after_wall.state74;
    }
    if(staged_audio){*audio=*staged_audio;result.source_audio_queued=queued;result.source_audio_dropped=dropped;
        actor.last_source_audio_queued=queued;actor.last_source_audio_dropped=dropped;}
    actor.source_behavior = next;
    actor.animation_cursor = cursor;
    actor.heading = next.behavior.heading_56;
    for (size_t i = 0; i < actor.source_origin.size(); ++i) actor.source_origin[i] = next.raw_14[i];
    actor.pose_selected_by_retail_state = true;
    if(body.defeat_801027cc){
        actor.source_released=true;actor.source_proxy_bound=false;
        actor.source_proxy_target={};
        result.defeated=true;result.reward_enqueued=true;
        result.defeat_effect_allocated=next.defeat_effect_368!=0;
        result.defeat_effect_exhausted=next.defeat_effect_368==0;
    }
    actor.last_behavior_status = result.status;
    result.committed = true;
    result.notify_detection_global=detection && detection->result.notify_global_800bbd84 &&
        !(actor.source_behavior->behavior.parameter_03&0x20u);
    result.request_changed = animation.request_changed;
    result.pose_changed = animation.record_changed;
    return result;
}
void bind_imported_actor_request0_caches(ActorAreaRuntime& area, const EbdDirectory& ebd,
                                       size_t cap) {
    if (area.decoded_resource_bytes > cap)
        throw std::runtime_error("actor pose cache area budget inconsistent");
    using Cache = std::shared_ptr<const ActorPoseSequenceCache>;
    std::vector<std::pair<uint32_t, Cache>> shared;
    std::vector<std::pair<size_t, Cache>> pending;
    size_t added_bytes = 0;
    for (size_t i = 0; i < area.actors.size(); ++i) {
        const auto& actor = area.actors[i];
        if (actor.animation_bank) throw std::runtime_error("actor request0 cache conflicts with request bank");
        if (actor.source_class != 0x20 || actor.subtype != 3 ||
            actor.initializer != 0x80102410 || !actor.pose_selected_by_retail_state ||
            !actor.pose || actor.pose->request != 0 || actor.pose->record_index != 0 ||
            actor.request0_pose_cache) continue;
        if (!actor.resource) throw std::runtime_error("actor pose cache requires bound mesh");
        Cache cache;
        for (const auto& entry : shared) if (entry.first == actor.row) cache = entry.second;
        if (!cache) {
            auto decoded = decode_ebd_actor_pose_sequence(ebd, actor.row, *actor.resource, 0,
                cap - area.decoded_resource_bytes - added_bytes);
            added_bytes += decoded.decoded_bytes;
            cache = std::make_shared<const ActorPoseSequenceCache>(std::move(decoded));
            shared.emplace_back(actor.row, cache);
        }
        for (const auto& pose : cache->poses)
            if (pose.base_translation.size() != actor.pose->base_translation.size())
                throw std::runtime_error("actor pose cache live part counts inconsistent");
        pending.emplace_back(i, std::move(cache));
    }
    // Every decode and potentially throwing allocation has finished.
    for (const auto& entry : pending) {
        auto& actor = area.actors[entry.first];
        actor.request0_pose_cache = entry.second;
        actor.request0_sequence = actor_sequence_begin(0, entry.second->records[0]);
    }
    area.decoded_resource_bytes += added_bytes;
    area.decoded_pose_cache_bytes += added_bytes;
    area.unique_pose_cache_count += shared.size();
}
bool initialize_imported_actor_fresh_pose_proxy_subset(ImportedActorInstance& actor) noexcept {
    if(actor.source_class!=0x20||actor.subtype!=3||actor.initializer!=0x80102410||
       !actor.source_behavior||actor.original_pool_slot>=16||actor.source_proxy_bound)return false;
    actor.source_pose_scratch_shift=0;actor.source_pose_preserved_186=0;actor.source_shadow_work76=0;
    actor.source_proxy_target={};
    actor.source_proxy_target.source_address=0x8009f108u+uint32_t(actor.original_pool_slot)*0x3c0u+0x33cu;
    actor.source_behavior->packet_33c=0;
    actor.source_proxy_bound=true;return true;
}
ImportedActorPoseProxyTick tick_imported_actor_pose_proxy(ImportedActorInstance& actor,
    const ReferenceTables& references,SourceHitProxyBank& bank,
    std::optional<uint32_t> source_mask) noexcept {
    ImportedActorPoseProxyTick out;
    if(!actor.source_proxy_bound||!actor.source_behavior||!actor.pose)return out;
    // This API is a separate diagnostic tail. It cannot clear a packet that
    // the body transaction held, including one not yet copied from the bank.
    if(actor.source_proxy_target.packet_00||actor.source_behavior->packet_33c){
        out.proxy.status=Class20_03ProxyStatus::NeedsBodyTransaction;return out;
    }
    ActorPoseCollisionInputs pose;
    pose.reference=&references;pose.heading_56=actor.source_behavior->behavior.heading_56;
    for(size_t axis=0;axis<3;++axis)pose.raw_14[axis]=actor.source_behavior->raw_14[axis];
    pose.scale_158=actor.raw_scale;pose.root_translation=actor.pose->root_translation;
    pose.flags_06=actor.source_dispatch_flag6;pose.scratch_shift_1f800074=actor.source_pose_scratch_shift;
    pose.preserved_186=actor.source_pose_preserved_186;
    const auto produced=produce_actor_pose_collision_point_8003346c(pose);
    out.pose_status=produced.status;
    if(!produced.point_180)return out;
    Class20_03ProxyInputs input;
    input.pose_point_180=produced.point_180;
    input.source_work_01=uint8_t(0x20u|uint8_t(actor.original_pool_slot));
    input.producer_330=uint16_t(actor.source_behavior->behavior.work_330);
    input.target_33c=&actor.source_proxy_target;input.mask_80098798=source_mask;
    out.proxy=class20_03_finalize_proxy_80102398(*actor.source_behavior,bank,input);
    out.committed=out.proxy.status==Class20_03ProxyStatus::SourceBackedPartial;
    return out;
}
ImportedActorAnimationTick tick_imported_actor_request0(ActorAreaRuntime& area) noexcept {
    ImportedActorAnimationTick out;
    for (auto& actor : area.actors) {
        if (!actor.request0_pose_cache || !actor.request0_sequence) continue;
        if (actor.source_class != 0x20 || actor.subtype != 3 ||
            actor.initializer != 0x80102410 || !actor.pose_selected_by_retail_state ||
            !actor.pose || actor.pose->request != 0) { ++out.unsupported; continue; }
        // Original class20 dispatcher admits this low-bit intersection. These
        // are fresh parser/allocator bytes, not a port of distance/pause owners.
        if ((actor.source_dispatch_flag0 & actor.source_dispatch_flag6 & 1u) == 0) continue;
        const auto& cache = *actor.request0_pose_cache;
        auto cursor = *actor.request0_sequence;
        const auto result = actor_sequence_step(cursor, cache.records.data(), cache.records.size());
        if (result.status != ActorSequenceStatus::SourceBackedPartial ||
            cursor.record_index >= cache.poses.size()) { ++out.unsupported; continue; }
        const auto& source = cache.poses[cursor.record_index];
        auto& pose = *actor.pose;
        if (source.base_translation.size() != pose.base_translation.size()) {
            ++out.unsupported; continue;
        }
        if (result.record_changed) {
            pose.record_index = source.record_index;
            pose.frame_index = source.frame_index;
            pose.next_control = source.next_control;
            pose.sequencing_record = source.sequencing_record;
            pose.root_translation = source.root_translation;
            pose.rotations = source.rotations;
            std::copy(source.base_translation.begin(), source.base_translation.end(),
                      pose.base_translation.begin());
            pose.sequence_source_offset = source.sequence_source_offset;
            pose.pose_source_offset = source.pose_source_offset;
            pose.translations_source_offset = source.translations_source_offset;
            ++out.pose_changed;
        }
        pose.countdown = cursor.countdown_a5;
        actor.request0_sequence = cursor;
        ++out.admitted;
    }
    return out;
}
void bind_imported_actor_floor(ImportedActorInstance& actor, int16_t raw_floor_y) {
    actor.source_origin[1] = raw_floor_y;
    actor.floor_sample_available = true;
    if (actor.source_behavior && !actor.source_behavior_initialized) {
        Class20_03InitializeInputs input;
        input.source_floor_word = raw_floor_y;
        // Both original statistics tables' fresh index0 records select profile4
        // (80036478). This does not establish the HP/global selector. Restrict
        // admission to the source-tested zero context/index inputs.
        if (actor.source_behavior->flags_0c == 0 && actor.source_context_high_byte == 0)
            input.source_detection_profile = 4;
        // Statistics globals and HP ownership remain unknown. Profile is not
        // inferred here; the initializer binds only independently proved fields.
        actor.last_behavior_status = class20_03_initialize_scalar_state(*actor.source_behavior, input);
        actor.source_behavior_initialized = actor.last_behavior_status == Class20_03CallbackStatus::SourceBackedPartial;
    }
}
ImportedActorFloorSample sample_imported_actor_floor(const ImportedActorInstance& actor,
                                                   const RuntimeMap& map, size_t area) {
    ImportedActorFloorSample out;
    const bool known_initializer = (actor.subtype == 0x35 && actor.initializer == 0x80106974) ||
                                   (actor.subtype == 3 && actor.initializer == 0x80102410);
    if (!known_initializer || !actor.floor_bounds || !actor.floor_query_required ||
        actor.source_class != 0x20 || actor.floor_sample_available) {
        out.status = RuntimeContactStatus::Unsupported;
        return out;
    }
    const auto& bounds = *actor.floor_bounds;
    const int16_t x = actor.source_origin[0], y = actor.source_origin[1], z = actor.source_origin[2];
    const auto find_cell = [&](int32_t world_x, int32_t world_z) -> std::optional<RuntimeCellRef> {
        if (area >= map.areas.size()) return std::nullopt;
        const auto& data = map.areas[area];
        const auto grid = [](int32_t value) { return value >= 0 ? value / 512 : -((-value + 511) / 512); };
        const int32_t gx = grid(world_x + 0x8000), gz = grid(world_z + 0x8000);
        const int32_t lx = gx - data.start_x, lz = gz - data.start_z;
        if (lx < 0 || lz < 0 || lx >= data.width || lz >= data.height) return std::nullopt;
        const uint64_t index = uint64_t(data.cell_first) + uint64_t(lz) * data.width + uint64_t(lx);
        if (index >= map.cells.size() || index > UINT32_MAX) return std::nullopt;
        RuntimeCellRef cell;
        cell.area_index = area; cell.cell_index = static_cast<uint32_t>(index);
        cell.packed_cell = map.cells[static_cast<size_t>(index)];
        return cell;
    };
    const int32_t nx = (uint16_t(x) & 511u) < 257u ? -512 : 512;
    const int32_t nz = (uint16_t(z) & 511u) < 257u ? -512 : 512;
    const std::array<std::array<int32_t, 2>, 4> offsets{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    std::array<RuntimeCellRef, 4> candidates{};
    for (const auto& delta : offsets) {
        const auto cell = find_cell(int32_t(x) + delta[0], int32_t(z) + delta[1]);
        if (!cell || !cell->packed_cell) continue;
        bool duplicate = false;
        if (cell->packed_cell > 0)
            for (uint32_t i = 0; i < out.candidate_cells; ++i)
                if (candidates[i].packed_cell == cell->packed_cell) duplicate = true;
        if (!duplicate) candidates[out.candidate_cells++] = *cell;
    }
    if (!out.candidate_cells) return out;
    out.status = RuntimeContactStatus::SourceBackedPartial;
    const auto containing = find_cell(x, z);
    const auto wrap = [](int32_t v) { return static_cast<int16_t>(static_cast<uint16_t>(v)); };
    int32_t best = 32767;
    for (unsigned pass = 0; pass < 4; ++pass) {
        const int32_t previous = best;
        for (uint32_t rev = out.candidate_cells; rev > 0; --rev) {
            const auto& cell = candidates[rev - 1];
            if (cell.packed_cell < 0) {
                if (!containing || containing->cell_index != cell.cell_index) continue;
                const int32_t height = -int32_t((uint16_t(cell.packed_cell) & 0x1f00u) >> 1) - 1 - bounds.y1;
                if (height < best) {
                    best = height; out.cell_index = cell.cell_index; out.primitive_index.reset();
                }
                continue;
            }
            const auto* idx = runtime_map_cell_idx(map, cell);
            const auto* list = runtime_map_cell_contact_list(map, cell);
            if (!idx || !list) { ++out.unsupported_primitives; continue; }
            for (uint32_t gi = 0; gi < list->group_count; ++gi) {
                const uint64_t gix = uint64_t(list->group_first) + gi;
                if (gix >= map.contact_groups.size()) { ++out.unsupported_primitives; continue; }
                const auto& group = map.contact_groups[static_cast<size_t>(gix)];
                if (!group.primitive_count) continue;
                if (group.primitive_first >= map.contact_primitives.size()) { ++out.unsupported_primitives; continue; }
                const auto family = map.contact_primitives[group.primitive_first].code;
                if (family >= 256u) continue; // original admission rejects this group.
                const int32_t raw_x0 = int32_t(idx->x) + group.x0;
                const int32_t raw_x1 = int32_t(idx->x) + group.x1;
                const int32_t raw_z0 = int32_t(idx->z) + group.z0;
                const int32_t raw_z1 = int32_t(idx->z) + group.z1;
                const int16_t x0 = wrap(raw_x0 - bounds.x1);
                const int16_t x1 = wrap(raw_x1 - bounds.x0);
                const int16_t z0 = wrap(raw_z0 - bounds.z1);
                const int16_t z1 = wrap(raw_z1 - bounds.z0);
                const bool inside_envelope = x0 < x && x < x1 && z0 < z && z < z1;
                if ((family & 0xf0u) >= 0x20u) {
                    // Floor classifiers8004E490/8004E750/8004EA10/8004ECD0
                    // retain strict X/Z bounds inside this conservative envelope
                    // for the proved symmetric profiles. Ordered, unwrapped
                    // extents keep their source division denominators positive.
                    // This excludes unrelated groups only; diagonal intervals and
                    // every potentially intersecting group remain unsupported.
                    const bool bounded = bounds.x1 >= 0 && bounds.z1 >= 0 &&
                        int32_t(bounds.x0) == -int32_t(bounds.x1) &&
                        int32_t(bounds.z0) == -int32_t(bounds.z1) &&
                        raw_x0 < raw_x1 && raw_z0 < raw_z1 &&
                        raw_x0 - bounds.x1 >= INT16_MIN && raw_x1 - bounds.x0 <= INT16_MAX &&
                        raw_z0 - bounds.z1 >= INT16_MIN && raw_z1 - bounds.z0 <= INT16_MAX;
                    if ((family & 0xf0u) <= 0x50u && bounded && !inside_envelope) continue;
                    ++out.unsupported_primitives;
                    continue;
                }
                if (!inside_envelope) continue;
                for (uint32_t pi = 0; pi < group.primitive_count; ++pi) {
                    const uint64_t pix = uint64_t(group.primitive_first) + pi;
                    if (pix >= map.contact_primitives.size()) { ++out.unsupported_primitives; continue; }
                    const auto& primitive = map.contact_primitives[static_cast<size_t>(pix)];
                    // Class20 group bits are nonzero, giving the source 0x8000 filter.
                    if (primitive.flags & 0x8000u) continue;
                    if (primitive.code != 1u && primitive.code != 3u && primitive.code != 15u) {
                        ++out.unsupported_primitives; continue;
                    }
                    const auto interval = runtime_contact_floor_interval_8004a060(
                        *idx, group, primitive, bounds, x, z);
                    if (!interval) { ++out.unsupported_primitives; continue; }
                    if (interval->y0 < best && std::min<int32_t>(y, best) < interval->y1) {
                        best = interval->y0; out.cell_index = cell.cell_index;
                        out.primitive_index = static_cast<uint32_t>(pix);
                    }
                }
            }
        }
        if (previous == best) break;
    }
    if (out.unsupported_primitives) {
        out.status = RuntimeContactStatus::Unsupported;
        out.cell_index.reset(); out.primitive_index.reset();
    } else if (best != 32767) out.floor_y = wrap(best);
    return out;
}
void unload_imported_actors(ActorAreaRuntime& area) {
    ActorAreaRuntime empty;
    area = std::move(empty);
}

ImportedActorPoseMatrices build_imported_actor_pose_matrices(const ImportedActorInstance& actor) {
    if (!actor.resource || !actor.pose) throw std::runtime_error("actor matrices require mesh and pose");
    const auto& bindings = actor.resource->bindings;
    const auto& pose = *actor.pose;
    if (bindings.empty() || bindings.size() > kActorClass20PartLimit ||
        pose.base_translation.size() != bindings.size())
        throw std::runtime_error("actor matrix part counts inconsistent");
    std::array<PlayerPoseMatrix, kActorClass20PartLimit> rotation{};
    std::array<PlayerPosePoint, kActorClass20PartLimit> position{};
    std::array<bool, kActorClass20PartLimit> transform_initialized{};
    const std::array<float, 3> scale{{float(actor.raw_scale[0]) / 4096.0f,
                                     float(actor.raw_scale[1]) / 4096.0f,
                                     float(actor.raw_scale[2]) / 4096.0f}};
    const auto yaw = player_pose_rotation_xyz(0, actor.heading, 0);
    const auto& root_angles = pose.rotations[0];
    rotation[0] = player_pose_mul(yaw, player_pose_rotation_xyz(root_angles[0], root_angles[1], root_angles[2]));
    const auto root_delta = player_pose_transform_point(yaw,
        {float(pose.root_translation[0]) * 8.0f * scale[0],
         float(pose.root_translation[1]) * 8.0f * scale[1],
         float(pose.root_translation[2]) * 8.0f * scale[2]});
    position[0] = {float(actor.source_origin[0]) + root_delta.x,
                   float(actor.source_origin[1]) + root_delta.y,
                   float(actor.source_origin[2]) + root_delta.z};
    ImportedActorPoseMatrices out;
    out.part_count = bindings.size();
    out.draw_enabled[0] = bindings[0].draw_control >= 0;
    transform_initialized[0] = true;
    for (size_t i = 1; i < bindings.size(); ++i) {
        const auto& b = bindings[i];
        if (b.parent < 0) {
            // The source skips a detached transform; do not render uninitialized work.
            out.draw_enabled[i] = false;
            continue;
        }
        if (b.parent >= static_cast<int>(i) || b.rotation_translation_index >= pose.rotations.size())
            throw std::runtime_error("actor matrix hierarchy invalid");
        if (b.rotation_translation_index != 0 && !transform_initialized[static_cast<size_t>(b.parent)])
            continue; // no derived transform may consume skipped source work.
        transform_initialized[i] = true;
        out.draw_enabled[i] = b.draw_control >= 0;
        if (b.rotation_translation_index == 0) {
            rotation[i] = rotation[0]; position[i] = position[0];
            continue;
        }
        const size_t parent = static_cast<size_t>(b.parent);
        const auto& angle = pose.rotations[b.rotation_translation_index];
        rotation[i] = player_pose_mul(rotation[parent], player_pose_rotation_xyz(angle[0], angle[1], angle[2]));
        const auto& t = pose.base_translation[i];
        const auto delta = player_pose_transform_point(rotation[parent],
            {float(t[0]) * scale[0], float(t[1]) * scale[1], float(t[2]) * scale[2]});
        position[i] = {position[parent].x + delta.x, position[parent].y + delta.y, position[parent].z + delta.z};
    }
    for (size_t i = 0; i < bindings.size(); ++i) {
        auto& matrix = out.world[i];
        matrix = rotation[i];
        for (size_t r = 0; r < 3; ++r)
            for (size_t c = 0; c < 3; ++c) matrix.m[r * 4 + c] *= scale[c];
        matrix.m[3] = position[i].x; matrix.m[7] = position[i].y; matrix.m[11] = position[i].z;
    }
    return out;
}
} // namespace l3
