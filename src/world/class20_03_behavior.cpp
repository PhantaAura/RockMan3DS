#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/class20_03_hit.h"
#include "legends3ds/class20_03_reaction.h"
#include "legends3ds/class20_03_action3.h"
#include "legends3ds/class20_03_action9.h"
#include "legends3ds/class20_03_action4.h"
#include "legends3ds/class20_03_late_actions.h"

namespace l3 {
namespace {
int16_t signed16(uint16_t value) {
    return value < 0x8000u ? static_cast<int16_t>(value)
                         : static_cast<int16_t>(static_cast<int32_t>(value) - 65536);
}
int16_t decrement(int16_t value) {
    return signed16(static_cast<uint16_t>(static_cast<uint16_t>(value) - 1u));
}
int32_t shift_difference(int16_t actor, int16_t player) {
    const int32_t d = static_cast<int32_t>(actor) - player;
    return d < 0 ? -((-d + 1) / 2) : d / 2;
}
}

uint32_t class20_03_distance_work80(int16_t actor_x, int16_t actor_z,
                                   int16_t player_x, int16_t player_z) {
    const int64_t dx = shift_difference(actor_x, player_x);
    const int64_t dz = shift_difference(actor_z, player_z);
    const auto sum = static_cast<uint32_t>(dx * dx + dz * dz);
    return sum > 0x10000000u ? UINT32_MAX : sum << 2;
}

int16_t class20_03_turn_step(uint16_t target, uint16_t heading, int16_t limit) {
    const uint32_t delta = (static_cast<uint32_t>(target) - heading) & 0xfffu;
    uint32_t magnitude = static_cast<uint16_t>(limit);
    const auto doubled = static_cast<uint32_t>(static_cast<int32_t>(limit) * 2);
    if (delta < 2048u) {
        if (delta < doubled) magnitude = delta;
    } else {
        const uint32_t inverse = 4096u - delta;
        if (inverse < doubled) magnitude = inverse;
    }
    return signed16(static_cast<uint16_t>(delta <= 2048u ? magnitude : 0u - magnitude));
}

Class20_03ActionStatus class20_03_step_action(Class20_03BehaviorState& state,
                                           const Class20_03ActionInputs& input) {
    auto next = state;
    if (next.action_09 == 0) {
        if (next.phase_0a == 0) {
            next.phase_0a = (next.parameter_03 & 0x80u) ? 2 : 1;
            next.animation_ac = 0;
            next.timer_32c = 30;
            next.work_330 = 0;
            next.parameter_03 &= 0x7fu;
            // Source falls through the phase1 timer body even when +80
            // changed the stored phase to2 on this first invocation.
            next.timer_32c = decrement(next.timer_32c);
            if (next.timer_32c < 0) next.flags_328 |= 1u;
        } else if (next.phase_0a == 1) {
            next.timer_32c = decrement(next.timer_32c);
            if (next.timer_32c < 0) next.flags_328 |= 1u;
        } else if (next.phase_0a == 2) {
            bool begin = false;
            if (next.parameter_03 & 0x20u) {
                if (!input.predicate_8001da58) return Class20_03ActionStatus::NeedsPredicate;
                begin = *input.predicate_8001da58;
            } else {
                if (!input.work_80) return Class20_03ActionStatus::NeedsWork80;
                begin = *input.work_80 <= 0x0031ffffu;
                if (!begin) {
                    if (!input.packet_33c) return Class20_03ActionStatus::NeedsPacket;
                    begin = (*input.packet_33c & 0x00ffffffu) != 0;
                }
            }
            next.flags_328 |= 0x20u;
            if (begin) {
                next.action_09 = 3;
                next.phase_0a = 0;
            }
        }
    } else if (next.action_09 == 1) {
        if (next.phase_0a <= 1) {
            if (!input.target_heading_80031198) return Class20_03ActionStatus::NeedsTargetHeading;
            if (next.phase_0a == 0) {
                next.phase_0a = 1;
                next.animation_ac = 1;
                const int32_t delta = class20_03_turn_step(*input.target_heading_80031198,
                                                          next.heading_56, 2048);
                next.turn_66 = static_cast<int16_t>((delta < 0 ? -delta : delta) >> 3);
                next.timer_32c = 8;
                next.work_330 = 0;
            }
            next.timer_32c = decrement(next.timer_32c);
            if (next.timer_32c < 0) next.flags_328 |= 1u;
            const auto delta = class20_03_turn_step(*input.target_heading_80031198,
                                                   next.heading_56, next.turn_66);
            next.work_36c = delta;
            next.heading_56 = static_cast<uint16_t>((next.heading_56 + delta) & 0xfffu);
        }
    } else {
        return Class20_03ActionStatus::UnknownAction;
    }
    state = next;
    return Class20_03ActionStatus::SourceBackedPartial;
}

void class20_03_select_action(Class20_03BehaviorState& state, uint16_t sample) {
    const uint16_t low = sample & 15u;
    state.action_09 = 2;
    state.phase_0a = 0;
    if (low < 6u) state.turn_66 = 0;
    else if (low < 10u) state.turn_66 = 16;
    else if (low < 14u) state.turn_66 = -16;
    else state.action_09 = 0;
}

namespace {
Class20_03CallbackResult hold(Class20_03CallbackStatus status) {
    Class20_03CallbackResult result;
    result.status = status;
    return result;
}

Class20_03CallbackStatus action_status(Class20_03ActionStatus status) {
    switch (status) {
    case Class20_03ActionStatus::SourceBackedPartial:
        return Class20_03CallbackStatus::SourceBackedPartial;
    case Class20_03ActionStatus::UnknownAction:
        return Class20_03CallbackStatus::UnknownAction;
    case Class20_03ActionStatus::NeedsWork80:
        return Class20_03CallbackStatus::NeedsWork80;
    case Class20_03ActionStatus::NeedsPacket:
        return Class20_03CallbackStatus::NeedsPacket;
    case Class20_03ActionStatus::NeedsPredicate:
        return Class20_03CallbackStatus::NeedsPredicate;
    case Class20_03ActionStatus::NeedsTargetHeading:
        return Class20_03CallbackStatus::NeedsTargetHeading;
    }
    return Class20_03CallbackStatus::UnknownAction;
}

Class20_03CallbackResult hit_subset(Class20_03CallbackState& next,
                                   const Class20_03CallbackInputs& inputs) {
    Class20_03HitInputs hit;
    hit.packet_33c=inputs.action.packet_33c;
    if(inputs.combat_record_byte09&&inputs.hit_record_variant_32e)
        hit.selected_record=Class20_03HitRecordByte09{*inputs.hit_record_variant_32e,*inputs.combat_record_byte09};
    hit.hit_word_34a=inputs.hit_word_34a;hit.angle_80031198=inputs.hit_angle_80031198;
    hit.sound_consumer_ready=inputs.sound_consumer_ready;
    const auto consumed=class20_03_consume_hit_801041b0(next,hit);
    using H=Class20_03HitStatus;
    if(consumed.status!=H::SourceBackedPartial){
        switch(consumed.status){
        case H::NeedsPacket:return hold(Class20_03CallbackStatus::NeedsPacket);
        case H::NeedsHealth:return hold(Class20_03CallbackStatus::NeedsHealth);
        case H::NeedsSoundConsumer:return hold(Class20_03CallbackStatus::NeedsSoundConsumer);
        default:return hold(Class20_03CallbackStatus::NeedsHitConsumer);
        }
    }
    Class20_03CallbackResult result;
    result.sound_8001a050=consumed.sound_8001a050;
    result.sound_pointed_words=consumed.sound_pointed_words;
    return result;
}

Class20_03CallbackResult action2_subset(Class20_03CallbackState& next,
                                       const Class20_03CallbackInputs& input) {
    auto& behavior = next.behavior;
    if (behavior.phase_0a > 1) return {};
    if (!input.prepared_action2) return hold(Class20_03CallbackStatus::NeedsAction2Motion);
    const auto& prepared = *input.prepared_action2;
    if (!prepared.source_a6) return hold(Class20_03CallbackStatus::NeedsAnimationCue);
    Class20_03CallbackResult result;
    if (*prepared.source_a6 == 1) {
        if (!input.sound_consumer_ready) return hold(Class20_03CallbackStatus::NeedsSoundConsumer);
        result.sound_8001a050 = 176;
    }
    const auto entry_raw = next.raw_14;
    const auto entry_fixed = next.fixed_1c;
    if (behavior.phase_0a == 0) {
        behavior.phase_0a = 1;
        behavior.animation_ac = 1;
        behavior.work_330 = 0;
    }
    behavior.heading_56 = static_cast<uint16_t>(
        (static_cast<int32_t>(behavior.heading_56) + behavior.turn_66) & 0xfff);
    next.raw_14 = prepared.raw_14;
    next.fixed_1c = prepared.fixed_1c;
    next.work_370 = prepared.source_contact_word;
    next.raw_14[1] = signed16(static_cast<uint16_t>(prepared.source_floor_word));
    const uint32_t fixed_y = static_cast<uint32_t>(prepared.source_floor_word) << 16;
    next.fixed_1c[1] = fixed_y <= 0x7fffffffu ? static_cast<int32_t>(fixed_y)
        : static_cast<int32_t>(static_cast<int64_t>(fixed_y) - 0x100000000LL);
    const int32_t floor_change = static_cast<int32_t>(next.raw_14[1]) - entry_raw[1];
    const bool rollback = floor_change >= 33 || floor_change <= -33;
    if (rollback) {
        next.raw_14 = entry_raw;
        next.fixed_1c = entry_fixed;
    }
    if (rollback || (prepared.source_contact_word & 0xf00u)) {
        behavior.action_09 = 1;
        behavior.phase_0a = 0;
        next.aux_0b = 0;
        next.restore_action_0e = behavior.flags_328 & 0x10u ? 4 : 2;
    } else if (behavior.flags_328 & 0x10u) {
        behavior.flags_328 |= 1u;
    }
    return result;
}
}

bool class20_03_detection_range_rejected(uint32_t distance, uint8_t range) {
    return distance >= (static_cast<uint32_t>(range) << 20);
}

Class20_03CallbackStatus class20_03_initialize_scalar_state(
    Class20_03CallbackState& state, const Class20_03InitializeInputs& input) {
    auto next = state;
    if (!(next.behavior.parameter_03 & 0x40u) && !input.source_floor_word)
        return Class20_03CallbackStatus::NeedsFloor;
    next.work_00 |= 2u;
    next.angle_54[0] = 0;
    next.angle_54[2] = 0;
    next.angle_54[1] = next.behavior.heading_56;
    next.behavior.turn_66 = 0;
    next.behavior.flags_328 = next.behavior.parameter_03 & 0x20u ? 0 : 14;
    next.behavior.timer_32c = 0;
    next.variant_32e = next.flags_0c & 0x10u ? 1 : 0;
    next.behavior.work_330 = 0;
    next.work_332 = 0;
    next.packet_33c = 0;
    next.detection_profile_352 = input.source_detection_profile;
    next.detection_phase_353 = 0;
    next.detection_offset_35b = -6;
    next.work_35c = 0;
    next.work_35e = 0;
    next.defeat_timer_360 = 4;
    next.work_362 = next.raw_14[3];
    next.behavior.work_36c = 0;
    next.work_370 = 0;
    next.child_374 = 0;
    next.health_6e = input.source_initial_health;
    next.initial_health_b0 = input.source_initial_health;
    next.previous_health_b2 = input.source_initial_health;
    next.state_08 = 1;
    next.behavior.phase_0a = 0;
    next.aux_0b = 0;
    if (next.behavior.parameter_03 & 0x40u) {
        next.behavior.action_09 = 8;
        next.raw_14[1] = signed16(static_cast<uint16_t>(
            static_cast<uint16_t>(next.raw_14[1]) + 32u));
        next.fixed_1c[1] = static_cast<int32_t>(next.raw_14[1]) * 65536;
    } else {
        next.behavior.action_09 = 0;
        next.raw_14[1] = signed16(static_cast<uint16_t>(*input.source_floor_word));
        const uint32_t fixed_y = static_cast<uint32_t>(*input.source_floor_word) << 16;
        next.fixed_1c[1] = fixed_y <= 0x7fffffffu ? static_cast<int32_t>(fixed_y)
            : static_cast<int32_t>(static_cast<int64_t>(fixed_y) - 0x100000000LL);
    }
    next.work_15e = 16912;
    state = next;
    return Class20_03CallbackStatus::SourceBackedPartial;
}

Class20_03CallbackResult class20_03_step_state1_tail(
    Class20_03CallbackState& state, std::optional<uint16_t> sample,
    bool defeat_ready, uint32_t defeat_effect_identity) {
    auto next = state;
    auto& behavior = next.behavior;
    if (behavior.flags_328 & 1u) {
        if (behavior.action_09 == 1) {
            behavior.action_09 = next.restore_action_0e;
        } else if (behavior.action_09 == 6) {
            if (!next.health_6e) return hold(Class20_03CallbackStatus::NeedsHealth);
            if (*next.health_6e < 51) behavior.action_09 = 5;
            else if (behavior.flags_328 & 0x10u) behavior.action_09 = 9;
            else {
                if (!sample) return hold(Class20_03CallbackStatus::NeedsRandomSample);
                class20_03_select_action(behavior, *sample);
            }
        } else if (behavior.flags_328 & 0x10u) {
            behavior.action_09 = 9;
        } else {
            if (!sample) return hold(Class20_03CallbackStatus::NeedsRandomSample);
            class20_03_select_action(behavior, *sample);
        }
        behavior.phase_0a = 0;
        next.aux_0b = 0;
        behavior.parameter_03 &= 0xbfu;
        behavior.flags_328 &= 0xfffeu;
    }
    if (next.work_35c) next.work_35c = decrement(next.work_35c);
    if (next.work_35e) next.work_35e = decrement(next.work_35e);
    if (next.work_332) next.work_332 = decrement(next.work_332);
    if (behavior.flags_328 & 0x40u) {
        next.defeat_timer_360 = decrement(next.defeat_timer_360);
        if (next.defeat_timer_360 < 0) {
            if (!defeat_ready) return hold(Class20_03CallbackStatus::NeedsDefeatLifecycle);
            //80031618 may return zero on exhaustion. Source release, reward
            // dispatch and sound161 still execute in that case.
            next.defeat_effect_368=defeat_effect_identity;
            next.work_00=0;
            state=next;
            Class20_03CallbackResult out;out.defeat_801027cc=true;return out;
        }
    }
    state = next;
    return {};
}

Class20_03CallbackResult class20_03_step_state1(
    Class20_03CallbackState& state, const Class20_03CallbackInputs& input) {
    if (state.state_08 != 1) return hold(Class20_03CallbackStatus::UnknownState);
    auto next = state;
    auto& behavior = next.behavior;
    if (behavior.parameter_03 & 0x20u) {
        behavior.flags_328 &= 0xffefu;
    } else {
        bool detected = false;
        if (input.detection_8003648c) {
            detected = *input.detection_8003648c;
        } else if (input.detection_range_byte2 && input.action.work_80 &&
                   class20_03_detection_range_rejected(*input.action.work_80,
                                                       *input.detection_range_byte2)) {
            next.detection_phase_353 = 0;
        } else {
            return hold(Class20_03CallbackStatus::NeedsDetection);
        }
        behavior.flags_328 = static_cast<uint16_t>(
            (behavior.flags_328 & 0xffefu) | (detected ? 0x10u : 0u));
    }
    if (input.action.packet_33c) next.packet_33c = *input.action.packet_33c;
    Class20_03CallbackResult result;
    if (behavior.flags_328 & 2u) {
        result = hit_subset(next, input);
        if (result.status != Class20_03CallbackStatus::SourceBackedPartial) return result;
    }
    if (behavior.action_09 == 2) {
        const auto action = action2_subset(next, input);
        if (action.status != Class20_03CallbackStatus::SourceBackedPartial) return action;
        if (action.sound_8001a050) {
            if (result.sound_8001a050) result.secondary_sound_8001a050 = action.sound_8001a050;
            else result.sound_8001a050 = action.sound_8001a050;
        }
    } else if (behavior.action_09 == 3) {
        Class20_03Action3Inputs args;
        args.packet_33c = input.action.packet_33c;
        args.combat_record_byte_a = input.combat_record_byte_a;
        args.target_heading_80031198 = input.action.target_heading_80031198;
        args.sound_consumer_ready = input.sound_consumer_ready;
        args.release_consumer_ready = input.release_consumer_ready;
        if (input.prepared_action3) {
            const auto& p = *input.prepared_action3;
            args.prepared_motion = Class20_03PreparedAction3{p.raw_14,p.fixed_1c,
                p.source_floor_word,p.source_contact_word,p.heading_after_turn};
        }
        const auto action = class20_03_step_action3(next,args);
        using S = Class20_03Action3Status;
        if (action.status != S::SourceBackedPartial) {
            auto status = Class20_03CallbackStatus::UnknownAction;
            switch(action.status) {
            case S::NeedsPacket: case S::StalePacket: status=Class20_03CallbackStatus::NeedsPacket;break;
            case S::NeedsCombatProfile: status=Class20_03CallbackStatus::NeedsCombatProfile;break;
            case S::NeedsDelay: case S::NeedsTurnLimit: status=Class20_03CallbackStatus::NeedsSourceWork;break;
            case S::NeedsTargetHeading: status=Class20_03CallbackStatus::NeedsTargetHeading;break;
            case S::NeedsMotion: case S::StaleMotion: status=Class20_03CallbackStatus::NeedsAction3Motion;break;
            case S::NeedsSoundConsumer: status=Class20_03CallbackStatus::NeedsSoundConsumer;break;
            case S::NeedsReleaseConsumer: status=Class20_03CallbackStatus::NeedsReleaseConsumer;break;
            default:break;
            }
            return hold(status);
        }
        // Release effects belong to the real allocator, never the scalar body;
        // action3 only reaches here with release_consumer_ready acknowledged.
        if(action.release_80031824)result.release_80031824=true;
        if(action.sound_8001a050) {
            if(result.sound_8001a050)result.secondary_sound_8001a050=action.sound_8001a050;
            else result.sound_8001a050=action.sound_8001a050;
        }
    } else if (behavior.action_09 == 4) {
        Class20_03Action4Inputs args;
        args.combat_record_byte_c=input.combat_record_byte_c;
        args.produced_child_374=input.produced_child_374;
        args.sound_consumer_ready=input.sound_consumer_ready;
        if(input.prepared_action4) {
            const auto& p=*input.prepared_action4;
            args.prepared_motion=Class20_03PreparedAction4{p.raw_14,p.fixed_1c,
                p.source_floor_word,p.source_forward,p.heading};
        }
        const auto action=class20_03_step_action4(next,args);
        using S=Class20_03Action4Status;
        if(action.status!=S::SourceBackedPartial) {
            auto status=Class20_03CallbackStatus::UnknownAction;
            switch(action.status) {
            case S::NeedsMotion: case S::StaleMotion: status=Class20_03CallbackStatus::NeedsAction4Motion;break;
            case S::NeedsCombatProfile: status=Class20_03CallbackStatus::NeedsCombatProfile;break;
            case S::NeedsChildConsumer: status=Class20_03CallbackStatus::NeedsChildConsumer;break;
            case S::NeedsSoundConsumer: status=Class20_03CallbackStatus::NeedsSoundConsumer;break;
            default:break;
            }
            return hold(status);
        }
        if(action.sound_8001a050) {
            if(result.sound_8001a050) {
                result.secondary_sound_8001a050=action.sound_8001a050;
                result.secondary_sound_pointed_words=action.sound_pointed_words;
            } else {
                result.sound_8001a050=action.sound_8001a050;
                result.sound_pointed_words=action.sound_pointed_words;
            }
        }
    } else if (behavior.action_09 == 7 || behavior.action_09 == 8) {
        Class20_03ReactionInputs args;
        args.profile=input.reaction_profile;args.motion=input.prepared_reaction;
        args.combat_byte_b=input.reaction_combat_byte_b;
        args.combat_record_variant=input.reaction_combat_record_variant;
        args.target_heading_80031198=input.action.target_heading_80031198;
        args.sound_consumer_ready=input.sound_consumer_ready;
        const auto action=class20_03_step_reaction(next,args);
        using S=Class20_03ReactionStatus;
        if(action.status!=S::SourceBackedPartial){
            auto status=Class20_03CallbackStatus::NeedsReactionMotion;
            if(action.status==S::NeedsHealth)status=Class20_03CallbackStatus::NeedsHealth;
            else if(action.status==S::NeedsProfile || action.status==S::NeedsCombatByteB)
                status=Class20_03CallbackStatus::NeedsCombatProfile;
            else if(action.status==S::NeedsSoundConsumer)status=Class20_03CallbackStatus::NeedsSoundConsumer;
            else if(action.status==S::NeedsTargetHeading)status=Class20_03CallbackStatus::NeedsTargetHeading;
            return hold(status);
        }
        if(action.sound_8001a050){
            if(result.sound_8001a050){
                result.secondary_sound_8001a050=action.sound_8001a050;
                result.secondary_sound_pointed_words=action.sound_pointed_words;
            }else {
                result.sound_8001a050=action.sound_8001a050;
                result.sound_pointed_words=action.sound_pointed_words;
            }
        }
    } else if (behavior.action_09 == 9) {
        Class20_03Action9Inputs args;
        args.work_80=input.action.work_80;
        args.target_angle_player_to_actor_80031198=input.angle_player_to_actor_80031198;
        args.player_heading_800b5206=input.player_heading_800b5206;
        args.turn_target_actor_to_player_80031198=input.action.target_heading_80031198;
        args.sound_consumer_ready=input.sound_consumer_ready;
        if(input.prepared_action9) {
            const auto& p=*input.prepared_action9;
            args.prepared_motion=Class20_03PreparedAction9{p.raw_14,p.fixed_1c,
                p.source_floor_word,p.source_contact_word,p.heading_after_turn,p.source_a6};
        }
        const auto action=class20_03_step_action9(next,args);
        using S=Class20_03Action9Status;
        if(action.status!=S::SourceBackedPartial) {
            auto status=Class20_03CallbackStatus::UnknownAction;
            switch(action.status) {
            case S::NeedsPlayerToActorAngle: case S::NeedsTurnTarget:
                status=Class20_03CallbackStatus::NeedsTargetHeading;break;
            case S::NeedsPlayerHeading: status=Class20_03CallbackStatus::NeedsPlayerHeading;break;
            case S::NeedsWork80: status=Class20_03CallbackStatus::NeedsWork80;break;
            case S::NeedsMotion: case S::StaleMotion: status=Class20_03CallbackStatus::NeedsAction9Motion;break;
            case S::NeedsAnimationCue: status=Class20_03CallbackStatus::NeedsAnimationCue;break;
            case S::NeedsSoundConsumer: status=Class20_03CallbackStatus::NeedsSoundConsumer;break;
            default:break;
            }
            return hold(status);
        }
        if(action.sound_8001a050) {
            if(result.sound_8001a050) {
                result.secondary_sound_8001a050=action.sound_8001a050;
                result.secondary_sound_pointed_words=action.sound_pointed_words;
            } else {
                result.sound_8001a050=action.sound_8001a050;
                result.sound_pointed_words=action.sound_pointed_words;
            }
        }
    } else if (class20_03_is_late_action(behavior.action_09)) {
        Class20_03LateActionInputs args;
        args.world = input.motion_world;
        if (next.variant_32e == 0) args.combat_record_byte_a = input.combat_record_byte_a;
        args.target_heading_80031198 = input.action.target_heading_80031198;
        args.sound_consumer_ready = input.sound_consumer_ready;
        const auto action = class20_03_step_late_action(next, args);
        using S = Class20_03LateActionStatus;
        if (action.status != S::SourceBackedPartial) {
            auto status = Class20_03CallbackStatus::UnknownAction;
            switch (action.status) {
            case S::NeedsCombatProfile: status = Class20_03CallbackStatus::NeedsCombatProfile; break;
            case S::NeedsTargetHeading: status = Class20_03CallbackStatus::NeedsTargetHeading; break;
            case S::NeedsAnimationCue: status = Class20_03CallbackStatus::NeedsAnimationCue; break;
            case S::NeedsSoundConsumer: status = Class20_03CallbackStatus::NeedsSoundConsumer; break;
            case S::NeedsWorld: case S::UnsupportedBoundary: case S::UnsupportedWall: case S::UnsupportedFloor:
                status = Class20_03CallbackStatus::NeedsLateActionMotion; break;
            default: break;
            }
            return hold(status);
        }
        for (uint8_t i = 0; i < action.sound_count; ++i) {
            const auto id = action.sounds[i]; const auto words = action.sound_words[i];
            if (!result.sound_8001a050) { result.sound_8001a050 = id; result.sound_pointed_words = words; }
            else if (!result.secondary_sound_8001a050) { result.secondary_sound_8001a050 = id; result.secondary_sound_pointed_words = words; }
            else { result.tertiary_sound_8001a050 = id; result.tertiary_sound_pointed_words = words; }
        }
        result.contact_state74 = action.state74_after;
    } else {
        const bool action0_phase2 = behavior.action_09 == 0 && behavior.phase_0a == 2;
        const auto action = class20_03_step_action(behavior, input.action);
        if (action != Class20_03ActionStatus::SourceBackedPartial) return hold(action_status(action));
        // The narrow action kernel lacks byte0B. Action0's phase2 start clears a
        // halfword at0A, so the wider source-used transaction also clears byte0B.
        if (action0_phase2 && behavior.action_09 == 3) next.aux_0b = 0;
    }
    const auto tail = class20_03_step_state1_tail(next, input.source_random_sample,
        input.defeat_consumer_ready,input.defeat_effect_identity);
    if (tail.status != Class20_03CallbackStatus::SourceBackedPartial) return tail;
    result.defeat_801027cc=tail.defeat_801027cc;
    next.angle_54[1] = behavior.heading_56;
    state = next;
    return result;
}

Class20_03CallbackResult class20_03_step_callback(
    Class20_03CallbackState& state, const Class20_03CallbackInputs& input) {
    if (!input.source_random_sample) return hold(Class20_03CallbackStatus::NeedsRandomSample);
    if (!input.action.packet_33c) return hold(Class20_03CallbackStatus::NeedsPacket);
    auto next = state;
    next.previous_animation_ad = next.behavior.animation_ac;
    next.previous_raw_2c = next.raw_14;
    next.previous_fixed_34 = next.fixed_1c;
    next.angle_54[1] = next.behavior.heading_56;
    next.previous_angle_5c = next.angle_54;
    next.work_15e = 16912;
    next.random_32a = *input.source_random_sample;
    auto result = class20_03_step_state1(next, input);
    if (result.status != Class20_03CallbackStatus::SourceBackedPartial) return result;
    if (!input.animation_consumer_ready) return hold(Class20_03CallbackStatus::NeedsAnimationConsumer);
    result.animation = next.previous_animation_ad == next.behavior.animation_ac
        ? Class20_03AnimationOperation::Advance : Class20_03AnimationOperation::Restart;
    if (next.work_00 & 2u) {
        if (!input.effect_consumer_ready) return hold(Class20_03CallbackStatus::NeedsEffectConsumer);
        result.effect_8002c8cc = true;
    }
    if ((next.work_00 & 1u) && (next.behavior.flags_328 & 4u)) {
        if (!input.proxy_consumer_ready) return hold(Class20_03CallbackStatus::NeedsProxyConsumer);
        next.previous_proxy_position_334 = next.proxy_position_180;
        result.proxy_800341bc = true;
    }
    next.packet_33c = 0;
    state = next;
    return result;
}

} // namespace l3
