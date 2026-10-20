// Enemy-reward pickups (0x80046B34 expansion, subtype-0x87 states, 0x80046A0C
// collection). Tables are authored test values, not the retail profile; RNG
// expectations replay the same BIOS recurrence the service models.
#include "legends3ds/source_reward_pickup.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <iostream>

namespace {
using namespace l3;
SourceRngProfile rng_profile(uint32_t seed){
    SourceRngProfile p;
    const auto digest=[](const char* hex){SourceRngDigest d{};for(size_t i=0;i<32;++i){unsigned v=0;std::sscanf(hex+i*2,"%2x",&v);d[i]=uint8_t(v);}return d;};
    p.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    p.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    p.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    p.trace_sha256.fill(23);p.validated_prefix_sha256=p.trace_sha256;
    p.scope=SourceRngCaptureScope::WholeBoundedCapture;p.warnings=3;
    p.origin_seed32=seed;p.origin_game_entry_count=1;p.origin_vsync=1062;
    p.actual_rand_returns=1;p.trace_rows=5;p.validated_prefix_rows=5;
    p.captured_bytes=1200;p.validated_prefix_bytes=1200;p.final_rand_ordinal=1;p.observed_consumer_count=1;
    return p;
}
uint16_t lcg(uint32_t& s){s=s*1103515245u+12345u;return uint16_t((s>>16)&0x7fffu);}
ReferenceTables refs(){
    ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096)),int16_t(std::lround(std::cos(a)*4096))};}
    for(size_t i=0;i<r.scalar_quarter.size();++i)r.scalar_quarter[i]=int16_t(std::lround(std::sin(double(i)*1.5707963267948966/1024.0)*4096));
    r.scalar_quarter_known=true;return r;
}
std::vector<uint8_t> profile_bytes(){
    std::vector<uint8_t> p;
    const auto h=[&](int v){p.push_back(uint8_t(v));p.push_back(uint8_t(unsigned(v)>>8));};
    for(int t=0;t<16;++t)for(int i=0;i<16;++i)p.push_back(t==0?0:t==1?9:uint8_t(i&1)); // table0: size0, table1: none
    for(int i=0;i<17;++i)h(i?4096/i:24);
    for(int i=0;i<5;++i)h(100+i*20);                 // scale
    for(int i=0;i<5;++i)h(12-i);                     // speed
    for(int i=0;i<5;++i)h(-30+i);                    // lift
    for(int i=0;i<8;++i)h(10+i);                     // radius [kind][size]
    for(int k=0;k<2;++k)for(int s=0;s<4;++s){h(-8);h(8);h(-8);h(8);h(-16+k*8);h(16-k*8);}
    for(int i=0;i<24;++i)h(i+1);                     // awards
    std::vector<uint8_t> out{'L','3','R','W'};
    wr_u16(out,1);wr_u16(out,32);wr_u32(out,uint32_t(32+p.size()));wr_u32(out,uint32_t(p.size()));
    wr_u32(out,crc32(p.data(),p.size()));out.resize(32,0);out.insert(out.end(),p.begin(),p.end());
    return out;
}
RuntimeMap floor_map(){
    RuntimeMap m;RuntimeMapArea a;a.start_x=60;a.start_z=60;a.width=8;a.height=8;a.cell_count=64;
    m.areas.push_back(a);m.cells.assign(64,1);m.idx.resize(2);m.idx[1].descriptor_index=0;
    RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});m.contact_groups.push_back({-8192,8192,-8192,8192,0,1});
    m.contact_primitives.push_back({0,1024,3,0});return m;
}
size_t live_pickups(const SourceEffectPool& p){
    size_t n=0;for(size_t s=0;s<64;++s)if((p.allocated_80098838[s/32]&(0x80000000u>>(s%32)))&&!(p.pending_release_800988c8[s/32]&(0x80000000u>>(s%32)))&&p.slots[s][2]==0x87)++n;
    return n;
}
}

