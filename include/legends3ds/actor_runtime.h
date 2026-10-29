#pragma once
#include "legends3ds/actor_resource.h"
#include "legends3ds/stage_runtime.h"
#include "legends3ds/player_pose.h"
#include "legends3ds/player_contact_runtime.h"
#include "legends3ds/source_angle.h"
#include "legends3ds/actor_animation_bank.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/actor_contact.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/actor_detection_profile.h"
#include "legends3ds/actor_action3_contact.h"
#include "legends3ds/actor_action9_contact.h"
#include "legends3ds/actor_action4_contact.h"
#include "legends3ds/actor_reaction_contact.h"
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/source_combat_profile.h"
#include "legends3ds/actor_detection_preparation.h"
#include "legends3ds/actor_pose_runtime.h"
#include "legends3ds/class20_03_proxy.h"
#include "legends3ds/source_actor_consumers.h"
#include "legends3ds/class20_1f_popout.h"
#include "legends3ds/class20_19.h"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace l3 {
class ChildRuntime;

struct ActorInitializerProfile {
    uint32_t source_initializer = 0;
    std::array<int16_t, 3> raw_scale{}; // Q12 model scale used by the initializer.
    uint16_t raw_shade = 0;
    bool raw_shade_verified = false;
    bool selects_first_class_subtype_row = false;
    bool floor_query_required = false;
    bool floor_query_uses_zero_bounds = true;
    std::optional<RetailPlayerBounds> floor_bounds;
    std::optional<uint8_t> initial_pose_request;
    uint8_t initial_pose_record = 0;
};

// Supported initializer evidence is keyed by class/subtype/parameter/dispatch,
// not room number. Unproved dispatch paths have no profile and do not spawn.
std::optional<ActorInitializerProfile> actor_initializer_profile_for(
    const StageRuntimeObject& object);

// Fresh class20/03 stats index0 only. The raw global selector at 0x800C1B6B
// chooses table 0x8008F060 when equal to2, otherwise 0x8008EDC4. Neither input
// defaults to an assumed game state. 0x80102528 multiplies the HP by
// ((context & 0x7F) + 1): context 0 and 0x80 are admitted, any low bit holds.
std::optional<int16_t> class20_03_initial_hp(uint8_t raw_stats_selector,
                                          uint8_t raw_context_high_byte);

enum class ActorSpawnState { UNSUPPORTED_INITIALIZER, RESOURCE_UNAVAILABLE,
                             RESOURCE_REJECTED, MESH_BOUND };
struct ActorSpawnDiagnostic {
    size_t source_index = 0;
    ActorSpawnState state = ActorSpawnState::UNSUPPORTED_INITIALIZER;
    uint32_t initializer = 0;
    uint32_t row = UINT32_MAX;
    std::string reason;
};

