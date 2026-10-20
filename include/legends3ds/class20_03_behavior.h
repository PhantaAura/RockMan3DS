#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {
struct SourceReactionProfile;
struct Class20_03MotionWorld;

// Scalar subset of ST00 class20/subtype03 work. Offset names deliberately
// survive where a full gameplay interpretation has not been established.
struct Class20_03BehaviorState {
    uint8_t parameter_03 = 0;
    uint8_t action_09 = 0;
    uint8_t phase_0a = 0;
    uint8_t animation_ac = 0;
    uint16_t flags_328 = 0;
    int16_t timer_32c = 0;
    int16_t work_330 = 0;
    uint16_t heading_56 = 0;
    int16_t turn_66 = 0;
    int16_t work_36c = 0;
};

struct Class20_03ActionInputs {
    // Original work+80 owner and atan helper are separate parity gates.
    std::optional<uint32_t> work_80;
    std::optional<uint32_t> packet_33c;
    std::optional<bool> predicate_8001da58;
    std::optional<uint16_t> target_heading_80031198;
};

enum class Class20_03ActionStatus : uint8_t {
    SourceBackedPartial,
    UnknownAction,
    NeedsWork80,
    NeedsPacket,
    NeedsPredicate,
    NeedsTargetHeading
};

// Translates only original action0 (80102884) and action1 (801029B4).
// This is NOT callback8010226C or a complete AI tick: detection, hit packet
// delivery, motion/collision, generic animation advancement and child attacks
// remain separate. Missing required evidence inputs leave state unchanged.
Class20_03ActionStatus class20_03_step_action(Class20_03BehaviorState& state,
                                           const Class20_03ActionInputs& inputs);

// Exact integer clamp/sign part of80031BEC, given the result of80031198.
// Does not approximate the original atan helper from world positions.
int16_t class20_03_turn_step(uint16_t target_heading, uint16_t heading,
                           int16_t source_limit);

//8003606C's class20 loop computes work+80 for the first player's raw X/Z.
// Signed differences are shifted before squaring; using float distance or
// squaring first changes odd/negative coordinates and the saturation edge.
uint32_t class20_03_distance_work80(int16_t actor_x, int16_t actor_z,
                                   int16_t player_x, int16_t player_z);

// Source80103FAC consumes the low four bits of an explicitly supplied source
// RNG sample. Random ownership/seeding is not implemented by this function.
void class20_03_select_action(Class20_03BehaviorState& state,
                             uint16_t source_random_32a);

// Source-used work fields for a bounded state1/callback transaction. The four
// raw/fixed/angle lanes retain the complete original copy sizes. heading_56 in
// behavior is authoritative for angle_54[1]. Unknown combat ownership remains
// optional rather than becoming an assumed health/statistics selector.
struct Class20_03CallbackState {
    Class20_03BehaviorState behavior;
    uint8_t work_00 = 0;
    uint8_t state_08 = 1;
    uint8_t aux_0b = 0;
    uint8_t flags_0c = 0;
    std::optional<int8_t> source_delay_0d;
    uint8_t restore_action_0e = 0;
    uint8_t previous_animation_ad = 0;
    std::array<int16_t, 4> raw_14{};
    std::array<int32_t, 4> fixed_1c{};
    std::array<uint16_t, 4> angle_54{};
    std::array<int16_t, 4> previous_raw_2c{};
    std::array<int32_t, 4> previous_fixed_34{};
    std::array<uint16_t, 4> previous_angle_5c{};
    std::optional<int16_t> work_64;
    std::array<int16_t, 4> proxy_position_180{};
    std::array<int16_t, 4> previous_proxy_position_334{};
    std::optional<int16_t> health_6e;
    std::optional<int16_t> initial_health_b0;
    std::optional<int16_t> previous_health_b2;
    uint16_t work_15e = 0;
    uint16_t random_32a = 0;
    int16_t variant_32e = 0;
    int16_t work_332 = 0;
    uint32_t packet_33c = 0;
    uint32_t hit_flags_340 = 0;
    std::array<int16_t,3> hit_point_344{};
    uint16_t hit_word_34a = 0;
    std::optional<uint8_t> detection_profile_352;
    std::array<int16_t, 3> detection_origin_34c{};
    uint8_t detection_phase_353 = 0;
    std::array<int16_t, 3> detection_vector_354{};
    uint8_t detection_countdown_35a = 0;
    int8_t detection_offset_35b = 0;
    int16_t work_35c = 0;
    int16_t work_35e = 0;
    int16_t defeat_timer_360 = 0;
    int16_t work_362 = 0;
    uint16_t work_36e = 0;
    uint32_t work_370 = 0;
    uint32_t child_374 = 0;
    uint32_t defeat_effect_368 = 0;
    // Reaction actions own these source fields; the actor bridge must retain
    // them with its wall-work velocity and keyed source table identity.
    std::array<int16_t,3> velocity_44{};
    // Action5 gravity step (+0x4E).
    int16_t work_4e = 0;
    std::optional<uint32_t> reaction_table_364;
};

