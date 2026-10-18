#include "legends3ds/source_actor_consumers.h"
#include <algorithm>
#include <climits>
namespace l3 {
namespace {
int16_t signed16(uint16_t v)noexcept{return v<32768?int16_t(v):int16_t(int32_t(v)-65536);}
int32_t shift6(int32_t v)noexcept{return v>=0?v/64:int32_t(-((-int64_t(v)+63)/64));}
}
SourceConsumerStatus register_source_collision_80051bac(
    SourceCollisionRegistry& q,uint32_t identity,uint8_t flags,uint16_t& contact) noexcept {
    if(q.count>q.identities.size())return SourceConsumerStatus::InvalidCount;
    if(flags&3u){
        if(!identity || (identity&3u))return SourceConsumerStatus::InvalidIdentity;
        if(q.count==q.identities.size())return SourceConsumerStatus::FullHeld;
        q.identities[q.count++]=identity;
    }
    contact=0;return SourceConsumerStatus::SourceBackedPartial;
}
SourceShadowPlan plan_source_shadow_8002c8cc(uint32_t count,const SourceShadowWork& work,
    uint32_t radius,int32_t scale,uint32_t type,std::optional<int32_t> supplied) noexcept {
    SourceShadowPlan plan;
    if(count>=64)return plan;
    int32_t floor=work.raw[1];
    if(work.work76==0){if(!supplied){plan.status=SourceConsumerStatus::NeedsFloor;return plan;}
        floor=std::max(floor,*supplied);}
    Class60_06ShadowRecord r;r.point={{work.raw[0],signed16(uint16_t(floor)),work.raw[2]}};
    r.heading_scale=uint16_t((work.heading56&4095u)|(uint32_t(shift6(scale))<<12));
    r.radius_type=uint16_t(radius|(type<<14));r.height_delta=uint16_t(uint32_t(floor)-uint32_t(int32_t(work.raw[1])));
    plan.append=r;return plan;
}
SourceConsumerStatus append_source_shadow_8002c8cc(
    SourceShadowQueue& q,const SourceShadowWork& work,uint32_t radius,int32_t scale,
    uint32_t type,std::optional<int32_t> supplied) noexcept {
    const auto plan=plan_source_shadow_8002c8cc(q.count,work,radius,scale,type,supplied);
    if(plan.append)q.records[q.count++]=*plan.append;
    return plan.status;
}
} //namespace l3