struct ImportedActorInstance {
    size_t source_index = 0;
    size_t original_pool_slot = 0;
    uint8_t source_class = 0, subtype = 0;
    uint32_t initializer = 0, row = 0;
    std::array<int16_t, 3> source_origin{};
    uint16_t heading = 0; // parser raw byte +7 <<4, 4096 units per turn.
    std::array<int16_t, 3> raw_scale{};
    uint16_t raw_shade = 0;
    bool raw_shade_verified = false;
    bool floor_query_required = true;
    bool floor_query_uses_zero_bounds = true;
    std::optional<RetailPlayerBounds> floor_bounds;
    bool floor_sample_available = false;
    bool pose_selected_by_retail_state = false;
    uint8_t source_dispatch_flag0 = 0, source_dispatch_flag6 = 0;
    uint8_t source_context_high_byte = 0;
    std::shared_ptr<const ActorMeshResource> resource;
    std::optional<ActorPoseResource> pose;
    std::shared_ptr<const ActorPoseSequenceCache> request0_pose_cache;
    std::optional<ActorSequenceCursor> request0_sequence;
    std::shared_ptr<const ActorAnimationBank> animation_bank;
    std::optional<ActorSequenceCursor> animation_cursor;
    std::optional<Class20_03CallbackState> source_behavior;
    bool source_behavior_initialized = false;
    Class20_03CallbackStatus last_behavior_status = Class20_03CallbackStatus::NeedsFloor;
    ActorDetectionStatus last_detection_status = ActorDetectionStatus::NeedsProfile;
    std::optional<ActorDetectionPreparationStatus> last_detection_preparation_status;
    uint8_t last_source_audio_queued = 0;
    uint8_t last_source_audio_dropped = 0;
    std::optional<uint8_t> source_combat_byte_a;
    std::optional<uint8_t> source_combat_byte_c;
    std::optional<uint8_t> source_combat_byte09;
    std::optional<uint8_t> source_combat_byte_b;
    std::optional<uint32_t> source_reward_word00;
    std::array<int16_t, 3> source_contact_velocity{};
    uint8_t source_contact_state74 = 0;
    // Explicit fresh source pool/init subset receipts. Kept unknown by default
    // and on reload; recycled source fields cannot be inferred from a mesh.
    std::optional<uint8_t> source_pose_scratch_shift;
    std::optional<int16_t> source_pose_preserved_186;
    std::optional<int8_t> source_shadow_work76;
    SourceHitProxyTarget source_proxy_target{};
    bool source_proxy_bound=false;
    // Native mirror of source80031824 class20 pool release. A released actor
    // remains as an area-local tombstone so it cannot respawn or reward twice.
    bool source_released=false;
    bool source_release_reported=false; // E0/00 defeated flag already raised
    // ST00 subtype 0x1F (callback 0x801062EC) until it rewrites itself into
    // subtype 3 parameter 0x40. Work +0C..+0E are kept for that handoff.
    std::optional<Class20_1FState> source_popout;
    std::array<uint8_t, 3> source_work_0c_0e{};
    std::array<uint16_t, 4> source_angle_54{};
    Class20_1FStatus last_popout_status = Class20_1FStatus::SourceBackedPartial;
    // ST00 subtype 0x19 (callback 0x80104864): the trap-room Reaverbots.
    std::optional<Class20_19State> source_reaverbot19;
    std::optional<Class20_19Record> source_record19;
    std::optional<uint16_t> source_floor_sample;      // 0x80048E6C at init
    ActorDetectionWork source_detection19{};          // +0x36C (profile at +0x372)
    std::array<int16_t,4> source_point_180{};         // last pose point (proxy/reward)
    bool source_point_180_known = false;
    bool source_hidden = false;                       // work +0 bit 1 clear
    Class20_19Status last_reaverbot19_status = Class20_19Status::SourceBackedPartial;
    // ST00 Area 3 event 3 owns a dynamic class40/02 boss rather than a placed
    // class40 record. AI/damage/defeat remain separate evidence gates.
    struct SourceArea3BossState {
        int16_t hp = 0;
        uint8_t action = 0, phase = 0, substate = 0;
        uint8_t requested_pose = 0;
        uint16_t phase_ticks = 0;
        // 0: wait for the scripted player entry; 1: phase3 turn; 2: slow
        // approach; 3: fast approach; 4: action6 handoff; 5: first action6
        // presentation cycle; 6: next action1 selection boundary.
        uint8_t intro_leg = 0;
    };
    std::optional<SourceArea3BossState> source_area3_boss;
};

struct ActorAreaRuntime {
    std::vector<ImportedActorInstance> actors;
    std::vector<ActorSpawnDiagnostic> diagnostics;
    size_t decoded_resource_bytes = 0;
    size_t unique_resource_count = 0;
    size_t decoded_pose_cache_bytes = 0; // included in decoded_resource_bytes.
    size_t unique_pose_cache_count = 0;
};