enum class Class20_03CallbackStatus : uint8_t {
    SourceBackedPartial,
    UnknownState,
    UnknownAction,
    NeedsRandomSample,
    NeedsDetection,
    NeedsPacket,
    NeedsPredicate,
    NeedsWork80,
    NeedsTargetHeading,
    NeedsAction2Motion,
    NeedsAnimationCue,
    NeedsHealth,
    NeedsCombatProfile,
    NeedsFloor,
    NeedsHitConsumer,
    NeedsSoundConsumer,
    NeedsAnimationConsumer,
    NeedsEffectConsumer,
    NeedsProxyConsumer,
    NeedsDefeatLifecycle,
    NeedsSourceWork,
    NeedsAction3Motion,
    NeedsReleaseConsumer,
    NeedsAction9Motion,
    NeedsPlayerHeading,
    NeedsAction4Motion,
    NeedsChildConsumer,
    NeedsReactionMotion,
    NeedsLateActionMotion
};

enum class Class20_03AnimationOperation : uint8_t { None, Restart, Advance };

struct Class20_03CallbackResult {
    Class20_03CallbackStatus status = Class20_03CallbackStatus::SourceBackedPartial;
    Class20_03AnimationOperation animation = Class20_03AnimationOperation::None;
    bool effect_8002c8cc = false;
    bool proxy_800341bc = false;
    bool defeat_801027cc = false;
    std::optional<uint16_t> sound_8001a050;
    std::optional<uint16_t> secondary_sound_8001a050;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
    std::optional<std::array<uint32_t,2>> secondary_sound_pointed_words;
    std::optional<uint16_t> tertiary_sound_8001a050;
    std::optional<std::array<uint32_t,2>> tertiary_sound_pointed_words;
    // Wall wrapper byte74 after an in-body move (actions 5, 6, 10).
    std::optional<uint8_t> contact_state74;
};

// Source movement/contact preparation produces these work values after wall
// correction and before the action's floor stores. The larger actor transaction
// must retain/commit the wall wrapper's velocity/byte74 changes alongside them.
struct Class20_03PreparedAction2 {
    std::array<int16_t, 4> raw_14{};
    std::array<int32_t, 4> fixed_1c{};
    int32_t source_floor_word = 0;
    uint32_t source_contact_word = 0;
    std::optional<int8_t> source_a6;
};

struct Class20_03PreparedReaction {
    std::array<int16_t,4> entry_raw_14{},raw_14{};
    std::array<int32_t,4> entry_fixed_1c{},fixed_1c{};
    std::array<int16_t,3> velocity_before_wall{},velocity_after_wall{};
    uint16_t heading=0;
    int16_t forward=0,vertical=0;
    uint32_t contact_word=0;
};

