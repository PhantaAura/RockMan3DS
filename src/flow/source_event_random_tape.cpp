#include "legends3ds/source_event_random_tape.h"
#include "legends3ds/source_event_pipeline.h"
#include <limits>

namespace l3 {
namespace {
constexpr uint8_t nibble(char c){return uint8_t(c<='9'?c-'0':c-'a'+10);}
constexpr SourceEventRandomTapeDigest digest(const char* text){SourceEventRandomTapeDigest out{};
    for(size_t i=0;i<32;++i){out[i]=uint8_t((nibble(text[2*i])<<4)|nibble(text[2*i+1]));}
    return out;}
constexpr auto kExe=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
constexpr auto kOverlay=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
constexpr auto kEbd=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");
constexpr auto kBios=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
uint16_t half(const uint8_t* p) noexcept{return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
uint32_t word(const uint8_t* p) noexcept{return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
uint64_t wide(const uint8_t* p) noexcept{return uint64_t(word(p))|(uint64_t(word(p+4))<<32);}
uint32_t crc(const uint8_t* p,size_t size) noexcept{uint32_t value=0xffffffffu;
    for(size_t i=0;i<size;++i){value^=p[i];for(unsigned bit=0;bit<8;++bit)value=(value>>1)^(0xedb88320u&(0u-(value&1u)));}
    return ~value;
}
bool nonzero(const SourceEventRandomTapeDigest& value) noexcept{for(uint8_t byte:value)if(byte)return true;return false;}
bool valid(const SourceEventRandomTapeProfile& profile) noexcept{
    if(profile.scope!=SourceEventRandomTapeMode::RecordedOpenBiosScene||profile.record_count==0||profile.record_count>64||
        profile.exe_sha256!=kExe||profile.overlay_sha256!=kOverlay||profile.ebd_sha256!=kEbd||profile.bios_sha256!=kBios||
        !nonzero(profile.semantic_trace_sha256)||!nonzero(profile.global_trace_sha256)||profile.constructor.scratch0>1||
        profile.constructor.cadence!=2||profile.constructor.main_phase!=3||profile.constructor.subphase!=2||
        profile.constructor.event_id!=1||profile.constructor.event_phase!=5)return false;
    constexpr uint32_t ras[]={0x80100dd4u,0x80100decu,0x80100e04u,0x80100e40u,0x80100e58u,0x80100e70u};
    for(size_t i=0;i<profile.record_count;++i){const auto& record=profile.records[i];
        if(record.event_frame!=i||record.actor_state08!=1||record.global_ordinal_before==0||record.global_ordinal_after>32767u||
            record.source_actor<0x800c3580u||(record.source_actor-0x800c3580u)%1444u||
            (record.source_actor-0x800c3580u)/1444u>=4||
            record.global_ordinal_before>std::numeric_limits<uint64_t>::max()-record.count||
            record.global_ordinal_after!=record.global_ordinal_before+record.count||
            (i&&record.global_ordinal_before<profile.records[i-1].global_ordinal_after))return false;
        unsigned occupied=0;for(uint32_t bitmap:record.pool80_before)for(unsigned bit=0;bit<32;++bit)occupied+=(bitmap>>bit)&1u;
        const unsigned expected=record.event_frame>=31?0u:(occupied>=64?0u:(occupied==63?3u:6u));
        if(record.count!=expected)return false;
        for(size_t call=0;call<record.calls.size();++call){const auto& receipt=record.calls[call];
            if(call<record.count){if(receipt.caller_ra!=ras[call]||receipt.return_v0>32767u||
                receipt.global_ordinal!=record.global_ordinal_before+call+1u)return false;
            }else if(receipt.caller_ra||receipt.return_v0||receipt.global_ordinal)return false;
        }
    }
    return true;
}
} // namespace
bool is_supported_source_event_random_tape_profile(const SourceEventRandomTapeProfile& profile) noexcept{return valid(profile);}
SourceEventRandomTapeStatus load_l3source_event_random_tape(const uint8_t* bytes,size_t size,SourceEventRandomTapeProfile& result) noexcept{
    using Status=SourceEventRandomTapeStatus;
    if(!bytes||size<256||bytes[0]!='L'||bytes[1]!='3'||bytes[2]!='S'||bytes[3]!='R'||half(bytes+4)!=1||half(bytes+6)!=256||
        word(bytes+8)!=1||word(bytes+12)==0||word(bytes+12)>64||word(bytes+16)!=144||word(bytes+20)!=size||
        size!=256+size_t(word(bytes+12))*144||word(bytes+28)!=0||word(bytes+24)!=crc(bytes+32,size-32))return Status::InvalidProfile;
    for(size_t i=232;i<256;++i)if(bytes[i])return Status::InvalidProfile;
    SourceEventRandomTapeProfile out;out.record_count=word(bytes+12);
    SourceEventRandomTapeDigest* digests[]={&out.exe_sha256,&out.overlay_sha256,&out.ebd_sha256,&out.bios_sha256,&out.semantic_trace_sha256,&out.global_trace_sha256};
    for(size_t i=0;i<6;++i)for(size_t j=0;j<32;++j)(*digests[i])[j]=bytes[32+i*32+j];
    out.constructor={half(bytes+224),bytes[226],bytes[227],bytes[228],bytes[229],bytes[230],bytes[231]};
    for(size_t i=0;i<out.record_count;++i){const uint8_t* p=bytes+256+i*144;auto& record=out.records[i];
        if(half(p+18)!=0||p[28]!=1||p[29]!=6||p[30]!=1||p[31]!=0)return Status::InvalidProfile;
        record.event_frame=word(p);record.source_actor=word(p+4);record.actor_state08=p[8];record.count=p[9];record.scratch_half=half(p+10);
        for(size_t axis=0;axis<3;++axis)record.position[axis]=half(p+12+axis*2);
        record.pool80_before={{word(p+20),word(p+24)}};record.global_ordinal_before=wide(p+32);record.global_ordinal_after=wide(p+40);
        for(size_t call=0;call<6;++call)record.calls[call]={word(p+48+call*16),word(p+52+call*16),wide(p+56+call*16)};
    }
    if(!valid(out)){return Status::InvalidProfile;}
    result=out;return Status::Ready;
}
struct SourceEventRandomTapeAccess {
    static SourceEventRandomTapeStatus install(SourceEventRandomTapeService& service,const SourceEventRandomTapeProfile& profile,SourceEventRandomTapeMode mode) noexcept{
        using Status=SourceEventRandomTapeStatus;
        if(mode==SourceEventRandomTapeMode::Disabled){invalidate(service);return Status::Disabled;}
        if(mode!=SourceEventRandomTapeMode::RecordedOpenBiosScene||!valid(profile)||service.generation_==std::numeric_limits<uint64_t>::max())return Status::InvalidProfile;
        service.profile_=profile;service.cursor_=0;service.admitted_=true;++service.generation_;return Status::Ready;
    }
    static void invalidate(SourceEventRandomTapeService& service) noexcept{
        service.admitted_=false;service.cursor_=0;
        if(service.generation_!=std::numeric_limits<uint64_t>::max())++service.generation_;
    }
    static SourceEventRandomTapePrepared prepare(const SourceEventRandomTapeService& service,const SourceEventRandomTapeContext& context) noexcept{
        using Status=SourceEventRandomTapeStatus;SourceEventRandomTapePrepared out;
        if(!service.admitted_){out.status=Status::NeedsProfile;return out;}
        if(service.cursor_>=service.profile_.record_count){out.status=Status::TapeExhausted;return out;}
        if(!context.resource_owner_admitted||!context.resource_owner||!context.current_resource_owner||context.resource_generation==0||context.current_resource_generation==0){out.status=Status::NeedsOwner;return out;}
        if(context.resource_owner!=context.current_resource_owner||context.resource_generation!=context.current_resource_generation){out.status=Status::StaleOwner;return out;}
        const auto& profile=service.profile_;
        if(context.exe_sha256!=profile.exe_sha256||context.overlay_sha256!=profile.overlay_sha256||context.ebd_sha256!=profile.ebd_sha256||context.bios_sha256!=profile.bios_sha256){out.status=Status::OwnerMismatch;return out;}
        if(!context.work||!context.scene||!context.scratch_half){out.status=Status::NeedsEventWork;return out;}
        const auto& work=*context.work;const auto& scene=*context.scene;const auto& record=profile.records[service.cursor_];
        for(size_t offset:{size_t(0),size_t(1),size_t(4),size_t(16),size_t(17),size_t(18),size_t(19),size_t(36),size_t(37),size_t(38),size_t(39)})if(!work.known[offset]){out.status=Status::NeedsEventWork;return out;}
        if(context.callback!=kSourceEventRandomEffectCallback||work.active00()!=1||work.event01()!=1||work.phase04()!=6||
            word(work.raw.data()+16)!=record.event_frame||word(work.raw.data()+36)!=record.source_actor||*context.scratch_half!=record.scratch_half||
            !scene.pool80.used[0]||!scene.pool80.used[1]||*scene.pool80.used[0]!=record.pool80_before[0]||*scene.pool80.used[1]!=record.pool80_before[1]){out.status=Status::ContextMismatch;return out;}
        const size_t slot=(record.source_actor-0x800c3580u)/1444u;const auto& actor=scene.pool40.slots[slot];
        if(!scene.pool40.used[0]||!(*scene.pool40.used[0]&(0x80000000u>>slot))||!actor.known[1]||actor.raw[1]!=uint8_t(0x40u|slot)){out.status=Status::ContextMismatch;return out;}
        for(size_t offset:{size_t(8),size_t(20),size_t(21),size_t(22),size_t(23),size_t(24),size_t(25)})if(!actor.known[offset]){out.status=Status::NeedsActorFields;return out;}
        if(actor.raw[8]!=record.actor_state08){out.status=Status::ContextMismatch;return out;}
        for(size_t axis=0;axis<3;++axis)if(half(actor.raw.data()+20+axis*2)!=record.position[axis]){out.status=Status::ContextMismatch;return out;}
        const auto plan=plan_source_event_random_effect_80100d6c(work,scene);
        if(plan.status!=SourceEventRandomEffectStatus::Ready||plan.rng_calls!=record.count){out.status=Status::ContextMismatch;return out;}
        out.status=Status::Ready;out.service=&service;out.cursor=service.cursor_;out.generation=service.generation_;
        out.receipts.owner=&service;out.receipts.generation=service.generation_;out.receipts.completed=true;
        out.receipts.callback=kSourceEventRandomEffectCallback;out.receipts.source_work=0x800c4c48;out.receipts.event_frame=record.event_frame;
        out.receipts.source_actor=record.source_actor;out.receipts.count=record.count;out.receipts.calls=record.calls;
        out.receipt_owner={&service,service.generation_};return out;
    }
    static SourceEventRandomTapeStatus commit(SourceEventRandomTapeService& service,const SourceEventRandomTapePrepared& prepared,bool completed,uint8_t count) noexcept{
        using Status=SourceEventRandomTapeStatus;
        if(!completed)return Status::AwaitingCommit;
        if(!service.admitted_||prepared.status!=Status::Ready||prepared.service!=&service||prepared.generation!=service.generation_||prepared.cursor!=service.cursor_)return Status::StaleOwner;
        if(service.cursor_>=service.profile_.record_count||count!=service.profile_.records[service.cursor_].count||
            prepared.receipts.count!=count||prepared.receipt_owner.owner!=&service||prepared.receipt_owner.generation!=service.generation_||
            service.generation_==std::numeric_limits<uint64_t>::max())return Status::InvalidCommit;
        const auto& record=service.profile_.records[service.cursor_];const auto& receipts=prepared.receipts;
        if(!receipts.completed||receipts.owner!=&service||receipts.generation!=service.generation_||
            receipts.callback!=kSourceEventRandomEffectCallback||receipts.source_work!=0x800c4c48u||
            receipts.event_frame!=record.event_frame||receipts.source_actor!=record.source_actor)return Status::InvalidCommit;
        for(size_t i=0;i<record.calls.size();++i)if(receipts.calls[i].caller_ra!=record.calls[i].caller_ra||
            receipts.calls[i].return_v0!=record.calls[i].return_v0||receipts.calls[i].global_ordinal!=record.calls[i].global_ordinal)return Status::InvalidCommit;
        ++service.cursor_;++service.generation_;return Status::Ready;
    }
};
SourceEventRandomTapeStatus install_source_event_random_tape(SourceEventRandomTapeService& s,const SourceEventRandomTapeProfile& p,SourceEventRandomTapeMode mode) noexcept{return SourceEventRandomTapeAccess::install(s,p,mode);}
void invalidate_source_event_random_tape(SourceEventRandomTapeService& s) noexcept{SourceEventRandomTapeAccess::invalidate(s);}
SourceEventRandomTapePrepared prepare_source_event_random_tape(const SourceEventRandomTapeService& s,const SourceEventRandomTapeContext& c) noexcept{return SourceEventRandomTapeAccess::prepare(s,c);}
SourceEventRandomTapeStatus commit_source_event_random_tape(SourceEventRandomTapeService& s,const SourceEventRandomTapePrepared& p,bool completed,uint8_t count) noexcept{return SourceEventRandomTapeAccess::commit(s,p,completed,count);}
} // namespace l3
