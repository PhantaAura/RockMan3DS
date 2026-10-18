#include "legends3ds/source_fade.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>

namespace {bool count_allocations=false;size_t allocations=0;}
void* operator new(size_t n){if(count_allocations)++allocations;auto p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace {
void require(bool condition,const char* why){if(!condition){std::cerr<<why<<'\n';std::exit(1);}}
template<size_t N> void put(l3::SourceFadeBytes<N>& bytes,size_t offset,size_t size,uint32_t value){
    for(size_t i=0;i<size;++i){bytes.raw[offset+i]=static_cast<uint8_t>(value>>(i*8));bytes.known[offset+i]=1;}}
template<size_t N> void fill(l3::SourceFadeBytes<N>& bytes,unsigned seed,size_t& offset){
    for(size_t i=0;i<N;++i,++offset){bytes.raw[i]=static_cast<uint8_t>(seed*37+offset*17);bytes.known[i]=(seed+offset)%3==0;}}
l3::SourceFadeContext authored(unsigned seed){l3::SourceFadeContext out;size_t offset=0;
    fill(out.work_80098a68,seed,offset);for(auto& p:out.tiles_80098a28)fill(p,seed,offset);
    for(auto& p:out.draw_modes_80098a10)fill(p,seed,offset);return out;}
template<size_t N> bool same(const l3::SourceFadeBytes<N>& a,const l3::SourceFadeBytes<N>& b){return a.raw==b.raw&&a.known==b.known;}
bool same(const l3::SourceFadeContext& a,const l3::SourceFadeContext& b){
    if(!same(a.work_80098a68,b.work_80098a68))return false;
    for(size_t i=0;i<2;++i)if(!same(a.tiles_80098a28[i],b.tiles_80098a28[i])||!same(a.draw_modes_80098a10[i],b.draw_modes_80098a10[i]))return false;
    return true;}
bool same_known(const l3::SourceFadeContext& a,const l3::SourceFadeContext& b){
    if(a.work_80098a68.known!=b.work_80098a68.known)return false;
    for(size_t i=0;i<2;++i)if(a.tiles_80098a28[i].known!=b.tiles_80098a28[i].known||a.draw_modes_80098a10[i].known!=b.draw_modes_80098a10[i].known)return false;
    return true;}
template<size_t N> void check_bytes(const l3::SourceFadeBytes<N>& bytes,const std::string& hex,size_t& offset){
    for(auto value:bytes.raw){require(offset+2<=hex.size(),"fixture bounds");require(value==std::stoul(hex.substr(offset,2),nullptr,16),"unchanged source instruction differential");offset+=2;}}
void check_context(const l3::SourceFadeContext& context,const std::string& hex){size_t offset=0;check_bytes(context.work_80098a68,hex,offset);
    for(const auto& p:context.tiles_80098a28)check_bytes(p,hex,offset);for(const auto& p:context.draw_modes_80098a10)check_bytes(p,hex,offset);
    require(offset==hex.size(),"fixture tail");}
void callbacks(const char* path){
    std::ifstream file(path);require(bool(file),"callback fixture missing");std::string line;size_t rows=0;
    while(std::getline(file,line)){
        std::istringstream row(line);unsigned kind,seed,busy,phase,intensity,step,hold,terminated;std::string hex,extra;
        require(bool(row>>kind>>seed>>busy>>phase>>intensity>>step>>hold>>terminated>>hex)&&!(row>>extra),"callback fixture row");
        require(kind<=1&&busy<=65535&&phase<=255&&intensity<=65535&&step<=65535&&hold<=255&&terminated<=1&&hex.size()==144,"callback fixture domain");
        auto context=authored(seed);auto& work=context.work_80098a68;
        put(work,0,1,0);put(work,1,1,phase);put(work,10,2,busy);put(work,12,2,step);put(work,14,2,intensity);
        auto expected=context;
        l3::SourceFadeCallbackState state{kind==1,17};l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;queue.count=1;
        const auto result=kind?l3::advance_source_fade_callback(context,state,static_cast<uint8_t>(hold)):
            l3::acknowledge_source_fade_scheduler_intent(context,queue,state,static_cast<uint8_t>(hold));
        require(result.committed&&result.native_scheduler_acknowledged==(kind==0),"callback acknowledgment");
        require(result.source_callback_terminated==(terminated!=0)&&result.yield_one_scheduler_pass==(terminated==0),"source boundary result");
        require(result.source_boundary==(terminated?0x80012eccu:0x80012e98u)&&state.running==(terminated==0)&&state.committed_iterations==18,"callback state boundary");
        require(queue.count==(kind?1u:0u),"native queue consumption");
        if(!kind){put(expected.work_80098a68,1,1,0);put(expected.work_80098a68,4,1,0);put(expected.work_80098a68,8,2,1);put(expected.work_80098a68,14,2,0);}
        if(terminated&&busy==1)put(expected.work_80098a68,8,2,0);
        require(same_known(context,expected),"callback retains unknown bytes exactly");
        check_context(context,hex);++rows;
    }
    require(rows==1632,"callback fixture count");
}
void packets(const char* path){
    std::ifstream file(path);require(bool(file),"presentation fixture missing");std::string line;size_t rows=0;
    while(std::getline(file,line)){
        std::istringstream row(line);unsigned seed,active,intensity,buffer;uint32_t input_tag,output_tag;std::string hex,extra;
        require(bool(row>>seed>>active>>intensity>>buffer>>input_tag>>output_tag>>hex)&&!(row>>extra),"presentation fixture row");
        require(active<=65535&&intensity<=65535&&buffer<=1&&hex.size()==144,"presentation fixture domain");
        auto context=authored(seed);put(context.work_80098a68,8,2,active);put(context.work_80098a68,14,2,intensity);
        for(size_t i=0;i<4;++i){context.tiles_80098a28[buffer].known[i]=1;context.draw_modes_80098a10[buffer].known[i]=1;}
        auto expected=context;
        const auto result=l3::bind_source_fade_presentation_packets(context,{static_cast<uint8_t>(buffer),input_tag});
        require(result.committed&&result.ordering_table_tag_116==output_tag,"source ordering table ownership");
        require(result.status==(active?l3::SourceFadePresentationStatus::SourceBackedPresentation:l3::SourceFadePresentationStatus::SourceInactive),"packet state");
        if(active)for(size_t i=4;i<7;++i)expected.tiles_80098a28[buffer].known[i]=1;
        require(same_known(context,expected),"packet validity preserve");check_context(context,hex);++rows;
    }
    require(rows==192,"presentation fixture count");
}
l3::SourceFadeContext ready(uint16_t busy=1,uint16_t step=128){
    auto context=authored(9);put(context.work_80098a68,10,2,0);
    l3::SourceFadeStartupProfile profile{1,busy,step};l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;
    require(l3::consume_source_fade_startup_80017c30_type2(context,&profile,&queue).committed,"startup setup");return context;
}
void guards_and_lifecycle(){
    l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;queue.count=1;
    l3::SourceFadeCallbackState state;auto context=ready();
    const auto hold=[&](l3::SourceFadeCallbackStatus status){const auto before=context;const auto count=queue.count;const auto iterations=state.committed_iterations;
        const auto result=l3::acknowledge_source_fade_scheduler_intent(context,queue,state,std::nullopt);
        require(!result.committed&&result.status==status&&same(context,before)&&queue.count==count&&state.committed_iterations==iterations,"atomic missing owner hold");};
    queue.initialized=false;hold(l3::SourceFadeCallbackStatus::NeedsSchedulerIntent);queue.initialized=true;
    queue.count=0;hold(l3::SourceFadeCallbackStatus::NeedsSchedulerIntent);queue.count=queue.entries.size()+1;hold(l3::SourceFadeCallbackStatus::NeedsSchedulerIntent);queue.count=1;
    queue.entries[0].slot=3;hold(l3::SourceFadeCallbackStatus::UnsupportedSchedulerIntent);queue.entries[0].slot=2;
    queue.entries[0].callback=0;hold(l3::SourceFadeCallbackStatus::UnsupportedSchedulerIntent);queue.entries[0].callback=0x80017e94u;
    queue.entries[0].source_function=0;hold(l3::SourceFadeCallbackStatus::UnsupportedSchedulerIntent);queue.entries[0].source_function=0x80012e10u;
    context.work_80098a68.known[0]=0;hold(l3::SourceFadeCallbackStatus::NeedsMode);context.work_80098a68.known[0]=1;
    context.work_80098a68.raw[0]=1;hold(l3::SourceFadeCallbackStatus::UnsupportedMode);context.work_80098a68.raw[0]=0;
    context.work_80098a68.known[10]=0;hold(l3::SourceFadeCallbackStatus::NeedsBusyHalfword);context.work_80098a68.known[10]=1;
    context.work_80098a68.known[12]=0;hold(l3::SourceFadeCallbackStatus::NeedsStep);context.work_80098a68.known[12]=1;
    context=ready(1,255);hold(l3::SourceFadeCallbackStatus::NeedsEndpointHoldByte);
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,uint8_t(2)).committed,"native callback acknowledgment");
    require(context.work_80098a68.raw[14]==0&&context.work_80098a68.raw[1]==2&&context.work_80098a68.raw[12]==2,"immediate first source iteration");
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,uint8_t(2)).status==l3::SourceFadeCallbackStatus::AlreadyRunning,"must not initialize twice");
    auto description=l3::describe_source_fade_presentation(context);
    require(description.visible&&description.subtract_from_scene&&description.grayscale==0&&description.width==320&&description.height==240,"native subtract description");
    require(l3::advance_source_fade_callback(context,state,std::nullopt).yield_one_scheduler_pass&&context.work_80098a68.raw[12]==1,"hold pass1");
    require(l3::advance_source_fade_callback(context,state,std::nullopt).yield_one_scheduler_pass&&context.work_80098a68.raw[12]==0,"hold pass2");
    require(l3::advance_source_fade_callback(context,state,std::nullopt).source_callback_terminated&&!state.running&&context.work_80098a68.raw[10]==0&&context.work_80098a68.raw[8]==0,"source completion fields");
    require(l3::describe_source_fade_presentation(context).status==l3::SourceFadePresentationStatus::SourceInactive,"busy1 hides at termination");
    require(!l3::advance_source_fade_callback(context,state,std::nullopt).committed,"terminated callback not restarted");
    context=ready(2,255);state={};queue.count=1;
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,uint8_t(0)).committed,"zero hold byte is present");
    require(l3::advance_source_fade_callback(context,state,std::nullopt).source_callback_terminated&&context.work_80098a68.raw[8]==1,"non1 busy preserves presentation ownership");
    require(l3::describe_source_fade_presentation(context).visible,"non1 busy retains tile");
    // FIFO validation and consumption preserve later pending owners.
    context=ready();state={};queue.count=2;queue.entries[1].slot=7;
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,uint8_t(2)).committed&&queue.count==1&&queue.entries[0].slot==7,"bounded FIFO acknowledgment");
    // Unknown input at a resumed endpoint rolls back the exact scalar state.
    context=ready(1,128);state={true,2};put(context.work_80098a68,1,1,0);put(context.work_80098a68,14,2,127);
    const auto before=context;const auto result=l3::advance_source_fade_callback(context,state,std::nullopt);
    require(!result.committed&&result.status==l3::SourceFadeCallbackStatus::NeedsEndpointHoldByte&&same(context,before)&&state.committed_iterations==2,"endpoint rollback");
    context.work_80098a68.known[1]=0;require(l3::advance_source_fade_callback(context,state,uint8_t(2)).status==l3::SourceFadeCallbackStatus::NeedsPhase,"unknown phase");
    context.work_80098a68.known[1]=1;context.work_80098a68.known[14]=0;require(l3::advance_source_fade_callback(context,state,uint8_t(2)).status==l3::SourceFadeCallbackStatus::NeedsIntensity,"unknown intensity");
}
void presentation_guards(){
    auto context=ready();l3::SourceFadeCallbackState state;l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;queue.count=1;
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,uint8_t(2)).committed,"visible setup");
    auto& work=context.work_80098a68;
    work.known[8]=0;require(!l3::describe_source_fade_presentation(context).visible,"unknown active cannot draw");work.known[8]=1;
    work.known[14]=0;require(!l3::describe_source_fade_presentation(context).visible,"unknown intensity cannot draw");work.known[14]=1;
    context.tiles_80098a28[1].known[7]=0;require(!l3::describe_source_fade_presentation(context).visible,"unknown packet semantics cannot draw");context.tiles_80098a28[1].known[7]=1;
    context.draw_modes_80098a10[0].raw[4]^=1;require(!l3::describe_source_fade_presentation(context).visible,"unsupported blend cannot draw");context.draw_modes_80098a10[0].raw[4]^=1;
    l3::SourceFadePacketInput input;
    const auto hold=[&](l3::SourceFadePresentationStatus expected){const auto before=context;const auto result=l3::bind_source_fade_presentation_packets(context,input);
        require(!result.committed&&result.status==expected&&same(context,before),"packet owner rollback");};
    hold(l3::SourceFadePresentationStatus::NeedsBufferSelector);input.buffer_1f800000=2;hold(l3::SourceFadePresentationStatus::UnsupportedBufferSelector);
    input.buffer_1f800000=0;hold(l3::SourceFadePresentationStatus::NeedsOrderingTableTag);input.ordering_table_tag_116=0;
    context.tiles_80098a28[0].known[0]=0;hold(l3::SourceFadePresentationStatus::NeedsPacketTag);
    // Zero is a real known ordering-tag value; it is not a missing optional.
    for(size_t i=0;i<4;++i){context.tiles_80098a28[0].known[i]=1;context.draw_modes_80098a10[0].known[i]=1;}
    require(l3::bind_source_fade_presentation_packets(context,input).committed,"zero ordering tag is owned");
    context=ready();put(context.work_80098a68,8,2,0);
    const auto before=context;require(l3::bind_source_fade_presentation_packets(context,{}).committed&&same(context,before),"inactive source no-op needs no buffer or tags");
}
void allocation_free(){
    auto context=ready(1,0);l3::SourceFadeCallbackState state;l3::SourceFadeSchedulerIntentQueue queue;queue.initialized=true;queue.count=1;
    require(l3::acknowledge_source_fade_scheduler_intent(context,queue,state,std::nullopt).committed,"zero step source loop");
    for(size_t i=0;i<4;++i){context.tiles_80098a28[0].known[i]=1;context.draw_modes_80098a10[0].known[i]=1;}
    count_allocations=true;
    for(size_t i=0;i<4096;++i){
        require(l3::advance_source_fade_callback(context,state,std::nullopt).yield_one_scheduler_pass,"zero step remains source loop");
        require(l3::describe_source_fade_presentation(context).visible,"hot description");
        require(l3::bind_source_fade_presentation_packets(context,{uint8_t(0),uint32_t(0)}).committed,"hot packets");
    }
    count_allocations=false;require(allocations==0,"fade callback hot-loop allocation");
}
}
int main(int argc,char** argv){require(argc==3,"usage: test_source_fade_callback callback.tsv presentation.tsv");
    callbacks(argv[1]);packets(argv[2]);guards_and_lifecycle();presentation_guards();allocation_free();
    std::cout<<"source fade callback:1632 unchanged-instruction cases,192 exact packet cases, atomic guards/lifecycle and4096 allocation-free updates passed\n";}
