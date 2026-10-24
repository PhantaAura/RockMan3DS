#pragma once
#include "legends3ds/source_actor_consumers.h"
#include "legends3ds/source_audio_queue.h"
#include "legends3ds/source_hit_proxy.h"
#include "legends3ds/source_player_proxy.h"
#include "legends3ds/source_effects.h"
#include "legends3ds/source_reward_pickup.h"

namespace l3 {
enum class ChildRuntimeStatus : uint8_t {
    SourceBackedPartial, NotSpawnBranch, UnsupportedSubtype,
    NeedsSoundConsumer, StaleDelivery, InvalidIdentity, InvalidPool,
    NeedsMap, NeedsProxyMask, NeedsProxyPointPadding, ProxyHeld, RegistryHeld, ShadowHeld,
    LifecycleHeld,EffectHeld,NeedsRewardProfile,RewardQueueFull
};

class ChildRuntime;
// A checked, bounded producer transaction. Only ChildRuntime can author its
// allocation/audio receipts; a public scalar child token is not a receipt.
class ParentChildDelivery {
public:
    bool prepared() const noexcept { return prepared_; }
    uint32_t source_identity() const noexcept { return identity_; }
    const SourceAudioQueue& staged_audio() const noexcept { return audio_after_; }
    uint8_t audio_queued() const noexcept { return queued_; }
    uint8_t audio_dropped() const noexcept { return dropped_; }
private:
    friend class ChildRuntime;
    const ChildRuntime* owner_=nullptr;
    bool prepared_=false;
    uint32_t identity_=0, allocated_before_=0, pending_before_=0, allocated_after_=0;
    Class20_03BehaviorState behavior_{};
    std::array<int16_t,4> raw_{};
    std::array<int32_t,4> fixed_{};
    int16_t variant_=0;
    Class60_06Work slot_before_{},slot_after_{};
    SourceAudioQueue audio_before_{},audio_after_{};
    uint8_t queued_=0,dropped_=0;
};

constexpr size_t source_enemy_reward_capacity=32;
struct SourceEnemyRewardRequest {
    std::array<int16_t,4> point_180{};
    uint32_t source_descriptor_word00=0;
};
struct SourceEnemyRewardQueue {
    std::array<SourceEnemyRewardRequest,source_enemy_reward_capacity> records{};
    size_t count=0;
};
// Atomic native owner for source defeat effect128/65, reward call80047228 and
// sound161. Reward requests retain the exact call operands; their RNG-driven
// effect135 producer80046B34 remains a later consumer.
class EnemyDefeatDelivery {
public:
    bool prepared() const noexcept {return prepared_;}
    uint32_t effect_identity() const noexcept {return effect_identity_;}
    bool effect_exhausted() const noexcept {return effect_identity_==0;}
    const SourceAudioQueue& staged_audio() const noexcept {return audio_after_;}
    uint8_t audio_queued() const noexcept {return queued_;}
    uint8_t audio_dropped() const noexcept {return dropped_;}
private:
    friend class ChildRuntime;
    const ChildRuntime* owner_=nullptr;
    bool prepared_=false;
    uint32_t effect_identity_=0;
    std::array<uint32_t,2> allocated_before_{},pending_before_{},allocated_after_{},pending_after_{};
    std::optional<size_t> effect_slot_;
    SourceEffectWork slot_before_{},slot_after_{};
    size_t reward_count_before_=0;
    SourceEnemyRewardRequest reward_{};
    SourceAudioQueue audio_before_{},audio_after_{};
    uint8_t queued_=0,dropped_=0;
};

struct ChildFrameContext {
    const RuntimeMap* map=nullptr;
    size_t area_index=0;
    const ReferenceTables* references=nullptr;
    const Class60_06Profile* profile=nullptr;
    std::optional<uint16_t> player_heading_800b5206;
    // This must come from the admitted source global owner, not a guessed mask.
    std::optional<uint32_t> proxy_mask_80098798;
    // The source stack vector stores three shorts, leaving its fourth lane
    // retained. It matters only when a masked owner+BC update copies8 bytes.
    std::optional<uint16_t> proxy_stack_padding_2e;
    SourceAudioQueue* audio=nullptr;
};
struct ChildRuntimeTick {
    ChildRuntimeStatus status=ChildRuntimeStatus::LifecycleHeld;
    Class60_06Status lifecycle_status=Class60_06Status::Inactive;
    SourceHitProxyStatus proxy_status=SourceHitProxyStatus::SourceBackedPartial;
    SourceConsumerStatus registry_status=SourceConsumerStatus::SourceBackedPartial;
    SourceConsumerStatus shadow_status=SourceConsumerStatus::SourceBackedPartial;
    RuntimeContactStatus floor_status=RuntimeContactStatus::NoData;
    bool committed=false,released=false;
    SourceEffectStatus effect_status=SourceEffectStatus::SourceBackedPartial;
    uint8_t effects_allocated=0,effects_exhausted=0;
    uint8_t audio_queued=0,audio_dropped=0;
};

constexpr size_t source_buster_capacity=16;
constexpr uint32_t source_buster_base=0x800bfb20u;
using SourceBusterWork=Class60_06Work;
struct SourceBusterPool {
    std::array<SourceBusterWork,source_buster_capacity> slots{};
    uint32_t allocated_800989f0=0,pending_release_80098a60=0;
};
enum class SourceBusterStatus : uint8_t {
    SourceBackedPartial,NeedsMuzzle,NeedsSubtype,NeedsCounter,NeedsProxy,
    NeedsProxyMask,NeedsPlayerFields,InvalidProxy,InvalidPool
};
// Every selected lane is an admitted read from the real player pose/work and
// the matching owner-local table row. These are consumer premises, not values
// inferred from animation number, native float pose, or desktop projectile.
struct SourceBusterLane {
    int8_t selection_170_or_171=0;
    std::optional<std::array<uint16_t,3>> muzzle_3c8_or_3e0;
    std::optional<uint8_t> subtype_80089a8b;
    std::optional<int16_t> counter_4fa;
};
struct SourceBusterPlayer {
    std::optional<uint8_t> class_01;
    std::optional<uint16_t> heading_56;
    std::array<SourceBusterLane,2> lanes{};
};
struct SourceBusterSpawn {
    SourceBusterStatus status=SourceBusterStatus::NeedsMuzzle;
    uint32_t source_identity=0;
    int32_t lease_index=-1;
    bool counter_written=false;
};
// The segment the subtype-80 callback last handed to 0x8003430C: first = +14/+18
// (the endpoint just read from player +400/+418), second = +2C/+30 before
// 0x80043708..24 copies +14 over it (in state 1, the owner +14 written by
// 0x800434F0). After that copy both fields are equal, so presentation must read
// this record, not the work. Native bookkeeping only; gameplay never reads it.
struct SourceBusterSegment {
    std::array<int16_t,3> first_14{};
    std::array<int16_t,3> second_2c{};
};
enum class SourceBusterCallbackStatus : uint8_t {
    SourceBackedPartial,Inactive,InvalidPool,UnsupportedSubtype,UnknownState,
    NeedsOwnerPosition,NeedsOwnerEndpoint,NeedsOwnerBusy,NeedsProxyMask,
    NeedsMapCollision,NeedsSoundConsumer,ProxyHeld,EffectHeld,
    NeedsShotProfile,UnsupportedAimPitch
};
// Subtype-0x81 Mega Buster shot (weapon row 1). Damage/range from the
// PTR_DAT_80089EAC row at player +0x4FC/+0x4FE, radius 0x80097DCC, speed
// 0x80097DD4 (cooked L3BP/2).
struct SourceBusterShot81Stats {
    int16_t damage_800375d4=0,range_80037634=0;
    uint8_t radius_98=0,speed=0;
};
struct SourceBusterFrameContext {
    std::optional<std::array<uint32_t,2>> owner_position_14;
    std::optional<std::array<uint32_t,2>> owner_endpoint_400;
    std::optional<std::array<uint32_t,2>> owner_endpoint_418;
    std::optional<uint32_t>* owner_busy_198=nullptr;
    std::optional<uint32_t>* proxy_mask_80098798=nullptr;
    std::optional<bool> map_collision_80049c24;
    std::optional<uint16_t> midpoint_padding_06;
    SourceAudioQueue* audio=nullptr;
    // Subtype 0x81 owners. The shot's own wall test 0x80049C24 -> 0x80049C54
    // reads the map; 0x80039B80 aim pitch is nonzero only under a lock-on
    // owner (0x800A3960) whose writer is untraced, so only pitch 0 runs.
    const RuntimeMap* map=nullptr;
    size_t area_index=0;
    const ReferenceTables* references=nullptr;
    std::optional<SourceBusterShot81Stats> shot81;
    uint16_t aim_pitch_80039b80=0;
};
struct SourceBusterBatch {
    SourceBusterCallbackStatus status=SourceBusterCallbackStatus::SourceBackedPartial;
    uint8_t committed=0,held=0,registered=0,impacts=0,released=0;
    uint8_t map_impacts=0;
};
// Exact cold slice8002E7F4..8002E83C clears all16 records, sets01=16|slot,
// and clears the two original allocation/deferred-release masks.
void initialize_source_buster_pool_8002e7f4(SourceBusterPool&) noexcept;

// Exact80048E6C argument wrapper: +14, bounds token+78, signed Y(+16)-
// previousY(+2E), class byte+1. Only independently proved null bounds are
// admitted. The inherited bounded80048EA4 query holds unknown geometry.
ActorFloorContact query_class60_shadow_floor_80048e6c(
    const Class60_06Work&,const RuntimeMap&,size_t area_index) noexcept;

class ChildRuntime {
public:
    ChildRuntime() noexcept;
    ChildRuntime(const ChildRuntime&)=delete;
    ChildRuntime& operator=(const ChildRuntime&)=delete;
    void initialize() noexcept;
    const Class60SourcePool& pool() const noexcept { return pool_; }
    const SourceEffectPool& effect_pool() const noexcept { return effects_; }
    const SourceEnemyRewardQueue& enemy_reward_queue() const noexcept {return rewards_;}
    const SourceBusterPool& buster_pool() const noexcept { return busters_; }
    const std::array<SourceHitProxyTarget,source_buster_capacity>& buster_proxy_targets() const noexcept { return buster_targets_; }
    const std::array<SourceBusterSegment,source_buster_capacity>& buster_registered_segments() const noexcept { return buster_segments_; }
    // Original80035724 scalar producer + real80031598 allocation and3545C
    // lease. Target comes from this owner's registered proxy bank. Missing
    // admitted player/table fields hold all owners atomically. This does not
    // execute the later projectile callback or invent firing cadence/damage.
    SourceBusterSpawn spawn_source_buster_80035724(SourceBusterPlayer& player,
        uint32_t stack10,int32_t selected_proxy_index,
        std::optional<uint32_t>& proxy_mask_80098798) noexcept;
    SourceBusterBatch tick_source_busters_80017154(
        const SourceBusterFrameContext&) noexcept;
    SourceEffectBatch tick_effects(const SourceEffect128Inputs& inputs = {}) noexcept;
    // 0x80047228 -> 0x80046B34 for the oldest queued defeat reward, then pop
    // it. A hold (missing profile/trig/RNG) keeps the request queued.
    SourceRewardExpansion expand_next_enemy_reward(const SourceRewardProfile&,
        const ReferenceTables&,SourceRngService&,uint64_t source_generation) noexcept;
    SourceRewardTick tick_reward_pickups(const SourceRewardProfile&,const ReferenceTables&,
        SourceRngService&,uint64_t source_generation,const SourceRewardWorld&,
        int32_t& money_800c1b2c,int16_t& hp,int16_t hp_max) noexcept;
    SourceCollisionRegistry& collision_registry() noexcept { return registry_; }
    const SourceCollisionRegistry& collision_registry() const noexcept { return registry_; }
    SourceShadowQueue& shadow_queue() noexcept { return shadows_; }
    const SourceShadowQueue& shadow_queue() const noexcept { return shadows_; }
    SourceHitProxyBank& hit_proxy_bank() noexcept { return proxies_; }
    const SourceHitProxyBank& hit_proxy_bank() const noexcept { return proxies_; }
    const std::array<SourceHitProxyTarget,class60_pool_capacity>& proxy_targets() const noexcept { return targets_; }

