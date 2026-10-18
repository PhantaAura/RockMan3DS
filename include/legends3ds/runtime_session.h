#pragma once
#include "legends3ds/bottom_screen.h"
#include "legends3ds/mml_message.h"
#include "legends3ds/player_runtime.h"
#include "legends3ds/player_collision.h"
#include "legends3ds/player_contact_runtime.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/st00_event.h"
#include "legends3ds/save_state.h"
#include "legends3ds/tutorial_runtime.h"
#include "legends3ds/l3map.h"
#include "legends3ds/actor_detection.h"
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/scene_e0.h"
#include "legends3ds/source_main_loop.h"
#include "legends3ds/source_scene_actor40.h"
#include "legends3ds/source_scene_actor40_pose.h"
#include "legends3ds/source_scene_actor40_init.h"
#include "legends3ds/source_scene_actor40_update.h"
#include "legends3ds/source_effects.h"
#include "legends3ds/source_effect_dispatch.h"
#include "legends3ds/source_loop_snapshot.h"
#include "legends3ds/source_event_random_tape.h"
#include "legends3ds/source_event_package.h"
#include "legends3ds/source_event_command_package.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_map>
#include <array>

namespace l3 {
struct SourceSceneViewProfile;

struct RuntimeVec3i { int32_t x=0,y=0,z=0; };
struct SceneE0FirstUpdatePass {
    SceneE0Status status=SceneE0Status::NeedsFields;
    uint8_t initialized_count=0,updated_count=0,held_count=0;
    std::array<SceneE0Result,32> slots{};
    // Exact stores, CANDIDATE chronology: the full original main-phase/busy
    // gate is not owned by RuntimeSession. This service must be opt-in.
    bool diagnostic_chronology=true;
};

// Diagnostic replay initialization, never a save/progression contract. These
// scalar fields are sufficient to start a measured ordinary-locomotion window
// without exposing a writable Player_work object or importing executable data.
struct RuntimePlayerReplaySeed {
    RuntimeVec3i position{};
    std::array<int32_t,3> fixed_position{};
    uint16_t yaw = 0;
    std::array<uint8_t,3> state{{1,0,1}};
    std::array<int16_t,3> velocity_work{};
    uint8_t work_0b = 0, contact_state = 0;
    uint8_t animation_request = 0, animation_pose = 0;
    uint8_t animation_countdown = 1, animation_record_byte2 = 0;
    int8_t animation_signed_control = 0;
    int8_t direction = 0, previous_direction = 0;
    int16_t direction_yaw_offset = 0, turn_accumulator = 0;
};

struct RuntimeInputFrame {
    uint16_t held_psx_mask = 0;
    bool fire_pressed = false;
    bool lock_pressed = false;
    bool lock_held = false;
    bool advance_dialogue = false;
    bool pause_pressed = false;
    bool skip_held = false;
    // Platform-delivered button edges retained between fixed gameplay updates.
    // This feeds the original +11E field without pretending the key is held.
    uint16_t pressed_psx_mask = 0;
};

struct RuntimeTarget {
    uint32_t id = 0;
    RuntimeVec3i position{};
    uint16_t radius = 0x40;
    int32_t hp = 1;
    bool active = true;
    bool targetable = true;
};

struct RuntimeProjectile {
    uint32_t serial = 0;
    RuntimeVec3i position{};
    RuntimeVec3i velocity{};
    uint16_t age = 0;
    uint16_t lifetime = 60;
    uint16_t radius = 0x20;
    int16_t damage = 1;
    bool active = true;
};

struct RuntimeCombatTuning {
    // These values live outside Classic compatibility work on purpose.  They are
    // integration defaults until their owning original routines are translated;
    // parity code can replace them without changing the session API.
    int32_t projectile_speed = 0x80;
    uint16_t projectile_lifetime = 60;
    uint16_t projectile_radius = 0x20;
    int16_t projectile_damage = 1;
    uint16_t max_projectiles = 8;
    int32_t lock_range = 0x00D0; // retail USA lock acceptance threshold (< 0xD0)
};

struct RuntimeCameraPose {
    RuntimeVec3i eye{};
    RuntimeVec3i look_at{};
    uint16_t yaw = 0;
};

enum class RuntimeCameraOwner : uint8_t { Gameplay, Script };

struct RuntimeCameraState {
    RuntimeCameraOwner owner = RuntimeCameraOwner::Gameplay;
    RuntimeCameraPose current{};
    RuntimeCameraPose start{};
    RuntimeCameraPose target{};
    uint16_t transition_frame = 0;
    uint16_t transition_frames = 0;
};

struct RuntimeAudioRequest {
    MessageRef source{};
    std::vector<uint8_t> args;
    uint32_t command_id = 0;
};


struct RuntimeScriptRequest {
    MessageRef source{};
    uint8_t opcode = 0;
    MmlRuntimeEventKind kind = MmlRuntimeEventKind::Control;
    std::vector<uint8_t> args;
};

struct RuntimeDialogueState {
    bool active = false;
    bool waiting_for_advance = false;
    bool waiting_for_script = false;
    bool waiting_for_audio = false;
    uint32_t waiting_audio_id = 0;
    uint16_t wait_frames = 0;
    MessageRef ref{};
    std::vector<uint8_t> visible_glyphs;
    std::vector<RuntimeAudioRequest> audio_requests;
    std::vector<RuntimeScriptRequest> script_requests;
};

struct RuntimeCheckpoint {
    RuntimeSaveState save{};
};
struct RuntimeCutsceneCompletionContract {
    uint32_t id = 0;
    std::vector<uint16_t> flags_on;
    std::vector<uint16_t> flags_off;
    std::optional<size_t> destination_location;
    RuntimeVec3i destination_position{};
    uint16_t destination_yaw = 0;
    bool buster_enabled = true;
    bool stop_dialogue = true;
};


struct RuntimeAuxTrigger {
    size_t record_index = 0;
    St00AuxMatchKind kind = St00AuxMatchKind::None;
    St00ContactSample sample{};
};


struct RuntimeLocationRegistration {
    RuntimeMap map;
    size_t map_area_index = 0;
    TutorialRuntimeArea area;
};

struct RuntimeRetailContactResolution {
    int32_t result = 0;
    L3RetailContactScratch scratch{};
};

struct RuntimeMapProbe {
    bool in_area = false;
    std::optional<RuntimeCellRef> cell;
    const RuntimeMapIdx* idx = nullptr;
    const RuntimeMapDescriptor* descriptor = nullptr;
    std::optional<uint16_t> contact_list_index;
    std::vector<RuntimeContactGroupRef> contact_groups;
};

// Portable integration session.  It deliberately composes proved compatibility
// helpers and cooked disc-derived data but keeps still-unproved combat/camera
// constants in explicit tuning structs.  That lets parity research replace one
// subsystem at a time without creating a second gameplay implementation.
class RuntimeSession {
public:
    const ActorDetectionGlobals& actor_detection_globals() const noexcept {return actor_detection_globals_;}
    void notify_actor_detection() noexcept {notify_actor_detection_8005d4c0(actor_detection_globals_);}
    // Explicit original fresh-game initialization; generic sessions and restored
    // saves do not infer the selector from an authored/native save field.
    void initialize_fresh_source_statistics_8001326c() noexcept {source_stats_selector_=1;}
    const std::optional<uint8_t>& source_stats_selector() const noexcept {return source_stats_selector_;}
    SourceAudioQueue& source_audio_queue() noexcept {return source_audio_queue_;}
    const SourceAudioQueue& source_audio_queue() const noexcept {return source_audio_queue_;}
    // Explicit source-owner bindings. Loading a poll package alone does not
    // invent original initial work, flag values, transition or busy globals.
    void bind_source_event_profile(SourceEventPollProfile profile,int8_t source_stage);
    void bind_source_event_state(SourceEventWork work,SourceEventFlagSnapshot flags,
        std::optional<int16_t> player_hp,std::optional<int8_t> transition,
        std::optional<int16_t> busy) noexcept;
    void update_source_event_globals(std::optional<int16_t> player_hp,
        std::optional<int8_t> transition,std::optional<int16_t> busy) noexcept;
    const std::optional<SourceEventWork>& source_event_work() const noexcept {return source_event_work_;}
    const SourceEventStepResult& source_event_result() const noexcept {return source_event_result_;}
    const SourceEventFlagSnapshot& source_event_flags() const noexcept {return source_event_flags_;}
    // Explicit fresh ST00/player/logical-driver prefixes only. This diagnostic
    // entry does not execute the full original NewGame/fade/cutscene sequence.
    bool initialize_fresh_source_event_subset_st00() noexcept;
    void bind_source_event_consumers(SourceEventPlayerKnown player_known,
        SourceEventDriverContext driver,SourceEventDriverIntentQueue intents,
        std::optional<uint8_t> byte_800c1b93={}) noexcept;
    const SourceEventDriverIntentQueue& source_event_driver_intents() const noexcept {return source_event_driver_intents_;}
    const SourceEventPlayerKnown& source_event_player_known() const noexcept {return source_event_player_known_;}
    void bind_source_event_phase2_package(SourceEventPhase2Package package) noexcept;
    const SourceEventCommandQueue& source_event_command_queue() const noexcept {return source_event_commands_;}
    std::optional<SourceEventCommandRequest> source_event_command_request() const noexcept {
        return source_event_command_front_request(source_event_commands_);
    }
    // Metadata admits cold source fields only. Transport receipts must come
    // from a real owning backend; packet acceptance never implies CD success.
    void bind_source_event_command_package(SourceEventCommandPackage package) noexcept;
    bool deliver_source_event_cd_receipts(const SourceEventCommandInputs& inputs) noexcept;
    size_t source_event_cd_receipts_pending() const noexcept {return source_event_cd_inputs_.receipt_count;}
    const SourceEventCommandResult& source_event_command_result() const noexcept {return source_event_command_result_;}
    const SourceEventCommandContext& source_event_command_context() const noexcept {return source_event_command_context_;}
    const SourceEventCdMixQueue& source_event_cd_mix_intents() const noexcept {return source_event_cd_mix_intents_;}
    bool acknowledge_source_event_cd_mix(size_t count) noexcept;
    // Profile binds startup only; pending scheduler intents do not acknowledge
    // original BIOS callback execution or fade presentation/completion.
    bool bind_source_fade_profile(SourceFadeStartupProfile profile) noexcept;
    const SourceFadeContext& source_fade_context() const noexcept { return source_fade_context_; }
    const SourceFadeSchedulerIntentQueue& source_fade_intents() const noexcept { return source_fade_intents_; }
    // Explicit native scheduler pass; never a PS1 BIOS thread receipt or a
    // guessed wall-clock fade duration. Fresh ST00 owns the source budget2
    // initialization; unsupported ownership must invalidate that byte.
    SourceFadeCallbackResult service_source_fade_scheduler() noexcept;
    void bind_source_fade_scheduler_budget(std::optional<uint8_t> value) noexcept {
        source_fade_endpoint_hold_=value;
    }
    void invalidate_source_fade_scheduler_budget() noexcept { source_fade_endpoint_hold_.reset(); }
    const SourceFadeCallbackState& source_fade_callback_state() const noexcept { return source_fade_callback_; }
    SourceFadePresentation source_fade_presentation() const noexcept {
        return describe_source_fade_presentation(source_fade_context_);
    }
    void bind_source_event_phase5_profile(SourceEventPhase5Profile profile) noexcept;
    SourceEventPhase5ResourceStatus bind_source_event_phase5_resources() noexcept;
    const SourceEventPhase5ResourceBinding& source_event_phase5_resources() const noexcept { return source_event_phase5_resources_; }
    const SourceEventPhase5Context& source_event_phase5_context() const noexcept { return source_event_phase5_context_; }
    const SourceEventPhase6Context& source_event_phase6_context() const noexcept { return source_event_phase6_context_; }
    const SourcePropForeignGlobals& source_prop_foreign_globals() const noexcept { return source_prop_foreign_globals_; }
    // A source-produced request is retained until the destination preparation
    // commits. This never supplies missing display/CD/heading ownership.
    std::optional<SourceEventHandoffTransitionIntent> source_area_handoff() const noexcept;
    bool can_commit_source_area_handoff() const noexcept;
    // Prepare the destination before publishing the exact original spawn
    // stores. Failed preparation retains the source intent and player work.
    bool commit_source_area_handoff();
    // Expose a renderer work request only after the original phase-7 caller
    // reaches its missing head-UV reset owner. A frame counter is not a lease.
    std::optional<uint64_t> source_handoff_display_generation() const noexcept;
    bool bind_source_handoff_display_receipt(const SourceEventHandoffDisplayReceipt&) noexcept;

