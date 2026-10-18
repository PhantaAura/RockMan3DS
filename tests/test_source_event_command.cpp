#include "legends3ds/source_event_command.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace l3;
namespace {size_t allocations=0;}
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
namespace {
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void put32(uint8_t* p,uint32_t v){for(size_t i=0;i<4;++i)p[i]=static_cast<uint8_t>(v>>(i*8));}
uint8_t pattern(size_t i,unsigned seed){return static_cast<uint8_t>(i*37+seed*11);}
std::string hex(const uint8_t* p,size_t n){const char* digits="0123456789abcdef";std::string out;for(size_t i=0;i<n;++i){out+=digits[p[i]>>4];out+=digits[p[i]&15];}return out;}
template<size_t N>std::array<uint8_t,N> bytes(const std::string& h){require(h.size()==N*2,"malformed source hex fixture");std::array<uint8_t,N> out{};for(size_t i=0;i<N;++i)out[i]=static_cast<uint8_t>(std::stoul(h.substr(i*2,2),nullptr,16));return out;}
SourceEventCommandQueue queue(unsigned seed=0,uint32_t phase=0,uint32_t counter=0,size_t count=1){
    SourceEventCommandQueue q;q.initialized=true;q.count=count;q.current_index=0;
    q.phase_800989c4=phase;q.counter_80098828=counter;q.byte_800988d0=73;q.byte_80098964=1;q.word_800989c8=0x31415926;
    for(size_t i=0;i<256;++i){q.records[i/16].raw[i%16]=pattern(i,seed);q.records[i/16].known[i%16]=1;}
    for(size_t i=0;i<16;++i)put32(q.records[i].raw.data(),i==0?4:i<count?2:0);
    return q;
}
SourceEventCommandContext context(unsigned seed=0,uint8_t status=0){
    SourceEventCommandContext c;c.byte_8009896c=static_cast<uint8_t>(seed);c.byte_80098ab8=status;c.byte_80098b42=2;
    c.half_800ad142=static_cast<uint16_t>(0x9235^seed);c.half_800ad14e=54321;c.byte_800988ec=200;
    c.byte_800988c0=71;c.word_800989b0=0x81234567;c.callback_80091ffc=0;c.callback_80092000=0;
    return c;
}
std::array<uint32_t,12> scalars(const SourceEventCommandQueue& q,const SourceEventCommandContext& c){
    return {{q.phase_800989c4.value(),q.counter_80098828.value(),c.byte_8009896c.value(),q.byte_800988d0.value(),q.byte_80098964.value(),c.byte_80098ab8.value(),c.byte_80098b42.value(),c.half_800ad142.value(),c.half_800ad14e.value(),c.byte_800988ec.value(),c.byte_800988c0.value(),c.word_800989b0.value()}};
}
SourceEventCdReceipt receipt(uint8_t command,std::array<uint8_t,4> parameters={},uint8_t n=0,uint8_t status=0,uint32_t wrapper=0x8001d2bc){
    SourceEventCdReceipt r;r.request={wrapper,command,n,parameters};r.poll_result=r.control_result=1;r.response.fill(19);r.response[0]=status;r.response_known.fill(1);return r;
}
void differential(const char* path){
    std::ifstream file(path);require(bool(file),"missing source command fixture");std::string line;size_t dispatch=0,encode=0,decode=0,callback=0,boundary=0,tail=0;
    while(std::getline(file,line)){
        if(line.empty()||line[0]=='#')continue;std::istringstream row(line);char kind;row>>kind;
        if(kind=='D'){
            unsigned seed,phase,status;uint32_t counter;size_t count,mixcount;std::array<uint32_t,12> expected{};std::string queuehex,mixhex,extra;
            require(bool(row>>seed>>phase>>counter>>status>>count),"bad dispatch inputs");for(auto& value:expected)require(bool(row>>value),"bad dispatch expected globals");
            require(bool(row>>queuehex>>mixcount>>mixhex)&&!(row>>extra),"bad dispatch tail");
            auto q=queue(seed,phase,counter,count);auto c=context(seed,static_cast<uint8_t>(status));SourceEventCdMixQueue mix;mix.initialized=true;
            const auto r=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);
            require(r.committed&&scalars(q,c)==expected,"unchanged source type4 scalar differential mismatch");
            std::array<uint8_t,256> raw{};for(size_t i=0;i<256;++i)raw[i]=q.records[i/16].raw[i%16];
            require(raw==bytes<256>(queuehex),"unchanged source complete queue compaction mismatch");
            if(mix.count!=mixcount||r.mix_receipts!=mixcount){std::cerr<<"dispatch seed "<<seed<<" native mix "<<mix.count<<" source "<<mixcount<<'\n';require(false,"unchanged source CDmix call count mismatch");}
            std::ostringstream mixtext;for(size_t i=0;i<mix.count;++i){if(i)mixtext<<',';mixtext<<hex(mix.records[i].values.data(),4);}
            require((mix.count?mixtext.str():"-")==mixhex,"original CDmix scalar packets mismatch");
            require(q.count==(mixcount?count-1:count)&&q.current_index==0,"source queue pointer/count mismatch");++dispatch;
        }else if(kind=='E'){
            int64_t sector;std::string expected;require(bool(row>>sector>>expected),"bad sector encode fixture");
            require(source_event_sector_to_bcd_80077e88(static_cast<int32_t>(sector))==bytes<3>(expected),"original signed sector/BCD encode differs");++encode;
        }else if(kind=='L'){
            std::string bcd;uint32_t expected;require(bool(row>>bcd>>expected),"bad BCD decode fixture");
            require(uint32_t(source_event_bcd_to_sector_80077f8c(bytes<3>(bcd)))==expected,"original arbitrary BCD nibble decode differs");++decode;
        }else if(kind=='C'){
            unsigned code,ready,state,error;uint32_t pointer;require(bool(row>>code>>ready>>state>>error>>pointer),"bad sync callback fixture");
            auto q=queue();q.byte_80098964=0;auto c=context(code);c.callback_80091ffc=0x8001cc08;
            const auto r=source_event_cd_sync_callback_8001cc08(q,c,static_cast<uint8_t>(code));
            require(r.committed&&q.byte_80098964==ready&&c.byte_800988c0==state&&c.byte_8009896c==error&&c.callback_80091ffc==pointer,"original sync callback raw fields differ");++callback;
        }else if(kind=='B'){
            unsigned command;std::string params;uint32_t end;require(bool(row>>command>>params>>end),"bad source CD boundary");
            auto q=queue(11,2);auto c=context(11);c.byte_80098b42=0;
            put32(q.records[0].raw.data()+4,7);put32(q.records[0].raw.data()+8,0xb3123456);put32(q.records[0].raw.data()+12,0xe2654321);
            const SourceEventCommandProfile profile{7,51789,true};SourceEventCommandInputs in;
            in.receipts[0]=receipt(13,{1,19,0,0},2);const auto bcd=source_event_sector_to_bcd_80077e88(51789+0x123456);
            in.receipts[1]=receipt(2,{bcd[0],bcd[1],bcd[2],0},3);in.receipts[2]=receipt(27);
            in.receipt_count=command==13?0:command==2?1:2;
            const auto r=consume_source_event_command4_8001b89c(q,c,profile,in);
            require(!r.committed&&r.status==SourceEventCommandStatus::NeedsTransportReceipt&&r.pending_request&&r.pending_request->command==command,"source CD boundary request not exposed");
            require((r.pending_request->parameter_bytes?hex(r.pending_request->parameters.data(),r.pending_request->parameter_bytes):"-")==params,"source CD boundary parameter bytes differ");
            require(end==51789+0x654321&&c.word_800989b0==0x81234567,"missing receipt published source end-sector early");++boundary;
        }else if(kind=='T'){
            unsigned status,phase,write,mixcount;std::string queuehex;require(bool(row>>status>>phase>>write>>mixcount>>queuehex),"bad authored response tail");
            auto q=queue(11,2);auto c=context(11);c.byte_80098b42=0;
            put32(q.records[0].raw.data()+4,7);put32(q.records[0].raw.data()+8,0xb3123456);put32(q.records[0].raw.data()+12,0xe2654321);
            SourceEventCommandInputs in;in.receipt_count=3;in.receipts[0]=receipt(13,{1,19,0,0},2);const auto bcd=source_event_sector_to_bcd_80077e88(51789+0x123456);
            in.receipts[1]=receipt(2,{bcd[0],bcd[1],bcd[2],0},3);in.receipts[2]=receipt(27,{},0,static_cast<uint8_t>(status));
            SourceEventCdMixQueue mix;mix.initialized=true;const auto r=consume_source_event_command4_8001b89c(q,c,{7,51789,true},in,&mix);
            std::array<uint8_t,256> raw{};for(size_t i=0;i<256;++i)raw[i]=q.records[i/16].raw[i%16];
            require(r.committed&&r.popped&&r.status_copied_from_transport&&c.byte_80098ab8==status&&q.phase_800989c4==phase&&q.count==write/16&&mix.count==mixcount&&raw==bytes<256>(queuehex),"explicit transport response/source caller tail differs");++tail;
        }else require(false,"unknown source fixture kind");
    }
    require(dispatch==335&&encode==512&&decode==512&&callback==256&&boundary==3&&tail==1,"source command fixture coverage changed");
    std::cout<<dispatch<<" full source dispatch/"<<encode+decode<<" BCD/"<<callback<<" callbacks/"<<boundary<<" transport boundary/"<<tail<<" explicit response tail PASS; ";
}
void guards(){
    auto q=queue();auto c=context();c.byte_800988ec=0;const auto before=scalars(q,c);
    const auto held=consume_source_event_command4_8001b89c(q,c,{});
    require(!held.committed&&held.status==SourceEventCommandStatus::NeedsTransportReceipt&&held.pending_request&&held.pending_request->command==14&&scalars(q,c)==before,"cold source mode request fabricated CD acknowledgement");
    SourceEventCommandInputs in;in.receipt_count=1;in.receipts[0]=receipt(14,{200,0,0,0},1);
    auto bad=in;bad.receipts[0].poll_result=0;require(consume_source_event_command4_8001b89c(q,c,{},bad).status==SourceEventCommandStatus::TransportNotReady&&scalars(q,c)==before,"zero poll result acknowledged");
    bad=in;bad.receipts[0].control_result=0;require(consume_source_event_command4_8001b89c(q,c,{},bad).status==SourceEventCommandStatus::TransportNotReady&&scalars(q,c)==before,"zero control result acknowledged");
    bad=in;bad.receipts[0].request.command=13;require(consume_source_event_command4_8001b89c(q,c,{},bad).status==SourceEventCommandStatus::ReceiptMismatch&&scalars(q,c)==before,"mismatched receipt consumed");
    bad=in;bad.receipts[0].response_known[7]=0;require(consume_source_event_command4_8001b89c(q,c,{},bad).status==SourceEventCommandStatus::NeedsResponse&&scalars(q,c)==before,"unknown response bytes promoted");
    auto r=consume_source_event_command4_8001b89c(q,c,{},in);require(r.committed&&q.phase_800989c4==2&&q.counter_80098828==1&&c.byte_800988ec==200&&!r.status_copied_from_transport&&c.byte_80098ab8==0,"original mode-change/countdown fallthrough differs");
    r=consume_source_event_command4_8001b89c(q,c,{});require(r.committed&&q.phase_800989c4==2&&q.counter_80098828==0&&c.byte_80098ab8==0,"mode delay does not follow source decrement-return");
    c.byte_80098b42=0;r=consume_source_event_command4_8001b89c(q,c,{});require(!r.committed&&r.status==SourceEventCommandStatus::NeedsProfile&&q.phase_800989c4==2,"seek missing profile bypassed");
    q=queue(0,3);c=context(0,32);SourceEventCdMixQueue mix;mix.initialized=true;mix.count=130;
    const auto initial_q=q;const auto initial_c=scalars(q,c);r=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);
    require(!r.committed&&r.status==SourceEventCommandStatus::MixQueueFull&&q.count==initial_q.count&&q.records[0].raw==initial_q.records[0].raw&&scalars(q,c)==initial_c&&mix.count==130,"mix overflow partially popped source command");
    mix.count=0;q.records[1].known[13]=0;r=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);require(!r.committed&&r.status==SourceEventCommandStatus::NeedsCommandField&&mix.count==0,"queue sentinel retained tail fabricated");
    q=queue(0,3);c=context(0,32);q.records[1].raw[0]=1;r=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);require(!r.committed&&r.status==SourceEventCommandStatus::MissingQueueSentinel,"queue inconsistent sentinel admitted");
    q=queue();c=context();q.byte_80098964=0;c.byte_800988ec=128;
    in={};in.receipt_count=5;in.receipts[0]=receipt(9,{},0,1,0x8001d254);in.receipts[0].synchronous_callback_code=2;in.receipts[1]=receipt(14,{200,0,0,0},1);
    c.byte_80098b42=0;put32(q.records[0].raw.data()+4,7);put32(q.records[0].raw.data()+8,0xb3123456);put32(q.records[0].raw.data()+12,0xe2654321);
    in.receipts[2]=receipt(13,{1,19,0,0},2);const auto bcd=source_event_sector_to_bcd_80077e88(51789+0x123456);in.receipts[3]=receipt(2,{bcd[0],bcd[1],bcd[2],0},3);in.receipts[4]=receipt(27,{},0,33);
    r=consume_source_event_command4_8001b89c(q,c,{7,51789,true},in,&mix);
    require(r.committed&&r.popped&&r.mix_receipts==254&&r.transport_receipts_consumed==5&&mix.count==254&&c.byte_80098ab8==33,"bounded complete authored transport owner transaction failed");
    q=queue(0,3);c=context(0,0);const auto count_allocations=allocations;
    for(size_t i=0;i<4096;++i){r=consume_source_event_command4_8001b89c(q,c,{});require(r.committed&&!r.popped&&c.byte_80098ab8==0&&q.count==1,"waiting invented status or progression");}
    require(allocations==count_allocations,"per-frame event command allocation");
    q=queue();c=context();c.byte_8009896c.reset();require(!source_event_cd_sync_callback_8001cc08(q,c,1).committed,"callback error invented retained mask");
    require(source_event_cd_sync_callback_8001cc08(q,c,2).committed&&q.byte_80098964==1&&!c.byte_8009896c,"success callback modified unknown error field");
    q=queue(0,3);q.byte_800988d0=2;c=context(0,0);c.word_800989b0=1000;c.word_800989d8=0;
    const auto original=scalars(q,c);r=consume_source_event_command_dispatch_8001c95c(q,c,{});
    require(!r.committed&&r.pending_request&&r.pending_request->command==1&&scalars(q,c)==original&&c.word_800989d8==0,"outer dispatch omitted real status owner");
    in={};in.receipt_count=1;in.receipts[0]=receipt(1,{},0,32);
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in);
    require(!r.committed&&r.pending_request&&r.pending_request->command==17&&scalars(q,c)==original,"status receipt bypassed required original location poll");
    in.receipt_count=2;in.receipts[1]=receipt(17);const auto location=source_event_sector_to_bcd_80077e88(75);
    std::copy(location.begin(),location.end(),in.receipts[1].response.begin()+5);mix.count=0;
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
    require(r.committed&&r.popped&&r.transport_receipts_consumed==2&&r.status_copied_from_transport&&c.byte_80098ab8==32&&c.word_800989d8==75&&mix.count==127,"actual status/location owner not connected to source dispatch");
    q=queue(0,3);q.byte_800988d0=2;c=context(0,0);c.word_800989b0=75;q.word_800989bc=0;q.request_800989b8=0x20000003;
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
    require(!r.committed&&r.status==SourceEventCommandStatus::NeedsEndOfStreamConsumer&&r.missing_source_address==0x8001cb7c&&c.byte_80098ab8==0&&q.count==1,"reached-end pause invented");
    q.word_800989bc=0x40000000;r=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
    require(!r.committed&&r.status==SourceEventCommandStatus::NeedsLoopProducer&&r.pending_source_request==0x20000003,"reached-end original loop producer discarded");
    c.byte_8009896c=1;r=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
    require(!r.committed&&r.status==SourceEventCommandStatus::NeedsResetConsumer&&r.missing_source_address==0x800778c0,"CD reset recovery invented");
    c.byte_8009896c=0;const auto outer_allocations=allocations;
    for(size_t i=0;i<4096;++i){r=consume_source_event_command_dispatch_8001c95c(q,c,{});require(!r.committed&&r.pending_request&&r.pending_request->command==1&&c.byte_80098ab8==0,"outer missing transport inferred status");}
    require(allocations==outer_allocations,"outer per-frame event command allocation");
    q=queue();q.count=0;put32(q.records[0].raw.data(),0);q.byte_800988d0=2;c=context(0,0);
    in={};in.receipt_count=1;in.receipts[0]=receipt(1,{},0,2);
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in);
    require(r.committed&&r.status==SourceEventCommandStatus::Empty&&r.transport_receipts_consumed==1&&r.status_copied_from_transport&&c.byte_80098ab8==2&&q.count==0,"empty source no-op discarded actual outer drive-status receipt");
    // Native GetlocP has only admitted absolute-location fields. Preserve
    // UNKNOWN subchannel prefix instead of inventing track/index values.
    c.word_800989b0=1000;in.receipt_count=2;in.receipts[0]=receipt(1,{},0,34);
    in.receipts[0].response_known.fill(0);in.receipts[0].response_known[0]=1;
    in.receipts[1]=receipt(17);in.receipts[1].response_known.fill(0);
    const auto partial_location=source_event_sector_to_bcd_80077e88(120);
    for(size_t i=0;i<3;++i){in.receipts[1].response[5+i]=partial_location[i];in.receipts[1].response_known[5+i]=1;}
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in);
    require(r.committed&&c.word_800989d8==120&&!c.response_known[0]&&c.response_known[7],"unused GetlocP bytes invented or blocked used fields");
    in.receipts[1].response_known[6]=0;const auto retained=scalars(q,c);
    r=consume_source_event_command_dispatch_8001c95c(q,c,{},in);
    require(!r.committed&&r.status==SourceEventCommandStatus::NeedsResponse&&scalars(q,c)==retained,"missing used GetlocP byte admitted");
    c.byte_80098ab8=2;
    q.byte_800988d0=0;r=consume_source_event_command_dispatch_8001c95c(q,c,{});
    require(r.committed&&r.status==SourceEventCommandStatus::Empty&&c.byte_80098ab8==2,"cold source empty callback invented or lost retained status");
    std::cout<<"explicit transport/atomic queue+mix/validity/4096 no-allocation/status-not-inferred guards PASS\n";
}
void outer_differential(const std::string& path){
    std::ifstream file(path);require(bool(file),"missing source outer command fixture");std::string line;size_t cases=0;
    while(std::getline(file,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);unsigned seed,status;size_t count,mixcount;std::string bcdhex,queuehex,mixhex,extra;std::array<uint32_t,12> expected{};uint32_t sector;
        require(bool(row>>seed>>status>>count>>bcdhex),"bad outer source inputs");
        for(auto& value:expected)require(bool(row>>value),"bad outer source expected globals");
        require(bool(row>>sector>>queuehex>>mixcount>>mixhex)&&!(row>>extra),"bad outer source result");
        auto q=queue(seed,3,7,count);q.byte_800988d0=2;auto c=context(seed,0);c.byte_8009896c=0;c.word_800989b0=1000000;c.word_800989d8=0;
        SourceEventCommandInputs in;in.receipt_count=1;in.receipts[0]=receipt(1,{},0,static_cast<uint8_t>(status));
        if(status&32){in.receipt_count=2;in.receipts[1]=receipt(17);const auto bcd=bytes<3>(bcdhex);std::copy(bcd.begin(),bcd.end(),in.receipts[1].response.begin()+5);}
        SourceEventCdMixQueue mix;mix.initialized=true;const auto r=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
        require(r.committed&&scalars(q,c)==expected&&c.word_800989d8==sector,"original outer status/location/type4 ordering differs");
        std::array<uint8_t,256> raw{};for(size_t i=0;i<256;++i)raw[i]=q.records[i/16].raw[i%16];
        require(raw==bytes<256>(queuehex)&&mix.count==mixcount&&r.transport_receipts_consumed==in.receipt_count,"original outer queue/mix/receipt counts differ");
        std::ostringstream text;for(size_t i=0;i<mix.count;++i){if(i)text<<',';text<<hex(mix.records[i].values.data(),4);}
        require((mix.count?text.str():"-")==mixhex,"original outer mix packets differ");++cases;
    }
    require(cases==32,"outer source differential coverage changed");std::cout<<cases<<" unchanged outer caller cases with explicit transport boundaries PASS\n";
}
void command2_prefix(){
    const auto type2=[](uint32_t phase,uint8_t ready,size_t count,size_t current){
        auto q=queue(17,phase,29,count);q.current_index=current;q.byte_80098964=ready;q.byte_800988d0=0;
        for(size_t i=0;i<count;++i)put32(q.records[i].raw.data(),2);
        put32(q.records[current].raw.data()+8,1);return q;
    };
    const auto raw=[](const SourceEventCommandQueue& q){std::array<uint8_t,256> bytes{};for(size_t i=0;i<256;++i)bytes[i]=q.records[i/16].raw[i%16];return bytes;};
    const auto popped=[](SourceEventCommandQueue q){for(size_t i=1;i<=q.count;++i)q.records[i-1]=q.records[i];put32(q.records[q.count].raw.data(),0);--q.count;q.phase_800989c4=0;q.word_800989c8=0;return q;};
    size_t tails=0,full=0,guards=0;
    // These are the same bounded premises as the unchanged-instruction oracle:
    // current_index2 still compacts the queue from its base.
    for(const auto phase:{1u,2u,3u,0xffffffffu})for(const auto ready:{0u,1u,255u})
    for(const size_t count:{size_t(1),size_t(7),size_t(15)})for(const size_t current:{size_t(0),size_t(2)}){
        if(current>=count)continue;auto q=type2(phase,uint8_t(ready),count,current);auto c=context(17,83);
        const auto before=q;const auto initial=scalars(q,c);const auto allocation_count=allocations;
        const auto result=consume_source_event_command2_8001b858(q,c);
        require(result.committed&&allocations==allocation_count&&!result.transport_receipts_consumed&&!result.pending_mix_program,"type2 source tail fabricated transport/mix");
        if(phase==1&&ready){require(result.popped&&raw(q)==raw(popped(before))&&q.count==count-1&&q.phase_800989c4==0&&q.word_800989c8==0&&c.byte_80098b42==1,"type2 source readiness/pop tail differs");}
        else require(!result.popped&&raw(q)==raw(before)&&scalars(q,c)==initial,"type2 wait/unknown-phase tail progressed");
        ++tails;
    }
    for(const auto stereo:{0u,1u})for(const auto ready:{0u,1u,255u})for(const auto callback:{0u,2u,3u})
    for(const size_t count:{size_t(1),size_t(7)}){
        const size_t current=count==1?0:2;auto q=type2(0,uint8_t(ready),count,current);auto c=context(17,83);c.half_800ad142=uint16_t(0x1234|stereo);c.byte_8009896c=0;
        const auto initial_q=q;const auto initial_c=c;const auto initial=scalars(q,c);
        SourceEventCdMixQueue mix;mix.initialized=true;const auto allocation_count=allocations;
        auto result=consume_source_event_command_dispatch_8001c95c(q,c,{}, {},&mix);
        require(!result.committed&&result.status==SourceEventCommandStatus::NeedsTransportReceipt&&result.pending_request&&result.pending_request->source_function==0x8001d254&&result.pending_request->command==9&&result.pending_mix_program&&result.pending_mix_program->count==254&&raw(q)==raw(initial_q)&&scalars(q,c)==initial&&mix.count==0&&allocations==allocation_count,"type2 pause bypassed ordered mixer/transport owners");
        const auto program=*result.pending_mix_program;const auto digest=source_event_cd_mix_program_digest(program);
        for(size_t i=0;i<program.count;++i){const auto volume=uint8_t(127-i%127);require(program.records[i].values==std::array<uint8_t,4>{{volume,uint8_t(stereo?volume:0),volume,uint8_t(stereo?volume:0)}},"type2 original double-ramp program differs");}
        SourceEventCommandInputs in;in.receipt_count=1;in.receipts[0]=receipt(9,{},0,19,0x8001d254);
        in.receipts[0].applied_mix_prefix_count=254;in.receipts[0].applied_mix_prefix_digest=digest;
        if(callback)in.receipts[0].synchronous_callback_code=uint8_t(callback);
        for(unsigned bad=0;bad<7;++bad){
            auto supplied=in;
            switch(bad){case 0:supplied.receipts[0].applied_mix_prefix_count=0;break;case 1:supplied.receipts[0].applied_mix_prefix_count=127;break;case 2:supplied.receipts[0].applied_mix_prefix_digest^=1;break;case 3:supplied.receipts[0].poll_result=0;break;case 4:supplied.receipts[0].control_result=0;break;case 5:supplied.receipts[0].request.source_function=0x8001d2bc;break;default:supplied.receipts[0].request.parameters[0]=1;break;}
            const auto held=consume_source_event_command_dispatch_8001c95c(q,c,{},supplied,&mix);
            require(!held.committed&&raw(q)==raw(initial_q)&&scalars(q,c)==initial&&mix.count==0,"type2 stale/different prefix or unowned pause committed");++guards;
        }
        // The receipt represents actual prefix application followed by pause;
        // no copy of those254 calls may be queued again after the pause.
        if(ready||callback==2){
            auto malformed=q;malformed.records[count].known[13]=0;auto held_context=c;
            const auto held=consume_source_event_command_dispatch_8001c95c(malformed,held_context,{},in,&mix);
            require(!held.committed&&held.status==SourceEventCommandStatus::NeedsCommandField&&!held.transport_receipts_consumed&&!held.mix_receipts&&mix.count==0&&scalars(malformed,held_context)==initial,"atomic source tail consumed receipt or released applied prefix");
        }
        const auto count_allocations=allocations;
        result=consume_source_event_command_dispatch_8001c95c(q,c,{},in,&mix);
        const bool should_pop=ready||callback==2;
        require(result.committed&&result.popped==should_pop&&result.transport_receipts_consumed==1&&result.mix_receipts==254&&mix.count==0&&!result.pending_mix_program&&allocations==count_allocations,"type2 applied prefix republished or pause completion lost");
        require(q.byte_800988d0==1&&c.byte_80098ab8==0&&c.half_800ad142==uint16_t(0x9234|stereo)&&c.mix_800ad154==program.records.back().values,"type2 original pause scalar tail differs");
        require(q.byte_80098964==(callback==2?1:ready)&&c.byte_8009896c==(callback==3?8:0)&&c.byte_800988c0==(callback==2?0:callback==3?128:*initial_c.byte_800988c0)&&c.callback_80092000==0&&c.callback_80091ffc==(callback?0:0x8001cc08),"type2 actual sync callback scalar effect differs");
        if(should_pop)require(raw(q)==raw(popped(initial_q))&&q.phase_800989c4==0&&q.word_800989c8==0&&c.byte_80098b42==1,"type2 single source queue pop differs");
        else require(raw(q)==raw(initial_q)&&q.phase_800989c4==1&&c.byte_80098b42==initial_c.byte_80098b42,"type2 pause invented callback readiness");
        if(count==1&&should_pop){const auto after=raw(q);result=consume_source_event_command_dispatch_8001c95c(q,c,{}, {},&mix);require((callback==3?(!result.committed&&result.status==SourceEventCommandStatus::NeedsResetConsumer):(result.committed&&result.status==SourceEventCommandStatus::Empty))&&!result.popped&&raw(q)==after&&mix.count==0,"type2 completion popped twice or replayed ramp");}
        ++full;
    }
    // Existing type4 pause prefixes use the same ordering receipt and publish
    // only the127 later fade-up calls, never the already-applied fade-down.
    auto q=queue();q.byte_80098964=0;auto c=context();SourceEventCdMixQueue mix;mix.initialized=true;
    auto result=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);
    require(!result.committed&&result.pending_mix_program&&result.pending_mix_program->count==127,"type4 pause prefix unavailable to real owner");
    SourceEventCommandInputs in;in.receipt_count=1;in.receipts[0]=receipt(9,{},0,19,0x8001d254);in.receipts[0].synchronous_callback_code=2;
    in.receipts[0].applied_mix_prefix_count=127;in.receipts[0].applied_mix_prefix_digest=source_event_cd_mix_program_digest(*result.pending_mix_program);
    result=consume_source_event_command4_8001b89c(q,c,{},in,&mix);
    require(result.committed&&!result.popped&&result.mix_receipts==127&&mix.count==0&&q.phase_800989c4==3,"type4 acknowledged prefix replayed after pause");
    // Explicit independently supplied source drive status, as in the oracle's
    // phase3 cases; the pause receipt never implies this later reading state.
    c.byte_80098ab8=32;result=consume_source_event_command4_8001b89c(q,c,{}, {},&mix);
    require(result.committed&&result.popped&&result.mix_receipts==127&&mix.count==127&&mix.records[0].values[0]==0&&mix.records[126].values[0]==126,"type4 later fade-up source sequence differs");
    require(tails==60&&full==36&&guards==252,"type2 proof coverage changed");
    std::cout<<tails<<" type2 source tails/"<<full<<" ordered prefix-pause transactions/"<<guards<<" stale-prefix+transport guards; no duplicate mix/pop; type4 prefix ordering PASS\n";
}
}
int main(int argc,char** argv){try{require(argc==2,"source command fixture argument required");differential(argv[1]);guards();const std::string path=argv[1];outer_differential(path.substr(0,path.find_last_of("/\\")+1)+"source_event_command_outer_01424.tsv");command2_prefix();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