    ChildRuntimeStatus prepare_parent_delivery(const Class20_03CallbackState&,
        const ReferenceTables&,const SourceAudioQueue&,ParentChildDelivery&) noexcept;
    bool validate_parent_delivery(const ParentChildDelivery&,
        const Class20_03CallbackState&,const SourceAudioQueue&) const noexcept;
    // Preflight validation belongs before the parent's body/animation work.
    // Commit only after all parent consumers succeed, before committing parent
    // state/audio. No queue/audio is overwritten by this final pool-only commit.
    bool commit_parent_delivery(ParentChildDelivery&) noexcept;
    // The defeat effect copies the actor's +0x14 words (x, y, z, +0x1A) to
    // its +0x38 (0x801027CC class20/03, 0x80104D44 class20/19).
    ChildRuntimeStatus prepare_enemy_defeat_delivery(
        const std::array<int16_t,4>& point_180,std::optional<uint32_t> reward_word00,
        const SourceAudioQueue&,EnemyDefeatDelivery&,
        const std::array<int16_t,4>& effect_point_14={}) noexcept;
    bool validate_enemy_defeat_delivery(const EnemyDefeatDelivery&,
        const SourceAudioQueue&) const noexcept;
    bool commit_enemy_defeat_delivery(EnemyDefeatDelivery&) noexcept;
    ChildRuntimeTick tick(uint32_t source_identity,const ChildFrameContext&) noexcept;

