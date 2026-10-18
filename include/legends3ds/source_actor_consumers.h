#pragma once
#include "legends3ds/class60_06.h"
namespace l3 {
// Native queue bound. Original80051BAC did not bounds-check the append.
//128 is a native safety budget, not an inferred retail capacity.
constexpr size_t source_collision_registry_native_capacity=128;
struct SourceCollisionRegistry {
    std::array<uint32_t,source_collision_registry_native_capacity> identities{};
    size_t count=0;
};
enum class SourceConsumerStatus:uint8_t {
    SourceBackedPartial,FullHeld,InvalidIdentity,InvalidCount,NeedsFloor
};
// Original80051BAC appends iffwork00&3 thenalwaysclearswork7C. Native overflow
// holds both work and queue. Pair-contact80051BE4 is a separate consumer.
SourceConsumerStatus register_source_collision_80051bac(
    SourceCollisionRegistry&,uint32_t identity,uint8_t work00,uint16_t& work7c) noexcept;
struct SourceShadowWork {
    std::array<int16_t,3> raw{};
    uint16_t heading56=0;
    int8_t work76=0;
};
struct SourceShadowQueue {
    std::array<Class60_06ShadowRecord,64> records{};
    uint32_t count=0;
};
struct SourceShadowPlan {
    SourceConsumerStatus status=SourceConsumerStatus::SourceBackedPartial;
    std::optional<Class60_06ShadowRecord> append;
};
// Read-only exact producer preflight; the queue count bypasses all work/floor
// reads at64 just as the original. Caller owns synchronous publication.
SourceShadowPlan plan_source_shadow_8002c8cc(uint32_t count,const SourceShadowWork&,
    uint32_t radius,int32_t scale,uint32_t type,std::optional<int32_t> floor) noexcept;
// Exact8002C8CC producer, including fullqueue skip beforefloorquery and16bit
// packing. Radius/scale/type are original call arguments, not newgame rules.
SourceConsumerStatus append_source_shadow_8002c8cc(
    SourceShadowQueue&,const SourceShadowWork&,uint32_t radius,int32_t scale,
    uint32_t type,std::optional<int32_t> floor_80048e6c) noexcept;
} //namespace l3
