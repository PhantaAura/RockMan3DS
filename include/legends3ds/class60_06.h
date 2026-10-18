#pragma once
#include "legends3ds/class20_03_action4.h"
#include "legends3ds/actor_contact.h"

namespace l3 {
using Class60_06Work=std::array<uint8_t,class60_slot_bytes>;
struct Class60_06Profile {
    std::array<std::array<uint16_t,2>,6> normal_uv{},alternate_uv{};
    uint8_t source_stage=0;
};
enum class Class60_06Status : uint8_t {
    SourceBackedPartial,Inactive,UnknownSubtype,UnknownState,WrongClass,
    NeedsTrig,NeedsProfile,UnknownUvFrame,NeedsPlayerHeading,NeedsContact,
    StaleContact,UnsupportedBounds,UnsupportedBoundary,UnsupportedWall,
    NeedsContactSound,NeedsEffectConsumer,UnsupportedSpecialStage,
    NeedsProxyConsumer,StaleProxy,NeedsCollisionRegistry,NeedsShadowConsumer,
    NeedsShadowFloor
};
struct Class60_06EffectIntent {
    uint32_t source_identity=0;
    uint8_t work00=0,subtype02=0,parameter03=0;
    std::array<int16_t,4> raw_to_38{};
    std::array<uint8_t,52> primitive_to_14{};
    bool copy_primitive=false,copy_raw=false;
};
struct Class60_06Contact {
    Class60_06Status status=Class60_06Status::NeedsContact;
    Class60_06Work before{},after{};
    int32_t source_wall_result=0;
    uint16_t source_return=0;
    std::optional<uint16_t> sound;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
    std::optional<Class60_06EffectIntent> effect;
};
struct Class60_06ProxyIntent {
    std::array<int16_t,3> point{};
    uint8_t source_class=0;
    uint16_t extent=32,mode=0;
    uint32_t work9c=0,worka0=0,packet_mask=0x08000000u;
    uint32_t extra_flags=128;
};
struct Class60_06ProxyCommit {
    Class60_06ProxyIntent before;
    uint32_t worka0_after=0;
};
struct Class60_06ShadowRecord {
    std::array<int16_t,3> point{};
    uint16_t heading_scale=0,radius_type=60,height_delta=0;
};
struct Class60_06Inputs {
    const ReferenceTables* references=nullptr;
    const Class60_06Profile* profile=nullptr;
    std::optional<uint16_t> player_heading_800b5206;
    std::optional<Class60_06Contact> contact;
    // Source80031618 allocation result for body-release effect. A present zero
    // means independently proved exhaustion, never an unavailable consumer.
    std::optional<uint32_t> effect_allocation_80031618;
    std::optional<Class60_06ProxyCommit> proxy;
    bool collision_registry_ready=false;
    std::optional<uint32_t> shadow_count_800987d8;
    std::optional<int32_t> shadow_floor_80048e6c;
};
struct Class60_06Result {
    Class60_06Status status=Class60_06Status::SourceBackedPartial;
    std::optional<Class60_06Work> contact_request;
    std::optional<Class60_06EffectIntent> effect_request;
    std::optional<Class60_06ProxyIntent> proxy_request;
    // Exact work at8002C8CC boundary, for a real owner's floor/queue staging.
    std::optional<Class60_06Work> shadow_request;
    std::optional<Class60_06ShadowRecord> shadow;
    bool register_collision=false,release_800318d8=false;
};
// Exact pool boot subset8002E8D8..8002E924. No retail bytes are embedded.
void initialize_class60_source_pool_8002e8d8(Class60SourcePool&) noexcept;
// Original complete scalar velocity/accumulator helper80110070. Fourth fixed
// and raw lanes survive; wrap16 precedes each signed24576 clamp.
void class60_06_integrate_80110070(Class60_06Work&) noexcept;
//8010F354's source scalar lifecycle and explicit external consumer receipts.
// Missing/stale inputs hold all268bytes. A NeedsContact result publishes only
// the exact prepared call-boundary work for a subsequent contact preparation.
// Source proxy/registry/shadow must really be staged by the native owner.
Class60_06Result class60_06_step_8010f354(
    Class60_06Work&,const Class60_06Inputs&) noexcept;
//8010FDD4 using admitted source wall query and original scalar response.
// Null bounds are supported; stage-specific special effects remain held.
Class60_06Contact prepare_class60_06_contact_8010fdd4(
    const Class60_06Work&,const RuntimeMap&,size_t area,
    std::optional<uint8_t> source_stage,bool source_sound_consumer_ready,
    std::optional<uint32_t> effect_allocation_80031618) noexcept;
struct Class60_06Primitive {
    std::array<int16_t,3> center{};
    uint16_t half_extent=0,clut=0,tpage=0;
    std::array<uint8_t,2> uv{};
    uint32_t color_command=0;
    bool source_fog=true;
};
// Source80029AAC renderer selection +80029C18 primitive contract. Does not
// replace the original projection/fog or stateful texture resolver.
std::optional<Class60_06Primitive> class60_06_primitive(const Class60_06Work&) noexcept;
// Original preprojection upperleft/upperright/center points and source packet
// UVwords. PackedUV additions preserve the source carry, not separate U/V wrap.
std::array<std::array<int16_t,3>,3> class60_06_camera_points(
    std::array<int32_t,3> camera_center,uint16_t extent) noexcept;
std::array<uint32_t,4> class60_06_texture_words(const Class60_06Primitive&) noexcept;
} // namespace l3