enum class SourceArea3BossStatus : uint8_t {
    NoEventOwner, DuplicateEventOwner, ResourceUnavailable, ResourceRejected,
    SourceBackedPresentation
};
struct SourceArea3BossSpawnSpec {
    size_t event_source_index = 0;
    uint8_t event_id = 3, object_class = 0x40, subtype = 2, parameter = 1;
    uint32_t callback = 0x801076e8u, initializer = 0x801078b8u;
    std::array<int16_t,3> spawn_position{{-256,-385,1001}};
    uint16_t heading = 0;
    std::array<int16_t,3> scale{{512,512,512}};
    uint16_t shade = 0x4210, hp = 1024;
    uint8_t action = 1, phase = 0, substate = 1, pose_request = 0, pose_record = 0;
};
struct SourceArea3BossSpawnResult {
    SourceArea3BossStatus status = SourceArea3BossStatus::NoEventOwner;
    std::optional<SourceArea3BossSpawnSpec> spec;
    size_t actor_index = SIZE_MAX;
};
// Event ownership is resolved independently of resource decoding so the class00
// record can be regression-tested without shipping retail EBD bytes.
SourceArea3BossSpawnResult source_area3_boss_spawn_spec_801076e8(
    const StageRuntimeArea&) noexcept;
// Load-time only. Appends one dynamic class40/02 presentation owner using the
// original Area 3 EBD row. Failure preserves the existing actor area.
SourceArea3BossSpawnResult append_source_area3_boss_801076e8(
    ActorAreaRuntime&,const StageRuntimeArea&,const EbdDirectory&,size_t source_area_index,
    size_t decoded_memory_cap=512*1024);

enum class SourceArea3BossTickStatus : uint8_t {
    NotBoss, SourceBackedIntro, SourceBackedCombatPresentation, CombatReady,
    MissingAnimation
};
struct SourceArea3BossTickResult {
    SourceArea3BossTickStatus status = SourceArea3BossTickStatus::NotBoss;
    bool committed = false, state_changed = false, pose_changed = false;
};
// 0x80107B48 plus intro substates 0/3/1/2 and the first bounded 0x80107FAC
// action-6 presentation cycle observed from a clean original Area 3 entry.
// Attack selection after the returned action1 boundary, damage, camera, HUD
// and defeat are intentionally not inferred here.
SourceArea3BossTickResult tick_source_area3_boss_intro_80107b48(
    ImportedActorInstance&,int32_t player_z) noexcept;
bool source_area3_boss_forces_player_entry(const ImportedActorInstance&,
                                           int32_t player_z) noexcept;

struct SourceArea3BossAction1Selection {
    uint16_t flags_4e0 = 0;
    uint8_t substate = 0, phase = 0, saved_substate = 0;
    bool handler_complete = false, death_started = false;
};
// Pure post-dispatch chooser from 0x80107B48. flags_4e0 is the value after
// 0x8003648C refreshed proximity bit 0x20; bit 0 is the dispatched handler's
// completion receipt. Sound/effect calls are returned as death_started.
SourceArea3BossAction1Selection source_area3_boss_action1_select_80107b48(
    int16_t hp,uint8_t substate,uint8_t phase,uint8_t saved_substate,
    uint16_t flags_4e0,bool death_suppressed) noexcept;

struct ImportedActorTargetSample {
    uint32_t source_work_80 = 0;
    std::optional<uint32_t> source_heading_80031198;
    std::optional<uint32_t> source_player_to_actor_angle_80031198;
};
// Original class20/03 distance and heading query only. Signed raw coordinates
// must fit the original work mirrors; out-of-range positions reject the query.
// Missing owner angle data leaves heading UNKNOWN. This neither turns the
// actor nor runs detection, RNG, damage, lock-on or the full AI callback.
// Allocation-free; the caller can use this evidence in a future admitted AI
// input and in the native diagnostic trace without introducing guessed rules.
std::optional<ImportedActorTargetSample> sample_imported_actor_target(
    const ImportedActorInstance& actor, int32_t player_x, int32_t player_z,
    const SourceAngleSteps* steps) noexcept;