    bool bind_source_event_phase6_profile(SourceEventPhase6Profile profile) noexcept;
    std::optional<SourceEventSceneOverlayPresentation> source_event_scene_overlay() const noexcept;
    bool acknowledge_source_event_scene_overlay(SourceEventSceneOverlayLease lease) noexcept;
    SceneE0FirstUpdatePass service_diagnostic_scene_e0_first_updates(const SceneE0Resources& resources) noexcept;
    // Diagnostic original callee sweep; main-loop scheduling remains CANDIDATE.
    SceneE0FirstUpdatePass service_diagnostic_scene_e0_updates(const SceneE0Resources& resources,SourceRngService* candidate_rng=nullptr) noexcept;
    bool bind_source_prop_lift_profile(SourcePropLiftProfile profile) noexcept;
    bool initialize_source_scene_projection_from_executable_boot(const SourceSceneViewProfile& profile) noexcept;
    // Explicit source snapshot admission only. The native frontend must not
    // supply its frame counter or normalized controller buttons as this owner.
    void bind_source_main_loop_half_snapshot_1f800002(uint16_t raw) noexcept;
    bool bind_source_loop_snapshot_replay(const SourceLoopSnapshot&) noexcept;
    const SourceLoopSnapshot* source_loop_snapshot_replay() const noexcept {return source_loop_snapshot_?&*source_loop_snapshot_:nullptr;}
    bool source_loop_snapshot_admitted() const noexcept {return source_loop_snapshot_admitted_;}
    SourceLoopSnapshotStatus source_loop_snapshot_status() const noexcept {return source_loop_snapshot_status_;}
    bool bind_source_scene_rng_replay(const SourceEventRandomTapeProfile&) noexcept;
    bool source_recorded_scene_admitted() const noexcept {return source_loop_snapshot_admitted_||source_scene_rng_constructor_admitted_;}
    const SourceEventRandomTapeService& source_scene_rng_replay() const noexcept {return source_scene_rng_tape_;}
    SourceEventRandomTapeStatus source_scene_rng_replay_status() const noexcept {return source_scene_rng_tape_status_;}
    SourceMainLoopPhaseResult source_main_loop_recorded_phase_gate() const noexcept;
    bool service_source_main_loop_tail_80011ecc() noexcept;
    SourceSceneActor40PrefixResult service_diagnostic_scene_actor40_prefix(const SourceSceneActor40Resources&) noexcept;
    SourceSceneActor40PoseCommit service_diagnostic_scene_actor40_initial_pose(const SourceSceneActor40Resources&) noexcept;
    SourceSceneActor40InitializerResult service_diagnostic_scene_actor40_initializer(const SourceSceneActor40Resources&) noexcept;
    SourceSceneActor40SequenceTick service_diagnostic_scene_actor40_sequence(const SourceSceneActor40Resources&) noexcept;
    SourceSceneActor40LaterResult service_diagnostic_scene_actor40_update(const SourceSceneActor40Resources&) noexcept;
    SourceEffectPhase5Batch service_diagnostic_scene_effects() noexcept;
    bool bind_source_effect_dispatch_profile(SourceEffectDispatchProfile) noexcept;
    SourceEffectDispatchBatch service_diagnostic_scene_lower_effects(
        const SourceEffectDispatchInputProvider& = {}) noexcept;
    SourceEffectPhase5Batch service_diagnostic_scene_effect_releases() noexcept;
    const SourceSceneActor40InitializerGlobals& source_scene_actor40_initializer_globals() const noexcept {return source_scene_actor40_initializer_globals_;}
    void initialize_source_main_loop_pads_from_executable_boot_800177cc() noexcept;
    // Platform digital-packet translation; original decoder/repeat is separate.
    // Latches delivered edges for one simulation tick, never seeds scratch2.
    SourceMainLoopPadResult service_source_main_loop_digital_pad_adapter(uint16_t held,uint16_t pressed) noexcept;
    const SourceMainLoopPads& source_main_loop_pads() const noexcept {return source_main_loop_pads_;}
    const SourceMainLoopPadResult& source_main_loop_pad_result() const noexcept {return source_main_loop_pad_result_;}