int main(){
    SourceRewardProfile prof;auto bytes=profile_bytes();
    assert(load_source_reward_profile(bytes,prof)&&prof.spread_80097efa[2]==2048&&prof.bounds_8008a618[1][0].y1==8);
    auto bad=bytes;bad.back()^=1;SourceRewardProfile rejected;assert(!load_source_reward_profile(bad,rejected)&&!rejected.loaded);
    const auto r=refs();
    SourceEffectPool pool;initialize_source_effect_pool_8002e924(pool);
    // RNG not admitted: the expansion holds and changes nothing.
    SourceRngService off;const std::array<int16_t,4> point{1000,-1,-2000,0};
    auto held=expand_source_reward_80046b34(pool,prof,r,off,1,point,0x01010002u);
    assert(held.status==SourceRewardStatus::NeedsRng&&live_pickups(pool)==0);

    SourceRngService rng;const uint32_t seed=0x1234u;
    assert(install_source_rng_service(rng,rng_profile(seed),SourceRngMode::CandidateOpenBios)==SourceRngStatus::Ready);
    // Descriptor: 2 shards from table0 (always size 0), 1 energy from table1 (never).
    const auto e=expand_source_reward_80046b34(pool,prof,r,rng,1,point,0x01010002u);
    assert(e.status==SourceRewardStatus::SourceBackedPartial&&e.spawned==2&&e.discarded==1&&e.pool_full==0);
    assert(live_pickups(pool)==2&&rng.native_global_ordinal()==7);
    // Replay the original call order to check the first shard exactly.
    uint32_t s=seed;const uint32_t angle=lcg(s)&0xfffu;(void)lcg(s);const uint16_t roll=lcg(s);
    size_t first=64;for(size_t i=0;i<64;++i)if(pool.slots[i][2]==0x87){first=i;break;}
    const auto& w=pool.slots[first];
    const auto g=[&](size_t o){return int16_t(uint16_t(w[o]|(w[o+1]<<8)));};
    assert(w[0]==0x15&&w[3]==0&&w[10]==0&&g(0x38)==1000&&g(0x3a)==-21&&g(0x3c)==-2000);
    const int32_t speed=12+(roll&3);
    assert(g(0x2c)==int16_t((reference_rcos_8007a12c(r,int32_t(angle))*speed)>>12));
    assert(g(0x2e)==-30+(roll&3)&&g(0x30)==int16_t((reference_rsin_8007a060(r,int32_t(angle))*speed)>>12));
    assert(g(0x34)==5&&g(0x3e)==100&&uint16_t(g(0x0c))==(roll&0xfffu));
    consume_source_effect_releases_80031b84(pool);

    // Lifecycle with the player far away: fly, land on the floor, rest 150
    // frames, blink 30..37 frames, then despawn. Nothing is collected.
    const auto map=floor_map();SourceAudioQueue audio;initialize_source_audio_queue_80019e14(audio);
    SourceRewardWorld world;world.map=&map;world.area_index=0;world.player={-3000,-1,3000};world.audio=&audio;
    int32_t money=0;int16_t hp=50;
    auto lifecycle=pool;int frames=0,landed_at=-1,blink_at=-1;
    while(live_pickups(lifecycle)&&frames<400){
        const auto t=tick_source_reward_pickups(lifecycle,prof,r,rng,1,world,money,hp,100);
        assert(t.held==0&&t.collected==0);consume_source_effect_releases_80031b84(lifecycle);++frames;
        const auto& lw=lifecycle.slots[first];
        if(landed_at<0&&lw[8]==2){landed_at=frames;assert(int16_t(uint16_t(lw[0x3a]|(lw[0x3b]<<8)))==-16);}
        if(blink_at<0&&lw[8]==3)blink_at=frames;
    }
    assert(landed_at>0&&blink_at-landed_at==150&&frames-blink_at>=31&&frames-blink_at<=39&&money==0);
    assert(audio.count>=2); // spawn cue 0x9B for each shard

    // Collection: the player stands on the landing spot. The shard rises,
    // shrinks, plays 0x9D and releases; it pays out exactly once.
    auto collect=pool;world.player={1000,-1,-2000};money=0;
    initialize_source_audio_queue_80019e14(audio);
    int collected=0,f=0;
    while(live_pickups(collect)&&f<200){
        const auto t=tick_source_reward_pickups(collect,prof,r,rng,1,world,money,hp,100);
        collected+=t.collected;consume_source_effect_releases_80031b84(collect);++f;
    }
    assert(collected==2&&money==2*9); // selector 1, money size 0 = awards[8] = 9
    // Energy: authored table 2 alternates sizes 0/1; HP clamps at the maximum.
    SourceEffectPool energy;initialize_source_effect_pool_8002e924(energy);
    const auto ee=expand_source_reward_80046b34(energy,prof,r,rng,1,point,0x02010000u);
    assert(ee.spawned==1&&ee.discarded==0);
    consume_source_effect_releases_80031b84(energy);
    hp=95;money=0;int energy_got=0;
    for(int i=0;i<200&&live_pickups(energy);++i){
        const auto t=tick_source_reward_pickups(energy,prof,r,rng,1,world,money,hp,100);
        energy_got+=t.energy_awarded;consume_source_effect_releases_80031b84(energy);
    }
    assert((energy_got==13||energy_got==14)&&hp==100); // awards[12/13]
    std::cout<<"PASS: reward expansion RNG/launch, discard, lifecycle 150-frame rest + blink, collection once, energy clamp, holds\n";
}
