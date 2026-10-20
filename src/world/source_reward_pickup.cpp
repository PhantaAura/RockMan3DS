#include "legends3ds/source_reward_pickup.h"
#include "legends3ds/actor_contact.h"
#include "legends3ds/endian_io.h"

namespace l3 {
namespace {
constexpr size_t kPayloadBytes=256+34+10+10+10+16+96+48;
uint16_t g16(const SourceEffectWork& w,size_t o){return uint16_t(w[o]|(uint16_t(w[o+1])<<8));}
int16_t s16(const SourceEffectWork& w,size_t o){return int16_t(g16(w,o));}
void p16(SourceEffectWork& w,size_t o,uint32_t v){w[o]=uint8_t(v);w[o+1]=uint8_t(v>>8);}
void p32(SourceEffectWork& w,size_t o,uint32_t v){for(size_t i=0;i<4;++i)w[o+i]=uint8_t(v>>(8*i));}
uint32_t work_address(size_t slot){return source_effect_base+uint32_t(slot*source_effect_slot_bytes);}

// Staged rand() through the shared service: one transaction per caller.
struct RandStage {
    SourceRngService& rng;SourceRngUpdateTransaction txn;bool ok=false;
    RandStage(SourceRngService& r,uint64_t frame):rng(r){
        SourceRngUpdateContext c;c.native_frame=frame;c.native_update_ordinal=frame;
        c.phase=SourceRngUpdatePhase::RewardPickupCallback;
        ok=begin_source_rng_update(rng,txn,c)==SourceRngStatus::Ready;
    }
    std::optional<uint16_t> draw(uint32_t ra,uint32_t work,uint8_t slot,uint64_t gen){
        if(!ok)return {};
        SourceRngRewardContext c;c.caller_ra=ra;c.work=work;c.slot=slot;c.source_generation=gen;
        const auto s=stage_source_rng_reward(txn,c);
        if(s.status!=SourceRngStatus::Ready){ok=false;return {};}
        return s.sample32a;
    }
    bool commit(){
        if(!ok){rollback_source_rng_update(txn);return false;}
        if(!txn.call_count()){rollback_source_rng_update(txn);return true;}
        return commit_source_rng_update(rng,txn)==SourceRngStatus::Ready;
    }
    void rollback(){rollback_source_rng_update(txn);}
};

void reset_scale_matrix(SourceEffectWork& w){
    const uint16_t scale=g16(w,0x3e);
    for(size_t o:{0x1a,0x1c,0x1e,0x22,0x24,0x26})p16(w,o,0);
    p16(w,0x18,scale);p16(w,0x20,scale);p16(w,0x28,scale);
}
// 0x800469A4 (shard): scale matrix, then the spin angle +0xC advances 0x100.
// The RotMatrixY 0x8007BC68 product is rendering-only and is not formed here.
void spin_shard_800469a4(SourceEffectWork& w){reset_scale_matrix(w);p16(w,0x0c,uint16_t(s16(w,0x0c)+0x100));}
// 0x80046890 (energy): one rand(); +0xE += 0x80+(r&0x7F), +0xC += 0x40+(r&0x3F).
bool spin_energy_80046890(SourceEffectWork& w,RandStage& r,size_t slot,uint64_t gen){
    const auto v=r.draw(0x800468ecu,work_address(slot),uint8_t(slot),gen);if(!v)return false;
    p16(w,0x0e,uint16_t(s16(w,0x0e)+0x80+(*v&0x7f)));
    p16(w,0x0c,uint16_t(s16(w,0x0c)+0x40+(*v&0x3f)));
    return true;
}
// 0x8004AC8C: reflect velocity per contacted axis (0x300 X, 0xC00 Z, 0x3000 Y).
void reflect_8004ac8c(SourceEffectWork& w,int32_t r){
    const auto mag=[](int16_t v){return int16_t(v<0?-v:v);};
    int16_t vx=s16(w,0x2c),vy=s16(w,0x2e),vz=s16(w,0x30);
    const int16_t ox=vx,oz=vz;
    if(r&0x300){
        if(!(r&0xc00)) vx=(r&0x100)?int16_t(-mag(ox)):mag(ox);
        else {
            // Both horizontal axes: the original swaps X and Z magnitudes.
            vx=(r&0x100)?int16_t(-mag(oz)):mag(oz);
            vz=(r&0x400)?int16_t(-mag(ox)):mag(ox);
        }
    }
    if(!(r&0x300)&&(r&0xc00)) vz=(r&0x400)?int16_t(-mag(oz)):mag(oz);
    if(r&0x3000) vy=(r&0x1000)?int16_t(-mag(vy)):mag(vy);
    p16(w,0x2c,uint16_t(vx));p16(w,0x2e,uint16_t(vy));p16(w,0x30,uint16_t(vz));
}
struct Contact { bool ok=false; int32_t result=0; };
// 0x8004B5CC: 0x8004B6AC with the pickup bounds, contact state 1 and the
// 0x8000 primitive filter; negative restores the previous pose, positive
// copies the masked corrected axes.
Contact contact_8004b5cc(const SourceRewardWorld& world,SourceEffectWork& w,
                         const std::array<int16_t,3>& prev,const RetailPlayerBounds& b){
    Contact out;if(!world.map)return out;
    ActorContactWork work;work.object_class=0x20;work.state74=1;
    work.raw={s16(w,0x38),s16(w,0x3a),s16(w,0x3c),0};
    work.previous_raw={prev[0],prev[1],prev[2],0};
    const auto core=query_actor_wall_contact_8004b6ac(*world.map,world.area_index,work,b);
    if(core.status!=RuntimeContactStatus::SourceBackedPartial)return out;
    out.ok=true;out.result=core.result;
    if(core.result<0){p16(w,0x38,uint16_t(prev[0]));p16(w,0x3a,uint16_t(prev[1]));p16(w,0x3c,uint16_t(prev[2]));}
    else if(core.result>0){
        if(core.result&0x300)p16(w,0x38,uint16_t(core.scratch.x));
        if(core.result&0xc00)p16(w,0x3c,uint16_t(core.scratch.z));
        if(core.result&0x3000)p16(w,0x3a,uint16_t(core.scratch.y));
    }
    return out;
}
std::optional<int32_t> floor_80048ea4(const SourceRewardWorld& world,const SourceEffectWork& w,const RetailPlayerBounds& b){
    if(!world.map)return {};
    ActorContactWork work;work.object_class=0x20;work.state74=1;
    work.raw={s16(w,0x38),s16(w,0x3a),s16(w,0x3c),0};
    work.previous_raw={work.raw[0],int16_t(work.raw[1]-s16(w,0x2e)),work.raw[2],0};
    const auto f=query_actor_floor_contact_80048ea4(*world.map,world.area_index,work,b);
    if(f.status!=RuntimeContactStatus::SourceBackedPartial)return {};
    return f.result;
}
} // namespace

bool load_source_reward_profile(const std::vector<uint8_t>& b,SourceRewardProfile& out) noexcept {
    out=SourceRewardProfile{};
    if(b.size()!=32+kPayloadBytes||b[0]!='L'||b[1]!='3'||b[2]!='R'||b[3]!='W')return false;
    const auto u16=[&](size_t o){return uint16_t(b[o]|(uint16_t(b[o+1])<<8));};
    const auto u32=[&](size_t o){return uint32_t(b[o])|(uint32_t(b[o+1])<<8)|(uint32_t(b[o+2])<<16)|(uint32_t(b[o+3])<<24);};
    if(u16(4)!=1||u16(6)!=32||u32(8)!=b.size()||u32(12)!=kPayloadBytes)return false;
    if(crc32(b.data()+32,kPayloadBytes)!=u32(16))return false;
    size_t o=32;SourceRewardProfile p;
    for(auto& t:p.size_rolls_8008a5a8)for(auto& v:t)v=b[o++];
    for(auto& v:p.spread_80097efa){v=u16(o);o+=2;}
    for(auto& v:p.scale_80097ee0){v=u16(o);o+=2;}
    for(auto& v:p.speed_80097f1c){v=int16_t(u16(o));o+=2;}
    for(auto& v:p.lift_80097f24){v=int16_t(u16(o));o+=2;}
    for(auto& k:p.radius_80097eec)for(auto& v:k){v=u16(o);o+=2;}
    for(auto& k:p.bounds_8008a618)for(auto& v:k){
        v.x0=int16_t(u16(o));v.x1=int16_t(u16(o+2));v.z0=int16_t(u16(o+4));
        v.z1=int16_t(u16(o+6));v.y0=int16_t(u16(o+8));v.y1=int16_t(u16(o+10));o+=12;}
    for(auto& v:p.awards_8008a678){v=u16(o);o+=2;}
    p.loaded=true;out=p;return true;
}

SourceRewardExpansion expand_source_reward_80046b34(SourceEffectPool& pool,
    const SourceRewardProfile& profile,const ReferenceTables& refs,
    SourceRngService& rng,uint64_t gen,const std::array<int16_t,4>& point,uint32_t desc) noexcept {
    SourceRewardExpansion out;
    if(!profile.loaded)return out;
    if(!refs.scalar_quarter_known){out.status=SourceRewardStatus::NeedsTrig;return out;}
    if(!rng.admitted()){out.status=SourceRewardStatus::NeedsRng;return out;}
    uint32_t n0=desc&0xffu,n1=(desc>>16)&0xffu;const uint32_t t0=(desc>>8)&0xffu,t1=desc>>24;
    if(n0>16)n0=16;
    if(n1>16)n1=16;
    if((n0&&t0>=16)||(n1&&t1>=16)){out.status=SourceRewardStatus::UnsupportedTable;return out;}
    SourceEffectPool staged=pool;RandStage r(rng,gen);
    const auto first=r.draw(0x80046bacu,0,255,gen);if(!first){out.status=SourceRewardStatus::RngFailed;r.rollback();return out;}
    // Shards: angle = rand()&0xFFF stepped by spread[count].
    uint32_t angle=*first&0xfffu;
    const auto launch=[&](SourceEffectWork& w,uint8_t size,uint16_t roll,uint32_t a){
        w[10]=size;p16(w,0x3e,profile.scale_80097ee0[size]);
        p16(w,0x0c,roll&0xfffu);p16(w,0x0e,uint16_t(roll>>4));
        const int32_t extra=roll&3,speed=profile.speed_80097f1c[size]+extra;
        p16(w,0x2c,uint32_t((reference_rcos_8007a12c(refs,int32_t(a))*speed)>>12));
        p16(w,0x2e,uint32_t(profile.lift_80097f24[size]+extra));
        p16(w,0x30,uint32_t((reference_rsin_8007a060(refs,int32_t(a))*speed)>>12));
        p16(w,0x32,0);p16(w,0x34,5);p16(w,0x36,0);
    };
    for(uint32_t i=0;i<n0;++i,angle+=profile.spread_80097efa[n0]){
        const auto alloc=allocate_source_effect_80031618(staged);
        if(!alloc.source_identity){++out.pool_full;continue;}
        const size_t slot=*source_effect_slot(alloc.source_identity);auto& w=staged.slots[slot];
        w[0]=0x15;w[3]=0;w[2]=kSourceRewardPickupSubtype;
        if(!r.draw(0x80046bf0u,work_address(slot),uint8_t(slot),gen)){out.status=SourceRewardStatus::RngFailed;r.rollback();return out;}
        w[9]=0;
        p16(w,0x38,uint16_t(point[0]));p16(w,0x3a,uint16_t(point[1]));p16(w,0x3c,uint16_t(point[2]));p16(w,0x3e,uint16_t(point[3]));
        p16(w,0x3a,uint16_t(s16(w,0x3a)-0x14));
        const auto roll=r.draw(0x80046c30u,work_address(slot),uint8_t(slot),gen);
        if(!roll){out.status=SourceRewardStatus::RngFailed;r.rollback();return out;}
        const uint8_t size=profile.size_rolls_8008a5a8[t0][*roll&0xfu];
        if(size<5){
            p32(w,0x44,0x8009a6c0u+uint32_t(size)*20u);launch(w,size,*roll,angle);
            p32(w,0x40,0x2808080u);reset_scale_matrix(w);++out.spawned;
        } else {mark_source_effect_release_80031988(staged,alloc.source_identity);++out.discarded;}
    }
    // Energy: opposite side, angle = (last angle + 0x800) & 0xFFF.
    uint32_t angle1=(angle+0x800)&0xfffu;
    for(uint32_t i=0;i<n1;++i,angle1+=profile.spread_80097efa[n1]){
        const auto alloc=allocate_source_effect_80031618(staged);
        if(!alloc.source_identity){++out.pool_full;continue;}
        const size_t slot=*source_effect_slot(alloc.source_identity);auto& w=staged.slots[slot];
        w[0]=0x19;w[3]=1;w[2]=kSourceRewardPickupSubtype;
        if(!r.draw(0x80046dd8u,work_address(slot),uint8_t(slot),gen)){out.status=SourceRewardStatus::RngFailed;r.rollback();return out;}
        p32(w,0x44,0x8009a710u);w[9]=0;
        p16(w,0x38,uint16_t(point[0]));p16(w,0x3a,uint16_t(point[1]));p16(w,0x3c,uint16_t(point[2]));p16(w,0x3e,uint16_t(point[3]));
        const auto roll=r.draw(0x80046e18u,work_address(slot),uint8_t(slot),gen);
        if(!roll){out.status=SourceRewardStatus::RngFailed;r.rollback();return out;}
        const uint8_t size=profile.size_rolls_8008a5a8[t1][*roll&0xfu];
        if(size<5){launch(w,size,*roll,angle1);p32(w,0x40,0x2202020u);++out.spawned;}
        else {mark_source_effect_release_80031988(staged,alloc.source_identity);++out.discarded;}
    }
    if(!r.commit()){out.status=SourceRewardStatus::RngFailed;return out;}
    // Discarded slots release at the frame's 0x80031B84 like every free.
    pool=staged;out.status=SourceRewardStatus::SourceBackedPartial;return out;
}

SourceRewardTick tick_source_reward_pickups(SourceEffectPool& pool,
    const SourceRewardProfile& profile,const ReferenceTables& refs,
    SourceRngService& rng,uint64_t gen,const SourceRewardWorld& world,
    int32_t& money,int16_t& hp,int16_t hp_max) noexcept {
    (void)refs;
    SourceRewardTick out;
    if(!profile.loaded){out.status=SourceRewardStatus::NeedsProfile;return out;}
    for(size_t slot=0;slot<source_effect_capacity;++slot){
        if(!(pool.allocated_80098838[slot/32]&(0x80000000u>>(slot%32))))continue;
        if(pool.pending_release_800988c8[slot/32]&(0x80000000u>>(slot%32)))continue;
        SourceEffectWork w=pool.slots[slot];
        if(w[2]!=kSourceRewardPickupSubtype||!(w[0]&w[6]&1u))continue;
        const uint8_t kind=w[3]?1:0;
        if(w[10]>3){++out.held;out.status=SourceRewardStatus::UnsupportedTable;continue;}
        const auto& b=profile.bounds_8008a618[kind][w[10]];
        RandStage r(rng,gen);
        bool hold=false,release=false;int32_t money_gain=0,energy_gain=0;
        // 0x80046A0C: collect inside the unsigned window around the pickup.
        const auto collect=[&](){
            const uint32_t radius=profile.radius_80097eec[kind][w[10]];
            const auto window=[&](int32_t delta,uint32_t extent){return uint32_t(delta)+radius+50u<=extent;};
            if(!window(int32_t(world.player[0])-s16(w,0x38),radius*2+100))return;
            if(!window(int32_t(world.player[2])-s16(w,0x3c),radius*2+100))return;
            if(uint32_t(int32_t(world.player[1])-s16(w,0x3a))+radius>radius*2+150u)return;
            w[8]=5;
            const int32_t sel=world.stats_selector_800c1b6b;
            if(sel<0||sel>2){hold=true;return;}
            const uint16_t amount=profile.awards_8008a678[size_t(sel*8+kind*4+w[10])];
            if(kind==0)money_gain=amount; else energy_gain=amount;
        };
        const auto spin=[&]()->bool{
            if(kind==0){spin_shard_800469a4(w);return true;}
            return spin_energy_80046890(w,r,slot,gen);
        };
        switch(w[8]){
        case 0: // 0x800461A4
            w[0]|=2;w[8]=1;
            if(world.audio){std::array<uint32_t,2> words{uint32_t(g16(w,0x38))|(uint32_t(g16(w,0x3a))<<16),uint32_t(g16(w,0x3c))|(uint32_t(g16(w,0x3e))<<16)};
                (void)enqueue_source_audio_8001a050(*world.audio,0x9b,words);}
            if(!spin()){hold=true;break;}
            collect();w[11]=0xff;break;
        case 1: { // 0x8004622C
            const std::array<int16_t,3> prev{s16(w,0x38),s16(w,0x3a),s16(w,0x3c)};
            for(size_t a=0;a<3;++a){
                const int16_t v=int16_t(s16(w,0x2c+a*2)+s16(w,0x32+a*2));
                p16(w,0x2c+a*2,uint16_t(v));p16(w,0x38+a*2,uint16_t(s16(w,0x38+a*2)+v));
            }
            if(!spin()){hold=true;break;}
            const auto c=contact_8004b5cc(world,w,prev,b);
            if(!c.ok){hold=true;break;}
            if(c.result!=0){
                bool settle_check=(c.result&0x1000)!=0;
                if(!settle_check){w[11]=uint8_t(w[11]-1);settle_check=w[11]==0;}
                if(settle_check){
                    if((s16(w,0x2e)>>1)<2){w[11]=0x96;p16(w,0x2e,0);reset_scale_matrix(w);w[8]=uint8_t(w[8]+1);}
                    p16(w,0x2e,uint16_t(s16(w,0x2e)>>1));
                }
                reflect_8004ac8c(w,c.result);
            }
            collect();break; }
        case 2: case 3: { // 0x80046438 rest / 0x800465C8 blink
            if(w[8]==2){
                w[11]=uint8_t(w[11]-1);
                if(w[11]==0){
                    w[8]=3;const auto v=r.draw(0x80046480u,work_address(slot),uint8_t(slot),gen);
                    if(!v){hold=true;break;}
                    w[11]=uint8_t((*v&7)+0x1e);
                }
            } else {
                const uint8_t t=uint8_t(w[11]-1);w[11]=t;w[0]^=2;if(t==0)w[8]=4;
            }
            const auto floor=floor_80048ea4(world,w,b);
            if(!floor){hold=true;break;}
            const std::array<int16_t,3> prev{s16(w,0x38),s16(w,0x3a),s16(w,0x3c)};
            if(int32_t(s16(w,0x3a))<*floor-(b.y1+1)){
                const int16_t v=int16_t(s16(w,0x2e)+s16(w,0x34));p16(w,0x2e,uint16_t(v));
                p16(w,0x3a,uint16_t(s16(w,0x3a)+v));
            }
            if(!contact_8004b5cc(world,w,prev,b).ok){hold=true;break;}
            collect();
            if(kind==0)spin_shard_800469a4(w);
            break; }
        case 4: release=true;break; // 0x8004674C
        case 5: p16(w,0x2e,0xffd8u);p16(w,0x34,5);w[8]=6;break; // 0x8004676C
        case 6: { // 0x8004678C rise and shrink
            const int16_t v=int16_t(s16(w,0x2e)+s16(w,0x34));p16(w,0x2e,uint16_t(v));
            p16(w,0x3a,uint16_t(s16(w,0x3a)+v));
            p16(w,0x3e,uint16_t(s16(w,0x3e)-(profile.scale_80097ee0[w[10]]>>3)));
            if(v>=0)w[8]=7;
            break; }
        case 7: // 0x80046848
            if(world.audio){std::array<uint32_t,2> words{uint32_t(g16(w,0x38))|(uint32_t(g16(w,0x3a))<<16),uint32_t(g16(w,0x3c))|(uint32_t(g16(w,0x3e))<<16)};
                (void)enqueue_source_audio_8001a050(*world.audio,kind?0x9e:0x9d,words);}
            release=true;break;
        default: hold=true;break;
        }
        // 0x8004602C tail: the energy sparkle phase cycles 0..3 while visible.
        if(!hold&&!release&&kind==1&&(w[0]&2)&&w[8]<5)w[9]=uint8_t(w[9]+1>3?0:w[9]+1);
        if(hold||!r.commit()){r.rollback();++out.held;if(out.status==SourceRewardStatus::Inactive)out.status=SourceRewardStatus::NeedsContact;continue;}
        pool.slots[slot]=w;++out.updated;
        if(money_gain){money+=money_gain;if(money>999999)money=999999;out.money_awarded+=money_gain;++out.collected;}
        if(energy_gain){
            const int32_t next=int32_t(int16_t(uint16_t(hp)+uint16_t(energy_gain)));
            hp=int16_t(next>hp_max?hp_max:next);out.energy_awarded+=energy_gain;++out.collected;
        }
        if(release){mark_source_effect_release_80031988(pool,work_address(slot));++out.released;}
    }
    if(out.updated&&out.status==SourceRewardStatus::Inactive)out.status=SourceRewardStatus::SourceBackedPartial;
    return out;
}

} // namespace l3