    RuntimeSession(RuntimeMap map,
                   TutorialRuntimeArea area,
                   std::vector<MmlMessageFile> message_banks,
                   L3QolSettings settings);
    RuntimeSession(const RuntimeSession&) = default;
    RuntimeSession(RuntimeSession&&) = default;
    RuntimeSession& operator=(RuntimeSession&&) = default;
    // Prepare the full copy before replacing any cursor/bank or scene owner.
    RuntimeSession& operator=(const RuntimeSession& other);

    void reset_player(RuntimeVec3i position, uint16_t yaw, uint16_t hp, uint16_t hp_max);
    // Candidate diagnostic grid-boundary recovery, not retail physics. Updates
    // position mirrors and the collision snapshot without reloading the area or
    // resetting action, velocity, animation, encounter or queued scene state.
    // Rejects positions outside signed16 storage or the active map area.
    bool recover_player_position_diagnostic(RuntimeVec3i position);
    // Rejects inconsistent mirrors and action states outside the translated
    // ordinary 0/1/2/4 subset. Contact and wider gameplay parity stay unproved.
    bool seed_player_replay_diagnostic(const RuntimePlayerReplaySeed& seed);
    void set_area(size_t area_index, TutorialRuntimeArea area);
    void register_area(size_t area_index, TutorialRuntimeArea area);
    bool transition_to_area(size_t area_index, RuntimeVec3i position, uint16_t yaw);
    bool has_registered_area(size_t area_index) const;
    // Non-owning area lookup for native resource preparation. Invalidated by
    // registry mutation; it does not switch collision, events or player state.
    const TutorialRuntimeArea* registered_area(size_t area_index) const;
    // Global location ids decouple save/progression identity from a map's local area index.
    // This is the path used by multi-stage packs; the tutorial wrappers above remain compatible.
    void register_location(size_t location_id, RuntimeMap map, size_t map_area_index, TutorialRuntimeArea area);
    bool transition_to_location(size_t location_id, RuntimeVec3i position, uint16_t yaw);
    bool has_registered_location(size_t location_id) const;
    // Removing an inactive location releases its registry ownership of the map
    // and area. The active location cannot be removed; saves for removed ids
    // require re-registration before they can be restored.
    bool unregister_location(size_t location_id);
    // Returns the number of location registrations removed. Shared active-map
    // storage stays alive while any session or other registration still owns it.
    size_t release_inactive_locations();
    size_t map_area_index() const { return map_area_index_; }
    // Read-only map inspection; references must not outlive a location change or this session.
    const RuntimeMap& active_map() const { return *map_; }
    const TutorialRuntimeArea& active_area() const { return area_; }
    // Non-owning observation for memory-lifecycle diagnostics. Merely retaining
    // this observer does not prevent an unloaded map from being freed.
    std::weak_ptr<const RuntimeMap> active_map_lifetime() const { return map_; }
    void set_objective(std::optional<MessageRef> objective);
    void set_reference_tables(ReferenceTables tables);
    bool has_reference_tables() const { return reference_tables_.has_value(); }

