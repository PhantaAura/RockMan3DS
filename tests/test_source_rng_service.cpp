#include "legends3ds/source_rng_service.h"
#include "legends3ds/source_effect2.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/source_prop_lift.h"
#include "legends3ds/source_effect34.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <vector>

namespace { size_t allocations = 0; bool measuring = false; }
void* operator new(std::size_t n) { if (measuring) ++allocations; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
namespace {
using Status = l3::SourceRngStatus;
using Package = std::array<uint8_t, l3::kSourceRngProfileBytes>;
l3::SourceRngDigest digest(const char* text) {
    l3::SourceRngDigest out{};
    auto nibble=[](char c) { return static_cast<unsigned>(c <= '9' ? c-'0' : c-'a'+10); };
    for (size_t i=0;i!=32;++i) out[i]=static_cast<uint8_t>((nibble(text[2*i])<<4)|nibble(text[2*i+1]));
    return out;
}
// Entirely authored admission/profile fixture. Scalar checkpoints below come
// independently from unchanged original selected-BIOS instruction execution.
l3::SourceRngProfile fixture(uint32_t seed=0) {
    l3::SourceRngProfile p;
    p.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    p.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    p.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    p.trace_sha256.fill(23); p.validated_prefix_sha256=p.trace_sha256;
    p.scope=l3::SourceRngCaptureScope::WholeBoundedCapture; p.warnings=3;
    p.origin_seed32=seed; p.origin_game_entry_count=1; p.origin_vsync=1062;
    p.actual_rand_returns=1; p.trace_rows=5; p.validated_prefix_rows=5;
    p.captured_bytes=1200; p.validated_prefix_bytes=1200; p.final_rand_ordinal=1; p.observed_consumer_count=1;
    return p;
}
void wr(Package& b,size_t offset,uint32_t value) { for(size_t i=0;i!=4;++i)b[offset+i]=static_cast<uint8_t>(value>>(8*i)); }
void fix_crc(Package& b) {
    uint32_t crc=0xffffffffu;
    for(size_t i=32;i!=b.size();++i){crc^=b[i];for(unsigned bit=0;bit!=8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
    wr(b,20,~crc);
}
Package package(const l3::SourceRngProfile& p) {
    Package b{};b[0]='L';b[1]='3';b[2]='R';b[3]='G';b[4]=1;b[6]=32;
    wr(b,8,1);wr(b,12,256);wr(b,16,224);
    const l3::SourceRngDigest* d[]={&p.bios_sha256,&p.exe_sha256,&p.overlay_sha256,&p.trace_sha256,&p.validated_prefix_sha256};
    for(size_t i=0;i!=5;++i)std::copy(d[i]->begin(),d[i]->end(),b.begin()+32+i*32);
    const uint32_t fields[]={static_cast<uint32_t>(p.scope),p.warnings,p.origin_seed32,p.origin_rand_ordinal,p.origin_vsync,
        p.origin_game_entry_count,p.actual_rand_returns,p.actual_reseed_returns,p.trace_rows,p.validated_prefix_rows,p.excluded_rows,
        p.captured_bytes,p.validated_prefix_bytes,p.final_rand_ordinal,p.observed_consumer_count,0};
    for(size_t i=0;i!=16;++i)wr(b,192+i*4,fields[i]);fix_crc(b);return b;
}
l3::SourceRngClass03Context actor(uint8_t slot) { return {l3::kSourceClass03RngCallerRa,0x8009f108u+uint32_t(slot)*0x3c0u,slot,1,4,255}; }
l3::SourceRngUpdateContext update(uint64_t frame=42) { return {frame,frame/2,l3::SourceRngUpdatePhase::Class03Callbacks}; }
void profile_guards() {
    const auto p=fixture(0xffffffff);const auto b=package(p);l3::SourceRngProfile out;
    assert(l3::load_l3source_rng_profile(b.data(),b.size(),out)==Status::Ready&&out.origin_seed32==0xffffffff);
    assert(l3::load_l3source_rng_profile(nullptr,b.size(),out)==Status::InvalidProfile);
    assert(l3::load_l3source_rng_profile(b.data(),b.size()-1,out)==Status::InvalidProfile);
    for(size_t offset : {size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(28),size_t(32),size_t(64),size_t(96),size_t(252)}) {
        auto bad=b;bad[offset]^=1;if(offset>=24)fix_crc(bad);
        assert(l3::load_l3source_rng_profile(bad.data(),bad.size(),out)==Status::InvalidProfile);
        assert(out.origin_seed32==0xffffffff); // Failed decoding cannot publish.
    }
    for(unsigned test=0;test!=18;++test) {
        auto bad=p;
        switch(test) {
        case 0:bad.trace_sha256.fill(0);break;case 1:bad.validated_prefix_sha256.fill(0);break;
        case 2:bad.warnings=1;break;case 3:bad.warnings=67;break;case 4:bad.origin_rand_ordinal=1;break;
        case 5:bad.origin_game_entry_count=0;break;case 6:bad.actual_rand_returns=0;break;
        case 7:bad.actual_reseed_returns=1;break;case 8:bad.final_rand_ordinal=2;break;
        case 9:bad.observed_consumer_count=0;break;case 10:bad.observed_consumer_count=2;break;
        case 11:bad.trace_rows=65537;break;case 12:bad.validated_prefix_rows=3;break;
        case 13:bad.excluded_rows=1;break;case 14:bad.validated_prefix_bytes=1201;break;
        case 15:bad.captured_bytes=0;break;case 16:bad.scope=static_cast<l3::SourceRngCaptureScope>(3);break;
        default:bad.scope=l3::SourceRngCaptureScope::CompletedCapturePrefix;break;
        }
        auto bytes=package(bad);
        assert(l3::load_l3source_rng_profile(bytes.data(),bytes.size(),out)==Status::InvalidProfile);
    }
    auto prefix=p;prefix.scope=l3::SourceRngCaptureScope::CompletedCapturePrefix;prefix.warnings=31;
    prefix.trace_rows+=2;prefix.excluded_rows=2;prefix.captured_bytes+=200;prefix.trace_sha256[0]^=1;
    const auto bytes=package(prefix);
    assert(l3::load_l3source_rng_profile(bytes.data(),bytes.size(),out)==Status::Ready);
    l3::SourceRngService service;
    assert(l3::install_source_rng_service(service,p,l3::SourceRngMode::Disabled)==Status::Disabled&&!service.admitted());
    assert(l3::install_source_rng_service(service,p,l3::SourceRngMode::CandidateOpenBios)==Status::Ready&&service.seed32()==0xffffffff);
    auto bad=p;bad.origin_rand_ordinal=1;
    assert(l3::install_source_rng_service(service,bad,l3::SourceRngMode::CandidateOpenBios)==Status::InvalidProfile&&service.seed32()==0xffffffff);
}
void transactions() {
    l3::SourceRngService s;l3::SourceRngUpdateTransaction tx;
    assert(l3::begin_source_rng_update(s,tx,update())==Status::NeedsObservedProfile);
    assert(l3::install_source_rng_service(s,fixture(),l3::SourceRngMode::CandidateOpenBios)==Status::Ready);
    auto bad_update=update();bad_update.phase=static_cast<l3::SourceRngUpdatePhase>(0);
    assert(l3::begin_source_rng_update(s,tx,bad_update)==Status::InvalidContext);
    assert(l3::begin_source_rng_update(s,tx,update())==Status::Ready);
    for(unsigned i=0;i!=3;++i) {
        auto a=actor(2);if(i==0)a.caller_ra=0x801022fc;if(i==1)a.slot=16;if(i==2)a.work+=4;
        assert(l3::stage_source_rng_class03(tx,a).status==Status::InvalidContext&&tx.call_count()==0&&s.seed32()==0);
    }
    auto first=l3::stage_source_rng_class03(tx,actor(2));
    assert(first.status==Status::Ready&&first.sample32a==0&&first.native_global_ordinal==1);
    assert(s.seed32()==0&&s.native_global_ordinal()==0&&s.last_call_count()==0);
    assert(l3::stage_source_rng_class03(tx,actor(2)).status==Status::DuplicateCallback&&tx.call_count()==1);
    l3::rollback_source_rng_update(tx);assert(s.seed32()==0&&s.native_global_ordinal()==0);
    assert(l3::commit_source_rng_update(s,tx)==Status::StaleTransaction);
    assert(l3::begin_source_rng_update(s,tx,update())==Status::Ready);
    assert(l3::stage_source_rng_class03(tx,actor(2)).sample32a==first.sample32a);
    auto stale=tx;assert(l3::commit_source_rng_update(s,tx)==Status::Ready&&s.seed32()==12345&&s.native_global_ordinal()==1);
    assert(l3::commit_source_rng_update(s,stale)==Status::StaleTransaction);
    assert(l3::stage_source_rng_class03(stale,actor(3)).status==Status::StaleTransaction);
    assert(s.last_call_count()==1&&s.last_calls()[0].actor.slot==2&&s.last_calls()[0].update.native_frame==42);
    // A later actor's native hold cannot undo an earlier published actor/RNG.
    assert(l3::begin_source_rng_update(s,tx,update())==Status::Ready);
    assert(l3::stage_source_rng_class03(tx,actor(3)).native_global_ordinal==2);
    l3::rollback_source_rng_update(tx);assert(s.seed32()==12345&&s.native_global_ordinal()==1&&s.last_call_count()==1);
    // Entire-update staging permits rollback of all16 only if all actor state
    // is likewise staged; no per-actor generators or conditional sample use.
    assert(l3::begin_source_rng_update(s,tx,update(44))==Status::Ready);
    for(uint8_t slot=0;slot!=16;++slot)assert(l3::stage_source_rng_class03(tx,actor(slot)).status==Status::Ready);
    assert(tx.call_count()==16&&s.native_global_ordinal()==1);
    l3::rollback_source_rng_update(tx);assert(s.seed32()==12345&&s.native_global_ordinal()==1);
    assert(l3::begin_source_rng_update(s,tx,update(44))==Status::Ready);
    for(uint8_t slot=0;slot!=16;++slot)assert(l3::stage_source_rng_class03(tx,actor(slot)).status==Status::Ready);
    assert(l3::commit_source_rng_update(s,tx)==Status::Ready&&s.last_call_count()==16&&s.native_global_ordinal()==17);
    for(size_t i=0;i!=16;++i){const auto& r=s.last_calls()[i];assert(r.actor.slot==i&&r.native_global_ordinal==i+2&&r.update.native_frame==44&&r.update.native_update_ordinal==22);}
    assert(l3::begin_source_rng_update(s,tx,update())==Status::Ready);
    assert(l3::install_source_rng_service(s,fixture(),l3::SourceRngMode::Disabled)==Status::Disabled);
    assert(l3::stage_source_rng_class03(tx,actor(0)).status==Status::StaleTransaction);
}
void scalar_checkpoints(const char* path) {
    std::ifstream file(path);assert(file);size_t cases=0;
    uint64_t authored=0,ordinal=0,before=0,after=0,value=0;
    while(file>>authored>>ordinal>>before>>after>>value) {
        assert(ordinal==1&&authored==before);
        const auto first_before=before,first_after=after,first_value=value,seed=authored;
        assert(file>>authored>>ordinal>>before>>after>>value);assert(ordinal==16&&authored==seed);
        l3::SourceRngService s;l3::SourceRngUpdateTransaction tx;
        assert(l3::install_source_rng_service(s,fixture(static_cast<uint32_t>(seed)),l3::SourceRngMode::CandidateOpenBios)==Status::Ready);
        assert(l3::begin_source_rng_update(s,tx,update(cases))==Status::Ready);
        for(uint8_t slot=0;slot!=16;++slot)assert(l3::stage_source_rng_class03(tx,actor(slot)).status==Status::Ready);
        const auto& first=tx.calls()[0];const auto& final=tx.calls()[15];
        assert(first.seed_before32==first_before&&first.seed_after32==first_after&&first.sample32a==first_value);
        assert(final.seed_before32==before&&final.seed_after32==after&&final.sample32a==value);
        assert(l3::commit_source_rng_update(s,tx)==Status::Ready&&s.seed32()==after);++cases;
    }
    assert(cases==256);std::cout<<"selected original BIOS scalar checkpoints "<<cases*2<<" across "<<cases*16<<" shared calls PASS\n";
}
void zero_heap() {
    l3::SourceRngService s;l3::SourceRngUpdateTransaction tx;
    assert(l3::install_source_rng_service(s,fixture(),l3::SourceRngMode::CandidateOpenBios)==Status::Ready);
    measuring=true;
    for(uint64_t frame=0;frame!=4096;++frame) {
        const auto old_seed=s.seed32();const auto old_ordinal=s.native_global_ordinal();
        assert(l3::begin_source_rng_update(s,tx,update(frame))==Status::Ready);
        for(uint8_t slot=0;slot!=16;++slot)assert(l3::stage_source_rng_class03(tx,actor(slot)).status==Status::Ready);
        if(frame%4==0){l3::rollback_source_rng_update(tx);assert(s.seed32()==old_seed&&s.native_global_ordinal()==old_ordinal);}
        else assert(l3::commit_source_rng_update(s,tx)==Status::Ready&&s.native_global_ordinal()==old_ordinal+16);
    }
    measuring=false;assert(allocations==0);
    std::cout<<"4096 fixed updates / 65536 staged calls; hot allocations "<<allocations<<" PASS\n";
}
void effect2_transactions() {
    using namespace l3;
    size_t cases=0,guards=0;const auto before_allocations=allocations;measuring=true;
    for(uint32_t seed:{0u,0xffffffffu,0x12345678u,0x80000000u})for(uint8_t slot:{0u,31u,32u,63u}){
        SourceRngService rng;assert(install_source_rng_service(rng,fixture(seed),SourceRngMode::CandidateOpenBios)==Status::Ready);
        // The scene uses the same service after an already committed actor.
        SourceRngUpdateTransaction tx;assert(begin_source_rng_update(rng,tx,update())==Status::Ready);
        assert(stage_source_rng_class03(tx,actor(2)).status==Status::Ready);
        assert(commit_source_rng_update(rng,tx)==Status::Ready);
        SourceEventPhase5Context scene;
        for(size_t i=0;i<64;++i){scene.pool80.slots[i].raw[1]=uint8_t(128u|i);scene.pool80.slots[i].known[1]=1;}
        scene.pool80.used={0u,0u};scene.pool80_pending_release_800988c8={0u,0u};
        auto& parent=scene.pool80.slots[slot];parent.known.fill(1);
        parent.raw[0]=1;parent.raw[2]=2;parent.raw[6]=3;parent.raw[14]=7;
        parent.raw[28]=0xfb;parent.raw[29]=0xff;parent.raw[30]=31;parent.raw[32]=17;
        *scene.pool80.used[slot/32]|=0x80000000u>>(slot&31u);
        SourceEffect2Inputs input;input.source_generation=91;
        const uint32_t identity=source_effect_base+uint32_t(slot)*72u;
        const SourceRngUpdateContext clock{42,21,SourceRngUpdatePhase::Effect2Callback};
        const auto before_parent=parent;const auto before_used=scene.pool80.used;
        // Unknown child identity is a late leaf rejection after RNG staging.
        const size_t child=slot==0?1:0;scene.pool80.slots[child].known[1]=0;
        const auto held=tick_source_effect2_candidate_rng(scene,identity,input,rng,clock);
        assert(!held.committed&&rng.native_global_ordinal()==1&&parent.raw==before_parent.raw&&scene.pool80.used==before_used);++guards;
        scene.pool80.slots[child].known[1]=1;
        auto unknown=input;unknown.source_generation=0;
        assert(!tick_source_effect2_candidate_rng(scene,identity,unknown,rng,clock).committed&&rng.native_global_ordinal()==1);++guards;
        SourceEffect2RngReceipt bad_receipt;unknown=input;unknown.rng=&bad_receipt;unknown.rng_count=1;
        assert(!tick_source_effect2_candidate_rng(scene,identity,unknown,rng,clock).committed&&rng.native_global_ordinal()==1);++guards;
        const auto success=tick_source_effect2_candidate_rng(scene,identity,input,rng,clock);
        assert(success.committed&&success.child_allocated&&success.rng_consumed==3&&rng.native_global_ordinal()==4);
        assert(parent.raw[11]==1&&rng.last_call_count()==3);
        for(size_t i=0;i<3;++i){const auto& row=rng.last_calls()[i];assert(row.actor.work==identity&&row.actor.slot==slot&&
            row.actor.caller_ra==success.rng_return_addresses[i]&&row.source_generation==91&&row.native_global_ordinal==i+2);}
        // A nonspawning source callback must not reserve/advance a sample.
        assert(tick_source_effect2_candidate_rng(scene,identity,input,rng,clock).committed&&parent.raw[11]==2&&rng.native_global_ordinal()==4);
        assert(begin_source_rng_update(rng,tx,update(44))==Status::Ready);
        assert(stage_source_rng_class03(tx,actor(3)).native_global_ordinal==5);
        assert(commit_source_rng_update(rng,tx)==Status::Ready);++cases;
    }
    SourceRngService rng;assert(install_source_rng_service(rng,fixture(),SourceRngMode::CandidateOpenBios)==Status::Ready);
    SourceRngUpdateTransaction tx;SourceRngUpdateContext clock{1,1,SourceRngUpdatePhase::Effect2Callback};
    assert(begin_source_rng_update(rng,tx,clock)==Status::Ready);
    assert(stage_source_rng_class03(tx,actor(0)).status==Status::InvalidContext);++guards;
    SourceRngEffect2Context effect{source_effect_base,7,0,1};
    assert(stage_source_rng_effect2(tx,effect).status==Status::InvalidContext&&tx.call_count()==0);++guards;
    effect.call_index=0;assert(stage_source_rng_effect2(tx,effect).status==Status::Ready);
    assert(commit_source_rng_update(rng,tx)==Status::InvalidContext&&rng.native_global_ordinal()==0);++guards;
    effect.call_index=1;effect.source_generation=8;assert(stage_source_rng_effect2(tx,effect).status==Status::InvalidContext&&tx.call_count()==1);++guards;
    effect.source_generation=7;assert(stage_source_rng_effect2(tx,effect).status==Status::Ready);
    effect.call_index=2;assert(stage_source_rng_effect2(tx,effect).status==Status::Ready);
    assert(stage_source_rng_effect2(tx,effect).status==Status::InvalidContext&&tx.call_count()==3);++guards;
    assert(commit_source_rng_update(rng,tx)==Status::Ready&&rng.native_global_ordinal()==3);
    assert(begin_source_rng_update(rng,tx,update())==Status::Ready);effect.call_index=0;
    assert(stage_source_rng_effect2(tx,effect).status==Status::InvalidContext);++guards;
    measuring=false;assert(allocations==before_allocations&&cases==16&&guards==54);
    std::cout<<cases<<" mixed actor/effect transactions; "<<guards<<" ordered/unknown-owner guards; zero allocations PASS\n";
}
void scene_event_transactions() {
    using namespace l3;const auto before_allocations=allocations;measuring=true;
    for(uint8_t free_slots:{0u,1u,2u}){
        SourceEventPhase5Context scene;scene.pool80.used={0xffffffffu,0xffffffffu};
        for(size_t i=0;i<64;++i){scene.pool80.slots[i].raw[1]=uint8_t(128u|i);scene.pool80.slots[i].known[1]=1;}
        for(size_t i=0;i<free_slots;++i)*scene.pool80.used[1]&=~(0x80000000u>>(30u+unsigned(i)));
        scene.pool40.used[0]=0x80000000u;auto& actor=scene.pool40.slots[0];actor.known.fill(1);actor.raw[1]=0x40;
        actor.raw[20]=17;actor.raw[22]=29;actor.raw[24]=41;
        SourceEventWork work;work.known.fill(1);work.raw[0]=work.raw[1]=1;work.raw[4]=6;
        work.raw[0x24]=0x80;work.raw[0x25]=0x35;work.raw[0x26]=0x0c;work.raw[0x27]=0x80;
        SourceRngService rng;assert(install_source_rng_service(rng,fixture(),SourceRngMode::CandidateOpenBios)==Status::Ready);
        SourceEventCandidateRng candidate;candidate.service=&rng;candidate.source_generation=7;
        auto missing=candidate;missing.service=nullptr;
        const auto held=consume_source_event_random_effect_candidate(work,scene,missing);
        assert(held.committed==(free_slots==0)&&rng.native_global_ordinal()==0);
        const auto result=consume_source_event_random_effect_candidate(work,scene,candidate);
        assert(result.committed&&result.allocated_count==free_slots&&result.committed_call_count==free_slots*3&&
            rng.native_global_ordinal()==free_slots*3);
        for(size_t i=0;i<free_slots;++i){const auto& child=scene.pool80.slots[62+i];assert(child.raw[0]==1&&child.raw[2]==131);}
        for(size_t i=0;i<rng.last_call_count();++i)assert(rng.last_calls()[i].source_generation==7&&
            rng.last_calls()[i].update.phase==SourceRngUpdatePhase::SceneEventCallback);
        const auto ordinal=rng.native_global_ordinal();work.raw[16]=31;
        assert(consume_source_event_random_effect_candidate(work,scene,candidate).committed&&rng.native_global_ordinal()==ordinal);
        SourceRngUpdateTransaction tx;
        assert(begin_source_rng_update(rng,tx,update())==Status::Ready);
        assert(stage_source_rng_class03(tx,l3::SourceRngClass03Context{l3::kSourceClass03RngCallerRa,0x8009f108u,0,1,0,0}).native_global_ordinal==ordinal+1);
        assert(commit_source_rng_update(rng,tx)==Status::Ready);
    }
    SourceRngService rng;assert(install_source_rng_service(rng,fixture(),SourceRngMode::CandidateOpenBios)==Status::Ready);
    SourceRngUpdateTransaction tx;const SourceRngUpdateContext clock{5,5,SourceRngUpdatePhase::SceneEventCallback};
    assert(begin_source_rng_update(rng,tx,clock)==Status::Ready);
    assert(stage_source_rng_scene_event(tx,{7,0,1}).status==Status::InvalidContext&&tx.call_count()==0);
    assert(stage_source_rng_scene_event(tx,{7,0,6}).status==Status::Ready);
    assert(stage_source_rng_scene_event(tx,{8,1,6}).status==Status::InvalidContext&&tx.call_count()==1);
    assert(stage_source_rng_scene_event(tx,{7,1,3}).status==Status::InvalidContext&&tx.call_count()==1);
    for(uint8_t i=1;i<3;++i)assert(stage_source_rng_scene_event(tx,{7,i,6}).status==Status::Ready);
    assert(commit_source_rng_update(rng,tx)==Status::InvalidContext&&rng.native_global_ordinal()==0);
    for(uint8_t i=3;i<6;++i)assert(stage_source_rng_scene_event(tx,{7,i,6}).status==Status::Ready);
    assert(commit_source_rng_update(rng,tx)==Status::Ready&&rng.native_global_ordinal()==6);
    assert(stage_source_rng_scene_event(tx,{7,0,3}).status==Status::StaleTransaction);
    measuring=false;assert(allocations==before_allocations);
    std::cout<<"Scene-event 0/3/6-call allocation branches, mixed stream and partial-commit guards; zero allocations PASS\n";
}
void prop_lift_transactions() {
    using namespace l3;const auto before_allocations=allocations;measuring=true;unsigned spawned=0,single=0;
    for(uint32_t seed=0;seed<8;++seed){
        SourceEventPhase5Context scene;scene.pool80.used={0u,0u};
        for(size_t i=0;i<64;++i){scene.pool80.slots[i].raw[1]=uint8_t(128u|i);scene.pool80.slots[i].known[1]=1;}
        scene.pool_e0.used[0]=0x80000000u>>14;auto& prop=scene.pool_e0.slots[14];prop.known.fill(1);
        prop.raw[0]=1;prop.raw[1]=0xee;prop.raw[2]=3;prop.raw[3]=2;prop.raw[6]=3;prop.raw[8]=prop.raw[9]=prop.raw[10]=1;
        scene.pool40.used[0]=0x80000000u;auto& a=scene.pool40.slots[0];a.known.fill(1);a.raw[1]=0x40;
        a.raw[153]=0x10;a.raw[154]=0x50;
        SourceEventWork event;event.known.fill(1);event.raw[0x24]=0x80;event.raw[0x25]=0x35;event.raw[0x26]=0x0c;event.raw[0x27]=0x80;
        SourcePropLiftCurveReceipt curves;curves.admitted=true;curves.source_table_identity=kSourcePropLiftCurveTable;
        curves.curve_identities={{0x8011c958u,0x8011c998u,0x8011c9d8u}};curves.windows[0][0]=5;
        SourcePropLiftActorFrameReceipt frame;frame.admitted=true;frame.source_actor=0x800c3580u;
        frame.sequence_identity=0x501000u;frame.source_generation=17;
        SourcePropLiftInputs inputs;inputs.curves=&curves;inputs.frame=&frame;inputs.source_generation=17;
        SourceRngService rng;assert(install_source_rng_service(rng,fixture(seed),SourceRngMode::CandidateOpenBios)==Status::Ready);
        SourcePropLiftCandidateRng candidate;candidate.service=&rng;
        const uint32_t id=0x800bf118u+14u*80u;const auto before=prop;
        frame.admitted=false;assert(!consume_source_prop_lift_candidate(scene,event,id,inputs,candidate).committed&&rng.native_global_ordinal()==0&&prop.raw==before.raw);
        frame.admitted=true;
        if(seed==0){scene.pool80.slots[0].known[1]=0;
            const auto held=consume_source_prop_lift_candidate(scene,event,id,inputs,candidate);
            assert(!held.committed&&held.status==SourcePropLiftStatus::NeedsPoolHeader&&rng.native_global_ordinal()==0&&prop.raw==before.raw&&*scene.pool80.used[0]==0);
            scene.pool80.slots[0].known[1]=1;
        }
        const auto result=consume_source_prop_lift_candidate(scene,event,id,inputs,candidate);
        assert(result.committed&&result.rng_consumed==(result.child_allocated?2:1)&&rng.native_global_ordinal()==result.rng_consumed);
        assert(prop.raw[58]==0xfb&&prop.raw[59]==0xff);
        for(size_t i=0;i<rng.last_call_count();++i)assert(rng.last_calls()[i].actor.work==id&&rng.last_calls()[i].source_generation==17&&
            rng.last_calls()[i].actor.caller_ra==result.rng_return_addresses[i]);
        if(result.child_allocated){++spawned;assert(scene.pool80.slots[0].raw[2]==3);}else ++single;
        const auto ordinal=rng.native_global_ordinal();prop.raw[58]=0xfa;prop.raw[59]=0xff;
        const auto quiet=consume_source_prop_lift_candidate(scene,event,id,inputs,candidate);
        assert(quiet.committed&&quiet.rng_consumed==0&&rng.native_global_ordinal()==ordinal);
    }
    measuring=false;assert(allocations==before_allocations&&spawned&&single);
    std::cout<<"Prop lift/frame owner, one/two-call dust branches and late-rejection rollback; zero allocations PASS\n";
}
void effect34_transactions() {
    using namespace l3;const auto before_allocations=allocations;measuring=true;
    for(uint8_t subtype:{3u,4u})for(uint8_t free_slots:{0u,1u,24u}){
        SourceEventPhase5Context scene;scene.pool80.used={0xffffffffu,0xffffffffu};scene.pool80_pending_release_800988c8={0u,0u};
        for(size_t i=0;i<64;++i){scene.pool80.slots[i].raw[1]=uint8_t(128u|i);scene.pool80.slots[i].known[1]=1;}
        for(size_t i=1;i<=free_slots;++i)*scene.pool80.used[0]&=~(0x80000000u>>i);
        auto& parent=scene.pool80.slots[0];parent.known.fill(1);parent.raw[0]=1;parent.raw[2]=subtype;parent.raw[6]=3;
        SourceEffect34ControlReceipt controls;controls.admitted=true;controls.sequence3_identity=source_effect3_sequence;controls.rows4_identity=source_effect4_rows;
        for(size_t i=0;i<10;++i)controls.sequence3[i]=uint16_t(11u+i*7u);controls.sequence3[10]=65535;
        for(size_t i=0;i<4;++i)controls.rows4[i]={{uint8_t(20+i*3),uint8_t(250+i),2,uint8_t(1+i)}};
        SourceEffect34Inputs input;input.controls=&controls;input.source_generation=19;
        const uint32_t callback=subtype==3?source_effect3_callback:source_effect4_callback;
        const SourceRngUpdateContext clock{7,7,SourceRngUpdatePhase::Effect34Callback};
        SourceRngService rng;assert(install_source_rng_service(rng,fixture(),SourceRngMode::CandidateOpenBios)==Status::Ready);
        if(free_slots){const auto before=parent;controls.admitted=false;
            assert(!tick_source_effect34_candidate_rng(scene,source_effect_base,callback,input,rng,clock).committed&&rng.native_global_ordinal()==0&&parent.raw==before.raw);
            controls.admitted=true;
        }
        const auto result=tick_source_effect34_candidate_rng(scene,source_effect_base,callback,input,rng,clock);
        const size_t children=subtype==3?size_t(free_slots!=0):free_slots;
        const size_t calls=children*(subtype==3?2u:5u);
        assert(result.committed&&result.allocated_count==children&&result.rng_consumed==calls&&rng.native_global_ordinal()==calls&&rng.last_call_count()==calls);
        for(size_t i=0;i<calls;++i)assert(rng.last_calls()[i].actor.caller_ra==result.rng_return_addresses[i]&&
            rng.last_calls()[i].source_generation==19&&rng.last_calls()[i].actor.work==source_effect_base);
        if(calls==120)assert(rng.last_calls()[59].actor.caller_ra==0x801178b4u&&rng.last_calls()[60].actor.caller_ra==0x801177e8u);
        SourceRngUpdateTransaction tx;assert(begin_source_rng_update(rng,tx,update())==Status::Ready);
        assert(stage_source_rng_class03(tx,actor(0)).native_global_ordinal==calls+1);
        assert(commit_source_rng_update(rng,tx)==Status::Ready&&rng.last_call_count()==1&&rng.last_calls()[0].source_generation==0&&rng.last_calls()[1].source_generation==0);
    }
    SourceRngService rng;assert(install_source_rng_service(rng,fixture(),SourceRngMode::CandidateOpenBios)==Status::Ready);
    SourceRngUpdateTransaction tx;const SourceRngUpdateContext clock{1,1,SourceRngUpdatePhase::Effect34Callback};
    assert(begin_source_rng_update(rng,tx,clock)==Status::Ready);
    SourceRngEffect34Context context{source_effect_base,source_effect4_callback,11,0,0,120};
    auto bad=context;bad.call_count=119;assert(stage_source_rng_effect34(tx,bad).status==Status::InvalidContext&&tx.call_count()==0);
    assert(stage_source_rng_effect34(tx,context).status==Status::Ready);
    assert(commit_source_rng_update(rng,tx)==Status::InvalidContext&&rng.native_global_ordinal()==0);
    context.call_index=1;context.source_generation=12;
    assert(stage_source_rng_effect34(tx,context).status==Status::InvalidContext&&tx.call_count()==1);
    rollback_source_rng_update(tx);assert(rng.native_global_ordinal()==0);
    measuring=false;assert(allocations==before_allocations);
    std::cout<<"Effect3/4 empty/partial/full bursts through120calls, shared actor continuation and bounded audit clearing; zero allocations PASS\n";
}
void measured_profile(const char* path) {
    std::ifstream f(path,std::ios::binary);assert(f);std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)),{});
    l3::SourceRngProfile p;assert(l3::load_l3source_rng_profile(b.data(),b.size(),p)==Status::Ready);
    assert(p.origin_seed32==0&&p.origin_rand_ordinal==0&&p.actual_rand_returns==11638);
    assert(p.scope==l3::SourceRngCaptureScope::CompletedCapturePrefix&&p.excluded_rows==2&&p.warnings==31);
    std::cout<<"private measured origin/profile decode PASS; no measured actor replay asserted\n";
}
} // namespace
int main(int argc,char** argv) {
    profile_guards();transactions();zero_heap();effect2_transactions();scene_event_transactions();prop_lift_transactions();effect34_transactions();if(argc>1)scalar_checkpoints(argv[1]);if(argc>2)measured_profile(argv[2]);
    std::cout<<"source_rng_service CANDIDATE guards/commit/rollback PASS; consumer chronology and retail parity unproved\n";
}