struct ActorReloadRevisionState {
    uint32_t observed_source_revision = 0;
    uint32_t committed_actor_revision = 0;
    std::optional<uint32_t> failed_source_revision;
};

// Validate a fully prepared presentation candidate before a nonthrowing move
// commit. Resource failures, missing mesh/pose and the decoded-element cap
// reject it while preserving active. The cap excludes other CPU/GPU memory.
void commit_imported_actor_reload_candidate(ActorAreaRuntime& active,
                                           ActorAreaRuntime&& candidate,
                                           size_t decoded_memory_cap = 512 * 1024);

// Loading work only. The loader must use the existing capped actor decoder.
// Consumes a source revision before construction, including on failure, so the
// same revision is not retried every frame. An exception propagates unchanged;
// old presentation survives but the caller's logical source group is not rolled
// back. Old and candidate decoded elements may coexist up to twice the cap.
template<class Loader>
bool reload_imported_actors_for_revision(ActorAreaRuntime& active,
                                        ActorReloadRevisionState& revisions,
                                        uint32_t source_revision, Loader&& loader,
                                        size_t decoded_memory_cap = 512 * 1024) {
    if (revisions.observed_source_revision == source_revision) return false;
    revisions.observed_source_revision = source_revision;
    try {
        auto candidate = std::forward<Loader>(loader)();
        commit_imported_actor_reload_candidate(active, std::move(candidate), decoded_memory_cap);
        revisions.committed_actor_revision = source_revision;
        revisions.failed_source_revision.reset();
        return true;
    } catch (...) {
        revisions.failed_source_revision = source_revision;
        throw;
    }
}

// Area-load operation only: bounded original class20 capacity (16), shared
// decoded geometry for a shared row and a global decoded-byte cap. Unsupported
// initializers are recorded rather than converted into guessed actor types.
ActorAreaRuntime spawn_imported_actors(const StageRuntimeArea& stage,
                                      const EbdDirectory& ebd,
                                      size_t decoded_memory_cap = 512 * 1024);

// Binds a source pose for explicit diagnostic playback. The bool must remain
// false unless the caller has established the retail state/flag-selected request.
// Strong failure guarantee: an invalid pose leaves the old one intact.
void bind_imported_actor_pose(ImportedActorInstance& actor, const EbdDirectory& ebd,
                             uint8_t request, uint8_t record_index,
                             bool selected_by_retail_state = false);

// Optional load-time diagnostic support, only for initializer-selected class03
// request0. Cache storage shares one row/request, counts against the area's
// decoded-element cap and commits after every eligible actor has validated.
// Existing static poses remain intact if cache preparation fails.
void bind_imported_actor_request0_caches(ActorAreaRuntime& area, const EbdDirectory& ebd,
                                       size_t decoded_memory_cap = 512 * 1024);
struct ImportedActorAnimationTick {
    uint32_t admitted = 0, pose_changed = 0, unsupported = 0;
};
// Allocation-free sequencer/pose-copy subset. Caller owns the independent
// diagnostic cadence gate; this does not execute class03 AI or select requests.
ImportedActorAnimationTick tick_imported_actor_request0(ActorAreaRuntime& area) noexcept;

// Optional load-time banks for initializer-selected class03. Explicit requests
// share one immutable row bank; no request0 substitution or per-tick decoding.
// Preparation validates every actor before any binding is published.
void bind_imported_actor_animation_banks(ActorAreaRuntime& area, const EbdDirectory& ebd,
    const uint8_t* requests, size_t request_count, size_t decoded_memory_cap = 512 * 1024);

