#include "legends3ds/source_event_phase2.h"
#include "legends3ds/source_event_pipeline.h"

namespace l3 {
namespace {
uint16_t read16(const uint8_t* p) noexcept {return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t read32(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
template<size_t N> void write(std::array<uint8_t,N>& raw,std::array<uint8_t,N>& known,
    size_t offset,size_t bytes,uint32_t value) noexcept {
    for(size_t i=0;i<bytes;++i){raw[offset+i]=static_cast<uint8_t>(value>>(i*8));known[offset+i]=1;}
}
bool known(const uint8_t* p,size_t offset,size_t count) noexcept {
    for(size_t i=offset;i<offset+count;++i)if(!p[i])return false;
    return true;
}
void stop_voice(SourceEventDriverContext& driver,SourceEventDriverIntentQueue& intents,
    size_t index) noexcept {
    driver.voice_800c0bfa=static_cast<uint16_t>(index);
    auto& voice=driver.voices[index];
    write(voice.raw,voice.known,0,2,0);write(voice.raw,voice.known,4,2,0);
    write(voice.raw,voice.known,0x1d,1,0);
    const unsigned half=index<16?0:1,bit=static_cast<unsigned>(index)&15;
    driver.pending_800c5680[half]=static_cast<uint16_t>(*driver.pending_800c5680[half]|(1u<<bit));
    SourceEventDriverIntent record;record.source_function=0x800742c8;
    record.voice=static_cast<int16_t>(index);
    for(unsigned i=0;i<2;++i){
        driver.active_8009ae40[i]=static_cast<uint16_t>(*driver.active_8009ae40[i]&~*driver.pending_800c5680[i]);
        record.pending[i]=*driver.pending_800c5680[i];record.active[i]=*driver.active_8009ae40[i];
    }
    // Direct742C8 leaves v0 as the final active high-half value. Unlike the
    // phase1 wrapper71220, it has no explicit success return store.
    record.source_return=record.active[1];
    intents.records[intents.count++]=record;
}
}
void initialize_source_event_phase2_from_executable_boot(SourceEventPhase2Context& context,
    SourceEventCommandQueue& queue,const std::array<uint8_t,32>& descriptor_load,
    const std::array<uint8_t,256>& queue_load) noexcept {
    for(size_t i=0;i<descriptor_load.size();++i){
        context.descriptors[i/8].raw[i%8]=descriptor_load[i];context.descriptors[i/8].known[i%8]=1;
    }
    for(size_t i=0;i<queue_load.size();++i){
        queue.records[i/16].raw[i%16]=queue_load[i];queue.records[i/16].known[i%16]=1;
    }
}
void initialize_source_event_phase2_descriptors_80019f0c(SourceEventPhase2Context& context) noexcept {
    for(auto& slot:context.descriptors){slot.raw[2]=255;slot.known[2]=1;}
}
void initialize_source_event_command_queue_8001b44c(SourceEventCommandQueue& queue) noexcept {
    for(auto& record:queue.records)write(record.raw,record.known,0,4,0);
    queue.count=0;queue.current_index=0;queue.initialized=true;
    queue.counter_80098828=0;queue.word_80098968=0;queue.word_800989c8=0;queue.phase_800989c4=0;
    queue.byte_800988d0=0;queue.byte_80098964=1;
}
SourceEventPhase2Result consume_source_event1_phase2_8001b33c(SourceEventWork& work,
    SourceEventPhase2Context& context,const SourceEventPhase2Profile& profile,
    SourceEventCommandQueue& commands,SourceEventPhase2IntentQueue* sound_intents,
    SourceEventDriverContext* driver,SourceEventDriverIntentQueue* driver_intents) noexcept {
    SourceEventPhase2Result out;
    if(!work.known[0]||!work.known[1]||!work.known[4])return out;
    if(!work.raw[0]||work.raw[1]!=1||work.raw[4]!=2){out.status=SourceEventPhase2Status::WrongEventPhase;return out;}
    struct Call {bool active=false;int16_t id=0;uint8_t program=0;size_t owner=0;uint32_t function=0;uint8_t stops=0;};
    std::array<Call,4> calls{};size_t sound_count=0,stop_count=0;
    const auto hold=[&](SourceEventPhase2Status status,size_t offset) noexcept {
        out.status=status;out.missing_offset=static_cast<uint16_t>(offset);
    };
    for(size_t index=0;index<calls.size();++index){
        const auto& descriptor=context.descriptors[index];
        if(!known(descriptor.known.data(),0,2)){hold(SourceEventPhase2Status::NeedsDescriptor,index*8);return out;}
        auto& call=calls[index];call.id=static_cast<int16_t>(read16(descriptor.raw.data()));
        if(call.id==-1)continue;
        if(!descriptor.known[2]){hold(SourceEventPhase2Status::NeedsDescriptor,index*8+2);return out;}
        if(descriptor.raw[2]==255)continue;
        call.active=true;call.function=descriptor.raw[2]==0?0x80070114:0x8007013c;
        if(descriptor.raw[2]!=0){
            if(!descriptor.known[4]){hold(SourceEventPhase2Status::NeedsDescriptor,index*8+4);return out;}
            call.program=descriptor.raw[4];
        }
        size_t matches=0;
        for(size_t owner=0;owner<context.sequences.size();++owner){
            const auto& sequence=context.sequences[owner];
            if(sequence.id&&sequence.program&&*sequence.id==call.id&&*sequence.program==call.program){call.owner=owner;++matches;}
        }
        if(matches!=1){out.status=SourceEventPhase2Status::NeedsSequenceOwner;return out;}
        const auto& sequence=context.sequences[call.owner];
        for(const auto range: {std::array<size_t,2>{0x98,4},{0x84,4},{0x8c,4},{0x56,2},{4,4}})
            if(!known(sequence.known.data(),range[0],range[1])){hold(SourceEventPhase2Status::NeedsSequenceField,range[0]);return out;}
        if(!context.voice_count_800bc820){out.status=SourceEventPhase2Status::NeedsVoiceCount;return out;}
        if(*context.voice_count_800bc820>kSourceEventVoiceCount){out.status=SourceEventPhase2Status::UnsupportedVoiceCount;return out;}
        const auto key=static_cast<int16_t>(uint16_t(call.id)|(uint16_t(call.program)<<8));
        for(size_t voice=0;voice<*context.voice_count_800bc820;++voice){
            if(!driver||!known(driver->voices[voice].known.data(),0x10,2)){hold(SourceEventPhase2Status::NeedsDriverField,voice*54+0x10);return out;}
            if(static_cast<int16_t>(read16(driver->voices[voice].raw.data()+0x10))==key)++call.stops;
        }
        stop_count+=call.stops;++sound_count;
    }
    if(sound_count){
        if(!sound_intents||!sound_intents->initialized){out.status=SourceEventPhase2Status::NeedsIntentQueue;return out;}
        if(sound_intents->count>sound_intents->records.size()){out.status=SourceEventPhase2Status::MalformedIntentQueue;return out;}
        if(sound_intents->records.size()-sound_intents->count<sound_count){out.status=SourceEventPhase2Status::IntentQueueFull;return out;}
    }
    if(stop_count){
        if(!driver_intents||!driver_intents->initialized){out.status=SourceEventPhase2Status::NeedsIntentQueue;return out;}
        if(driver_intents->count>driver_intents->records.size()){out.status=SourceEventPhase2Status::MalformedIntentQueue;return out;}
        if(driver_intents->records.size()-driver_intents->count<stop_count){out.status=SourceEventPhase2Status::IntentQueueFull;return out;}
        for(unsigned half=0;half<2;++half)if(!driver->pending_800c5680[half]||!driver->active_8009ae40[half]){
            out.status=SourceEventPhase2Status::NeedsDriverField;return out;
        }
    }
    if(!profile.initialized){out.status=SourceEventPhase2Status::NeedsProfile;return out;}
    if(profile.request_8001d58c!=0x20000003){out.status=SourceEventPhase2Status::UnsupportedRequest;return out;}
    if(!commands.initialized){out.status=SourceEventPhase2Status::NeedsCommandQueue;return out;}
    if(commands.count>commands.records.size()||commands.current_index>=commands.records.size()){
        out.status=SourceEventPhase2Status::MalformedCommandQueue;return out;
    }
    // Original producer has no bounds check. Reserve its source type0 sentinel
    // in the sixteen-slot native owner; overflow holds the complete transaction.
    if(commands.count>=commands.records.size()-1){out.status=SourceEventPhase2Status::CommandQueueFull;return out;}
    if(commands.current_index!=commands.count && !known(commands.records[commands.current_index].known.data(),8,8)){
        out.status=SourceEventPhase2Status::NeedsCurrentCommandField;return out;
    }
    for(const auto& call:calls)if(call.active){
        auto& sequence=context.sequences[call.owner];
        write(sequence.raw,sequence.known,0x98,4,(read32(sequence.raw.data()+0x98)&~uint32_t(0x40b))|4u);
        const auto key=static_cast<int16_t>(uint16_t(call.id)|(uint16_t(call.program)<<8));
        for(size_t voice=0;voice<*context.voice_count_800bc820;++voice)
            if(static_cast<int16_t>(read16(driver->voices[voice].raw.data()+0x10))==key)stop_voice(*driver,*driver_intents,voice);
        context.halfword_800b5168=0;
        const auto word84=read32(sequence.raw.data()+0x84),word8c=read32(sequence.raw.data()+0x8c);
        const auto half56=read16(sequence.raw.data()+0x56);
        const auto word04=read32(sequence.raw.data()+4);
        for(const size_t offset:{size_t(20),size_t(28),size_t(24),size_t(25),size_t(30),size_t(26),size_t(27),size_t(31),size_t(23),size_t(33),size_t(29),size_t(21),size_t(22)})
            write(sequence.raw,sequence.known,offset,1,0);
        write(sequence.raw,sequence.known,0x88,4,0);write(sequence.raw,sequence.known,0x90,4,word84);
        write(sequence.raw,sequence.known,0x94,4,word8c);write(sequence.raw,sequence.known,0x54,2,half56);
        write(sequence.raw,sequence.known,0,4,word04);write(sequence.raw,sequence.known,8,4,word04);
        for(size_t channel=0;channel<16;++channel){
            write(sequence.raw,sequence.known,55+channel,1,static_cast<uint32_t>(channel));
            write(sequence.raw,sequence.known,39+channel,1,64);write(sequence.raw,sequence.known,96+channel*2,2,127);
        }
        write(sequence.raw,sequence.known,92,2,127);write(sequence.raw,sequence.known,94,2,127);
        auto& intent=sound_intents->records[sound_intents->count++];
        intent={call.function,call.id,call.program,call.stops};
    }
    auto& packet=commands.records[commands.count];
    write(packet.raw,packet.known,0,4,4);write(packet.raw,packet.known,4,4,profile.id);
    write(packet.raw,packet.known,8,4,profile.pair_words[0]);write(packet.raw,packet.known,12,4,profile.pair_words[1]);
    // Source performs these loads after producer writes, including when the
    // current record aliases the write record in the freshly initialized queue.
    commands.request_800989b8=profile.request_8001d58c;
    commands.word_800989bc=read32(commands.records[commands.current_index].raw.data()+8);
    commands.word_800989c0=read32(commands.records[commands.current_index].raw.data()+12);
    ++commands.count;work.raw[4]=3;work.known[4]=1;
    out.status=SourceEventPhase2Status::Consumed;out.committed=true;
    out.sound_receipt_count=static_cast<uint8_t>(sound_count);
    out.driver_receipt_count=static_cast<uint8_t>(stop_count);out.command_receipt_count=1;return out;
}
std::optional<SourceEventCommandRequest> source_event_command_front_request(const SourceEventCommandQueue& queue) noexcept {
    if(!queue.initialized||queue.count>queue.records.size()||queue.current_index>=queue.records.size())return {};
    const auto& record=queue.records[queue.current_index];
    if(!known(record.known.data(),0,16))return {};
    SourceEventCommandRequest request;
    for(size_t i=0;i<4;++i)request.words[i]=read32(record.raw.data()+i*4);
    if(request.words[0]==4)request.source_function=0x8001b89c;
    else return {};
    return request;
}
} // namespace l3
