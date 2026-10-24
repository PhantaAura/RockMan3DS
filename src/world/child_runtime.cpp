#include "legends3ds/child_runtime.h"
#include <tuple>

namespace l3 {
namespace {
uint16_t read16(const Class60_06Work& w,size_t offset) noexcept {
    return uint16_t(uint16_t(w[offset])|(uint16_t(w[offset+1])<<8));
}
uint32_t read32(const Class60_06Work& w,size_t offset) noexcept {
    uint32_t value=0;
    for(size_t i=0;i<4;++i)value|=uint32_t(w[offset+i])<<(i*8);
    return value;
}
int16_t signed16(uint16_t value) noexcept {
    return value<32768u?int16_t(value):int16_t(int32_t(value)-65536);
}
void write16(Class60_06Work& w,size_t offset,uint16_t value) noexcept {
    w[offset]=uint8_t(value);w[offset+1]=uint8_t(value>>8);
}
void write32(Class60_06Work& w,size_t offset,uint32_t value) noexcept {
    for(size_t i=0;i<4;++i)w[offset+i]=uint8_t(value>>(i*8));
}
bool same_audio(const SourceAudioQueue& a,const SourceAudioQueue& b) noexcept {
    if(a.count!=b.count || a.initialized!=b.initialized)return false;
    for(size_t i=0;i<a.records.size();++i)
        if(a.records[i].raw!=b.records[i].raw)return false;
    return true;
}
bool audio_ready(const SourceAudioQueue& q) noexcept {
    return q.initialized && q.count<=q.records.size();
}
bool same_behavior(const Class20_03BehaviorState& a,const Class20_03BehaviorState& b) noexcept {
    const auto fields=[](const Class20_03BehaviorState& w) noexcept {
        return std::tie(w.parameter_03,w.action_09,w.phase_0a,w.animation_ac,
            w.flags_328,w.timer_32c,w.work_330,w.heading_56,w.turn_66,w.work_36c);
    };
    return fields(a)==fields(b);
}
bool same_shadow(const Class60_06ShadowRecord& a,const Class60_06ShadowRecord& b) noexcept {
    return a.point==b.point && a.heading_scale==b.heading_scale &&
        a.radius_type==b.radius_type && a.height_delta==b.height_delta;
}
bool append_audio(SourceAudioQueue& q,uint16_t cue,
    const std::optional<std::array<uint32_t,2>>& pointed,
    uint8_t& queued,uint8_t& dropped) noexcept {
    if(!pointed)return false;
    const auto status=enqueue_source_audio_8001a050(q,cue,*pointed);
    if(status==SourceAudioQueueStatus::Queued){++queued;return true;}
    if(status==SourceAudioQueueStatus::DroppedFull){++dropped;return true;}
    return false;
}
SourceHitProxyTarget proxy_target(uint32_t identity,const Class60_06Work& work) noexcept {
    SourceHitProxyTarget target;target.source_address=identity+0x9cu;
    target.packet_00=read32(work,0x9c);target.flags_04=read32(work,0xa0);
    for(size_t i=0;i<3;++i)target.point_08[i]=signed16(read16(work,0xa4+i*2));
    target.word_0e=read16(work,0xaa);return target;
}
SourceHitProxyTarget buster_target(uint32_t identity,const SourceBusterWork& work) noexcept {
    SourceHitProxyTarget target;target.source_address=identity+0xc4u;
    target.packet_00=read32(work,0xc4);target.flags_04=read32(work,0xc8);
    for(size_t i=0;i<3;++i)target.point_08[i]=signed16(read16(work,0xcc+i*2));
    target.word_0e=read16(work,0xd2);return target;
}
uint32_t buster_bit(size_t slot) noexcept {return 0x80000000u>>slot;}
bool valid_buster_pool(const SourceBusterPool& pool) noexcept {
    return (pool.pending_release_80098a60&~pool.allocated_800989f0)==0;
}
void effect_write16(SourceEffectWork& work,size_t offset,uint16_t value) noexcept {
    work[offset]=uint8_t(value);work[offset+1]=uint8_t(value>>8);
}
} // namespace

ActorFloorContact query_class60_shadow_floor_80048e6c(
    const Class60_06Work& work,const RuntimeMap& map,size_t area) noexcept {
    if(read32(work,0x78)!=0)return {};
    ActorContactWork argument;argument.object_class=work[1];
    for(size_t i=0;i<4;++i){
        argument.raw[i]=signed16(read16(work,0x14+i*2));
        argument.previous_raw[i]=signed16(read16(work,0x2c+i*2));
    }
    return query_actor_floor_contact_80048ea4(map,area,argument,{0,0,0,0,0,0});
}

ChildRuntime::ChildRuntime() noexcept { initialize(); }
void initialize_source_buster_pool_8002e7f4(SourceBusterPool& pool) noexcept {
    for(size_t i=0;i<pool.slots.size();++i){
        pool.slots[i].fill(0);pool.slots[i][1]=uint8_t(16u|i);
    }
    pool.allocated_800989f0=0;pool.pending_release_80098a60=0;
}
void ChildRuntime::initialize() noexcept {
    initialize_class60_source_pool_8002e8d8(pool_);
    initialize_source_effect_pool_8002e924(effects_);
    initialize_source_buster_pool_8002e7f4(busters_);
    buster_segments_={};
    rewards_={};
    proxies_.count=0;registry_.count=0;shadows_.count=0;
    owner_identities_.fill(0);owner_points_={};
    for(size_t i=0;i<targets_.size();++i)
        targets_[i]=proxy_target(class60_source_pool_base+uint32_t(i*class60_slot_bytes),pool_.slots[i]);
    for(size_t i=0;i<buster_targets_.size();++i)
        buster_targets_[i]=buster_target(source_buster_base+uint32_t(i*class60_slot_bytes),busters_.slots[i]);
}

SourceBusterSpawn ChildRuntime::spawn_source_buster_80035724(
    SourceBusterPlayer& player,uint32_t stack10,int32_t selected,
    std::optional<uint32_t>& mask) noexcept {
    SourceBusterSpawn result;
    if((busters_.pending_release_80098a60&~busters_.allocated_800989f0)!=0){
        result.status=SourceBusterStatus::InvalidPool;return result;
    }
    size_t slot=0;
    while(slot<source_buster_capacity &&
          (busters_.allocated_800989f0&(0x80000000u>>slot)))++slot;
    // Original exhaustion returns before reading the selected target/player.
    if(slot==source_buster_capacity){result.status=SourceBusterStatus::SourceBackedPartial;return result;}
    auto& lane=player.lanes[stack10?0:1];
    if(!lane.muzzle_3c8_or_3e0)return result;
    if(!lane.subtype_80089a8b){result.status=SourceBusterStatus::NeedsSubtype;return result;}
    if(!lane.counter_4fa){result.status=SourceBusterStatus::NeedsCounter;return result;}
    if(!player.class_01 || !player.heading_56){result.status=SourceBusterStatus::NeedsPlayerFields;return result;}
    SourceHitProxyTarget* target=nullptr;
    if(selected>=0){
        if(proxies_.count>proxies_.records.size()){result.status=SourceBusterStatus::InvalidProxy;return result;}
        if(size_t(selected)>=proxies_.count){result.status=SourceBusterStatus::NeedsProxy;return result;}
        const auto& row=proxies_.records[size_t(selected)];target=row.target;
        if(!target || (target->source_address&0x007fffffu)!=(row.encoded_identity&0x007fffffu)){
            result.status=SourceBusterStatus::InvalidProxy;return result;
        }
        if(!mask){result.status=SourceBusterStatus::NeedsProxyMask;return result;}
    }
    const uint32_t identity=source_buster_base+uint32_t(slot*class60_slot_bytes);
    auto next=busters_.slots[slot];
    // Exact80031598 reuse.00/01 and fields outside these ranges survive.
    write32(next,0x10,0);write32(next,0x78,0);write16(next,2,0);
    write32(next,4,0);write32(next,0x0c,0);write32(next,8,0);
    next[6]=3;write32(next,0x74,0);next[3]=0;
    if(target){
        SourceHitProxyOwnerPoint point{{read32(next,0xbc),read32(next,0xc0)}};
        const auto lease=allocate_proxy_lease(mask,identity,point,*target);
        result.lease_index=lease.index;next[0x0d]=uint8_t(lease.index);
        if(lease.index>=0)next[3]=0x80;
    }
    next[0]=1;next[2]=*lane.subtype_80089a8b;
    for(size_t i=0;i<3;++i){
        const auto raw=(*lane.muzzle_3c8_or_3e0)[i];
        write16(next,0x14+i*2,raw);write32(next,0x1c+i*4,uint32_t(raw)<<16);
    }
    const int selected_row=lane.selection_170_or_171;
    if(*lane.counter_4fa!=32767 && selected_row!=4 && selected_row!=7 &&
       selected_row!=12 && selected_row!=13){
        lane.counter_4fa=signed16(uint16_t(uint16_t(*lane.counter_4fa)-1u));
        result.counter_written=true;
    }
    write16(next,0x1a,uint16_t(stack10));next[0x0c]=*player.class_01;
    write16(next,0x64,*player.heading_56);
    busters_.slots[slot]=next;busters_.allocated_800989f0|=0x80000000u>>slot;
    result.status=SourceBusterStatus::SourceBackedPartial;result.source_identity=identity;
    return result;
}

SourceBusterBatch ChildRuntime::tick_source_busters_80017154(
    const SourceBusterFrameContext& context) noexcept {
    SourceBusterBatch batch;
    if(!valid_buster_pool(busters_)){
        batch.status=SourceBusterCallbackStatus::InvalidPool;batch.held=1;return batch;
    }
    const auto hold=[&](SourceBusterCallbackStatus status) noexcept {
        if(batch.held==0)batch.status=status;
        if(batch.held!=255)++batch.held;
    };
    for(size_t slot=0;slot<source_buster_capacity;++slot){
        if(!(busters_.allocated_800989f0&buster_bit(slot)))continue;
        auto& retained=busters_.slots[slot];
        if(!(int8_t(retained[0])&retained[6]&1))continue;
        if(retained[2]==0x81){
            // Mega Buster shot: 0x800437A8 states 0x800437F8 / 0x80043A8C.
            if(retained[8]>1){hold(SourceBusterCallbackStatus::UnknownState);continue;}
            if(!context.shot81||!context.references){hold(SourceBusterCallbackStatus::NeedsShotProfile);continue;}
            if(!context.proxy_mask_80098798||!*context.proxy_mask_80098798){hold(SourceBusterCallbackStatus::NeedsProxyMask);continue;}
            if(!context.audio||!audio_ready(*context.audio)){hold(SourceBusterCallbackStatus::NeedsSoundConsumer);continue;}
            const auto& stats=*context.shot81;
            auto next=retained;
            auto target=buster_target(source_buster_base+uint32_t(slot*class60_slot_bytes),next);
            auto mask=*context.proxy_mask_80098798;
            const auto release_lease=[&]() noexcept {
                if(next[3]>0x7f)(void)source_hit_proxy_release_lease_800354e0(mask,next[0x0d]);
            };
            // 0x80043E78: current +14 to previous +2C, both radius +98.
            const auto registration=[&]() {
                SourceHitProxyCompositeRegistration input;
                input.first_words={{read32(next,0x14),read32(next,0x18)}};
                input.second_words={{read32(next,0x2c),read32(next,0x30)}};
                input.first_extent=input.second_extent=read16(next,0x98);
                input.stack10=0x200;input.stack14=next[0x0c]?0x6000u:0x4000u;
                input.stack18=next[1];
                input.stack20=uint32_t(int32_t(stats.damage_800375d4))|0x08000000u;
                input.stack24=0;input.stack28=0x80;input.target=&target;input.mask_80098798=mask;
                input.midpoint_padding_06=context.midpoint_padding_06;
                for(size_t i=0;i<owner_identities_.size();++i)
                    if(owner_identities_[i])input.owners_800be478[i]=&owner_points_[i];
                return input;
            };
            const auto raw_from_fixed=[&]() noexcept {
                write16(next,0x14,read16(next,0x1e));write16(next,0x16,read16(next,0x22));write16(next,0x18,read16(next,0x26));
            };
            const auto previous_from_current=[&]() noexcept {
                for(size_t i=0;i<16;i+=4)write32(next,0x34+i,read32(next,0x1c+i));
                write32(next,0x2c,read32(next,0x14));write32(next,0x30,read32(next,0x18));
            };
            const auto publish=[&](const SourceHitProxyCompositeRegistration& input) {
                buster_targets_[slot]=target;auto committed=input;committed.target=&buster_targets_[slot];
                (void)source_hit_proxy_register_composite_8003430c(proxies_,committed);
                const auto point=[](const std::array<uint32_t,2>& w) {
                    return std::array<int16_t,3>{{int16_t(uint16_t(w[0])),int16_t(uint16_t(w[0]>>16)),int16_t(uint16_t(w[1]))}};
                };
                buster_segments_[slot]={point(input.first_words),point(input.second_words)};
                write32(next,0xc4,buster_targets_[slot].packet_00);write32(next,0xc8,buster_targets_[slot].flags_04);
                ++batch.registered;
            };
            const auto release=[&]() noexcept {
                next[0]=0;busters_.pending_release_80098a60=(busters_.pending_release_80098a60|buster_bit(slot))&busters_.allocated_800989f0;
                ++batch.released;
            };
            if(next[8]==0){
                if(context.aim_pitch_80039b80!=0){hold(SourceBusterCallbackStatus::UnsupportedAimPitch);continue;}
                next[0]|=0x2a;next[6]|=0x20;
                write32(next,0xc4,0);write32(next,0xc8,0);
                // The proxy target is +0xC4 itself: rebuild it after the clear,
                // or a reused slot republishes its last hit packet and the new
                // shot "impacts" on its second frame.
                target=buster_target(source_buster_base+uint32_t(slot*class60_slot_bytes),next);
                write32(next,0x78,0x80097da0u);
                // RotMatrix(pitch 0, heading +64, 0) applied to (0,0,0xF000):
                // the third column of RotY, exactly -sin / 0 / -cos in q12.
                const auto& trig=context.references->trig[read16(next,0x64)&0xfffu];
                const std::array<int32_t,3> unit{{-int32_t(trig.sin_q12),0,-int32_t(trig.cos_q12)}};
                for(size_t i=0;i<3;++i)write16(next,0x44+i*2,uint16_t(int16_t((unit[i]*int32_t(stats.speed))>>4)));
                write32(next,0xa4,uint32_t(uint16_t(unit[0]))|(uint32_t(uint16_t(unit[1]))<<16));
                write32(next,0xa8,uint32_t(uint16_t(unit[2])));
                for(size_t i=0x4c;i<0x52;++i)next[i]=0;
                raw_from_fixed();previous_from_current();
                next[0x7c]=0;next[0x7d]=0;next[0x0b]=0;
                write16(next,0x98,stats.radius_98);
                next[9]=0;++next[8];
                const uint16_t extent=next[0x0a]?0x6208u:0x4208u;
                write16(next,0x1a,extent);write16(next,0x32,extent);
                if(next[3]>0x7f&&read32(next,0xbc)==0&&read32(next,0xc0)==0){release_lease();next[3]=0;}
                const auto input=registration();
                if(source_hit_proxy_check_register_composite_8003430c(proxies_,input).status!=SourceHitProxyStatus::SourceBackedPartial){
                    hold(SourceBusterCallbackStatus::ProxyHeld);continue;
                }
                audio_scratch_=*context.audio;
                const auto audio=enqueue_source_audio_8001a050(audio_scratch_,0x9a,{{read32(next,0x14),read32(next,0x18)}});
                if(audio!=SourceAudioQueueStatus::Queued&&audio!=SourceAudioQueueStatus::DroppedFull){
                    hold(SourceBusterCallbackStatus::NeedsSoundConsumer);continue;
                }
                publish(input);*context.audio=audio_scratch_;
                retained=next;*context.proxy_mask_80098798=mask;++batch.committed;continue;
            }
            // State 1. 0x80037694: a published pair packet ends the shot,
            // else 0x80049C24 tests +14 against the map (bounds +78).
            uint32_t hit=0;
            if(read32(next,0xc4)&0xff000000u)hit=0x7fff;
            else {
                if(!context.map){hold(SourceBusterCallbackStatus::NeedsMapCollision);continue;}
                const int16_t px=signed16(read16(next,0x14)),py=signed16(read16(next,0x16)),pz=signed16(read16(next,0x18));
                const int16_t qx=signed16(read16(next,0x2c)),qz=signed16(read16(next,0x30));
                const auto far=[](int16_t v){return (v<0?-int32_t(v):int32_t(v))>0x4000;};
                if(((uint16_t(px)^uint16_t(qx))&0x8000u)&&far(px))hit=0x80000000u;
                else if(((uint16_t(pz)^uint16_t(qz))&0x8000u)&&far(pz))hit=0x80000000u;
                else {
                    // 0x80097DA0 bounds; class 0x10 skips flag-0x8000 primitives.
                    const auto test=runtime_point_embedded_80049c54(*context.map,context.area_index,px,py,pz,
                        RetailPlayerBounds{-16,16,-16,16,-16,16},true);
                    if(test.status==RuntimePointEmbedding::Unsupported){hold(SourceBusterCallbackStatus::NeedsMapCollision);continue;}
                    hit=test.raw;
                }
            }
            if(hit==0&&read16(next,0x7c)==0){
                previous_from_current();
                const uint8_t age=next[0x0b];++next[0x0b];
                if(int32_t(age)<int32_t(stats.range_80037634)){
                    // 0x80043B54: a leased shot (+3 bit 7) steers toward the target
                    // point +BC/+BE/+C0, or drops the lease when it is zero.
                    if(next[3]>0x7f){
                        if(read32(next,0xbc)==0&&read32(next,0xc0)==0){release_lease();next[3]=0;}
                        else {
                            if(!context.normalization){hold(SourceBusterCallbackStatus::NeedsNormalization);continue;}
                            std::array<int32_t,3> d{};
                            bool far=false;
                            for(size_t i=0;i<3;++i){
                                d[i]=int32_t(signed16(read16(next,0xbc+i*2)))-signed16(read16(next,0x14+i*2));
                                far=far||uint32_t(d[i]+26750)>53500u;
                            }
                            if(far)for(auto& v:d)v>>=2;                 // 0x80043BDC
                            const auto unit=source_normalize_8007a694(d,context.normalization);
                            const std::array<int16_t,3> previous{{signed16(read16(next,0xa4)),signed16(read16(next,0xa6)),signed16(read16(next,0xa8))}};
                            // +0B was already advanced: ages 1..3, 4..8, then 9+ (last turn).
                            const uint8_t aged=next[0x0b];
                            const int16_t turn=aged<4?1060:aged<9?143:71;
                            const uint32_t limit=aged<4?0x0F746EA4u:aged<9?0x0FFD813Cu:0x0FFF604Cu;
                            const auto steered=unit?source_interpolate_80033f04(*unit,previous,turn,limit,context.normalization):std::nullopt;
                            if(!steered){hold(SourceBusterCallbackStatus::NeedsNormalization);continue;}
                            if(aged>=9){release_lease();next[3]=0;}          // 0x80043C98
                            for(size_t i=0;i<3;++i){
                                write16(next,0xa4+i*2,uint16_t((*steered)[i]));
                                write16(next,0x44+i*2,uint16_t(int16_t((int32_t((*steered)[i])*int32_t(stats.speed))>>4)));
                            }
                        }
                    }
                    for(size_t i=0;i<3;++i){
                        const int32_t v=signed16(read16(next,0x44+i*2));
                        write32(next,0x1c+i*4,read32(next,0x1c+i*4)+uint32_t(v*0x100));
                    }
                    raw_from_fixed();
                    write16(next,0x9a,uint16_t(read16(next,0x9a)+5u));
                    const auto input=registration();
                    if(source_hit_proxy_check_register_composite_8003430c(proxies_,input).status!=SourceHitProxyStatus::SourceBackedPartial){
                        hold(SourceBusterCallbackStatus::ProxyHeld);continue;
                    }
                    publish(input);retained=next;*context.proxy_mask_80098798=mask;++batch.committed;continue;
                }
            } else {
                // 0x80043E00: impact effect 0x82; +0C marks a map hit (sound 0x9B).
                effect_scratch_=effects_;const auto effect=allocate_source_effect_80031618(effect_scratch_);
                if(effect.status!=SourceEffectStatus::SourceBackedPartial){hold(SourceBusterCallbackStatus::EffectHeld);continue;}
                if(effect.source_identity){
                    const auto effect_slot=source_effect_slot(effect.source_identity);
                    if(!effect_slot){hold(SourceBusterCallbackStatus::EffectHeld);continue;}
                    auto& work=effect_scratch_.slots[*effect_slot];work[0]=9;work[2]=0x82;work[3]=1;
                    work[0x0c]=uint8_t(hit!=0x7fff);
                    effect_write16(work,0x1c,read16(next,0x14));
                    effect_write16(work,0x1e,read16(next,0x16));
                    effect_write16(work,0x20,read16(next,0x18));
                }
                effects_=effect_scratch_;++batch.impacts;if(hit!=0x7fff)++batch.map_impacts;
            }
            release();release_lease();
            retained=next;*context.proxy_mask_80098798=mask;++batch.committed;continue;
        }
        if(retained[2]!=0x80){hold(SourceBusterCallbackStatus::UnsupportedSubtype);continue;}
        if(retained[8]>2){hold(SourceBusterCallbackStatus::UnknownState);continue;}
        if(!context.owner_busy_198 || !*context.owner_busy_198){hold(SourceBusterCallbackStatus::NeedsOwnerBusy);continue;}
        auto next=retained;
        auto target=buster_target(source_buster_base+uint32_t(slot*class60_slot_bytes),next);
        const auto endpoint=[&]() -> const std::optional<std::array<uint32_t,2>>& {
            return next[0x0e]?context.owner_endpoint_400:context.owner_endpoint_418;
        };
        const auto registration=[&](const std::array<uint32_t,2>& current) {
            SourceHitProxyCompositeRegistration input;
            input.first_words=current;
            input.second_words={{read32(next,0x2c),read32(next,0x30)}};
            input.first_extent=32;input.second_extent=32;input.stack10=0x200;
            input.stack14=int8_t(next[0x0c])?0x6000u:0x4000u;
            input.stack18=next[1];input.stack20=0x28000008u;
            input.stack24=0;input.stack28=0x80;input.target=&target;
            if(context.proxy_mask_80098798)input.mask_80098798=*context.proxy_mask_80098798;
            input.midpoint_padding_06=context.midpoint_padding_06;
            for(size_t i=0;i<owner_identities_.size();++i)
                if(owner_identities_[i])input.owners_800be478[i]=&owner_points_[i];
            return input;
        };
        const auto commit_registration=[&](SourceHitProxyCompositeRegistration input) {
            buster_targets_[slot]=target;input.target=&buster_targets_[slot];
            (void)source_hit_proxy_register_composite_8003430c(proxies_,input);
            const auto point=[](const std::array<uint32_t,2>& words) {
                return std::array<int16_t,3>{{int16_t(uint16_t(words[0])),int16_t(uint16_t(words[0]>>16)),int16_t(uint16_t(words[1]))}};
            };
            buster_segments_[slot]={point(input.first_words),point(input.second_words)};
            write32(next,0xc4,buster_targets_[slot].packet_00);
            write32(next,0xc8,buster_targets_[slot].flags_04);
            for(size_t axis=0;axis<3;++axis)write16(next,0xcc+axis*2,uint16_t(buster_targets_[slot].point_08[axis]));
            write16(next,0xd2,buster_targets_[slot].word_0e);
            write32(next,0x2c,read32(next,0x14));write32(next,0x30,read32(next,0x18));
            ++batch.registered;
        };
        if(next[8]==0){
            if(!context.proxy_mask_80098798 || !*context.proxy_mask_80098798){hold(SourceBusterCallbackStatus::NeedsProxyMask);continue;}
            if(!context.audio || !audio_ready(*context.audio)){hold(SourceBusterCallbackStatus::NeedsSoundConsumer);continue;}
            next[0x0e]=uint8_t(read16(next,0x1a));
            if(!endpoint()){hold(SourceBusterCallbackStatus::NeedsOwnerEndpoint);continue;}
            write32(next,0x2c,(*endpoint())[0]);write32(next,0x30,(*endpoint())[1]);
            write32(next,0x14,(*endpoint())[0]);write32(next,0x18,(*endpoint())[1]);
            write32(next,0xc4,0);write32(next,0xc8,0);write32(next,0x78,0);
            target=buster_target(source_buster_base+uint32_t(slot*class60_slot_bytes),next);
            auto input=registration(*endpoint());
            const auto checked=source_hit_proxy_check_register_composite_8003430c(proxies_,input);
            if(checked.status!=SourceHitProxyStatus::SourceBackedPartial){hold(SourceBusterCallbackStatus::ProxyHeld);continue;}
            audio_scratch_=*context.audio;
            const auto audio=enqueue_source_audio_8001a050(audio_scratch_,0x96,{{read32(next,0x14),read32(next,0x18)}});
            if(audio!=SourceAudioQueueStatus::Queued&&audio!=SourceAudioQueueStatus::DroppedFull){hold(SourceBusterCallbackStatus::NeedsSoundConsumer);continue;}
            auto mask=*context.proxy_mask_80098798;
            if(source_hit_proxy_release_lease_800354e0(mask,next[0x0d])!=SourceHitProxyStatus::SourceBackedPartial){hold(SourceBusterCallbackStatus::NeedsProxyMask);continue;}
            next[0x0b]=11;++next[8];
            commit_registration(input);retained=next;*context.proxy_mask_80098798=mask;
            *context.audio=audio_scratch_;**context.owner_busy_198=0;++batch.committed;continue;
        }
        if(next[8]==1){
            if(!context.owner_position_14){hold(SourceBusterCallbackStatus::NeedsOwnerPosition);continue;}
            if(!endpoint()){hold(SourceBusterCallbackStatus::NeedsOwnerEndpoint);continue;}
            if(!context.map_collision_80049c24){hold(SourceBusterCallbackStatus::NeedsMapCollision);continue;}
            write32(next,0x2c,(*context.owner_position_14)[0]);write32(next,0x30,(*context.owner_position_14)[1]);
            --next[0x0b];
            if(next[0x0b]==0){
                next[0]=0;busters_.pending_release_80098a60=(busters_.pending_release_80098a60|buster_bit(slot))&busters_.allocated_800989f0;
                retained=next;++batch.committed;++batch.released;continue;
            }
            const bool impact=(target.packet_00&0xff000000u)!=0||*context.map_collision_80049c24;
            if(impact){
                effect_scratch_=effects_;const auto effect=allocate_source_effect_80031618(effect_scratch_);
                if(effect.status!=SourceEffectStatus::SourceBackedPartial){hold(SourceBusterCallbackStatus::EffectHeld);continue;}
                if(effect.source_identity){
                    const auto effect_slot=source_effect_slot(effect.source_identity);
                    if(!effect_slot){hold(SourceBusterCallbackStatus::EffectHeld);continue;}
                    auto& work=effect_scratch_.slots[*effect_slot];work[0]=1;work[2]=0x84;work[3]=0;
                    effect_write16(work,0x1c,read16(next,0x14));
                    effect_write16(work,0x1e,read16(next,0x16));
                    effect_write16(work,0x20,read16(next,0x18));
                }
                effects_=effect_scratch_;++next[8];retained=next;**context.owner_busy_198=1;
                ++batch.committed;++batch.impacts;continue;
            }
            write32(next,0x14,(*endpoint())[0]);write32(next,0x18,(*endpoint())[1]);
            auto input=registration(*endpoint());
            const auto checked=source_hit_proxy_check_register_composite_8003430c(proxies_,input);
            if(checked.status!=SourceHitProxyStatus::SourceBackedPartial){hold(SourceBusterCallbackStatus::ProxyHeld);continue;}
            commit_registration(input);retained=next;++batch.committed;continue;
        }
        next[0]=0;busters_.pending_release_80098a60=(busters_.pending_release_80098a60|buster_bit(slot))&busters_.allocated_800989f0;
        retained=next;**context.owner_busy_198=0;++batch.committed;++batch.released;
    }
    return batch;
}

ChildRuntimeStatus ChildRuntime::prepare_parent_delivery(
    const Class20_03CallbackState& parent,const ReferenceTables& refs,
    const SourceAudioQueue& audio,ParentChildDelivery& delivery) noexcept {
    delivery.prepared_=false;delivery.owner_=nullptr;
    if(!class20_03_plan_action4(parent).spawn_child)return ChildRuntimeStatus::NotSpawnBranch;
    if(parent.variant_32e!=0 && pool_.allocated_bitmap!=0xffffffffu)
        return ChildRuntimeStatus::UnsupportedSubtype;
    producer_scratch_=pool_;
    const auto produced=class20_03_produce_child_80104078(
        parent,producer_scratch_,refs,audio_ready(audio));
    if(produced.status!=Class20_03Action4Status::SourceBackedPartial)
        return ChildRuntimeStatus::NeedsSoundConsumer;
    delivery.audio_before_=audio;delivery.audio_after_=audio;
    delivery.queued_=0;delivery.dropped_=0;
    if(produced.sound_8001a050 && !append_audio(delivery.audio_after_,
        *produced.sound_8001a050,produced.sound_pointed_words,delivery.queued_,delivery.dropped_))
        return ChildRuntimeStatus::NeedsSoundConsumer;
    delivery.identity_=produced.source_identity;
    delivery.allocated_before_=pool_.allocated_bitmap;
    delivery.pending_before_=pool_.pending_release_bitmap;
    delivery.allocated_after_=producer_scratch_.allocated_bitmap;
    if(const auto slot=class60_source_slot(produced.source_identity)){
        delivery.slot_before_=pool_.slots[*slot];delivery.slot_after_=producer_scratch_.slots[*slot];
    }else if(produced.source_identity!=0)return ChildRuntimeStatus::InvalidIdentity;
    delivery.behavior_=parent.behavior;delivery.raw_=parent.raw_14;
    delivery.fixed_=parent.fixed_1c;delivery.variant_=parent.variant_32e;
    delivery.owner_=this;delivery.prepared_=true;
    return ChildRuntimeStatus::SourceBackedPartial;
}
bool ChildRuntime::current_pool_matches(const ParentChildDelivery& d) const noexcept {
    if(!d.prepared_ || d.owner_!=this || pool_.allocated_bitmap!=d.allocated_before_ ||
       pool_.pending_release_bitmap!=d.pending_before_)return false;
    if(const auto slot=class60_source_slot(d.identity_))return pool_.slots[*slot]==d.slot_before_;
    return d.identity_==0 && pool_.allocated_bitmap==0xffffffffu;
}
bool ChildRuntime::validate_parent_delivery(const ParentChildDelivery& d,
    const Class20_03CallbackState& parent,const SourceAudioQueue& audio) const noexcept {
    return current_pool_matches(d) && same_behavior(d.behavior_,parent.behavior) &&
        d.raw_==parent.raw_14 && d.fixed_==parent.fixed_1c && d.variant_==parent.variant_32e &&
        class20_03_plan_action4(parent).spawn_child && same_audio(d.audio_before_,audio);
}
bool ChildRuntime::commit_parent_delivery(ParentChildDelivery& d) noexcept {
    if(!current_pool_matches(d))return false;
    if(const auto slot=class60_source_slot(d.identity_)){
        pool_.slots[*slot]=d.slot_after_;
        targets_[*slot]=proxy_target(d.identity_,d.slot_after_);
    }
    pool_.allocated_bitmap=d.allocated_after_;d.prepared_=false;return true;
}

ChildRuntimeStatus ChildRuntime::prepare_enemy_defeat_delivery(
    const std::array<int16_t,4>& point,std::optional<uint32_t> reward_word,
    const SourceAudioQueue& audio,EnemyDefeatDelivery& d,
    const std::array<int16_t,4>& effect_point) noexcept {
    d.prepared_=false;d.owner_=nullptr;
    if(!reward_word)return ChildRuntimeStatus::NeedsRewardProfile;
    if(rewards_.count>rewards_.records.size())return ChildRuntimeStatus::InvalidPool;
    if(rewards_.count==rewards_.records.size())return ChildRuntimeStatus::RewardQueueFull;
    if(!audio_ready(audio))return ChildRuntimeStatus::NeedsSoundConsumer;
    effect_scratch_=effects_;
    d.allocated_before_=effects_.allocated_80098838;
    d.pending_before_=effects_.pending_release_800988c8;
    const auto allocation=allocate_source_effect_80031618(effect_scratch_);
    if(allocation.status!=SourceEffectStatus::SourceBackedPartial)
        return ChildRuntimeStatus::EffectHeld;
    d.effect_identity_=allocation.source_identity;
    d.effect_slot_=source_effect_slot(allocation.source_identity);
    if(d.effect_slot_){
        d.slot_before_=effects_.slots[*d.effect_slot_];
        auto& work=effect_scratch_.slots[*d.effect_slot_];
        work[0]=5;work[2]=128;work[3]=65;
        for(size_t i=0;i<4;++i){work[0x38+i*2]=uint8_t(effect_point[i]);work[0x39+i*2]=uint8_t(uint16_t(effect_point[i])>>8);}
        d.slot_after_=work;
    }
    d.allocated_after_=effect_scratch_.allocated_80098838;
    d.pending_after_=effect_scratch_.pending_release_800988c8;
    d.reward_count_before_=rewards_.count;
    d.reward_.point_180=point;d.reward_.source_descriptor_word00=*reward_word;
    d.audio_before_=audio;d.audio_after_=audio;d.queued_=0;d.dropped_=0;
    const std::array<uint32_t,2> words{{uint32_t(uint16_t(point[0]))|
        (uint32_t(uint16_t(point[1]))<<16),uint32_t(uint16_t(point[2]))|
        (uint32_t(uint16_t(point[3]))<<16)}};
    if(!append_audio(d.audio_after_,161,words,d.queued_,d.dropped_))
        return ChildRuntimeStatus::NeedsSoundConsumer;
    d.owner_=this;d.prepared_=true;return ChildRuntimeStatus::SourceBackedPartial;
}
bool ChildRuntime::validate_enemy_defeat_delivery(const EnemyDefeatDelivery& d,
    const SourceAudioQueue& audio) const noexcept {
    if(!d.prepared_||d.owner_!=this||d.reward_count_before_!=rewards_.count||
       d.allocated_before_!=effects_.allocated_80098838||
       d.pending_before_!=effects_.pending_release_800988c8||
       !same_audio(d.audio_before_,audio))return false;
    return !d.effect_slot_||effects_.slots[*d.effect_slot_]==d.slot_before_;
}
bool ChildRuntime::commit_enemy_defeat_delivery(EnemyDefeatDelivery& d) noexcept {
    if(!d.prepared_||d.owner_!=this||d.reward_count_before_!=rewards_.count||
       d.allocated_before_!=effects_.allocated_80098838||
       d.pending_before_!=effects_.pending_release_800988c8||
       (d.effect_slot_&&effects_.slots[*d.effect_slot_]!=d.slot_before_))return false;
    if(d.effect_slot_)effects_.slots[*d.effect_slot_]=d.slot_after_;
    effects_.allocated_80098838=d.allocated_after_;
    effects_.pending_release_800988c8=d.pending_after_;
    rewards_.records[rewards_.count++]=d.reward_;
    d.prepared_=false;return true;
}

ChildRuntimeTick ChildRuntime::tick(uint32_t identity,const ChildFrameContext& context) noexcept {
    ChildRuntimeTick tick;
    const auto slot=class60_source_slot(identity);
    if(!slot){tick.status=ChildRuntimeStatus::InvalidIdentity;return tick;}
    if((pool_.pending_release_bitmap&~pool_.allocated_bitmap)!=0){
        tick.status=ChildRuntimeStatus::InvalidPool;return tick;
    }
    if(!(pool_.allocated_bitmap&(0x80000000u>>*slot))){
        tick.status=ChildRuntimeStatus::InvalidIdentity;return tick;
    }
    const auto& original=pool_.slots[*slot];
    Class60_06Work candidate=original;
    Class60_06Inputs input;input.references=context.references;input.profile=context.profile;
    input.player_heading_800b5206=context.player_heading_800b5206;
    const bool has_audio=context.audio && audio_ready(*context.audio);
    if(has_audio)audio_scratch_=*context.audio;
    effect_scratch_=effects_;bool effects_staged=false;
    const auto allocation=[&]() noexcept -> std::optional<uint32_t> {
        const auto result=allocate_source_effect_80031618(effect_scratch_);
        tick.effect_status=result.status;
        if(result.status!=SourceEffectStatus::SourceBackedPartial)return std::nullopt;
        effects_staged=true;
        if(result.source_identity)++tick.effects_allocated;else ++tick.effects_exhausted;
        return result.source_identity;
    };
    auto result=class60_06_step_8010f354(candidate,input);
    if(result.status==Class60_06Status::NeedsContact && result.contact_request){
        if(!context.map){tick.status=ChildRuntimeStatus::NeedsMap;tick.lifecycle_status=result.status;return tick;}
        input.contact=prepare_class60_06_contact_8010fdd4(*result.contact_request,
            *context.map,context.area_index,
            context.profile?std::optional<uint8_t>(context.profile->source_stage):std::nullopt,
            has_audio,std::nullopt);
        if(input.contact->status==Class60_06Status::NeedsEffectConsumer){
            const auto identity=allocation();
            if(!identity){tick.status=ChildRuntimeStatus::EffectHeld;tick.lifecycle_status=input.contact->status;return tick;}
            input.contact=prepare_class60_06_contact_8010fdd4(*result.contact_request,
                *context.map,context.area_index,
                context.profile?std::optional<uint8_t>(context.profile->source_stage):std::nullopt,
                has_audio,identity);
        }
        if(input.contact->status!=Class60_06Status::SourceBackedPartial){
            tick.lifecycle_status=input.contact->status;return tick;
        }
        if(input.contact->effect){
            tick.effect_status=initialize_child_effect(effect_scratch_,*input.contact->effect);
            if(tick.effect_status!=SourceEffectStatus::SourceBackedPartial){
                tick.status=ChildRuntimeStatus::EffectHeld;tick.lifecycle_status=Class60_06Status::NeedsEffectConsumer;return tick;
            }
        }
        if(input.contact->sound && (!has_audio || !append_audio(audio_scratch_,
            *input.contact->sound,input.contact->sound_pointed_words,tick.audio_queued,tick.audio_dropped))){
            tick.status=ChildRuntimeStatus::NeedsSoundConsumer;
            tick.lifecycle_status=Class60_06Status::NeedsContactSound;return tick;
        }
        result=class60_06_step_8010f354(candidate,input);
    }
    if(result.status==Class60_06Status::NeedsEffectConsumer && result.effect_request){
        input.effect_allocation_80031618=allocation();
        if(!input.effect_allocation_80031618){tick.status=ChildRuntimeStatus::EffectHeld;tick.lifecycle_status=result.status;return tick;}
        result=class60_06_step_8010f354(candidate,input);
        if(result.effect_request){
            tick.effect_status=initialize_child_effect(effect_scratch_,*result.effect_request);
            if(tick.effect_status!=SourceEffectStatus::SourceBackedPartial){tick.status=ChildRuntimeStatus::EffectHeld;tick.lifecycle_status=Class60_06Status::NeedsEffectConsumer;return tick;}
        }
    }
    bool proxy_staged=false,registry_staged=false,shadow_staged=false;
    if(result.status==Class60_06Status::NeedsProxyConsumer && result.proxy_request){
        if(!context.proxy_mask_80098798){
            tick.status=ChildRuntimeStatus::NeedsProxyMask;tick.lifecycle_status=result.status;return tick;
        }
        const auto& request=*result.proxy_request;
        target_scratch_=proxy_target(identity,original);
        target_scratch_.packet_00=request.work9c;target_scratch_.flags_04=request.worka0;
        auto masked=request.worka0&*context.proxy_mask_80098798;
        if(masked&0x80u)masked&=0xffffff00u;
        if((masked&0xffffff00u) && !context.proxy_stack_padding_2e){
            tick.status=ChildRuntimeStatus::NeedsProxyPointPadding;
            tick.lifecycle_status=result.status;return tick;
        }
        owner_scratch_=owner_points_;
        SourceHitProxyRegistration registration;
        registration.pointed_words={{uint32_t(uint16_t(request.point[0]))|
            (uint32_t(uint16_t(request.point[1]))<<16),uint32_t(uint16_t(request.point[2]))|
            (uint32_t(context.proxy_stack_padding_2e.value_or(0))<<16)}};
        registration.a1=request.extent;registration.a2=0;registration.a3=request.mode;
        registration.stack10=request.source_class;registration.stack18=request.packet_mask;
        registration.stack1c=0;registration.stack20=request.extra_flags;
        registration.target=&target_scratch_;registration.mask_80098798=context.proxy_mask_80098798;
        for(size_t i=0;i<owner_identities_.size();++i) {
            if(!owner_identities_[i])continue;
            size_t canonical=i;
            for(size_t prior=0;prior<i;++prior)
                if(owner_identities_[prior]==owner_identities_[i]){canonical=prior;break;}
            registration.owners_800be478[i]=&owner_scratch_[canonical];
        }
        proxy_scratch_.count=proxies_.count;
        const auto proxy=source_hit_proxy_register_800341bc(proxy_scratch_,registration);
        tick.proxy_status=proxy.status;
        if(proxy.status!=SourceHitProxyStatus::SourceBackedPartial){
            tick.status=ChildRuntimeStatus::ProxyHeld;tick.lifecycle_status=result.status;return tick;
        }
        for(size_t i=0;i<owner_identities_.size();++i)
            if(registration.owners_800be478[i])owner_scratch_[i]=*registration.owners_800be478[i];
        proxy_staged=true;
        input.proxy=Class60_06ProxyCommit{request,target_scratch_.flags_04};
        result=class60_06_step_8010f354(candidate,input);
        // Original calls80051BAC even for inactive flags. Its exact producer
        // clears work7C; the lifecycle receipt exposes the active append test.
        registry_scratch_.count=registry_.count;
        uint16_t contact=read16(original,0x7c);
        tick.registry_status=register_source_collision_80051bac(registry_scratch_,
            identity,result.register_collision?3u:0u,contact);
        if(tick.registry_status!=SourceConsumerStatus::SourceBackedPartial){
            tick.status=ChildRuntimeStatus::RegistryHeld;tick.lifecycle_status=result.status;return tick;
        }
        registry_staged=true;input.collision_registry_ready=true;
        result=class60_06_step_8010f354(candidate,input);
    }
    if(result.status==Class60_06Status::NeedsShadowConsumer && result.shadow_request){
        const auto& shadow_work=*result.shadow_request;
        std::optional<int32_t> floor;
        if(shadows_.count<64 && shadow_work[0x76]==0){
            if(!context.map){tick.status=ChildRuntimeStatus::NeedsMap;tick.lifecycle_status=Class60_06Status::NeedsShadowFloor;return tick;}
            const auto query=query_class60_shadow_floor_80048e6c(shadow_work,*context.map,context.area_index);
            tick.floor_status=query.status;
            if(query.status!=RuntimeContactStatus::SourceBackedPartial){
                tick.status=ChildRuntimeStatus::ShadowHeld;tick.lifecycle_status=Class60_06Status::NeedsShadowFloor;return tick;
            }
            floor=query.result;
        }
        SourceShadowWork work;
        for(size_t i=0;i<3;++i)work.raw[i]=signed16(read16(shadow_work,0x14+i*2));
        work.heading56=read16(shadow_work,0x56);
        work.work76=shadow_work[0x76]<128?int8_t(shadow_work[0x76]):int8_t(int16_t(shadow_work[0x76])-256);
        shadow_scratch_.count=shadows_.count;
        tick.shadow_status=append_source_shadow_8002c8cc(shadow_scratch_,work,60,512,0,floor);
        if(tick.shadow_status!=SourceConsumerStatus::SourceBackedPartial){
            tick.status=ChildRuntimeStatus::ShadowHeld;tick.lifecycle_status=Class60_06Status::NeedsShadowFloor;return tick;
        }
        shadow_staged=true;input.shadow_count_800987d8=shadows_.count;
        input.shadow_floor_80048e6c=floor;
        result=class60_06_step_8010f354(candidate,input);
        if(result.status==Class60_06Status::SourceBackedPartial && result.shadow &&
           (shadow_scratch_.count<=shadows_.count || !same_shadow(*result.shadow,shadow_scratch_.records[shadows_.count]))){
            tick.status=ChildRuntimeStatus::ShadowHeld;tick.lifecycle_status=Class60_06Status::NeedsShadowConsumer;return tick;
        }
    }
    tick.lifecycle_status=result.status;
    if(result.status!=Class60_06Status::SourceBackedPartial)return tick;
    if(!shadow_staged || (result.proxy_request && (!proxy_staged || !registry_staged)))return tick;
    if(proxy_staged){
        targets_[*slot]=target_scratch_;
        auto row=proxy_scratch_.records[proxies_.count];row.target=&targets_[*slot];
        proxies_.records[proxies_.count++]=row;owner_points_=owner_scratch_;
        publish_buster_proxy_points();
    }
    if(registry_staged && registry_scratch_.count>registry_.count){
        const auto index=registry_.count;
        registry_.identities[index]=registry_scratch_.identities[index];++registry_.count;
    }
    if(shadow_staged && shadow_scratch_.count>shadows_.count){
        const auto index=shadows_.count;
        shadows_.records[index]=shadow_scratch_.records[index];++shadows_.count;
    }
    pool_.slots[*slot]=candidate;
    if(effects_staged)effects_=effect_scratch_;
    if(result.release_800318d8){class60_mark_release_800318d8(pool_,identity);tick.released=true;}
    if(has_audio)*context.audio=audio_scratch_;
    tick.status=ChildRuntimeStatus::SourceBackedPartial;tick.committed=true;return tick;
}

bool ChildRuntime::bind_proxy_owner(size_t index,uint32_t identity,SourceHitProxyOwnerPoint point) noexcept {
    if(index>=owner_identities_.size() || !identity || (identity&3u))return false;
    owner_identities_[index]=identity;
    for(size_t i=0;i<owner_identities_.size();++i)
        if(owner_identities_[i]==identity)owner_points_[i]=point;
    publish_buster_proxy_points();
    return true;
}
SourceHitProxyLeaseResult ChildRuntime::allocate_proxy_lease(std::optional<uint32_t>& mask,
    uint32_t identity,SourceHitProxyOwnerPoint point,SourceHitProxyTarget& target) noexcept {
    if(!identity || (identity&3u))return {SourceHitProxyStatus::NeedsOwnerPoint,-1};
    const auto result=source_hit_proxy_allocate_lease_8003545c(mask,owner_identities_,identity,target);
    if(result.status==SourceHitProxyStatus::SourceBackedPartial)
        for(size_t i=0;i<owner_identities_.size();++i)
            if(owner_identities_[i]==identity)owner_points_[i]=point;
    return result;
}
SourceHitProxyStatus ChildRuntime::release_proxy_lease(std::optional<uint32_t>& mask,
    uint32_t index) noexcept {
    // Retaining the table/point is source behavior; the mask controls admission.
    return source_hit_proxy_release_lease_800354e0(mask,index);
}
void ChildRuntime::clear_proxy_owner(size_t index) noexcept {
    if(index<owner_identities_.size())owner_identities_[index]=0;
}
bool ChildRuntime::enqueue_reward_80047228(const std::array<int16_t,4>& point, uint32_t word00) noexcept {
    if(rewards_.count>=rewards_.records.size())return false;
    SourceEnemyRewardRequest request;request.point_180=point;request.source_descriptor_word00=word00;
    rewards_.records[rewards_.count++]=request;return true;
}
std::array<SourceHitProxyOwnerPoint*,24> ChildRuntime::proxy_owner_pointers() noexcept {
    std::array<SourceHitProxyOwnerPoint*,24> out{};
    for(size_t i=0;i<owner_identities_.size();++i){
        if(!owner_identities_[i])continue;
        // Aliases of one identity share the first slot's point (source pointer identity).
        size_t canonical=i;
        for(size_t prior=0;prior<i;++prior)if(owner_identities_[prior]==owner_identities_[i]){canonical=prior;break;}
        out[i]=&owner_points_[canonical];
    }
    return out;
}
void ChildRuntime::publish_proxy_owner_points() noexcept {
    for(size_t i=0;i<owner_identities_.size();++i){
        if(!owner_identities_[i])continue;
        for(size_t prior=0;prior<i;++prior)
            if(owner_identities_[prior]==owner_identities_[i]){owner_points_[i]=owner_points_[prior];break;}
    }
    publish_buster_proxy_points();
}
const SourceHitProxyOwnerPoint* ChildRuntime::proxy_owner_point(size_t index) const noexcept {
    return index<owner_identities_.size() && owner_identities_[index]?&owner_points_[index]:nullptr;
}
void ChildRuntime::publish_buster_proxy_points() noexcept {
    for(size_t i=0;i<owner_identities_.size();++i){
        const auto identity=owner_identities_[i];
        if(identity<source_buster_base)continue;
        const auto offset=identity-source_buster_base;
        if(offset%class60_slot_bytes || offset/class60_slot_bytes>=source_buster_capacity)continue;
        auto& work=busters_.slots[offset/class60_slot_bytes];
        write32(work,0xbc,owner_points_[i].raw_bc_words[0]);
        write32(work,0xc0,owner_points_[i].raw_bc_words[1]);
    }
}
SourcePlayerLeaseResult ChildRuntime::consume_player_leases(SourceGteRtpsState& state,
    std::optional<std::array<uint32_t,8>> camera,std::optional<uint32_t> mask) noexcept {
    SourcePlayerLeaseInputs inputs;inputs.scratch_camera_1f800008=camera;inputs.mask_80098798=mask;
    // The source dereferences only selected owners. Duplicate identities share
    // one physical point; clearing it also affects aliases outside this mask.
    for(size_t i=0;i<owner_points_.size();++i) {
        if(!mask || !(*mask&(0x80000000u>>i)) || !owner_identities_[i])continue;
        size_t canonical=i;
        for(size_t prior=0;prior<i;++prior)
            if(inputs.owners_800be478[prior] && owner_identities_[prior]==owner_identities_[i]){canonical=prior;break;}
        if(owner_points_[canonical].raw_bc_words!=owner_points_[i].raw_bc_words)
            return {SourcePlayerProxyStatus::NeedsOwnerPoint};
        inputs.owners_800be478[i]=&owner_points_[canonical];
    }
    const auto result=source_player_consume_leases_80035508(state,inputs);
    if(result.status==SourcePlayerProxyStatus::SourceBackedPartial)
        for(size_t i=0;i<owner_points_.size();++i)if(inputs.owners_800be478[i])
            for(size_t alias=0;alias<owner_points_.size();++alias)
                if(owner_identities_[alias]==owner_identities_[i])
                    owner_points_[alias]=*inputs.owners_800be478[i];
    if(result.status==SourcePlayerProxyStatus::SourceBackedPartial)publish_buster_proxy_points();
    return result;
}
SourceHitProxyResult ChildRuntime::consume_proxy_pairs(const std::optional<SourceHitProxyRules>& rules) noexcept {
    uint32_t registered=0,buster_registered=0;
    if(proxies_.count<=proxies_.records.size())
        for(size_t row=0;row<proxies_.count;++row){
            for(size_t slot=0;slot<class60_pool_capacity;++slot)
                if(proxies_.records[row].target==&targets_[slot])registered|=0x80000000u>>slot;
            for(size_t slot=0;slot<source_buster_capacity;++slot)
                if(proxies_.records[row].target==&buster_targets_[slot])buster_registered|=buster_bit(slot);
        }
    const auto result=source_hit_proxy_consume_pairs_8003450c(proxies_,rules);
    if(result.status!=SourceHitProxyStatus::SourceBackedPartial)return result;
    for(size_t i=0;i<class60_pool_capacity;++i){
        if(!(pool_.allocated_bitmap&registered&(0x80000000u>>i)))continue;
        auto& work=pool_.slots[i];const auto& target=targets_[i];
        write32(work,0x9c,target.packet_00);write32(work,0xa0,target.flags_04);
        for(size_t axis=0;axis<3;++axis)write16(work,0xa4+axis*2,uint16_t(target.point_08[axis]));
        write16(work,0xaa,target.word_0e);
    }
    for(size_t i=0;i<source_buster_capacity;++i){
        if(!(busters_.allocated_800989f0&buster_registered&buster_bit(i)))continue;
        auto& work=busters_.slots[i];const auto& target=buster_targets_[i];
        write32(work,0xc4,target.packet_00);write32(work,0xc8,target.flags_04);
        for(size_t axis=0;axis<3;++axis)write16(work,0xcc+axis*2,uint16_t(target.point_08[axis]));
        write16(work,0xd2,target.word_0e);
    }
    return result;
}
void ChildRuntime::reset_frame_queues() noexcept {
    proxies_.count=0;registry_.count=0;shadows_.count=0;
}
bool ChildRuntime::mark_release(uint32_t identity) noexcept {
    const auto slot=class60_source_slot(identity);
    if(!slot || !(pool_.allocated_bitmap&(0x80000000u>>*slot)))return false;
    return class60_mark_release_800318d8(pool_,identity);
}
uint32_t ChildRuntime::spawn_effect_80031618(uint8_t work00,uint8_t subtype02,uint8_t parameter03,
    size_t lane_offset,const std::array<int16_t,4>& lanes,uint8_t lane_count) noexcept {
    effect_scratch_=effects_;
    const auto allocation=allocate_source_effect_80031618(effect_scratch_);
    if(allocation.status!=SourceEffectStatus::SourceBackedPartial||!allocation.source_identity)return 0;
    if(!write_source_effect_header(effect_scratch_,allocation.source_identity,work00,subtype02,
        parameter03,lane_offset,lanes,lane_count))return 0;
    effects_=effect_scratch_;return allocation.source_identity;
}
SourceEffectBatch ChildRuntime::tick_effects(const SourceEffect128Inputs& inputs) noexcept {
    return tick_source_effect_batch(effects_,inputs);
}
SourceRewardExpansion ChildRuntime::expand_next_enemy_reward(const SourceRewardProfile& profile,
    const ReferenceTables& refs,SourceRngService& rng,uint64_t gen) noexcept {
    SourceRewardExpansion out;out.status=SourceRewardStatus::Inactive;
    if(!rewards_.count)return out;
    const auto& request=rewards_.records[0];
    out=expand_source_reward_80046b34(effects_,profile,refs,rng,gen,request.point_180,request.source_descriptor_word00);
    if(out.status!=SourceRewardStatus::SourceBackedPartial)return out;
    for(size_t i=1;i<rewards_.count;++i)rewards_.records[i-1]=rewards_.records[i];
    rewards_.records[--rewards_.count]=SourceEnemyRewardRequest{};
    return out;
}
SourceRewardTick ChildRuntime::tick_reward_pickups(const SourceRewardProfile& profile,const ReferenceTables& refs,
    SourceRngService& rng,uint64_t gen,const SourceRewardWorld& world,int32_t& money,int16_t& hp,int16_t hp_max) noexcept {
    return tick_source_reward_pickups(effects_,profile,refs,rng,gen,world,money,hp,hp_max);
}
void ChildRuntime::consume_pending_releases() noexcept {
    consume_source_effect_releases_80031b84(effects_);
    class60_apply_release_mask_80031b3c(pool_,pool_.pending_release_bitmap);
    pool_.pending_release_bitmap=0;
    busters_.allocated_800989f0&=~busters_.pending_release_80098a60;
    busters_.pending_release_80098a60=0;
}
} // namespace l3