struct ImportedActorBehaviorTick {
    Class20_03CallbackStatus status = Class20_03CallbackStatus::UnknownState;
    ActorAnimationStatus animation_status = ActorAnimationStatus::InvalidCache;
    bool committed = false, request_changed = false, pose_changed = false;
    bool notify_detection_global = false;
    bool proxy_finalized = false,proxy_registered = false,proxy_bank_full = false;
    bool shadow_appended = false;
    bool defeated=false,defeat_effect_allocated=false,defeat_effect_exhausted=false;
    bool reward_enqueued=false;
    bool released_80031824=false;   // action3 phase5 parameter mask20 pool release
    std::optional<SourceConsumerStatus> shadow_status;
    std::optional<Class20_03ProxyStatus> proxy_status;
    uint8_t source_audio_queued = 0, source_audio_dropped = 0;
};
enum class ImportedActorPacketStatus:uint8_t {
    SourceBackedPartial,NeedsOwner,InvalidIdentity,PendingPacketConflict
};
// Source33C/340/344/34A are aliases of this actor's owned proxy target. Publish
// all four together after identity/conflict validation. This delivers no HP,
// animation, effect or packet-clear side effects and preserves held packets.
ImportedActorPacketStatus stage_imported_actor_proxy_packet(ImportedActorInstance&) noexcept;
struct ImportedActorDetectionPreparation {
    ActorDetectionWork before, after;
    ActorDetectionResult result;
    std::array<int16_t,3> actor_raw{};
    uint16_t heading = 0;
    uint32_t work80 = 0;
};
// Original scalar detection preparation; persistent stores are published only
// with the complete actor state/pose transaction. Missing profile/reference/
// phase1 owners are reported rather than replaced with a vision approximation.
ImportedActorDetectionPreparation prepare_imported_actor_detection(
    const ImportedActorInstance& actor, const ActorDetectionInputs& input) noexcept;
struct ImportedActorAction2Preparation {
    RuntimeContactStatus status = RuntimeContactStatus::Unsupported;
    ActorContactWork before, after_wall;
    ActorWallContact wall;
    ActorFloorContact floor;
    std::optional<Class20_03PreparedAction2> inputs;
    uint16_t initial_heading = 0;
    int16_t initial_turn = 0;
};
// Source action2's 52-unit fixed motion and actual bounded wall/floor queries.
// Read-only preparation; unsupported contacts do not yield usable inputs.
ImportedActorAction2Preparation prepare_imported_actor_action2_contact(
    const ImportedActorInstance& actor, const RuntimeMap& map, size_t area_index,
    const ReferenceTables& references) noexcept;
ActorAction3ContactPreparation prepare_imported_actor_action3_contact(
    const ImportedActorInstance& actor, const RuntimeMap& map, size_t area_index,
    const ReferenceTables& references, std::optional<uint16_t> target_heading) noexcept;
ActorAction4ContactPreparation prepare_imported_actor_action4_contact(
    const ImportedActorInstance& actor, const RuntimeMap& map, size_t area_index,
    const ReferenceTables& references) noexcept;
// Stages hit work on a read-only actor copy before preparing the source-selected
// reaction. Nonzero packets remain pending in the live actor. Relative hit
// origin8011BF08 and record+0B remain explicit owners when their branches use them.
ActorReactionContactPreparation prepare_imported_actor_reaction_contact(
    const ImportedActorInstance&,const RuntimeMap&,size_t,const ReferenceTables&,
    const SourceReactionProfile*,const SourceAngleSteps*,std::optional<uint16_t> recovery_target,
    const SourceAudioQueue* source_audio=nullptr,
    std::optional<std::array<int16_t,3>> relative_origin_8011bf08=std::nullopt) noexcept;
// Detection must be the same admitted result used by the subsequent tick.
// The source body replaces flag10 before choosing the pursuit motion branch.
ActorAction9ContactPreparation prepare_imported_actor_action9_contact(
    const ImportedActorInstance& actor, const RuntimeMap& map, size_t area_index,
    const ReferenceTables& references, std::optional<uint32_t> work80,
    std::optional<uint16_t> target_heading, std::optional<bool> detected) noexcept;
