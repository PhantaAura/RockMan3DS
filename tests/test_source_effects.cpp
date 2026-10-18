#include "legends3ds/source_effects.h"
#include "legends3ds/source_event_phase5.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {size_t allocations=0;}
void* operator new(std::size_t size){++allocations;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
namespace {
using namespace l3;
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
uint32_t value(std::istringstream& row){uint64_t v;require(bool(row>>v),"effect fixture truncated");return uint32_t(v);}
SourceEffectWork work(std::istringstream& row){SourceEffectWork w;for(auto& v:w)v=uint8_t(value(row));return w;}
void equal(const SourceEffectWork& a,const SourceEffectWork& b,size_t row){
    if(a==b)return;
    for(size_t i=0;i<a.size();++i)if(a[i]!=b[i])std::cerr<<"case="<<row<<" offset="<<i<<" native="<<unsigned(a[i])<<" source="<<unsigned(b[i])<<'\n';
    throw std::runtime_error("effect all72-byte source differential mismatch");
}
SourceEffectPool pool(){SourceEffectPool p;initialize_source_effect_pool_8002e924(p);return p;}
void fixtures(const std::string& dir){
    for(const std::string name:{"allocator","callback131","callback128","release"}){
        std::ifstream f(dir+"/"+name+".tsv");require(bool(f),"missing source effect fixture");std::string line;std::getline(f,line);size_t rows=0;
        while(std::getline(f,line)){
            std::istringstream row(line);auto p=pool();
            if(name=="allocator"){
                const auto slot=value(row);p.allocated_80098838={{value(row),value(row)}};const auto expected_id=value(row);
                const std::array<uint32_t,2> masks{{value(row),value(row)}};const auto before=work(row),after=work(row);p.slots[std::min<size_t>(slot,63)]=before;
                const auto result=allocate_source_effect_80031618(p);
                require(result.status==SourceEffectStatus::SourceBackedPartial && result.source_identity==expected_id && p.allocated_80098838==masks,"original effect allocator order/exhaustion mismatch");equal(p.slots[std::min<size_t>(slot,63)],after,rows);
            }else if(name=="callback131"){
                const auto before=work(row),after=work(row);const std::array<uint32_t,2> pending{{value(row),value(row)}};
                p.allocated_80098838[0]=0x80000000;p.slots[0]=before;const auto result=tick_source_effect(p,source_effect_base);
                require(result.committed && result.status==SourceEffectStatus::SourceBackedPartial,"131 source callback held");equal(p.slots[0],after,rows);require(p.pending_release_800988c8==pending,"131 pending release differs");
            }else if(name=="callback128"){
                int32_t floor;require(bool(row>>floor),"128 floor fixture truncated");const auto count=value(row);
                const auto before=work(row),after=work(row);const auto expected_count=value(row);std::array<uint8_t,20> queued{};for(auto& v:queued)v=uint8_t(value(row));
                SourceAudioQueue audio;for(auto& v:audio.records)v.raw.fill(0xa5);initialize_source_audio_queue_80019e14(audio);audio.count=count;
                p.allocated_80098838[0]=0x80000000;p.slots[0]=before;SourceEffect128Inputs input;input.floor_8004920c=floor;input.audio=&audio;
                const auto raw=uint16_t(before[0x3a]|uint16_t(before[0x3b])<<8);const int16_t y=raw<32768?int16_t(raw):int16_t(int32_t(raw)-65536);
                input.resource=SourceEffect128Resource{uint8_t(int64_t(floor)-y<30?0:1),0x80161234};
                const auto result=tick_source_effect(p,source_effect_base,input);require(result.committed,"128 initialized with exact receipts held");equal(p.slots[0],after,rows);
                require(audio.count==expected_count && audio.records[std::min<uint32_t>(count,23)].raw==queued,"128 actual audio record/drop mismatch");
            }else{
                const auto slot=value(row);p.allocated_80098838={{value(row),value(row)}};p.pending_release_800988c8={{value(row),value(row)}};
                const std::array<uint32_t,2> interim{{value(row),value(row)}};const auto flags=value(row);const std::array<uint32_t,2> final{{value(row),value(row)}};
                p.slots[slot][0]=3;require(mark_source_effect_release_80031988(p,source_effect_base+slot*72),"source release rejected");
                require(p.pending_release_800988c8==interim && p.slots[slot][0]==flags,"original pending release mismatch");consume_source_effect_releases_80031b84(p);
                require(p.allocated_80098838==final && p.pending_release_800988c8==std::array<uint32_t,2>{},"original release consumer mismatch");
            }
            ++rows;
        }
        require(rows>60,"source effect differential coverage missing");std::cout<<name<<" original source cases="<<rows<<'\n';
    }
}
void guards(){
    auto p=pool();for(size_t i=0;i<64;++i){for(size_t j=0;j<72;++j)require(p.slots[i][j]==(j==1?uint8_t(128|i):0),"boot class/zero/reset mismatch");}
    for(size_t i=0;i<64;++i)require(allocate_source_effect_80031618(p).source_identity==source_effect_base+i*72,"source sequential allocation differs");
    const auto full=p;require(allocate_source_effect_80031618(p).source_identity==0 && p.slots==full.slots,"full source pool mutated slots");
    require(mark_source_effect_release_80031988(p,source_effect_base+32*72) && allocate_source_effect_80031618(p).source_identity==0,"pending release reused early");
    p.slots[32][0x47]=0xa5;consume_source_effect_releases_80031b84(p);require(allocate_source_effect_80031618(p).source_identity==source_effect_base+32*72 && p.slots[32][0x47]==0xa5,"reuse failed to retain primitive byte71");
    auto fresh=pool();const auto id=allocate_source_effect_80031618(fresh).source_identity;Class60_06EffectIntent e;e.source_identity=id;e.work00=1;e.subtype02=131;e.copy_primitive=true;
    for(size_t i=0;i<52;++i)e.primitive_to_14[i]=uint8_t(i*7+1);
    require(initialize_child_effect(fresh,e)==SourceEffectStatus::SourceBackedPartial,"child131 intent rejected");
    for(size_t i=0;i<52;++i)require(fresh.slots[0][0x14+i]==e.primitive_to_14[i],"original131 copied52 bytes changed");
    const auto before_alloc=allocations;size_t ticks=0;
    while(!fresh.pending_release_800988c8[0]){require(tick_source_effect(fresh,id).committed,"131 sequence held");++ticks;require(ticks<32,"131 release never occurred");}
    require(ticks==13 && allocations==before_alloc,"131 exact lifecycle count or hot-loop heap mismatch");
    require(fresh.allocated_80098838[0]==0x80000000 && !source_effect_primitive(fresh.slots[0]),"131 freed early or retained visible packet after release");
    consume_source_effect_releases_80031b84(fresh);require(fresh.allocated_80098838[0]==0,"131 release not consumed");
    fresh=pool();e.source_identity=allocate_source_effect_80031618(fresh).source_identity;e.work00=5;e.subtype02=128;e.parameter03=66;e.copy_primitive=false;e.copy_raw=true;e.raw_to_38={{-1,-2,3,0x1234}};
    require(initialize_child_effect(fresh,e)==SourceEffectStatus::SourceBackedPartial,"child128 initializer rejected");const auto before=fresh;
    require(tick_source_effect(fresh,e.source_identity).status==SourceEffectStatus::NeedsFloorConsumer && fresh.slots==before.slots,"missing128 floor consumer changed72 bytes");
    SourceEffect128Inputs in;in.floor_8004920c=0;
    require(tick_source_effect(fresh,e.source_identity,in).status==SourceEffectStatus::NeedsResourceConsumer && fresh.slots==before.slots,"missing128 resource consumer passed");
    in.resource=SourceEffect128Resource{1,0x80161234};require(tick_source_effect(fresh,e.source_identity,in).status==SourceEffectStatus::StaleResource && fresh.slots==before.slots,"wrong resource selector accepted");
    in.resource->selector_8002f9c4=0;require(tick_source_effect(fresh,e.source_identity,in).status==SourceEffectStatus::NeedsSoundConsumer && fresh.slots==before.slots,"missing real128 audio passed");
    // A retained64-slot frame batch remains bounded and allocation-free.
    auto batch_pool=pool();
    for(size_t slot=0;slot<64;++slot){
        Class60_06EffectIntent fade;fade.source_identity=allocate_source_effect_80031618(batch_pool).source_identity;fade.work00=1;fade.subtype02=131;fade.copy_primitive=true;
        require(initialize_child_effect(batch_pool,fade)==SourceEffectStatus::SourceBackedPartial,"64-slot fade setup failed");
    }
    const auto heap_before=allocations;
    for(size_t frame=0;frame<1024;++frame)tick_source_effect_batch(batch_pool);
    require(allocations==heap_before && batch_pool.pending_release_800988c8==std::array<uint32_t,2>{{0xffffffffu,0xffffffffu}},"bounded effect frame batches allocated heap or lost releases");
    require(!source_effect_slot(0) && !source_effect_slot(source_effect_base+1) && !source_effect_slot(source_effect_base+64*72),"bad source identities accepted");
    fresh.pending_release_800988c8[1]=1;const auto invalid=fresh;
    require(allocate_source_effect_80031618(fresh).status==SourceEffectStatus::InvalidPool && fresh.slots==invalid.slots && fresh.allocated_80098838==invalid.allocated_80098838,"invalid pool fabricated exhaustion/mutation");
}
bool same_effect_owner(const SourceEventPhase5Context& a,const SourceEventPhase5Context& b){
    if(a.pool80.used!=b.pool80.used||a.pool80_pending_release_800988c8!=b.pool80_pending_release_800988c8)return false;
    for(size_t i=0;i<64;++i)if(a.pool80.slots[i].raw!=b.pool80.slots[i].raw||a.pool80.slots[i].known!=b.pool80.slots[i].known)return false;
    return true;
}
void phase5_guards(){
    static SourceEventPhase5Context scene,snapshot;
    scene={};require(!scene.pool80_pending_release_800988c8[0]&&!scene.pool80_pending_release_800988c8[1],"default pending release fabricated knowledge");
    initialize_source_event_phase5_pools_8002e88c(scene);
    require(scene.pool80_pending_release_800988c8==std::array<std::optional<uint32_t>,2>{{0,0}},"cold pending release owner not known zero");
    auto& separate=scene.pool80.slots[0];separate.raw[0]=1;separate.raw[2]=2;separate.raw[6]=3;
    separate.known.fill(0);for(size_t offset:{size_t(0),size_t(2),size_t(6)})separate.known[offset]=1;
    scene.pool80.used[0]=0x80000000u;snapshot=scene;
    require(tick_source_phase5_effects_800173b4(scene).committed&&same_effect_owner(scene,snapshot),"subtype2 unrelated lanes held or changed");
    auto& fade=scene.pool80.slots[1];fade.raw[0]=1;fade.raw[2]=131;fade.raw[3]=0;fade.raw[6]=3;fade.raw[8]=0;
    fade.known.fill(0);for(size_t offset:{size_t(0),size_t(2),size_t(3),size_t(6),size_t(8)})fade.known[offset]=1;
    scene.pool80.used[0].reset();scene.pool80.used[1].reset();scene.pool80_pending_release_800988c8={};
    auto& later=scene.pool80.slots[63];later.raw[0]=1;later.raw[2]=131;later.raw[6]=3;later.known[8]=0;snapshot=scene;
    auto result=tick_source_phase5_effects_800173b4(scene);
    require(!result.committed&&result.status==SourceEffectStatus::NeedsField&&result.missing_slot==63&&result.missing_offset==8&&same_effect_owner(scene,snapshot),"later unknown record partially initialized early fade");
    later.raw[0]=0;const auto heap=allocations;result=tick_source_phase5_effects_800173b4(scene);
    require(result.committed&&result.updated==1&&allocations==heap&&!fade.known[0x1e]&&!fade.known[0x1f],"initialization invented bitmap/Y ownership or heap");
    fade.raw[3]=1;fade.known[3]=1;fade.raw[11]=3;
    result=tick_source_phase5_effects_800173b4(scene);
    require(result.committed&&!fade.known[0x1e]&&!fade.known[0x1f],"parameter1 required or promoted unread Y lanes");
    fade.raw[8]=4;fade.raw[11]=1;fade.known[3]=0;snapshot=scene;
    result=tick_source_phase5_effects_800173b4(scene);
    require(!result.committed&&result.status==SourceEffectStatus::NeedsPoolBitmap&&same_effect_owner(scene,snapshot),"terminal release ignored missing allocation owner");
    scene.pool80.used[0]=0xc0000001u;snapshot=scene;
    result=tick_source_phase5_effects_800173b4(scene);
    require(!result.committed&&result.status==SourceEffectStatus::NeedsReleaseBitmap&&same_effect_owner(scene,snapshot),"terminal release ignored missing pending owner");
    scene.pool80_pending_release_800988c8[0]=0;scene.pool80_pending_release_800988c8[1]=0;scene.pool80.used[1]=1;
    for(size_t slot:{size_t(31),size_t(63)}){auto& r=scene.pool80.slots[slot];r.raw[0]=1;r.raw[2]=131;r.raw[6]=3;r.raw[8]=4;r.raw[11]=1;
        r.known.fill(0);for(size_t offset:{size_t(0),size_t(2),size_t(6),size_t(8),size_t(11)})r.known[offset]=1;}
    result=tick_source_phase5_effects_800173b4(scene);
    require(result.committed&&result.released==3&&scene.pool80.used[0]==0xc0000001u&&scene.pool80.used[1]==1&&
        scene.pool80_pending_release_800988c8[0]==0x40000001u&&scene.pool80_pending_release_800988c8[1]==1,"same-bank/bank1 releases lost bits or freed early");
    scene.pool80_pending_release_800988c8[1].reset();snapshot=scene;result=consume_source_phase5_effect_releases_80031aa4(scene);
    require(!result.committed&&same_effect_owner(scene,snapshot),"cleanup partially published bank0 before unknown bank1");
    scene.pool80_pending_release_800988c8[1]=1;const auto rows=scene.pool80.slots;
    result=consume_source_phase5_effect_releases_80031aa4(scene);
    require(result.committed&&scene.pool80.used[0]==0x80000000u&&scene.pool80.used[1]==0&&scene.pool80_pending_release_800988c8[0]==0&&
        scene.pool80_pending_release_800988c8[1]==0,"cleanup failed deferred mask intersection");
    for(size_t slot=0;slot<64;++slot)require(scene.pool80.slots[slot].raw==rows[slot].raw&&scene.pool80.slots[slot].known==rows[slot].known,"cleanup edited raw/known bytes");
    initialize_source_event_phase5_pools_8002e88c(scene);
    for(size_t slot=0;slot<64;++slot){auto& r=scene.pool80.slots[slot];r.raw[0]=1;r.raw[2]=131;r.raw[6]=3;}
    scene.pool80.used={{0xffffffffu,0xffffffffu}};const auto hot=allocations;
    for(size_t invocation=0;invocation<13;++invocation){result=tick_source_phase5_effects_800173b4(scene);require(result.committed&&result.updated==64,"64-slot supported sweep held");}
    require(result.released==64&&allocations==hot,"phase5 exact13 callbacks/full sweep allocated heap or missed release");
    require(consume_source_phase5_effect_releases_80031aa4(scene).committed&&scene.pool80.used[0]==0&&scene.pool80.used[1]==0,"full sweep cleanup failed");
    scene.pool80.slots[0].raw[0]=1;scene.pool80.slots[0].raw[2]=128;snapshot=scene;
    require(tick_source_phase5_effects_800173b4(scene).status==SourceEffectStatus::UnknownSubtype&&same_effect_owner(scene,snapshot),"unported active callback changed owner");
}
uint8_t hexbyte(char c){return uint8_t(c>='a'?c-'a'+10:c-'0');}
SourceEffectWork hexwork(const std::string& s){require(s.size()==144,"known-lane fixture hex truncated");SourceEffectWork w{};
    for(size_t i=0;i<72;++i)w[i]=uint8_t((hexbyte(s[i*2])<<4)|hexbyte(s[i*2+1]));return w;}
void phase5_fixtures(const std::string& path){
    std::ifstream f(path);require(bool(f),"missing phase5 effect fixture");std::string line;std::getline(f,line);size_t cases=0;
    static SourceEventPhase5Context scene;
    while(std::getline(f,line)){std::istringstream row(line);std::string before,after,reads,writes;uint32_t pending;
        require(bool(row>>before>>after>>reads>>writes>>pending)&&reads.size()==72&&writes.size()==72,"known-lane fixture truncated");
        initialize_source_event_phase5_pools_8002e88c(scene);scene.pool80.used[0]=0x80000000u;
        auto& record=scene.pool80.slots[0];record.raw=hexwork(before);record.known.fill(0);
        for(size_t i=0;i<72;++i)record.known[i]=reads[i]=='1';
        for(size_t offset:{size_t(0),size_t(2),size_t(6)})record.known[offset]=1;
        auto expected=record.known;for(size_t i=0;i<72;++i)if(writes[i]=='1')expected[i]=1;
        const auto heap=allocations;const auto result=tick_source_phase5_effects_800173b4(scene);
        require(result.committed&&result.updated==1&&allocations==heap,"source known-lane callback held or allocated");
        equal(record.raw,hexwork(after),cases);require(record.known==expected&&scene.pool80_pending_release_800988c8[0]==pending,"source write knowledge/pending output mismatch");++cases;
    }
    require(cases==76,"known-lane source case coverage changed");std::cout<<"phase5 effect source read/write cases="<<cases<<'\n';
}
}
int main(int argc,char** argv){try{guards();phase5_guards();if(argc>1)fixtures(argv[1]);if(argc>2)phase5_fixtures(argv[2]);std::cout<<"PASS source effects; bounded64-slot allocator,131 lifecycle,128 explicit receipts,phase5 known-lane sweeps\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