    // Feeds the exact ST00 aux matcher with a collision/contact sample.  Matching
    // is data-driven; unproved event side effects stay queued for the script layer.
    void submit_contact_sample(const St00ContactSample& sample,
                               const St00Type3ObjectBounds* type3_bounds = nullptr);
    std::vector<RuntimeAuxTrigger> take_aux_triggers();
    RuntimeMapProbe probe_player_map() const;
    // Applies the exact post-solver position/rollback mutation from USA 0x8004B414.
    // Geometry discovery remains separate, so callers cannot accidentally turn a
    // guessed surface interpretation into retail behavior.
    int32_t apply_retail_contact_resolution(const RuntimeRetailContactResolution& resolution);
    const RuntimePlayerContact& player_contact() const { return player_contact_; }
    // Actual S-record state, not an invented launch delay. The source writes
    // record bytes directly to +A4..A7 (0x8003219C/0x800323B0). Returns false
    // for stale requests, so a previous animation cannot launch a new jump.
    bool submit_player_animation_record(uint8_t request, uint8_t pose_frame,
                                        uint8_t countdown, uint8_t record_byte2,
                                        int8_t signed_control);

    void register_target(RuntimeTarget target);
    void clear_targets();
    const std::vector<RuntimeTarget>& targets() const { return targets_; }
    const std::vector<RuntimeProjectile>& projectiles() const { return projectiles_; }
    std::optional<uint32_t> locked_target_id() const;