// Load-time binding only for exact source selector/class/index/context keys.
// Missing rows retain unknown HP/combat metadata; no guessed default selector.
bool bind_imported_actor_combat_profile(ImportedActorInstance& actor,
    const SourceCombatProfiles& profiles, std::optional<uint8_t> selector) noexcept;
// Optional exact outer proxy owner for a single body/animation/tail commit.
// Shared bank/reference/leases must remain stable for the synchronous call.
// Shadow-enabled branches require their real queue, current map and source
// work76/bounds; no readiness boolean can stand in for a missing owner.
struct ImportedActorProxyTailContext {
    const ReferenceTables* references=nullptr;
    SourceHitProxyBank* bank=nullptr;
    SourceShadowQueue* shadows=nullptr;
    const RuntimeMap* map=nullptr;
    size_t area_index=0;
    std::optional<uint32_t> mask_80098798;
    std::array<SourceHitProxyOwnerPoint*,24> owners_800be478{};
};
// State1 source-used-fields transaction, not the full callback/AI. Missing
// source inputs and missing/malformed pose banks preserve behavior and pose.
// The outer BIOS/effect/proxy owners and packet finalizer remain excluded;
// nonzero packets require the explicit atomic proxy tail below; sound intents
// require the real bounded producer
// queue and are retained without a claim of audible playback. Allocation-free.
ImportedActorBehaviorTick tick_imported_actor_state1(ImportedActorInstance& actor,
    const Class20_03CallbackInputs& inputs,
    const ImportedActorAction2Preparation* movement = nullptr,
    const ImportedActorDetectionPreparation* detection = nullptr,
    SourceAudioQueue* source_audio = nullptr,
    const ActorAction3ContactPreparation* action3 = nullptr,
    const ActorAction9ContactPreparation* action9 = nullptr,
    const ActorAction4ContactPreparation* action4 = nullptr,
    ChildRuntime* children = nullptr, const ReferenceTables* child_references = nullptr,
    const ActorReactionContactPreparation* reaction = nullptr,
    const ImportedActorProxyTailContext* proxy_tail = nullptr) noexcept;

struct ImportedActorPopoutTick {
    Class20_1FResult result;
    bool committed = false, morph_pending = false, pose_changed = false;
};
// One 0x801062EC callback for a subtype-0x1F actor. All-or-nothing; the hit
// proxy (flags bit 8, 0x800341BC) is reported, not registered. Allocation-free.
ImportedActorPopoutTick tick_imported_actor_popout_1f(ImportedActorInstance& actor,
    const ReferenceTables& references, std::optional<uint16_t> random_sample,
    std::optional<uint32_t> work_80) noexcept;
// Binds the 0x80098844[0x19] record (L3CB subtype 0x19 row) and the
// 0x80036478 detection profile (record +0x0F, offset -0x60). Fresh only.
bool bind_imported_actor_reaverbot19_record(ImportedActorInstance& actor,
    const SourceCombatProfiles& profiles, std::optional<uint8_t> selector) noexcept;
struct ImportedActorReaverbot19Tick {
    Class20_19Result result;
    bool committed = false, proxy_registered = false, pose_changed = false;
};
// One 0x80104864 callback for a subtype-0x19 actor: packet intake from the
// proxy target, reducer, animation step, 0x800341BC registration (radius
// 0x30) from the 0x8003346C pose point, and the +0x35C clear. Defeat effects
// and rewards are returned for the caller's child runtime. Allocation-free.
ImportedActorReaverbot19Tick tick_imported_actor_reaverbot19(ImportedActorInstance& actor,
    Class20_19Inputs inputs, const ReferenceTables& references, SourceHitProxyBank* bank,
    std::optional<uint32_t> mask_80098798,
    std::array<SourceHitProxyOwnerPoint*,24> owners_800be478 = {}) noexcept;

