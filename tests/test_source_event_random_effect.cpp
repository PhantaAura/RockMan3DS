#include "legends3ds/source_event_random_effect.h"
#include "legends3ds/source_event_pipeline.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceEventRandomEffectStatus;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<size_t N>std::array<uint8_t,N> authored(unsigned seed){std::array<uint8_t,N> out{};
    for(size_t i=0;i<N;++i)out[i]=uint8_t(seed*37+i*17+N*11);return out;
}
void put(uint8_t* data,size_t offset,uint32_t value,size_t count=4){for(size_t i=0;i<count;++i)data[offset+i]=uint8_t(value>>(8*i));}
uint16_t half(const l3::SourceEventPhase5Bytes<72>& record,size_t offset){return uint16_t(record.raw[offset])|uint16_t(uint16_t(record.raw[offset+1])<<8);}
l3::SourceEventWork event(unsigned seed,uint32_t frame,unsigned actor_slot){l3::SourceEventWork out;
    out.raw=authored<168>(seed);out.known.fill(1);out.raw[0]=out.raw[1]=1;out.raw[4]=6;
    put(out.raw.data(),16,frame);put(out.raw.data(),36,0x800c3580u+actor_slot*1444u);return out;
}
l3::SourceEventPhase5Context scene(unsigned seed,uint32_t used0,uint32_t used1,unsigned actor_slot,
    std::array<uint16_t,3> position){l3::SourceEventPhase5Context out;const auto pool=authored<64*72>(seed);
    for(size_t slot=0;slot<64;++slot){auto& record=out.pool80.slots[slot];
        for(size_t i=0;i<72;++i)record.raw[i]=pool[slot*72+i];record.raw[1]=uint8_t(0x80u|slot);record.known.fill(1);
    }
    out.pool80.used={{used0,used1}};out.pool40.used[0]=0x80000000u>>actor_slot;
    auto& actor=out.pool40.slots[actor_slot];actor.raw=authored<1444>(seed);actor.known.fill(1);actor.raw[1]=uint8_t(0x40u|actor_slot);
    for(size_t i=0;i<3;++i)put(actor.raw.data(),20+i*2,position[i],2);return out;
}
bool same(const l3::SourceEventPhase5Context& a,const l3::SourceEventPhase5Context& b){
    if(a.pool80.used!=b.pool80.used||a.pool40.used!=b.pool40.used)return false;
    for(size_t i=0;i<64;++i)if(a.pool80.slots[i].raw!=b.pool80.slots[i].raw||a.pool80.slots[i].known!=b.pool80.slots[i].known)return false;
    for(size_t i=0;i<4;++i)if(a.pool40.slots[i].raw!=b.pool40.slots[i].raw||a.pool40.slots[i].known!=b.pool40.slots[i].known)return false;
    return true;
}
l3::SourceEventRandomEffectReceipts receipts(const l3::SourceEventRandomEffectPlan& plan,const void* owner){
    l3::SourceEventRandomEffectReceipts out;out.owner=owner;out.generation=19;out.completed=true;
    out.callback=l3::kSourceEventRandomEffectCallback;out.source_work=0x800c4c48;out.event_frame=plan.frame;
    out.source_actor=plan.source_actor;out.count=plan.rng_calls;
    constexpr uint32_t values[]={0,63,31,32767,32767,32767};
    for(size_t i=0;i<out.count;++i)out.calls[i]={plan.caller_ras[i],values[i],1001+i};return out;
}
void authored_guards(){
    const int owner=0,other=0;l3::SourceEventRandomEffectReceiptOwner current{&owner,19};
    auto work=event(1,6,0);auto owned=scene(1,0,0,0,{{0,32768,65535}});
    const auto plan=l3::plan_source_event_random_effect_80100d6c(work,owned);
    require(plan.status==Status::Ready&&plan.rng_calls==6&&plan.allocation_count==2&&plan.slots[0]==0&&plan.slots[1]==1,
        "two source allocation attempts did not produce exact six-call plan");
    auto batch=receipts(plan,&owner);const auto before=owned;const auto allocated=allocations;
    auto result=l3::consume_source_event_random_effect_80100d6c(work,owned,&batch,&current);
    require(result.committed&&result.committed_call_count==6&&result.control_80122424==0&&allocations==allocated,
        "completed callback did not publish exact receipt counts/control or allocated");
    require(half(owned.pool80.slots[0],28)==65505&&half(owned.pool80.slots[0],30)==32784&&half(owned.pool80.slots[0],32)==65535&&
        half(owned.pool80.slots[1],28)==32&&half(owned.pool80.slots[1],30)==32784&&half(owned.pool80.slots[1],32)==31,
        "authored masked offsets/wrap stores failed");
    require(owned.pool80.slots[0].raw[0]==1&&owned.pool80.slots[0].raw[1]==0x80&&owned.pool80.slots[0].raw[2]==131&&
        owned.pool80.slots[0].raw[3]==0&&owned.pool80.slots[0].raw[6]==3,"source allocated header stores failed");
    for(unsigned test=0;test<19;++test){auto next=before;auto pending=batch;auto live=current;auto input=work;
        const l3::SourceEventRandomEffectReceipts* supplied=&pending;const l3::SourceEventRandomEffectReceiptOwner* current_ptr=&live;
        switch(test){
        case 0:supplied=nullptr;break;case 1:current_ptr=nullptr;break;case 2:pending.completed=false;break;
        case 3:pending.owner=nullptr;break;case 4:pending.generation=0;break;case 5:live.generation++;break;
        case 6:live.owner=&other;break;case 7:pending.callback++;break;case 8:pending.source_work++;break;
        case 9:pending.event_frame++;break;case 10:pending.source_actor+=1444;break;case 11:pending.count=5;break;
        case 12:pending.calls[3].caller_ra++;break;case 13:pending.calls[2].return_v0=32768;break;
        case 14:pending.calls[0].global_ordinal=0;break;case 15:pending.calls[3].global_ordinal++;break;
        case 16:pending.calls[0].global_ordinal=std::numeric_limits<uint64_t>::max();pending.calls[1].global_ordinal=0;break;
        case 17:next.pool80.slots[1].known[1]=0;break;default:next.pool40.slots[0].known[25]=0;break;
        }
        const auto snapshot=next;const auto held=l3::consume_source_event_random_effect_80100d6c(input,next,supplied,current_ptr);
        require(!held.committed&&!held.control_80122424&&held.committed_call_count==0&&same(next,snapshot),"incomplete callback/owner/receipt published partial pool or global store");
    }
    for(uint8_t count=0;count<6;++count){auto pending=batch;pending.count=count;auto next=before;
        require(!l3::consume_source_event_random_effect_80100d6c(work,next,&pending,&current).committed&&same(next,before),"partial completed six-call batch admitted");}
    for(uint32_t frame:{31u,0x7fffffffu}){auto input=event(1,frame,0);input.known[36]=0;auto next=before;
        next.pool80.used[0].reset();const auto snapshot=next;
        const auto skipped=l3::consume_source_event_random_effect_80100d6c(input,next);
        require(skipped.committed&&skipped.committed_call_count==0&&skipped.allocated_count==0&&!skipped.control_80122424&&same(next,snapshot),"source signed skip branch invented RNG/allocator dependencies");}
    for(uint32_t frame:{0x80000000u,0xffffffffu})require(l3::plan_source_event_random_effect_80100d6c(event(1,frame,0),before).rng_calls==6,"signed negative frame branch skipped RNG");
    auto full=scene(1,0xffffffff,0xffffffff,0,{{0,0,0}});auto no_actor=work;put(no_actor.raw.data(),36,0xabcdef01);
    full.pool40.used[0].reset();const auto full_before=full;
    result=l3::consume_source_event_random_effect_80100d6c(no_actor,full);
    require(result.committed&&result.allocated_count==0&&result.committed_call_count==0&&result.control_80122424==0&&same(full,full_before),"full original pool did not preserve null-allocation path");
    auto one=scene(1,0xffffffff,0xfffffffe,0,{{0,0,0}});auto one_plan=l3::plan_source_event_random_effect_80100d6c(work,one);
    require(one_plan.rng_calls==3&&one_plan.allocation_count==1&&one_plan.slots[0]==63,"one free original pool slot did not require three calls");
    auto one_receipts=receipts(one_plan,&owner);one_receipts.calls[3]={0x80100e40,2,1004};const auto one_before=one;
    require(!l3::consume_source_event_random_effect_80100d6c(work,one,&one_receipts,&current).committed&&same(one,one_before),"undeclared trailing RNG call ignored");
    one_receipts.calls[3]={};result=l3::consume_source_event_random_effect_80100d6c(work,one,&one_receipts,&current);
    require(result.committed&&result.committed_call_count==3&&result.allocated_slots[0]==63,"three-call batch did not consume original final slot");
    auto unused=before;unused.pool80.used[1].reset();require(l3::plan_source_event_random_effect_80100d6c(work,unused).status==Status::Ready,"unread second bitmap invented a dependency");
    unused.pool80.used[0]=0xffffffff;require(l3::plan_source_event_random_effect_80100d6c(work,unused).status==Status::NeedsPoolBitmap,"read second bitmap unknown admitted");
    const auto count_before=allocations;auto missing=before;
    for(unsigned i=0;i<4096;++i)require(!l3::consume_source_event_random_effect_80100d6c(work,missing).committed&&same(missing,before),"unknown RNG hold advanced pool");
    require(allocations==count_before,"unknown RNG holds allocated");
}
void apply_delta(l3::SourceEventPhase5Context& scene,const std::string& text){if(text=="-")return;
    std::istringstream list(text);std::string item;
    while(std::getline(list,item,',')){const auto separator=item.find(':');require(separator!=std::string::npos,"delta rejected");
        const size_t offset=std::stoul(item.substr(0,separator));const unsigned value=std::stoul(item.substr(separator+1));
        require(offset<64*72&&value<=255,"delta outside authored owner");scene.pool80.slots[offset/72].raw[offset%72]=uint8_t(value);}
}
void differential(const char* path){std::ifstream file(path);require(bool(file),"private differential fixture missing");std::string line;
    std::getline(file,line);size_t cases=0;const int owner=0;l3::SourceEventRandomEffectReceiptOwner current{&owner,19};
    while(std::getline(file,line)){unsigned seed,actor_slot,pos0,pos1,pos2,count,control;uint32_t frame,used0,used1,out0,out1;
        std::string slots,calltext,delta,extra;std::istringstream row(line);
        require(bool(row>>seed>>frame>>used0>>used1>>actor_slot>>pos0>>pos1>>pos2>>count>>slots>>calltext>>delta>>out0>>out1>>control)&&!(row>>extra),"private oracle row rejected");
        auto owned=scene(seed,used0,used1,actor_slot,{{uint16_t(pos0),uint16_t(pos1),uint16_t(pos2)}});auto expected=owned;
        apply_delta(expected,delta);expected.pool80.used={{out0,out1}};const auto work=event(seed,frame,actor_slot);
        const auto plan=l3::plan_source_event_random_effect_80100d6c(work,owned);require(plan.status==Status::Ready&&plan.rng_calls==count,"source allocation/RNG count differential failed");
        auto batch=receipts(plan,&owner);batch.calls={};size_t ordinal=0;
        if(calltext!="-"){std::istringstream calls(calltext);std::string token;
            while(std::getline(calls,token,',')){for(char& c:token)if(c==':')c=' ';std::istringstream fields(token);
                require(ordinal<batch.calls.size()&&bool(fields>>batch.calls[ordinal].caller_ra>>batch.calls[ordinal].return_v0>>batch.calls[ordinal].global_ordinal),"private receipt rejected");++ordinal;}}
        require(ordinal==count,"private receipt count changed");const auto allocated=allocations;
        const auto result=l3::consume_source_event_random_effect_80100d6c(work,owned,count?&batch:nullptr,count?&current:nullptr);
        require(result.committed&&result.committed_call_count==count&&result.allocated_count==count/3&&same(owned,expected)&&allocations==allocated,
            "unchanged original full family80 owner differential failed");
        require((frame==6&&control==0&&result.control_80122424==0)||(frame!=6&&control==255&&!result.control_80122424),"original frame6 global control store differential failed");
        std::string actual_slots;for(size_t i=0;i<result.allocated_count;++i){if(i)actual_slots+=',';actual_slots+=std::to_string(result.allocated_slots[i]);}
        if(actual_slots.empty())actual_slots="-";require(actual_slots==slots,"original source allocator slot order changed");++cases;
    }
    require(cases==42,"focused original oracle case coverage changed");std::cout<<"42 private unchanged-instruction differential cases passed\n";
}
} // namespace
int main(int argc,char** argv){try{require(argc<=2,"optional private fixture only");authored_guards();if(argc==2)differential(argv[1]);
    std::cout<<"source_event_random_effect passed\n";return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