    void start_dialogue(MessageRef ref);
    void stop_dialogue();
    const RuntimeDialogueState& dialogue() const { return dialogue_; }
    std::vector<RuntimeAudioRequest> take_audio_requests();
    std::vector<RuntimeScriptRequest> take_script_requests();
    void complete_audio(uint32_t command_id);
    void resume_dialogue_script_wait();

    void register_cutscene_completion(RuntimeCutsceneCompletionContract contract);
    void begin_cutscene(uint32_t contract_id);
    void end_cutscene();
    bool cutscene_active() const { return active_cutscene_id_.has_value(); }
    bool cutscene_skippable() const;

    void start_script_camera(const RuntimeCameraPose& target, uint16_t frames);
    void release_script_camera(uint16_t blend_frames);
    const RuntimeCameraState& camera() const { return camera_; }

    void tick(const RuntimeInputFrame& input,SourceRngService* candidate_rng=nullptr);
    // Source E0/00 gate/state2 subset. Records retain their retail fields;
    // callbacks, actor AI and group-destruction side effects are not implied.
    const StageObjectList& conditional_spawn_objects() const { return conditional_spawn_objects_; }
    uint32_t conditional_spawn_revision() const { return conditional_spawn_revision_; }
    int8_t conditional_spawn_qualifier() const { return conditional_spawn_qualifier_; }
    bool conditional_spawn_controller_active() const { return conditional_spawn_active_; }