// The frame after 0x801062EC rewrote work +2/+3/+8, the subtype-3 dispatcher
// runs state 0 (0x80102410). This performs that init: subtype-3 row, mesh and
// pose 0, parameter 0x40 (action 8). The caller then binds the combat profile,
// animation banks and the fresh proxy subset exactly as at area load.
bool morph_imported_actor_popout_to_class20_03(ActorAreaRuntime& area, size_t index,
    const EbdDirectory& ebd, size_t decoded_memory_cap = 512 * 1024);

// Opt-in diagnostic admission of original fresh class20 pool clear8002E844
// and standard shift store8003697C. Caller must own fresh pool chronology;
// never use for recycled slots, continued saves or a mere area reload.
bool initialize_imported_actor_fresh_pose_proxy_subset(ImportedActorInstance& actor) noexcept;
struct ImportedActorPoseProxyTick {
    ActorPoseCollisionStatus pose_status=ActorPoseCollisionStatus::NeedsScratchShift;
    Class20_03ProxyResult proxy;
    bool committed=false;
};
// Separate outer-tail transaction after the admitted body/pose update. This
// registers original actor points in the shared bank; it does not implement
// Buster/player GTE tests or defeat ownership. A pending source packet holds
// this separate tail until the complete body transaction is admitted.
ImportedActorPoseProxyTick tick_imported_actor_pose_proxy(ImportedActorInstance& actor,
    const ReferenceTables& references,SourceHitProxyBank& bank,
    std::optional<uint32_t> source_mask_80098798,
    std::array<SourceHitProxyOwnerPoint*,24> owners_800be478 = {}) noexcept;

// Supported initializers sample a floor before setting work y. Caller uses the
// area's collision query; this API does not substitute a guessed floor height.
void bind_imported_actor_floor(ImportedActorInstance& actor, int16_t raw_floor_y);

struct ImportedActorFloorSample {
    RuntimeContactStatus status = RuntimeContactStatus::NoData;
    std::optional<int16_t> floor_y;
    std::optional<uint32_t> cell_index;
    std::optional<uint32_t> primitive_index;
    uint32_t candidate_cells = 0;
    uint32_t unsupported_primitives = 0;
};
// Fresh 0x80048E6C floor calls have previous Y equal to source Y. Class20/35
// has null work+0x78 (six zero bounds); class20/03 installs 0x8011BED4
// {-60,60,-60,60,-128,0} before the call. Query only; actor XYZ
// are never run through the player movement solver. Supports the existing
// rectangle/negative-cell floor subset. Codes8..11 remain unsupported after
// full wrapper evidence exposed the inherited scratch-contract mismatch. Other geometry or malformed data return Unsupported and no
// height. Allocation-free, with at most four candidates.
ImportedActorFloorSample sample_imported_actor_floor(const ImportedActorInstance& actor,
                                                   const RuntimeMap& map, size_t area_index);
void unload_imported_actors(ActorAreaRuntime& area);

struct ImportedActorPoseMatrices {
    size_t part_count = 0;
    std::array<PlayerPoseMatrix, kActorClass20PartLimit> world{};
    std::array<bool, kActorClass20PartLimit> draw_enabled{};
};
// Float presentation bridge for source work +0x1e8 rotation matrices and +0x180
// positions. Returns world points in the same raw coordinate units as terrain.
// Source yaw is applied before root/child rotations; per-actor Q12 scale is
// applied once to each final matrix, not compounded at every hierarchy level.
// Root translation is source signed12 vector*8 through scaled yaw; child base
// translations go through the scaled parent rotation. Index0 children explicitly
// copy the root transform. PS1 integer/GTE rounding remains a separate parity gate.
ImportedActorPoseMatrices build_imported_actor_pose_matrices(const ImportedActorInstance& actor);

} // namespace l3
