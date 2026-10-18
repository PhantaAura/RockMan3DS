#include "legends3ds/source_prop_attach.h"
#include "legends3ds/source_event_pipeline.h"
#include <cstdlib>
#include <iostream>
#include <new>

namespace {size_t allocations=0;}
void* operator new(size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
namespace {
void check(bool b,const char* why){if(!b){std::cerr<<why<<'\n';std::exit(1);}}
template<class R>void put(R& r,size_t o,size_t n,uint32_t v){for(size_t i=0;i<n;++i){r.raw[o+i]=uint8_t(v>>(8*i));r.known[o+i]=1;}}
constexpr uint32_t id=0x800bf578u;
using Status=l3::SourcePropForeignGlobalsStatus;
l3::SourceEventPhase5Pool<32,80> roster(){l3::SourceEventPhase5Pool<32,80> r;r.used[0]=0x80000000u>>14;auto& prop=r.slots[14];prop.known.fill(1);prop.raw[0]=7;prop.raw[1]=0xee;prop.raw[2]=3;prop.raw[6]=3;prop.raw[9]=1;prop.raw[12]=1;return r;}
}
int main(){
    auto pool=roster();l3::SourcePropForeignGlobals state;
    check(l3::prepare_source_prop_foreign_release(state,id,1,pool).status==Status::NeedsFreshExecutableLoad,"no guessed startup");
    l3::initialize_source_prop_foreign_globals_from_executable_load(state,1,false);check(!state.pending_8009898c(),"unadmitted startup unknown");
    l3::initialize_source_prop_foreign_globals_from_executable_load(state,1,true);
    auto prepared=l3::prepare_source_prop_foreign_release(state,id,1,pool);
    check(prepared.status==Status::Ready&&prepared.owner.admitted&&!prepared.owner.used_word&&prepared.owner.used_bits_known_mask==0xfffffe00u&&prepared.owner.pending_word==std::optional<uint32_t>{0},"truthful partial counter");
    check(l3::prepare_source_prop_foreign_release(state,id,2,pool).status==Status::StaleSourceGeneration,"stale overlay owner");
    check(!l3::commit_source_prop_foreign_release(state,prepared,false),"held callback cannot commit");
    auto corrupt=prepared;corrupt.owner.pending_word=1;check(!l3::commit_source_prop_foreign_release(state,corrupt,true),"corrupt side effect rejected");
    l3::ReferenceTables refs;refs.trig[0]={0,4096};refs.trig[1024]={4096,0};
    l3::SourceEventPhase5Pool<4,1444> actors;actors.used[0]=0x80000000u;auto& actor=actors.slots[0];actor.known.fill(1);actor.raw[1]=0x40;actor.raw[2]=5;
    put(actor,672,2,4096);put(actor,680,2,4096);put(actor,688,2,4096);
    l3::SourceEventWork event;put(event,0x24,4,0x800c3580u);
    l3::SourcePropAttachInputs in;in.references=&refs;in.event_work=&event;in.actors=&actors;in.release_owner=&prepared.owner;
    auto prop=pool.slots[14];size_t before=allocations;
    const auto result=l3::update_source_prop_attach_80112d14(prop,id,in);
    check(allocations==before&&result.committed&&result.released&&prop.raw[0]==0&&*pool.used[0]==(0x80000000u>>14),"actual release branch/no E0free/no heap");
    check(l3::commit_source_prop_foreign_release(state,prepared,result.committed&&result.released)&&state.pending_8009898c()==std::optional<uint32_t>{0},"exact foreign pending publication");
    check(!l3::commit_source_prop_foreign_release(state,prepared,true),"one-use revision");
    // Exhaust the genuine bounded count domain without selecting a convenient
    // value. Every originalAND result agrees with the known-bits transaction.
    for(uint32_t count=0;count<=256;++count){
        auto exact=prepared.owner;exact.used_word=count;exact.pending_word=0;auto copy=pool.slots[14];in.release_owner=&exact;
        const auto r=l3::update_source_prop_attach_80112d14(copy,id,in);
        check(r.committed&&r.released&&exact.pending_word==std::optional<uint32_t>{0}&&copy.raw==prop.raw,"all 257possible counter values");
    }
    // A low counter bit cannot be proved zero: slot31 aliases bit100.
    pool.used[0]=*pool.used[0]|1u;pool.slots[31]=pool.slots[14];pool.slots[31].raw[1]=0xff;
    check(l3::prepare_source_prop_foreign_release(state,0x800bfac8u,1,pool).status==Status::NeedsKnownResult,"slot31 unknown low counter bit");
    check(l3::prepare_source_prop_foreign_release(state,0x800bf118u,1,pool).status==Status::NeedsRoster,"detached prop identity");
    pool.slots[31].raw[2]=1;
    check(l3::prepare_source_prop_foreign_release(state,id,1,pool).status==Status::UncheckedQueueProducerPresent&&!state.queue_bound_admitted(),"unchecked source producer revokes bound");
    pool.slots[31].raw[2]=3;check(l3::prepare_source_prop_foreign_release(state,id,1,pool).status==Status::UncheckedQueueProducerPresent,"revocation survives later roster change");
    l3::initialize_source_prop_foreign_globals_from_executable_load(state,2,true);pool.slots[31].known[2]=0;
    check(l3::prepare_source_prop_foreign_release(state,id,2,pool).status==Status::NeedsRoster&&!state.queue_bound_admitted(),"unknown roster revokes bound");
    l3::invalidate_source_prop_foreign_globals(state);check(!state.admitted()&&!state.pending_8009898c(),"unload invalidation");
    std::cout<<"source foreign globals: 257possible count values, startup/alias/roster/stale/atomic guards PASS; no invented exact counter or E0free\n";
}
