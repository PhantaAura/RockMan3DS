#include "legends3ds/source_effect_dispatch.h"
#include "legends3ds/l3map.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
using namespace l3;
namespace {
bool watch_allocations=false;
size_t observed_allocations=0;
}
void* operator new(size_t count){
    if(watch_allocations)++observed_allocations;
    if(void* p=std::malloc(count?count:1))return p;
    throw std::bad_alloc();
}
void* operator new[](size_t count){return ::operator new(count);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace {
void require(bool b,const char* why){if(!b){std::cerr<<why<<'\n';std::exit(1);}}
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);require(bool(f),"fixture open");return {std::istreambuf_iterator<char>(f),{}};}
void put16(std::vector<uint8_t>& p,size_t at,uint16_t v){p[at]=uint8_t(v);p[at+1]=uint8_t(v>>8);}
void put32(std::vector<uint8_t>& p,size_t at,uint32_t v){for(size_t i=0;i<4;++i)p[at+i]=uint8_t(v>>(8*i));}
void crc(std::vector<uint8_t>& p){put32(p,24,crc32(p.data()+32,p.size()-32));}
bool rejected(const std::vector<uint8_t>& p){try{(void)load_source_effect_dispatch_profile(p);return false;}catch(const std::exception&){return true;}}
SourceEventPhase5Context cold(){SourceEventPhase5Context c;initialize_source_event_phase5_pools_8002e88c(c);return c;}
void actor(SourceEventPhase5Context& c,size_t slot,uint8_t type,uint8_t parameter=2,uint8_t clock=0){
    auto& r=c.pool80.slots[slot];r.raw[0]=1;r.raw[2]=type;r.raw[3]=parameter;r.raw[6]=3;r.raw[11]=clock;
    *c.pool80.used[slot/32]|=0x80000000u>>(slot&31u);
}
struct ProviderWatch {std::array<uint32_t,64> identities{};size_t prepares=0,completes=0,published=0;};
void prepare2(void* ptr,const SourceEventPhase5Context&,uint32_t id,SourceEffect2Inputs&) noexcept {
    auto& w=*static_cast<ProviderWatch*>(ptr);w.identities[w.prepares++]=id;
}
void complete2(void* ptr,const SourceEventPhase5Context&,uint32_t,const SourceEffect2Result& r) noexcept {
    auto& w=*static_cast<ProviderWatch*>(ptr);++w.completes;if(r.committed)++w.published;
}
}
int main(int argc,char** argv){
    require(argc==1||argc==4||argc==5,"optional arguments: L3E2 P5 L3EBD-or-raw-EBD [L3RG]");
    std::vector<uint8_t> malformed(384);malformed[0]='L';malformed[1]='3';malformed[2]='E';malformed[3]='2';
    put16(malformed,4,1);put16(malformed,6,160);put32(malformed,8,1);put32(malformed,12,16);put32(malformed,16,8);put32(malformed,20,384);crc(malformed);
    require(rejected(malformed),"authored wrong source identities accepted");
    for(size_t n=0;n<384;++n)require(rejected(std::vector<uint8_t>(n)),"truncated package accepted");
    malformed.resize(385);require(rejected(malformed),"overlong package accepted");
    const SourceEffectDispatchProfile empty;SourceEffectDispatchLease absent;
    const SourceEventPhase5Profile p5;const SourceEventPhase5ResourceBinding resources;const EbdDirectory noebd;int owner=0;
    require(!absent.bind_overlay_load_80100354(empty,p5,resources,noebd,&owner,1),"empty lease admitted");
    auto context=cold();actor(context,0,2,2);const auto before=context.pool80.slots[0].raw;
    const auto held=tick_source_effect_dispatch_800172f4(context,empty,absent,p5,resources,noebd,&owner,1);
    require(held.status==SourceEffectDispatchStatus::NeedsLease&&held.visited==0&&context.pool80.slots[0].raw==before,"unbound dispatch mutated context");
    if(argc==1){std::cout<<"384 package framing/unknown-owner guards passed; owner-local positive cases require explicit private fixtures\n";return 0;}
    const auto package=read(argv[1]);auto profile=load_source_effect_dispatch_profile(package);
    require(profile.admitted()&&profile.callback(2)==source_effect2_callback&&profile.callback(3)==source_effect3_callback&&profile.callback(4)==source_effect4_callback,"owner scalar profile mismatch");
    require(!profile.callback(16)&&!profile.callback(255),"callback coverage overrun");
    size_t guards=0;
    for(size_t i=0;i<package.size();++i){auto bad=package;bad[i]^=1;if(i!=24&&i!=25&&i!=26&&i!=27)crc(bad);
        require(rejected(bad),"self-CRC mutated owner package accepted");++guards;}
    auto phase5=load_source_event_phase5_profile(read(argv[2]));const auto eb=read(argv[3]);
    auto ebd=eb.size()>=4&&eb[0]=='L'&&eb[1]=='3'&&eb[2]=='E'&&eb[3]=='B'?load_l3ebd_directory(eb):parse_ebd_directory(eb,0x8016c000u);
    auto binding=bind_source_event_phase5_resources(phase5,ebd);require(binding.admitted(),"private EBD source admission rejected");
    SourceEffectDispatchLease lease;require(lease.bind_overlay_load_80100354(profile,phase5,binding,ebd,&owner,7),"private dispatch lease rejected");
    require(lease.current(profile,phase5,binding,ebd,&owner,7),"fresh lease stale");
    auto profileCopy=profile;auto phaseCopy=phase5;auto bindingCopy=binding;int other=0;
    require(!lease.current(profileCopy,phase5,binding,ebd,&owner,7),"profile rebind accepted");
    require(!lease.current(profile,phaseCopy,binding,ebd,&owner,7),"P5 rebind accepted");
    require(!lease.current(profile,phase5,bindingCopy,ebd,&owner,7),"resource binding rebind accepted");
    require(!lease.current(profile,phase5,binding,ebd,&other,7)&&!lease.current(profile,phase5,binding,ebd,&owner,8),"scene generation/owner stale accepted");
    auto changed=ebd;changed.entries[0].resource_offset[0]^=4;require(!lease.current(profile,phase5,binding,changed,&owner,7),"changed source directory accepted");
    changed=ebd;std::vector<uint8_t> duplicate(changed.source_payload.begin(),changed.source_payload.end());changed.source_payload.assign(duplicate.begin(),duplicate.end());
    require(!lease.current(profile,phase5,binding,changed,&owner,7),"replaced identical payload accepted");
    changed.source_payload.clear();require(!lease.current(profile,phase5,binding,changed,&owner,7),"unloaded EBD accepted");
    const auto sweep=[&](SourceEventPhase5Context& c,const SourceEffectDispatchInputProvider& provider=SourceEffectDispatchInputProvider{}){
        const auto beforeAllocations=observed_allocations;watch_allocations=true;
        const auto batch=tick_source_effect_dispatch_800172f4(c,profile,lease,phase5,binding,ebd,&owner,7,provider);
        watch_allocations=false;require(observed_allocations==beforeAllocations,"hot sweep allocated heap storage");
        return batch;};
    if(argc==5){
        const auto bytes=read(argv[4]);SourceRngProfile rng_profile;
        require(load_l3source_rng_profile(bytes.data(),bytes.size(),rng_profile)==SourceRngStatus::Ready,"private RNG profile rejected");
        SourceRngService rng;require(install_source_rng_service(rng,rng_profile,SourceRngMode::CandidateOpenBios)==SourceRngStatus::Ready,"candidate install rejected");
        SourceEffectDispatchInputProvider candidate;candidate.effect2_candidate_rng=&rng;
        candidate.candidate_update={1,1,SourceRngUpdatePhase::Effect2Callback};
        candidate.effect34_candidate_rng=&rng;candidate.candidate34_update={1,1,SourceRngUpdatePhase::Effect34Callback};
        context=cold();actor(context,0,2,0);const auto result=sweep(context,candidate);
        require(result.callbacks==2&&result.committed==2&&result.children==1&&result.rng_consumed==3&&rng.native_global_ordinal()==3,
            "candidate parent/child did not run through original ascending sweep");
        require(context.pool80.slots[1].raw[13]==1&&context.pool80.slots[1].raw[11]==23,"new candidate child not ticked same sweep");
        require(sweep(context,candidate).rng_consumed==0&&rng.native_global_ordinal()==3,"nonspawn sweep advanced shared RNG");
        // A separately supplied provider must never be silently replaced by
        // candidate values, even when its receipts are missing.
        context=cold();actor(context,0,2,0);ProviderWatch watch;candidate.user=&watch;
        candidate.prepare2=&prepare2;candidate.complete2=&complete2;
        const auto explicit_hold=sweep(context,candidate);
        require(explicit_hold.held==1&&explicit_hold.committed==0&&rng.native_global_ordinal()==3,
            "candidate RNG bypassed explicit receipt provider");
        context=cold();actor(context,0,3,0);
        const auto dust=sweep(context,candidate);
        require(dust.callbacks==2&&dust.committed==2&&dust.children==1&&dust.rng_consumed==2&&rng.native_global_ordinal()==5,
            "candidate subtype3 parent/child sweep failed");
        context=cold();actor(context,0,4,0);
        const auto burst=sweep(context,candidate);
        require(burst.callbacks==25&&burst.committed==25&&burst.children==24&&burst.rng_consumed==120&&rng.native_global_ordinal()==125,
            "candidate subtype4 full burst/child sweep failed");
        candidate.prepare34=[](void*,const SourceEventPhase5Context&,uint32_t,uint32_t,SourceEffect34Inputs&) noexcept {};
        context=cold();actor(context,0,3,0);const auto held34=sweep(context,candidate);
        require(held34.held==1&&held34.committed==0&&rng.native_global_ordinal()==125,
            "candidate RNG bypassed explicit subtype34 provider");
        std::cout<<"owner-local candidate parent/child sweep and explicit-provider priority PASS; chronology CANDIDATE\n";
    }
    // Child in later slot is initialized then ticked in the same source sweep.
    context=cold();actor(context,0,2,1);auto result=sweep(context);
    require(result.status==SourceEffectDispatchStatus::Ready&&result.visited==64&&result.callbacks==2&&result.committed==2&&result.children==1,"later child not visited same sweep");
    require(context.pool80.slots[1].raw[11]==31&&context.pool80.slots[0].raw[11]==1,"later-child original timer order");
    // Newly allocated earlier slot waits for the following sweep.
    context=cold();actor(context,1,2,1);result=sweep(context);
    require(result.callbacks==1&&result.children==1&&context.pool80.slots[0].raw[11]==32,"earlier child visited too soon");
    result=sweep(context);require(result.callbacks==2&&context.pool80.slots[0].raw[11]==31,"earlier child missing next sweep");
    // Exact raw gate does not add an original bitmap eligibility requirement.
    context=cold();actor(context,63,2,2);context.pool80.used={0u,0u};result=sweep(context);
    require(result.callbacks==1&&result.committed==1,"original dispatcher invented bitmap gate");
    // Unsupported lower0 is visible; later supported callback still commits.
    context=cold();actor(context,0,0);actor(context,1,2,1,1);result=sweep(context);
    require(result.status==SourceEffectDispatchStatus::Partial&&result.held==1&&result.holds[0].kind==SourceEffectDispatchHoldKind::UnsupportedCallback&&result.holds[0].callback==0x80116d00u,"unsupported lower callback silently skipped");
    require(context.pool80.slots[1].raw[11]==2&&result.committed==1,"unsupported slot rolled back valid callback");
    // Builtin131 belongs to a later sweep, never this scalar pass.
    context=cold();actor(context,5,131);result=sweep(context);require(result.callbacks==0&&result.held==0,"builtin subtype invoked in lower sweep");
    // Both sibling child callbacks receive their guarded control owner.
    context=cold();actor(context,0,3,0);actor(context,1,4,0);
    context.pool80.slots[0].raw[13]=1;context.pool80.slots[0].raw[10]=1;
    context.pool80.slots[1].raw[13]=1;context.pool80.slots[1].raw[9]=1;context.pool80.slots[1].raw[14]=5;
    result=sweep(context);
    require(result.committed==2&&result.held==0&&context.pool80.slots[0].raw[30]==6&&context.pool80.slots[0].raw[10]==2,"subtype3 control owner/sweep missing");
    require(context.pool80.slots[1].raw[32]==5&&context.pool80.slots[1].raw[10]==1,"subtype4 control owner/sweep missing");
    context.pool80.slots[0].raw[8]=1;context.pool80.slots[1].raw[8]=1;result=sweep(context);
    require(result.committed==2&&result.released==2&&context.pool80.slots[0].raw[0]==0&&context.pool80.slots[1].raw[0]==0&&context.pool80_pending_release_800988c8[0]==0xc0000000u,"sibling deferred release publication");
    // Per-callback publication survives a later missing RNG owner.
    context=cold();actor(context,0,2,1,1);actor(context,1,2,0,0);const auto parent1=context.pool80.slots[1];
    ProviderWatch watch;SourceEffectDispatchInputProvider provider;provider.user=&watch;provider.prepare2=prepare2;provider.complete2=complete2;
    result=sweep(context,provider);
    require(result.committed==1&&result.held==1&&result.rng_consumed==0&&context.pool80.slots[0].raw[11]==2,"held RNG callback rolled back earlier publication");
    require(context.pool80.slots[1].raw==parent1.raw&&context.pool80.slots[1].known==parent1.known,"held RNG callback mutated its parent");
    require(watch.prepares==2&&watch.completes==2&&watch.published==1&&watch.identities[0]==source_effect_base&&watch.identities[1]==source_effect_base+72,"provider callback order/publication receipt");
    context=cold();actor(context,0,2,1,1);context.pool80.slots[9].known[6]=0;result=sweep(context);
    require(result.committed==1&&result.held==1&&result.holds[0].slot==9&&result.holds[0].missing_offset==6,"gate unknown not visibly held");
    // Rejected rebind invalidates the previous lease, rather than keeping it.
    require(!lease.bind_overlay_load_80100354(empty,phase5,binding,ebd,&owner,7),"invalid rebind admitted");
    require(!lease.current(profile,phase5,binding,ebd,&owner,7),"failed rebind retained stale lease");
    std::cout<<guards<<" self-CRC malformed guards; private profile/lease and ascending per-callback sweep checks passed\n";
}
