#include "legends3ds/retail_rng_tape.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::RetailRngTapeStatus;
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
struct Authored {
    std::array<l3::RetailRngObservedSample,4> samples{};
    l3::RetailRngTraceProof proof{};
    Authored(){
        proof.trace_sha256[0]=0x14;proof.trace_sha256[1]=0x21;
        proof.supported_source_identities=true;proof.whole_trace_validated=true;
        proof.continuous_since_installation=true;proof.pending_return_at_end=false;
        proof.actual_rand_returns=4;proof.initial_seed32=0xffffffffu;
        auto seed=proof.initial_seed32;
        for(size_t i=0;i<samples.size();++i){
            auto& s=samples[i];s.global_ordinal=static_cast<uint32_t>(i+1);
            s.reference_vsync=100u+static_cast<uint32_t>(i/2);
            s.caller_ra=i==0?0x801047acu:0x801022f8u;
            s.seed_before32=seed;seed=seed*1103515245u+12345u;
            s.seed_after32=seed;s.actual_v0=(seed>>16)&0x7fffu;
            if(i!=0){
                l3::RetailRngActorContext actor;actor.slot=static_cast<uint8_t>(i+1);
                actor.work=0x8009f108u+uint32_t(actor.slot)*0x3c0u;
                actor.state08=1;actor.action09=static_cast<uint8_t>(i*3);actor.phase0a=2;
                actor.stats_index0c=0;actor.context_high0f=0x10;
                actor.raw_position14={{-32768,32767,static_cast<int16_t>(i)}};
                actor.heading56=65535;s.class03=actor;
            }
        }
    }
    l3::RetailRngReplayBoundary boundary(size_t index)const{
        l3::RetailRngReplayBoundary b;b.trace_sha256=proof.trace_sha256;
        b.reference_vsync=samples[index].reference_vsync;b.global_ordinal=samples[index].global_ordinal;
        b.input_timeline_matches=true;b.callback_schedule_matches=true;return b;
    }
};
void proof_guards(){
    Authored fixture;l3::RetailRngTape tape;
    require(l3::bind_retail_rng_tape(tape,fixture.samples.data(),4,fixture.proof)==Status::Ready,"authored valid reference rejected");
    require(tape.size()==4 && tape.admitted(),"validated immutable view missing");
    for(unsigned field=0;field<8;++field){
        auto p=fixture.proof;
        switch(field){
            case 0:p.trace_sha256={};break;
            case 1:p.supported_source_identities=false;break;
            case 2:p.whole_trace_validated=false;break;
            case 3:p.continuous_since_installation=false;break;
            case 4:p.pending_return_at_end=true;break;
            case 5:p.actual_rand_returns=3;break;
            case 6:p.initial_seed32=0;break;
            default:p.actual_reseed_returns=1;break;
        }
        const auto status=l3::bind_retail_rng_tape(tape,fixture.samples.data(),4,p);
        require(status==(field==6?Status::InvalidSample:Status::NeedsValidatedTrace),"failed-prefix/gapped/profile/count proof admitted");
        require(!tape.admitted() && tape.size()==0,"failed rebinding retained earlier usable sample view");
    }
    require(l3::bind_retail_rng_tape(tape,nullptr,4,fixture.proof)==Status::NeedsValidatedTrace,"null owner admitted");
    require(l3::bind_retail_rng_tape(tape,fixture.samples.data(),0,fixture.proof)==Status::NeedsValidatedTrace,"empty tape admitted");
    require(l3::bind_retail_rng_tape(tape,fixture.samples.data(),65537,fixture.proof)==Status::NeedsValidatedTrace,"oversized tape dereferenced/admitted");
    for(unsigned mutation=0;mutation<12;++mutation){
        auto changed=fixture.samples;
        switch(mutation){
            case 0:++changed[1].global_ordinal;break;
            case 1:changed[2].reference_vsync=99;break;
            case 2:++changed[2].seed_before32;break;
            case 3:++changed[2].seed_after32;break;
            case 4:++changed[2].actual_v0;break;
            case 5:changed[1].caller_ra=0;break;
            case 6:changed[1].caller_ra|=1;break;
            case 7:changed[1].class03->slot=16;break;
            case 8:++changed[1].class03->work;break;
            case 9:changed[1].caller_ra=0x801047acu;break;
            case 10:changed[1].class03.reset();break;
            default:changed[1].caller_ra=0x801022fcu;break;
        }
        require(l3::bind_retail_rng_tape(tape,changed.data(),4,fixture.proof)==Status::InvalidSample,"malformed sample context/profile/chronology admitted");
    }
}
void admission_transactions(){
    Authored fixture;l3::RetailRngTape tape;l3::RetailRngTapeCursor cursor;
    require(l3::begin_retail_rng_tape(tape,cursor)==Status::NeedsValidatedTrace,"empty tape began");
    require(l3::bind_retail_rng_tape(tape,fixture.samples.data(),4,fixture.proof)==Status::Ready,"bind failed");
    require(l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(0),0x801047ac,std::nullopt).status==Status::NeedsValidatedTrace,"cursor from previous view reused");
    require(l3::begin_retail_rng_tape(tape,cursor)==Status::Ready,"validated installation boundary missing");
    require(l3::peek_retail_rng_sample(tape,cursor,std::nullopt,0x801047ac,std::nullopt).status==Status::NeedsChronologyBinding,"unbound native frame admitted");
    for(unsigned field=0;field<5;++field){
        auto b=fixture.boundary(0);
        switch(field){case 0:b.trace_sha256[1]^=1;break;case 1:b.input_timeline_matches=false;break;
            case 2:b.callback_schedule_matches=false;break;case 3:++b.global_ordinal;break;default:++b.reference_vsync;break;}
        require(l3::peek_retail_rng_sample(tape,cursor,b,0x801047ac,std::nullopt).status==
            (field<3?Status::NeedsChronologyBinding:Status::NextConsumerMismatch),"incorrect measured timeline accepted");
        require(cursor.position()==0,"failed boundary advanced global chronology");
    }
    require(l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(1),0x801022f8,fixture.samples[1].class03).status==Status::NextConsumerMismatch,"competing global RNG consumer silently skipped");
    auto first=l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(0),0x801047ac,std::nullopt);
    require(first.status==Status::Ready && first.ticket && cursor.position()==0,"peek committed on its own");
    const auto old=*first.ticket;
    require(l3::commit_retail_rng_sample(tape,cursor,*first.ticket)==Status::Ready && cursor.position()==1,"explicit committed consumer failed");
    require(l3::commit_retail_rng_sample(tape,cursor,old)==Status::StaleTicket && cursor.position()==1,"reused ticket repeated sample");
    for(unsigned field=0;field<11;++field){
        auto actor=*fixture.samples[1].class03;
        switch(field){case 0:++actor.work;break;case 1:++actor.slot;break;case 2:++actor.state08;break;
            case 3:++actor.action09;break;case 4:++actor.phase0a;break;case 5:++actor.stats_index0c;break;
            case 6:++actor.context_high0f;break;case 7:++actor.raw_position14[0];break;
            case 8:--actor.raw_position14[1];break;case 9:++actor.raw_position14[2];break;default:--actor.heading56;break;}
        require(l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(1),0x801022f8,actor).status==Status::ActorContextMismatch,"wrong source actor/state/position/heading accepted");
        require(cursor.position()==1,"rejected actor consumed scalar");
    }
    auto stage=l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(1),0x801022f8,fixture.samples[1].class03);
    require(stage.ticket && stage.ticket->sample32a==fixture.samples[1].actual_v0,"captured source scalar changed");
    // A failing larger actor transaction discards its staged cursor/body.
    auto staged_cursor=cursor;
    require(l3::commit_retail_rng_sample(tape,staged_cursor,*stage.ticket)==Status::Ready &&
        staged_cursor.position()==2 && cursor.position()==1,"staged transaction leaked chronology on rollback");
    for(size_t i=1;i<4;++i){
        auto observed=l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(i),0x801022f8,fixture.samples[i].class03);
        require(observed.status==Status::Ready && observed.ticket,"exact source context not accepted");
        require(l3::commit_retail_rng_sample(tape,cursor,*observed.ticket)==Status::Ready,"one-use commit rejected");
    }
    require(l3::peek_retail_rng_sample(tape,cursor,fixture.boundary(3),0x801022f8,fixture.samples[3].class03).status==Status::Exhausted,"tape wrapped/repeated after end");
    require(l3::commit_retail_rng_sample(tape,cursor,old)==Status::Exhausted,"exhausted tape accepted old ticket");
    require(l3::bind_retail_rng_tape(tape,fixture.samples.data(),4,fixture.proof)==Status::Ready,"identical immutable rebind failed");
    require(l3::commit_retail_rng_sample(tape,cursor,old)==Status::NeedsValidatedTrace,"same-pointer rebind failed to invalidate old cursor");
    require(l3::begin_retail_rng_tape(tape,cursor)==Status::Ready,"new view begin failed");
    require(l3::commit_retail_rng_sample(tape,cursor,old)==Status::StaleTicket,"same-pointer rebind failed to invalidate old ticket");
    const auto allocation_start=allocations;
    bool held=true;
    for(unsigned i=0;i<4096;++i)
        held&=l3::peek_retail_rng_sample(tape,cursor,std::nullopt,0x801022f8,fixture.samples[1].class03).status==Status::NeedsChronologyBinding;
    require(held && allocations==allocation_start && cursor.position()==0,"unbound hot-loop peek allocated or consumed random state");
}
}
int main(){
    try{proof_guards();admission_transactions();std::cout<<"retail_rng_tape: authored proof/context/order/rollback/exhaustion guards and 4096 zero-allocation holds PASS; no measured retail replay asserted\n";return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
