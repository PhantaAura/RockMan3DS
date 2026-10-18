#include "legends3ds/class20_03_action4.h"
#include <limits>

namespace l3 {
namespace {
int16_t signed16(uint16_t w) noexcept {return w<0x8000u?int16_t(w):int16_t(int32_t(w)-65536);}
int32_t signed32(uint32_t w) noexcept {return w<=INT32_MAX?int32_t(w):int32_t(int64_t(w)-0x100000000LL);}
int16_t decrement(int16_t n) noexcept {return signed16(uint16_t(uint16_t(n)-1u));}
std::array<uint32_t,2> words(const std::array<int16_t,4>& r) noexcept {
    return {{uint32_t(uint16_t(r[0]))|(uint32_t(uint16_t(r[1]))<<16),
             uint32_t(uint16_t(r[2]))|(uint32_t(uint16_t(r[3]))<<16)}};
}
Class20_03Action4Result hold(Class20_03Action4Status s) noexcept {
    Class20_03Action4Result r;r.status=s;return r;
}
uint16_t read16(const std::array<uint8_t,class60_slot_bytes>& s,size_t o) noexcept {
    return uint16_t(uint16_t(s[o])|(uint16_t(s[o+1])<<8));
}
void write16(std::array<uint8_t,class60_slot_bytes>& s,size_t o,uint16_t v) noexcept {
    s[o]=uint8_t(v);s[o+1]=uint8_t(v>>8);
}
void write32(std::array<uint8_t,class60_slot_bytes>& s,size_t o,uint32_t v) noexcept {
    for(size_t i=0;i<4;++i)s[o+i]=uint8_t(v>>(i*8));
}
}
Class20_03Action4Plan class20_03_plan_action4(const Class20_03CallbackState& s) noexcept {
    Class20_03Action4Plan p;const auto& b=s.behavior;
    p.phase_after=b.phase_0a;p.request_after=b.animation_ac;p.timer_after=b.timer_32c;
    if(b.action_09!=4 || p.phase_after>4)return p;
    if(p.phase_after==0){p.entry_sound=true;p.phase_after=1;p.request_after=21;p.timer_after=13;}
    if(p.phase_after==1){
        p.timer_after=decrement(p.timer_after);
        if(p.timer_after>=0){p.forward=-112;return p;}
        p.phase_after=2;p.timer_after=s.variant_32e==0?16:0;
    }
    if(p.phase_after==2){
        p.timer_after=decrement(p.timer_after);
        if(p.timer_after<=0){p.phase_after=3;p.request_after=uint8_t(p.request_after+1u);p.timer_after=11;p.spawn_child=true;}
        return p;
    }
    if(p.phase_after==3){
        p.timer_after=decrement(p.timer_after);
        if(p.timer_after>=0){p.forward=signed16(uint16_t(-int32_t(p.timer_after)*32));return p;}
        p.phase_after=4;p.timer_after=6;
    }
    if(p.phase_after==4){
        p.timer_after=decrement(p.timer_after);
        if(p.timer_after>=0)p.forward=224;
        else p.install_byte_c=true;
    }
    return p;
}
Class20_03Action4Result class20_03_step_action4(
    Class20_03CallbackState& s,const Class20_03Action4Inputs& in) noexcept {
    using Status=Class20_03Action4Status;
    if(s.behavior.action_09!=4)return hold(Status::UnknownAction);
    const auto p=class20_03_plan_action4(s);
    if(p.entry_sound && !in.sound_consumer_ready)return hold(Status::NeedsSoundConsumer);
    if(p.forward){
        if(!in.prepared_motion)return hold(Status::NeedsMotion);
        if(in.prepared_motion->source_forward!=*p.forward || in.prepared_motion->heading!=s.behavior.heading_56)
            return hold(Status::StaleMotion);
    }
    if(p.spawn_child && !in.produced_child_374)return hold(Status::NeedsChildConsumer);
    if(p.spawn_child && *in.produced_child_374!=0 && !class60_source_slot(*in.produced_child_374))
        return hold(Status::NeedsChildConsumer);
    if(p.install_byte_c && !in.combat_record_byte_c)return hold(Status::NeedsCombatProfile);
    auto next=s;auto& b=next.behavior;
    b.phase_0a=p.phase_after;b.animation_ac=p.request_after;b.timer_32c=p.timer_after;
    Class20_03Action4Result out;
    if(p.entry_sound){b.work_330=0;out.sound_8001a050=177;out.sound_pointed_words=words(s.raw_14);}
    if(p.spawn_child){next.child_374=*in.produced_child_374;out.child_80104078=true;}
    if(p.install_byte_c){b.flags_328|=1u;next.work_35c=*in.combat_record_byte_c;}
    if(p.forward){
        const auto& m=*in.prepared_motion;next.raw_14=m.raw_14;next.fixed_1c=m.fixed_1c;
        next.raw_14[1]=signed16(uint16_t(m.source_floor_word));
        next.fixed_1c[1]=signed32(uint32_t(m.source_floor_word)<<16);
        const auto diff=int32_t(next.raw_14[1])-next.previous_raw_2c[1];
        if(diff>=33 || diff<=-33){next.raw_14=next.previous_raw_2c;next.fixed_1c=next.previous_fixed_34;}
    }
    s=next;return out;
}
std::optional<size_t> class60_source_slot(uint32_t p) noexcept {
    if(p<class60_source_pool_base)return std::nullopt;
    const auto off=p-class60_source_pool_base;
    if(off%class60_slot_bytes!=0 || off/class60_slot_bytes>=class60_pool_capacity)return std::nullopt;
    return size_t(off/class60_slot_bytes);
}
uint32_t class60_allocate_80031518(Class60SourcePool& pool) noexcept {
    for(size_t i=0;i<class60_pool_capacity;++i){
        const auto bit=0x80000000u>>i;if(pool.allocated_bitmap&bit)continue;
        auto& s=pool.slots[i];pool.allocated_bitmap|=bit;
        write32(s,0x10,0);write32(s,0x78,0);write16(s,2,0);
        write32(s,4,0);write32(s,0xc,0);write32(s,8,0);s[6]=3;write32(s,0x74,0);
        return class60_source_pool_base+uint32_t(i*class60_slot_bytes);
    }
    return 0;
}
bool class60_mark_release_800318d8(Class60SourcePool& pool,uint32_t identity) noexcept {
    const auto slot=class60_source_slot(identity);if(!slot)return false;
    const auto bit=0x80000000u>>*slot;
    pool.pending_release_bitmap=(pool.pending_release_bitmap|bit)&pool.allocated_bitmap;
    pool.slots[*slot][0]=0;return true;
}
void class60_apply_release_mask_80031b3c(Class60SourcePool& pool,uint32_t mask) noexcept {
    pool.allocated_bitmap&=~mask;
}
Class20_03ChildResult class20_03_produce_child_80104078(
    const Class20_03CallbackState& s,Class60SourcePool& pool,
    const ReferenceTables& refs,bool audio) noexcept {
    Class20_03ChildResult out;
    if(pool.allocated_bitmap==0xffffffffu)return out;
    if(s.variant_32e==0 && !audio){out.status=Class20_03Action4Status::NeedsSoundConsumer;return out;}
    const auto identity=class60_allocate_80031518(pool);out.source_identity=identity;
    auto& child=pool.slots[*class60_source_slot(identity)];child[0]=1;
    child[2]=s.variant_32e==0?6:9;child[3]=s.variant_32e==0?0:3;
    std::array<int32_t,3> fixed{{s.fixed_1c[0],
        signed32(uint32_t(s.fixed_1c[1])-uint32_t((s.variant_32e==0?60:80)*65536)),s.fixed_1c[2]}};
    if(s.variant_32e==0){write32(child,0x9c,32);write32(child,0xa0,16);out.sound_8001a050=178;out.sound_pointed_words=words(s.raw_14);}
    write16(child,0x64,s.behavior.heading_56);
    const auto delta=reference_player_motion_delta(refs,signed16(read16(child,0x64)),1760,0,0);
    const std::array<int32_t,3> d{{delta.dx_fixed,delta.dy_fixed,delta.dz_fixed}};
    for(size_t i=0;i<3;++i){
        fixed[i]=signed32(uint32_t(fixed[i])+uint32_t(d[i]));
        write32(child,0x1c+i*4,uint32_t(fixed[i]));
        write16(child,0x14+i*2,uint16_t(reference_fixed_to_short_80030150(fixed[i])));
    }
    return out;
}
} // namespace l3
