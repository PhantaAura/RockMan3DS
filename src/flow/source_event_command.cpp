#include "legends3ds/source_event_command.h"

namespace l3 {
namespace {
uint32_t u32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
bool known(const uint8_t* p,size_t n) noexcept {for(size_t i=0;i<n;++i)if(!p[i])return false;return true;}
int32_t signed32(uint32_t v) noexcept {
    return v<=0x7fffffff?static_cast<int32_t>(v):static_cast<int32_t>(int64_t(v)-0x100000000LL);
}
int32_t wrap_add(int32_t a,int32_t b) noexcept {return signed32(uint32_t(a)+uint32_t(b));}
int32_t high_mul(int32_t a,uint32_t b) noexcept {
    const auto product=int64_t(a)*signed32(b);
    return signed32(static_cast<uint32_t>(static_cast<uint64_t>(product)>>32));
}
int32_t shift(int32_t x,unsigned bits) noexcept {
    if(x>=0)return x>>bits;
    return static_cast<int32_t>(-((-(int64_t(x))+(int64_t(1)<<bits)-1)>>bits));
}
void write_type(SourceEventCommandRecord& record,uint32_t value) noexcept {
    for(size_t i=0;i<4;++i){record.raw[i]=static_cast<uint8_t>(value>>(8*i));record.known[i]=1;}
}
bool same_request(const SourceEventCdRequest& a,const SourceEventCdRequest& b) noexcept {
    return a.source_function==b.source_function&&a.command==b.command&&a.parameter_bytes==b.parameter_bytes&&a.parameters==b.parameters;
}
}
uint64_t source_event_cd_mix_program_digest(const SourceEventCdMixProgram& program) noexcept {
    if(program.count>program.records.size())return 0;
    uint64_t digest=14695981039346656037ull;
    const auto byte=[&](uint8_t value) noexcept {digest=(digest^value)*1099511628211ull;};
    for(size_t i=0;i<8;++i)byte(static_cast<uint8_t>(uint64_t(program.count)>>(i*8)));
    for(size_t i=0;i<program.count;++i)for(const auto value:program.records[i].values)byte(value);
    return digest;
}
std::array<uint8_t,3> source_event_sector_to_bcd_80077e88(int32_t sector) noexcept {
    const auto raw=wrap_add(sector,150);
    const auto seconds=wrap_add(shift(high_mul(raw,0x1b4e81b5),3),raw<0);
    const auto minutes=wrap_add(shift(wrap_add(high_mul(seconds,0x88888889),seconds),5),seconds<0);
    const auto frames=wrap_add(raw,-signed32(uint32_t(seconds)*75));
    const auto remainder=wrap_add(seconds,-signed32(uint32_t(minutes)*60));
    const auto digit=[](int32_t v) noexcept {
        const auto tens=wrap_add(shift(high_mul(v,0x66666667),2),v<0);
        return static_cast<uint8_t>(uint32_t(tens)*16+uint32_t(v)-uint32_t(tens)*10);
    };
    return {digit(minutes),digit(remainder),digit(frames)};
}
int32_t source_event_bcd_to_sector_80077f8c(const std::array<uint8_t,3>& bcd) noexcept {
    const auto decimal=[](uint8_t v) noexcept{return uint32_t(v>>4)*10+(v&15);};
    return static_cast<int32_t>((decimal(bcd[0])*60+decimal(bcd[1]))*75+decimal(bcd[2]))-150;
}
SourceEventCommandResult source_event_cd_sync_callback_8001cc08(
    SourceEventCommandQueue& commands,SourceEventCommandContext& context,uint8_t code) noexcept {
    SourceEventCommandResult out;
    if(code!=2&&!context.byte_8009896c){out.status=SourceEventCommandStatus::NeedsSourceField;out.missing_source_address=0x8009896c;return out;}
    context.callback_80091ffc=0;
    if(code==2){commands.byte_80098964=1;context.byte_800988c0=0;}
    else{context.byte_800988c0=128;context.byte_8009896c=static_cast<uint8_t>(*context.byte_8009896c|8);}
    out.status=SourceEventCommandStatus::Consumed;out.committed=true;return out;
}
SourceEventCommandResult consume_source_event_command4_8001b89c(
    SourceEventCommandQueue& commands,SourceEventCommandContext& context,
    const SourceEventCommandProfile& profile,const SourceEventCommandInputs& inputs,
    SourceEventCdMixQueue* mix_intents) noexcept {
    SourceEventCommandResult out;
    const auto field=[&](bool present,uint32_t address) noexcept {
        if(present)return true;
        out.status=SourceEventCommandStatus::NeedsSourceField;
        out.missing_source_address=address;return false;
    };
    if(!commands.initialized)return out;
    if(commands.count>=commands.records.size()||commands.current_index>=commands.records.size()||
       (commands.count&&commands.current_index>=commands.count)){
        out.status=SourceEventCommandStatus::MalformedCommandQueue;return out;
    }
    const auto& current=commands.records[commands.current_index];
    if(!known(current.known.data(),4)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
    if(!u32(current.raw.data())){out.status=SourceEventCommandStatus::Empty;return out;}
    if(u32(current.raw.data())!=4){out.status=SourceEventCommandStatus::UnsupportedCommand;return out;}
    if(inputs.receipt_count>inputs.receipts.size()){out.status=SourceEventCommandStatus::MalformedReceiptList;return out;}
    if(!field(commands.phase_800989c4.has_value(),0x800989c4))return out;
    // Bounded staging preserves all owners on missing transport/resource data.
    auto next_commands=commands;auto next=context;
    std::array<SourceEventCdMixIntent,254> staged_mix{};size_t mix_count=0,receipt_index=0,applied_mix_count=0;
    bool staged_popped=false,staged_status_copied=false;
    const auto mix=[&](uint8_t volume) noexcept {
        if(!field(next.half_800ad142.has_value(),0x800ad142))return false;
        const bool stereo=(*next.half_800ad142&1)!=0;
        next.mix_800ad154={volume,static_cast<uint8_t>(stereo?volume:0),volume,static_cast<uint8_t>(stereo?volume:0)};
        next.mix_known.fill(1);staged_mix[mix_count++].values=next.mix_800ad154;return true;
    };
    const auto transport=[&](uint8_t command,const std::array<uint8_t,4>& parameters,uint8_t bytes,uint32_t wrapper=0x8001d2bc) noexcept {
        SourceEventCdRequest request;request.command=command;request.parameters=parameters;request.parameter_bytes=bytes;request.source_function=wrapper;
        const auto prefix=[&]() noexcept {
            if(command==9&&wrapper==0x8001d254&&mix_count){
                SourceEventCdMixProgram program;program.count=mix_count;
                for(size_t i=0;i<mix_count;++i)program.records[i]=staged_mix[i];
                out.pending_mix_program=program;
            }
        };
        if(receipt_index>=inputs.receipt_count){out.status=SourceEventCommandStatus::NeedsTransportReceipt;out.pending_request=request;prefix();return false;}
        const auto& receipt=inputs.receipts[receipt_index];
        if(!same_request(request,receipt.request)){out.status=SourceEventCommandStatus::ReceiptMismatch;out.pending_request=request;return false;}
        if(!receipt.poll_result||!receipt.control_result){out.status=SourceEventCommandStatus::TransportNotReady;out.pending_request=request;return false;}
        if(receipt.applied_mix_prefix_count){
            prefix();
            if(!out.pending_mix_program||receipt.applied_mix_prefix_count!=mix_count||
               receipt.applied_mix_prefix_digest!=source_event_cd_mix_program_digest(*out.pending_mix_program)){
                out.status=SourceEventCommandStatus::MixPrefixMismatch;out.pending_request=request;return false;
            }
            applied_mix_count=mix_count;out.pending_mix_program.reset();
        }
        if(!known(receipt.response_known.data(),8)){out.status=SourceEventCommandStatus::NeedsResponse;out.pending_request=request;return false;}
        next.response_80098a98=receipt.response;next.response_known=receipt.response_known;
        if(receipt.synchronous_callback_code){
            if(!field(next.callback_80091ffc.has_value(),0x80091ffc))return false;
            if(*next.callback_80091ffc!=0&&*next.callback_80091ffc!=0x8001cc08){out.status=SourceEventCommandStatus::NeedsSourceField;out.missing_source_address=*next.callback_80091ffc;return false;}
            if(*next.callback_80091ffc==0x8001cc08){
                const auto result=source_event_cd_sync_callback_8001cc08(next_commands,next,*receipt.synchronous_callback_code);
                if(!result.committed){out.status=result.status;out.missing_source_address=result.missing_source_address;return false;}
            }
        }
        ++receipt_index;return true;
    };
    auto phase=*next_commands.phase_800989c4;
    if(phase==0){
        if(!field(next_commands.byte_80098964.has_value(),0x80098964))return out;
        if(!*next_commands.byte_80098964){
            //8001CB7C:127..1 CD mix, pause callback installation, actual pause.
            for(unsigned i=127;i>0;--i)if(!mix(static_cast<uint8_t>(i)))return out;
            next.byte_8009896c=0;next.half_800ad142=static_cast<uint16_t>(*next.half_800ad142|0x8000);
            next.callback_80092000=0;next.callback_80091ffc=0x8001cc08;
            if(!transport(9,{},0,0x8001d254))return out;
            next_commands.byte_800988d0=0;next.byte_80098ab8=0;
        }
        next_commands.byte_800988d0=2;next_commands.phase_800989c4=++phase;
    }
    if(phase==1){
        if(!field(next_commands.byte_80098964.has_value(),0x80098964))return out;
        if(!*next_commands.byte_80098964)goto finish;
        next.half_800ad14e=127;
        if(!field(next.byte_800988ec.has_value(),0x800988ec))return out;
        next_commands.counter_80098828=0;
        if(*next.byte_800988ec!=200){
            if((*next.byte_800988ec&128)!=(200&128))next_commands.counter_80098828=2;
            next.byte_800988ec=200;
            if(!transport(14,{200,0,0,0},1))return out;
        }
        next_commands.phase_800989c4=++phase;
    }
    if(phase==2){
        if(!field(next_commands.counter_80098828.has_value(),0x80098828))return out;
        if(*next_commands.counter_80098828){--*next_commands.counter_80098828;goto finish;}
        if(!field(next.byte_80098b42.has_value(),0x80098b42))return out;
        next.byte_800988c0=0;next.byte_8009896c=0;next_commands.byte_80098964=0;
        if(*next.byte_80098b42!=2){
            if(!known(current.known.data()+4,12)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
            if(!profile.initialized){out.status=SourceEventCommandStatus::NeedsProfile;return out;}
            if(profile.id!=u32(current.raw.data()+4)){out.status=SourceEventCommandStatus::ProfileIdentityMismatch;return out;}
            const auto start=profile.file_sector_80082cd0+(u32(current.raw.data()+8)&0xffffff);
            next.word_800989b0=profile.file_sector_80082cd0+(u32(current.raw.data()+12)&0xffffff);
            const auto bcd=source_event_sector_to_bcd_80077e88(signed32(start));
            const std::array<uint8_t,4> location={bcd[0],bcd[1],bcd[2],0};
            if(!transport(13,{1,static_cast<uint8_t>(current.raw[11]&31),0,0},2)||
               !transport(2,location,3)||!transport(27,{},0))return out;
            next_commands.counter_80098828=0;next.byte_80098ab8=next.response_80098a98[0];
            staged_status_copied=true;
        }
        next_commands.phase_800989c4=++phase;
    }
    if(phase==3){
        if(!field(next.byte_80098ab8.has_value(),0x80098ab8))return out;
        if(!(*next.byte_80098ab8&32)){
            if(!field(next_commands.counter_80098828.has_value(),0x80098828))return out;
            ++*next_commands.counter_80098828;
            if(*next_commands.counter_80098828>=150){
                if(!field(next.byte_8009896c.has_value(),0x8009896c))return out;
                next.byte_8009896c=static_cast<uint8_t>(*next.byte_8009896c|16);
            }
            out.status=SourceEventCommandStatus::WaitingForSourceStatus;goto finish;
        }
        for(unsigned i=0;i<127;++i)if(!mix(static_cast<uint8_t>(i)))return out;
        next.byte_80098b42=0;next.half_800ad142=static_cast<uint16_t>(*next.half_800ad142&0x7fff);
        // Original8001CAAC compacts from queue base, not current pointer. It
        // shifts all four words until a type0 sentinel, then clears type only.
        size_t sentinel=0;
        for(;sentinel<next_commands.records.size();++sentinel){
            if(!known(next_commands.records[sentinel].known.data(),4)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
            if(!u32(next_commands.records[sentinel].raw.data()))break;
        }
        if(sentinel==next_commands.records.size()||sentinel!=next_commands.count){out.status=SourceEventCommandStatus::MissingQueueSentinel;return out;}
        if(!next_commands.count){out.status=SourceEventCommandStatus::MalformedCommandQueue;return out;}
        for(size_t i=1;i<=sentinel;++i){
            if(!known(next_commands.records[i].known.data(),16)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
            next_commands.records[i-1]=next_commands.records[i];
        }
        write_type(next_commands.records[sentinel],0);
        --next_commands.count;next_commands.word_800989c8=0;next_commands.phase_800989c4=0;staged_popped=true;
    }
finish:
    if(mix_count>applied_mix_count){
        if(!mix_intents||!mix_intents->initialized){out.status=SourceEventCommandStatus::NeedsMixQueue;return out;}
        if(mix_intents->count>mix_intents->records.size()){out.status=SourceEventCommandStatus::MalformedMixQueue;return out;}
        if(mix_intents->records.size()-mix_intents->count<mix_count-applied_mix_count){out.status=SourceEventCommandStatus::MixQueueFull;return out;}
        for(size_t i=applied_mix_count;i<mix_count;++i)mix_intents->records[mix_intents->count++]=staged_mix[i];
    }
    commands=next_commands;context=next;out.committed=true;
    out.popped=staged_popped;out.status_copied_from_transport=staged_status_copied;
    out.transport_receipts_consumed=static_cast<uint8_t>(receipt_index);out.mix_receipts=static_cast<uint16_t>(mix_count);
    if(out.status!=SourceEventCommandStatus::WaitingForSourceStatus)out.status=SourceEventCommandStatus::Consumed;
    return out;
}
SourceEventCommandResult consume_source_event_command2_8001b858(
    SourceEventCommandQueue& commands,SourceEventCommandContext& context,
    const SourceEventCommandInputs& inputs) noexcept {
    SourceEventCommandResult out;
    const auto field=[&](bool present,uint32_t address) noexcept {
        if(present)return true;
        out.status=SourceEventCommandStatus::NeedsSourceField;out.missing_source_address=address;return false;
    };
    if(!commands.initialized)return out;
    if(commands.count>=commands.records.size()||commands.current_index>=commands.records.size()||
       (commands.count&&commands.current_index>=commands.count)){
        out.status=SourceEventCommandStatus::MalformedCommandQueue;return out;
    }
    const auto& current=commands.records[commands.current_index];
    if(!known(current.known.data(),4)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
    if(!u32(current.raw.data())){out.status=SourceEventCommandStatus::Empty;return out;}
    if(u32(current.raw.data())!=2){out.status=SourceEventCommandStatus::UnsupportedCommand;return out;}
    if(!known(current.known.data()+8,4)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
    const auto index=u32(current.raw.data()+8);
    if(index==0){out.status=SourceEventCommandStatus::Consumed;out.committed=true;return out;}
    if(index!=1){out.status=SourceEventCommandStatus::UnsupportedCommand;return out;}
    if(inputs.receipt_count>inputs.receipts.size()){out.status=SourceEventCommandStatus::MalformedReceiptList;return out;}
    if(!field(commands.phase_800989c4.has_value(),0x800989c4))return out;
    auto next_commands=commands;auto next=context;
    auto phase=*commands.phase_800989c4;
    bool pause_consumed=false;
    if(phase==0){
        if(!field(next.half_800ad142.has_value(),0x800ad142))return out;
        SourceEventCdMixProgram program;program.count=254;
        const bool stereo=(*next.half_800ad142&1)!=0;
        for(size_t i=0;i<program.count;++i){const auto volume=static_cast<uint8_t>(127-i%127);
            program.records[i].values={volume,static_cast<uint8_t>(stereo?volume:0),volume,static_cast<uint8_t>(stereo?volume:0)};}
        SourceEventCdRequest request;request.source_function=0x8001d254;request.command=9;
        out.pending_request=request;out.pending_mix_program=program;
        if(!inputs.receipt_count){out.status=SourceEventCommandStatus::NeedsTransportReceipt;return out;}
        const auto& receipt=inputs.receipts[0];
        if(!same_request(request,receipt.request)){out.status=SourceEventCommandStatus::ReceiptMismatch;return out;}
        if(!receipt.applied_mix_prefix_count){out.status=SourceEventCommandStatus::NeedsMixPrefixReceipt;return out;}
        if(receipt.applied_mix_prefix_count!=program.count||
           receipt.applied_mix_prefix_digest!=source_event_cd_mix_program_digest(program)){
            out.status=SourceEventCommandStatus::MixPrefixMismatch;return out;
        }
        if(!receipt.poll_result||!receipt.control_result){out.status=SourceEventCommandStatus::TransportNotReady;return out;}
        next.mix_800ad154=program.records.back().values;next.mix_known.fill(1);
        next.half_800ad142=static_cast<uint16_t>(*next.half_800ad142|0x8000);
        next.byte_8009896c=0;next.callback_80092000=0;next.callback_80091ffc=0x8001cc08;
        next.response_80098a98=receipt.response;next.response_known=receipt.response_known;
        if(receipt.synchronous_callback_code){
            const auto callback=source_event_cd_sync_callback_8001cc08(next_commands,next,*receipt.synchronous_callback_code);
            if(!callback.committed){out.status=callback.status;out.missing_source_address=callback.missing_source_address;return out;}
        }
        //8001CBE4 clears these after the explicit transport boundary, then
        //8001B6A4 selects mode1 and advances the retained command phase.
        next_commands.byte_800988d0=1;next.byte_80098ab8=0;
        next_commands.phase_800989c4=++phase;
        pause_consumed=true;
    }
    if(phase==1){
        if(!field(next_commands.byte_80098964.has_value(),0x80098964))return out;
        if(!*next_commands.byte_80098964){out.status=SourceEventCommandStatus::WaitingForSourceStatus;}
        else{
            next.byte_80098b42=1;
            //8001CAAC compacts from queue base even when current_index!=0.
            size_t sentinel=0;
            for(;sentinel<next_commands.records.size();++sentinel){
                if(!known(next_commands.records[sentinel].known.data(),4)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
                if(!u32(next_commands.records[sentinel].raw.data()))break;
            }
            if(sentinel==next_commands.records.size()||sentinel!=next_commands.count){out.status=SourceEventCommandStatus::MissingQueueSentinel;return out;}
            if(!next_commands.count){out.status=SourceEventCommandStatus::MalformedCommandQueue;return out;}
            for(size_t i=1;i<=sentinel;++i){
                if(!known(next_commands.records[i].known.data(),16)){out.status=SourceEventCommandStatus::NeedsCommandField;return out;}
                next_commands.records[i-1]=next_commands.records[i];
            }
            write_type(next_commands.records[sentinel],0);--next_commands.count;
            next_commands.word_800989c8=0;next_commands.phase_800989c4=0;out.popped=true;
        }
    }
    commands=next_commands;context=next;out.committed=true;
    if(pause_consumed){out.transport_receipts_consumed=1;out.mix_receipts=254;}
    out.pending_request.reset();out.pending_mix_program.reset();
    if(out.status!=SourceEventCommandStatus::WaitingForSourceStatus)out.status=SourceEventCommandStatus::Consumed;
    return out;
}
SourceEventCommandResult consume_source_event_command_dispatch_8001c95c(
    SourceEventCommandQueue& commands,SourceEventCommandContext& context,
    const SourceEventCommandProfile& profile,const SourceEventCommandInputs& inputs,
    SourceEventCdMixQueue* mix_intents) noexcept {
    SourceEventCommandResult out;
    if(!commands.initialized)return out;
    if(inputs.receipt_count>inputs.receipts.size()){out.status=SourceEventCommandStatus::MalformedReceiptList;return out;}
    const auto missing=[&](uint32_t address) noexcept {
        out.status=SourceEventCommandStatus::NeedsSourceField;out.missing_source_address=address;
    };
    if(!context.byte_8009896c){missing(0x8009896c);return out;}
    if(*context.byte_8009896c){out.status=SourceEventCommandStatus::NeedsResetConsumer;out.missing_source_address=0x800778c0;return out;}
    if(!commands.byte_800988d0){missing(0x800988d0);return out;}
    auto staged_commands=commands;auto staged_context=context;
    SourceEventCommandInputs remaining=inputs;size_t used=0;bool copied_status=false;
    const auto transport=[&](uint8_t command) noexcept {
        SourceEventCdRequest request;request.command=command;
        if(used>=inputs.receipt_count){out.status=SourceEventCommandStatus::NeedsTransportReceipt;out.pending_request=request;return false;}
        const auto& receipt=inputs.receipts[used];
        if(!same_request(receipt.request,request)){out.status=SourceEventCommandStatus::ReceiptMismatch;out.pending_request=request;return false;}
        if(!receipt.poll_result||!receipt.control_result){out.status=SourceEventCommandStatus::TransportNotReady;out.pending_request=request;return false;}
        // The outer source reads only status[0] forGetstat andlocation[5..7]
        // forGetlocP. Unused subchannel bytes remain explicitly UNKNOWN.
        const bool response_admitted=command==1?receipt.response_known[0]!=0:
            known(receipt.response_known.data()+5,3);
        if(!response_admitted){out.status=SourceEventCommandStatus::NeedsResponse;out.pending_request=request;return false;}
        staged_context.response_80098a98=receipt.response;staged_context.response_known=receipt.response_known;
        if(receipt.synchronous_callback_code){
            if(!staged_context.callback_80091ffc){missing(0x80091ffc);return false;}
            if(*staged_context.callback_80091ffc!=0&&*staged_context.callback_80091ffc!=0x8001cc08){missing(*staged_context.callback_80091ffc);return false;}
            if(*staged_context.callback_80091ffc==0x8001cc08){
                const auto result=source_event_cd_sync_callback_8001cc08(staged_commands,staged_context,*receipt.synchronous_callback_code);
                if(!result.committed){out.status=result.status;out.missing_source_address=result.missing_source_address;return false;}
            }
        }
        ++used;return true;
    };
    if(*commands.byte_800988d0==2){
        if(!transport(1))return out;
        staged_context.byte_80098ab8=staged_context.response_80098a98[0];copied_status=true;
        if(*staged_context.byte_80098ab8&32){
            if(!transport(17))return out;
            const std::array<uint8_t,3> location={staged_context.response_80098a98[5],staged_context.response_80098a98[6],staged_context.response_80098a98[7]};
            const auto sector=uint32_t(source_event_bcd_to_sector_80077f8c(location));
            staged_context.word_800989d8=sector;
            if(!staged_context.word_800989b0){missing(0x800989b0);return out;}
            if(sector>=*staged_context.word_800989b0){
                if(!staged_commands.word_800989bc){missing(0x800989bc);return out;}
                if(*staged_commands.word_800989bc&0x40000000){
                    if(!staged_commands.request_800989b8){missing(0x800989b8);return out;}
                    out.status=SourceEventCommandStatus::NeedsLoopProducer;out.missing_source_address=0x8001d58c;
                    out.pending_source_request=staged_commands.request_800989b8;
                }else{out.status=SourceEventCommandStatus::NeedsEndOfStreamConsumer;out.missing_source_address=0x8001cb7c;}
                return out;
            }
        }
    }
    remaining.receipt_count=inputs.receipt_count-used;
    for(size_t i=0;i<remaining.receipt_count;++i)remaining.receipts[i]=inputs.receipts[used+i];
    SourceEventCdMixQueue staged_mix;
    if(mix_intents)staged_mix=*mix_intents;
    const bool type2=staged_commands.current_index<staged_commands.records.size()&&
        known(staged_commands.records[staged_commands.current_index].known.data(),4)&&
        u32(staged_commands.records[staged_commands.current_index].raw.data())==2;
    auto result=type2?consume_source_event_command2_8001b858(staged_commands,staged_context,remaining):
        consume_source_event_command4_8001b89c(staged_commands,staged_context,profile,remaining,mix_intents?&staged_mix:nullptr);
    // Source table type0 selects8001B4C4, whose complete body is a no-op
    // return. Poll/status stores performed by the outer owner still commit.
    if(result.status==SourceEventCommandStatus::Empty)result.committed=true;
    if(!result.committed)return result;
    commands=staged_commands;context=staged_context;if(mix_intents)*mix_intents=staged_mix;
    result.transport_receipts_consumed=static_cast<uint8_t>(result.transport_receipts_consumed+used);
    result.status_copied_from_transport=result.status_copied_from_transport||copied_status;
    return result;
}
} // namespace l3