    // Native owner bindings have explicit source identity tokens. The exact
    // proxy function receives pointers only to these owned native records.
    bool bind_proxy_owner(size_t table_index,uint32_t source_identity,
        SourceHitProxyOwnerPoint point) noexcept;
    // Source Buster80035724 lease producer. The admitted source mask and target
    // stay with their real owners; this adapter binds the selected source token
    // to native BC/C0 storage consumed by registration and GTE projection.
    SourceHitProxyLeaseResult allocate_proxy_lease(std::optional<uint32_t>& source_mask,
        uint32_t owner_source_identity,SourceHitProxyOwnerPoint,SourceHitProxyTarget&) noexcept;
    SourceHitProxyStatus release_proxy_lease(std::optional<uint32_t>& source_mask,
        uint32_t source_index) noexcept;
    void clear_proxy_owner(size_t table_index) noexcept;
    const SourceHitProxyOwnerPoint* proxy_owner_point(size_t table_index) const noexcept;
    // The 0x800BE478 owner table for registrations made outside this runtime.
    // A leased target (flags_04 lease bit) cannot register without it, and
    // 0x800341BC writes the target's point into each leasing owner. Call
    // publish_proxy_owner_points() after such registrations.
    std::array<SourceHitProxyOwnerPoint*,24> proxy_owner_pointers() noexcept;
    // 0x80047228 from an object without the enemy defeat effect (E0/05 box):
    // queues the reward call operands only. False when the queue is full.
    bool enqueue_reward_80047228(const std::array<int16_t,4>& point, uint32_t word00) noexcept;
    // 0x80031618 for a producer outside this runtime (E0/05, E0/08) plus its
    // header/lane stores. Zero when both masks are full (the producer skips).
    uint32_t spawn_effect_80031618(uint8_t work00,uint8_t subtype02,uint8_t parameter03,
        size_t lane_offset,const std::array<int16_t,4>& lanes,uint8_t lane_count) noexcept;
    void publish_proxy_owner_points() noexcept;
    SourcePlayerLeaseResult consume_player_leases(SourceGteRtpsState& state,
        std::optional<std::array<uint32_t,8>> source_camera,
        std::optional<uint32_t> source_mask) noexcept;
    SourceHitProxyResult consume_proxy_pairs(
        const std::optional<SourceHitProxyRules>&) noexcept;