    RuntimeVec3i player_position() const;
    uint16_t player_yaw() const;
    uint16_t hp() const { return hp_; }
    uint16_t hp_max() const { return hp_max_; }
    uint32_t frame_counter() const { return frame_counter_; }
    size_t area_index() const { return area_index_; }
    const L3PlayerCompatWork& compat_player() const { return player_; }
    // Exact byte owner used by the translated normal-Buster callback at
    // original player work +0x198. This does not assign broader semantics to
    // adjacent unknown work fields.
    std::uint32_t source_buster_busy_198() const noexcept {
        return std::uint32_t(player_.bytes[0x198])|
            (std::uint32_t(player_.bytes[0x199])<<8)|
            (std::uint32_t(player_.bytes[0x19a])<<16)|
            (std::uint32_t(player_.bytes[0x19b])<<24);
    }
    void set_source_buster_busy_198(std::uint32_t value) noexcept {
        for(std::size_t i=0;i<4;++i)player_.bytes[0x198+i]=std::uint8_t(value>>(i*8));
    }

    BottomScreenModel bottom_screen_model() const;
    BottomScreenLayout bottom_screen_layout() const;

    void set_story_flag(uint16_t bit, bool value);
    bool story_flag(uint16_t bit) const;
    RuntimeSaveState make_save_state(uint32_t checkpoint_id) const;
    void apply_save_state(const RuntimeSaveState& state);
    RuntimeCheckpoint make_checkpoint(uint32_t checkpoint_id) const;
    void restore_checkpoint(const RuntimeCheckpoint& checkpoint);