struct Class20_03CallbackInputs {
    // These are source-owned or explicitly external premises. They are never
    // generated by this reducer. Full callback requires one BIOS sample even
    // when its low bits are not used by the current action.
    std::optional<uint16_t> source_random_sample;
    std::optional<bool> detection_8003648c;
    // A proved out-of-range result also resets the detection phase. An admitted
    // range still requires the complete detection helper or an external result.
    std::optional<uint8_t> detection_range_byte2;
    Class20_03ActionInputs action;
    std::optional<Class20_03PreparedAction2> prepared_action2;
    const SourceReactionProfile* reaction_profile=nullptr;
    std::optional<Class20_03PreparedReaction> prepared_reaction;
    std::optional<uint8_t> reaction_combat_byte_b;
    std::optional<int16_t> reaction_combat_record_variant;
    // Original action3 dependencies; no default statistics selector or motion.
    std::optional<uint8_t> combat_record_byte_a;
    struct Action3Motion {
        std::array<int16_t,4> raw_14{};
        std::array<int32_t,4> fixed_1c{};
        int32_t source_floor_word = 0;
        uint32_t source_contact_word = 0;
        uint16_t heading_after_turn = 0;
    };
    std::optional<Action3Motion> prepared_action3;
    std::optional<uint8_t> combat_record_byte_c;
    // Exact selected hit record and packet context. Older L3CB packages leave
    // byte09 absent; a base record is never borrowed by another signed variant.
    std::optional<uint8_t> combat_record_byte09;
    std::optional<int16_t> hit_record_variant_32e;
    std::optional<uint16_t> hit_word_34a;
    std::optional<uint32_t> hit_angle_80031198;
    struct Action4Motion {
        std::array<int16_t,4> raw_14{};
        std::array<int32_t,4> fixed_1c{};
        int32_t source_floor_word = 0;
        int16_t source_forward = 0;
        uint16_t heading = 0;
    };
    std::optional<Action4Motion> prepared_action4;
    // Scalar reducer input only. The native actor bridge clears this value:
    // allocation alone does not establish a working child callback consumer.
    std::optional<uint32_t> produced_child_374;
    // Action9 uses two angle calls with opposite argument order and the raw
    // unsigned player heading. Neither value is a camera-facing substitute.
    std::optional<uint16_t> angle_player_to_actor_80031198;
    std::optional<uint16_t> player_heading_800b5206;
    struct Action9Motion {
        std::array<int16_t,4> raw_14{};
        std::array<int32_t,4> fixed_1c{};
        int32_t source_floor_word = 0;
        uint32_t source_contact_word = 0;
        uint16_t heading_after_turn = 0;
        std::optional<int8_t> source_a6;
    };
    std::optional<Action9Motion> prepared_action9;
    // Actions 5/6/10 move inside the body (class20_03_late_actions.h).
    const Class20_03MotionWorld* motion_world = nullptr;
    bool release_consumer_ready = false;
    // Exact source defeat tail801027CC..80102848 requires one transaction
    // owning effect allocation, class20 release, reward dispatch and sound161.
    bool defeat_consumer_ready = false;
    uint32_t defeat_effect_identity = 0;
    // Readiness acknowledges excluded consumers; it does not implement them.
    bool sound_consumer_ready = false;
    bool animation_consumer_ready = false;
    bool effect_consumer_ready = false;
    bool proxy_consumer_ready = false;
};

// Exact unsigned range rejection from8003648C. true proves false detection;
// false means the geometric/persistent detection path remains unresolved.
bool class20_03_detection_range_rejected(uint32_t work_80, uint8_t range_byte2);

struct Class20_03InitializeInputs {
    std::optional<int16_t> source_initial_health;
    std::optional<uint8_t> source_detection_profile;
    std::optional<int32_t> source_floor_word;
};

// Scalar stores from80102410, given independently owned HP/profile/floor.
// Excludes resource setup, pose consumer, allocation and statistics selection.
// Parameter40 selects the source +32Y/action8 path and does not need floor.
Class20_03CallbackStatus class20_03_initialize_scalar_state(
    Class20_03CallbackState& state, const Class20_03InitializeInputs& inputs);

// Common801026A0 tail only. No action/detection/hit/callback is implied. The
// random sample is required only when80103FAC is reached. Missing evidence or
// an expiring defeat lifecycle leaves every source-used field unchanged.
Class20_03CallbackResult class20_03_step_state1_tail(
    Class20_03CallbackState& state, std::optional<uint16_t> source_random_sample,
    bool defeat_consumer_ready = false, uint32_t defeat_effect_identity = 0);

//80102600 detection, bounded hit consumer, actions0/1 and prepared action2,
// common tail. Action2 never manufactures motion/contact/floor/animation cue.
// This intentionally excludes outer callback snapshot/RNG/pose/effect/proxy
// work and packet finalization. In particular, a missing unused BIOS sample
// can admit this source-used-fields subset without claiming a whole callback.
Class20_03CallbackResult class20_03_step_state1(
    Class20_03CallbackState& state, const Class20_03CallbackInputs& inputs);

//8010226C scalar callback wrapper around the bounded state1 reducer. Consumer
// calls are returned as explicit intents after readiness preflight; their
// excluded side effects remain external. Packet finalization commits last.
// Any failure leaves state completely unchanged and returns no consumer plan.
Class20_03CallbackResult class20_03_step_callback(
    Class20_03CallbackState& state, const Class20_03CallbackInputs& inputs);

} // namespace l3