    // Caller owns batch timing. Resetting producers does not prove physical
    // pair80051BE4/shadow rasterizer/audio playback dispatch.
    void reset_frame_queues() noexcept;
    bool mark_release(uint32_t source_identity) noexcept;
    void consume_pending_releases() noexcept;
private:
    bool current_pool_matches(const ParentChildDelivery&) const noexcept;
    void publish_buster_proxy_points() noexcept;
    Class60SourcePool pool_{};
    SourceEffectPool effects_{};
    SourceEnemyRewardQueue rewards_{};
    SourceBusterPool busters_{};
    std::array<SourceHitProxyTarget,source_buster_capacity> buster_targets_{};
    std::array<SourceBusterSegment,source_buster_capacity> buster_segments_{};
    std::array<SourceHitProxyTarget,class60_pool_capacity> targets_{};
    SourceHitProxyBank proxies_{};
    SourceCollisionRegistry registry_{};
    SourceShadowQueue shadows_{};
    std::array<uint32_t,24> owner_identities_{};
    std::array<SourceHitProxyOwnerPoint,24> owner_points_{};
    // Retained transaction scratch: no pool or queue-sized hot-loop stack
    // allocation, no heap. The producer reuses the exact shared32-slot API.
    Class60SourcePool producer_scratch_{};
    SourceEffectPool effect_scratch_{};
    SourceHitProxyBank proxy_scratch_{};
    SourceCollisionRegistry registry_scratch_{};
    SourceShadowQueue shadow_scratch_{};
    SourceAudioQueue audio_scratch_{};
    SourceHitProxyTarget target_scratch_{};
    std::array<SourceHitProxyOwnerPoint,24> owner_scratch_{};
};
} // namespace l3