    // 0.0.15.0: when the source-backed combat owner (ChildRuntime Buster pool,
    // imported-actor hit/defeat) drives gameplay, the historical candidate
    // projectile and candidate lock-on must not run in parallel. Disabling it
    // drops queued candidate projectiles and any candidate lock. Host sessions
    // keep the candidate path enabled by default for existing regressions.
    void set_candidate_combat_enabled(bool enabled) noexcept {
        candidate_combat_enabled_=enabled;
        if(!enabled){projectiles_.clear();locked_target_index_.reset();}
    }
    bool candidate_combat_enabled() const noexcept { return candidate_combat_enabled_; }
    // Source player fields consulted before any Buster producer is admitted.
    bool player_shot_enabled() const noexcept { return l3_pl00_shot_is_enabled(&player_)!=0; }
    bool player_busy_8004324c() const noexcept { return l3_pl00_state_busy_8004324c(&player_)!=0; }

    RuntimeCombatTuning& combat_tuning() { return combat_tuning_; }
    const RuntimeCombatTuning& combat_tuning() const { return combat_tuning_; }

private:
    struct SharedLocationRegistration {
        std::shared_ptr<const RuntimeMap> map;
        size_t map_area_index = 0;
        TutorialRuntimeArea area;
    };

    void sync_primary_position_to_compat();
    void sync_primary_position_from_fixed();
    void update_player_motion();
    void integrate_player_motion(bool vertical_motion = true);
    void update_ordinary_player_motion();
    void update_running_player_motion();
    void update_player_contact();
    void update_airborne_motion();
    void reset_player_contact();
    void update_input_compat(const RuntimeInputFrame& input);
    void update_lock_on(const RuntimeInputFrame& input);
    void spawn_buster_projectile();
    void update_projectiles();
    void update_dialogue(const RuntimeInputFrame& input);
    void update_camera();
    void reset_conditional_spawns();
    void update_conditional_spawns();
    void update_gameplay_camera_target();
    std::optional<size_t> nearest_target_index() const;
    bool apply_active_cutscene_completion();

    // Tutorial registrations share this immutable allocation. Transitions and
    // save restoration only change ownership, never duplicate the map buffers.
    std::shared_ptr<const RuntimeMap> map_;
    TutorialRuntimeArea area_;
    size_t area_index_ = 0; // global location id persisted by L3SV
    size_t map_area_index_ = 0; // local area index inside the active L3MP
    std::unordered_map<size_t,TutorialRuntimeArea> area_registry_;
    std::unordered_map<size_t,SharedLocationRegistration> location_registry_;
    // Cursors borrow bank records. Copies share immutable banks so each copied
    // cursor keeps its own position without borrowing the source session's life.
    std::shared_ptr<const std::vector<MmlMessageFile>> message_banks_;
    L3QolSettings settings_{};
    L3PlayerCompatWork player_{};
    RuntimeVec3i player_position_{};
    RuntimePlayerContact player_contact_{};
    std::optional<uint8_t> player_animation_record_request_;
    std::optional<ReferenceTables> reference_tables_;
    uint16_t hp_=0,hp_max_=0;
    uint32_t frame_counter_=0;
    // Fresh subsystem initialization only; area replacement preserves globals.
    // Unported script setters and original pause/cadence remain separate gates.
    ActorDetectionGlobals actor_detection_globals_=initialize_actor_detection_globals_8005c258();
    std::optional<uint8_t> source_stats_selector_;
    SourceAudioQueue source_audio_queue_;
    std::shared_ptr<const SourceEventPollProfile> source_event_profile_;
    std::optional<int8_t> source_event_stage_,source_event_transition_;
    std::optional<int16_t> source_event_hp_,source_event_busy_;
    std::optional<SourceEventWork> source_event_work_;
    SourceEventFlagSnapshot source_event_flags_;
    SourceEventStepResult source_event_result_;
    std::optional<SourceFadeStartupProfile> source_fade_profile_;
    SourceFadeContext source_fade_context_{};
    SourceFadeSchedulerIntentQueue source_fade_intents_{};
    SourceFadeCallbackState source_fade_callback_{};
    std::optional<uint8_t> source_fade_endpoint_hold_;
    std::optional<SourceEventPhase5Profile> source_event_phase5_profile_;
    SourceEventPhase5Context source_event_phase5_context_{};
    SourceEventPhase5ResourceBinding source_event_phase5_resources_{};
    std::optional<SourceEffectDispatchProfile> source_effect_dispatch_profile_;
    std::optional<SourcePropLiftProfile> source_prop_lift_profile_;
    SourcePropForeignGlobals source_prop_foreign_globals_;
    SourceEffectDispatchLease source_effect_dispatch_lease_;
    SourceEventPhase6Context source_event_phase6_context_{};
    std::optional<SourceEventHandoffDisplayReceipt> source_handoff_display_;
    SourceSceneActor40InitializerGlobals source_scene_actor40_initializer_globals_{};
    std::optional<SourceLoopSnapshot> source_loop_snapshot_;
    bool source_loop_snapshot_admitted_=false;
    SourceLoopSnapshotStatus source_loop_snapshot_status_=SourceLoopSnapshotStatus::Disabled;
    SourceEventRandomTapeService source_scene_rng_tape_;
    bool source_scene_rng_constructor_admitted_=false;
    SourceEventRandomTapeStatus source_scene_rng_tape_status_=SourceEventRandomTapeStatus::Disabled;
    SourceMainLoopPads source_main_loop_pads_{};
    bool source_main_loop_pads_bound_=false;
    SourceMainLoopPadResult source_main_loop_pad_result_{};

    std::optional<SourceEventPhase6Profile> source_event_phase6_profile_;
    struct NativeOverlayGeneration {
        uint64_t value;
        NativeOverlayGeneration() noexcept;
        NativeOverlayGeneration(const NativeOverlayGeneration&) noexcept;
        NativeOverlayGeneration(NativeOverlayGeneration&&) noexcept;
        NativeOverlayGeneration& operator=(const NativeOverlayGeneration&) noexcept;
        NativeOverlayGeneration& operator=(NativeOverlayGeneration&&) noexcept;
    } source_event_scene_overlay_generation_;
    SourceEventPlayerKnown source_event_player_known_{};
    SourceEventDriverContext source_event_driver_{};
    SourceEventDriverIntentQueue source_event_driver_intents_{};
    std::optional<uint8_t> source_event_byte_800c1b93_;
    bool source_event_consumers_bound_=false;
    std::optional<SourceEventPhase2Package> source_event_phase2_package_;
    SourceEventPhase2Context source_event_phase2_context_{};
    SourceEventCommandQueue source_event_commands_{};
    SourceEventPhase2IntentQueue source_event_phase2_intents_{};
    bool source_event_phase2_bound_=false;
    std::optional<SourceEventCommandPackage> source_event_command_package_;
    SourceEventCommandContext source_event_command_context_{};
    SourceEventCommandInputs source_event_cd_inputs_{};
    SourceEventCommandResult source_event_command_result_{};
    SourceEventCdMixQueue source_event_cd_mix_intents_{};
    bool source_event_command_bound_=false;


    std::vector<RuntimeTarget> targets_;
    std::vector<RuntimeProjectile> projectiles_;
    uint32_t next_projectile_serial_=1;
    std::optional<size_t> locked_target_index_;
    RuntimeCombatTuning combat_tuning_{};
    bool candidate_combat_enabled_=true;
    RuntimeCameraState camera_{};
    RuntimeDialogueState dialogue_{};
    std::optional<MmlMessageRuntimeCursor> dialogue_cursor_;
    DialogueHistory history_{};
    std::optional<MessageRef> objective_;
    std::vector<RuntimeAuxTrigger> aux_triggers_;
    std::array<uint8_t,kL3StoryFlagBytes> story_flags_{};
    StageObjectList conditional_spawn_objects_;
    std::optional<size_t> conditional_spawn_pending_;
    int8_t conditional_spawn_qualifier_ = -1;
    bool conditional_spawn_active_ = false;
    uint32_t conditional_spawn_revision_ = 0;
    std::unordered_map<uint32_t,RuntimeCutsceneCompletionContract> cutscene_contracts_;
    std::optional<uint32_t> active_cutscene_id_;
    uint16_t cutscene_skip_hold_frames_ = 0;
};

} // namespace l3
