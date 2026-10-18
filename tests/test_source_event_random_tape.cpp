#include "legends3ds/source_event_random_tape.h"
#include "legends3ds/source_event_pipeline.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceEventRandomTapeStatus;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
l3::SourceEventRandomTapeDigest digest(const char* text){l3::SourceEventRandomTapeDigest out{};
    const auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
    for(size_t i=0;i<32;++i)out[i]=uint8_t((nibble(text[2*i])<<4)|nibble(text[2*i+1]));return out;
}
l3::SourceEventRandomTapeProfile fixture(unsigned count=2,bool zero=false){
    l3::SourceEventRandomTapeProfile out;out.record_count=count;
    out.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    out.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    out.ebd_sha256=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");
    out.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    out.semantic_trace_sha256.fill(17);out.global_trace_sha256.fill(34);
    // Authored direct recorded constructor owner, deliberately unrelated to
    // first callback half100. No loop-listener/host-frame relation is assumed.
    out.constructor={60000,1,2,3,2,1,5};
    constexpr uint32_t ras[]={0x80100dd4u,0x80100decu,0x80100e04u,0x80100e40u,0x80100e58u,0x80100e70u};
    for(size_t i=0;i<count;++i){auto& record=out.records[i];record.event_frame=uint32_t(i);record.source_actor=0x800c3580;
        record.actor_state08=1;record.count=uint8_t(zero||i>=31?0:6);record.scratch_half=uint16_t(100+i);record.position={{33,55,65500}};
        record.pool80_before=zero?std::array<uint32_t,2>{{0xffffffffu,0xffffffffu}}:
            std::array<uint32_t,2>{{i?0xc0000000u:0,0}};
        if(i>=31)record.pool80_before={{0,0}};
        record.global_ordinal_before=100+(zero?0:i*8);record.global_ordinal_after=record.global_ordinal_before+record.count;
        for(size_t call=0;call<record.count;++call)record.calls[call]={ras[call],uint32_t(8+call*8),record.global_ordinal_before+call+1};
    }
    return out;
}
void put(uint8_t* bytes,size_t offset,uint64_t value,size_t count=4){for(size_t i=0;i<count;++i)bytes[offset+i]=uint8_t(value>>(i*8));}
void fix_crc(std::vector<uint8_t>& bytes){uint32_t crc=0xffffffffu;for(size_t i=32;i<bytes.size();++i){crc^=bytes[i];
    for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}put(bytes.data(),24,~crc);}
std::vector<uint8_t> package(const l3::SourceEventRandomTapeProfile& profile){
    std::vector<uint8_t> bytes(256+profile.record_count*144);bytes[0]='L';bytes[1]='3';bytes[2]='S';bytes[3]='R';
    put(bytes.data(),4,1,2);put(bytes.data(),6,256,2);put(bytes.data(),8,1);put(bytes.data(),12,profile.record_count);
    put(bytes.data(),16,144);put(bytes.data(),20,bytes.size());
    const l3::SourceEventRandomTapeDigest* digests[]={&profile.exe_sha256,&profile.overlay_sha256,&profile.ebd_sha256,&profile.bios_sha256,&profile.semantic_trace_sha256,&profile.global_trace_sha256};
    for(size_t i=0;i<6;++i)for(size_t j=0;j<32;++j)bytes[32+i*32+j]=(*digests[i])[j];
    put(bytes.data(),224,profile.constructor.scratch_half,2);bytes[226]=profile.constructor.scratch0;bytes[227]=profile.constructor.cadence;
    bytes[228]=profile.constructor.main_phase;bytes[229]=profile.constructor.subphase;bytes[230]=profile.constructor.event_id;bytes[231]=profile.constructor.event_phase;
    for(size_t i=0;i<profile.record_count;++i){const auto& r=profile.records[i];const size_t offset=256+i*144;
        put(bytes.data(),offset,r.event_frame);put(bytes.data(),offset+4,r.source_actor);bytes[offset+8]=r.actor_state08;bytes[offset+9]=r.count;
        put(bytes.data(),offset+10,r.scratch_half,2);for(size_t axis=0;axis<3;++axis)put(bytes.data(),offset+12+axis*2,r.position[axis],2);
        put(bytes.data(),offset+20,r.pool80_before[0]);put(bytes.data(),offset+24,r.pool80_before[1]);bytes[offset+28]=1;bytes[offset+29]=6;bytes[offset+30]=1;
        put(bytes.data(),offset+32,r.global_ordinal_before,8);put(bytes.data(),offset+40,r.global_ordinal_after,8);
        for(size_t call=0;call<6;++call){put(bytes.data(),offset+48+call*16,r.calls[call].caller_ra);put(bytes.data(),offset+52+call*16,r.calls[call].return_v0);put(bytes.data(),offset+56+call*16,r.calls[call].global_ordinal,8);}
    }
    fix_crc(bytes);return bytes;
}
void codec_guards(){const auto profile=fixture();const auto bytes=package(profile);l3::SourceEventRandomTapeProfile out;
    const auto allocated=allocations;require(l3::load_l3source_event_random_tape(bytes.data(),bytes.size(),out)==Status::Ready&&
        out.record_count==2&&out.constructor.scratch_half==60000&&out.records[0].scratch_half==100&&allocations==allocated,"canonical authored tape did not decode without allocation");
    out.record_count=777;require(l3::load_l3source_event_random_tape(nullptr,bytes.size(),out)==Status::InvalidProfile&&out.record_count==777,"null tape published");
    for(size_t size=0;size<bytes.size();++size)require(l3::load_l3source_event_random_tape(bytes.data(),size,out)==Status::InvalidProfile&&out.record_count==777,"truncated tape published");
    require(l3::load_l3source_event_random_tape(bytes.data(),bytes.size()+1,out)==Status::InvalidProfile&&out.record_count==777,"oversized tape admitted");
    for(size_t offset:{size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(28),size_t(32),size_t(64),size_t(96),size_t(128),size_t(227),size_t(232),size_t(256),size_t(264),size_t(265),size_t(274),size_t(284),size_t(285),size_t(286),size_t(287),size_t(288),size_t(296),size_t(304)}){
        auto bad=bytes;bad[offset]^=1;if(offset!=24)fix_crc(bad);
        require(l3::load_l3source_event_random_tape(bad.data(),bad.size(),out)==Status::InvalidProfile&&out.record_count==777,"re-CRC malformed tape published");
    }
    for(unsigned test=0;test<11;++test){auto bad=profile;
        switch(test){case 0:bad.record_count=0;break;case 1:bad.record_count=65;break;case 2:bad.records[0].actor_state08=2;break;
        case 3:bad.records[0].global_ordinal_before=0;break;case 4:bad.records[0].global_ordinal_after=32768;break;
        case 5:bad.constructor.scratch0=2;break;case 6:bad.semantic_trace_sha256.fill(0);break;case 7:bad.global_trace_sha256.fill(0);break;
        case 8:bad.records[0].calls[0].return_v0=32768;break;case 9:bad.records[0].source_actor++;break;default:bad.records[0].count=3;break;}
        require(!l3::is_supported_source_event_random_tape_profile(bad),"unsupported state/ordinal/identity profile admitted");
    }
    auto one=fixture(1);one.records[0].pool80_before={{0xffffffffu,0xfffffffeu}};one.records[0].count=3;one.records[0].global_ordinal_after=103;
    for(size_t i=3;i<6;++i)one.records[0].calls[i]={};require(l3::is_supported_source_event_random_tape_profile(one),"supported three-return recorded null-second allocation rejected");
}
l3::SourceEventWork event(const l3::SourceEventRandomTapeRecord& record){l3::SourceEventWork out;out.known.fill(1);out.raw[0]=out.raw[1]=1;out.raw[4]=6;
    put(out.raw.data(),16,record.event_frame);put(out.raw.data(),36,record.source_actor);return out;}
l3::SourceEventPhase5Context scene(const l3::SourceEventRandomTapeRecord& record){l3::SourceEventPhase5Context out;
    out.pool80.used={{record.pool80_before[0],record.pool80_before[1]}};
    for(size_t i=0;i<64;++i){out.pool80.slots[i].known.fill(1);out.pool80.slots[i].raw[1]=uint8_t(0x80u|i);}
    out.pool40.used[0]=0x80000000;auto& actor=out.pool40.slots[0];actor.known.fill(1);actor.raw[1]=0x40;actor.raw[8]=record.actor_state08;
    for(size_t axis=0;axis<3;++axis)put(actor.raw.data(),20+axis*2,record.position[axis],2);return out;
}
l3::SourceEventRandomTapeContext context(const l3::SourceEventRandomTapeProfile& profile,const l3::SourceEventWork& work,
    const l3::SourceEventPhase5Context& scene,const void* owner){l3::SourceEventRandomTapeContext out;
    out.work=&work;out.scene=&scene;out.callback=l3::kSourceEventRandomEffectCallback;out.scratch_half=profile.records[0].scratch_half;
    out.resource_owner_admitted=true;out.resource_owner=out.current_resource_owner=owner;out.resource_generation=out.current_resource_generation=19;
    out.exe_sha256=profile.exe_sha256;out.overlay_sha256=profile.overlay_sha256;out.ebd_sha256=profile.ebd_sha256;out.bios_sha256=profile.bios_sha256;return out;
}
void service_guards(){const auto profile=fixture();const int owner=0,other=0;l3::SourceEventRandomTapeService service;
    auto work=event(profile.records[0]);auto owned=scene(profile.records[0]);const auto live=context(profile,work,owned,&owner);
    require(l3::prepare_source_event_random_tape(service,live).status==Status::NeedsProfile,"disabled service invented a stream");
    require(l3::install_source_event_random_tape(service,profile,l3::SourceEventRandomTapeMode::RecordedOpenBiosScene)==Status::Ready,"recorded profile did not install");
    const auto before=service.cursor();const auto generation=service.generation();const auto allocated=allocations;
    for(unsigned test=0;test<13;++test){auto input=live;auto actor_scene=owned;auto event_work=work;input.work=&event_work;input.scene=&actor_scene;
        switch(test){case 0:input.resource_owner_admitted=false;break;case 1:input.current_resource_owner=&other;break;case 2:input.current_resource_generation++;break;
        case 3:input.ebd_sha256[0]^=1;break;case 4:input.scratch_half.reset();break;case 5:input.scratch_half=99;break;
        case 6:actor_scene.pool40.slots[0].raw[20]++;break;case 7:actor_scene.pool40.slots[0].raw[8]=2;break;
        case 8:actor_scene.pool80.used[0]=1;break;case 9:event_work.known[16]=0;break;case 10:actor_scene.pool40.slots[0].known[25]=0;break;
        case 11:input.callback=0x80100738;break;default:actor_scene.pool80.slots[0].known[1]=0;break;}
        require(l3::prepare_source_event_random_tape(service,input).status!=Status::Ready&&service.cursor()==before&&service.generation()==generation,
            "mismatched recorded context changed service cursor/owners");
    }
    require(allocations==allocated,"recorded guards allocated");const auto prepared=l3::prepare_source_event_random_tape(service,live);
    require(prepared.status==Status::Ready&&prepared.receipts.count==6,"matching recorded context did not prepare exact receipts");
    require(l3::commit_source_event_random_tape(service,prepared,false,0)==Status::AwaitingCommit&&service.cursor()==0&&service.generation()==generation,"held callback consumed tape");
    require(l3::commit_source_event_random_tape(service,prepared,true,3)==Status::InvalidCommit&&service.cursor()==0,"partial callback consumed tape");
    auto altered=prepared;altered.receipts.calls[0].return_v0++;
    require(l3::commit_source_event_random_tape(service,altered,true,6)==Status::InvalidCommit&&service.cursor()==0,"altered prepared receipt advanced tape");
    const auto consumed=l3::consume_source_event_random_effect_80100d6c(work,owned,&prepared.receipts,&prepared.receipt_owner);
    require(consumed.committed&&l3::commit_source_event_random_tape(service,prepared,true,consumed.committed_call_count)==Status::Ready&&service.cursor()==1,"completed callback did not atomically advance tape once");
    require(l3::commit_source_event_random_tape(service,prepared,true,6)==Status::StaleOwner&&service.cursor()==1,"duplicate callback advanced tape");
    auto next_work=event(profile.records[1]);auto next_live=context(profile,next_work,owned,&owner);next_live.scratch_half=profile.records[1].scratch_half;
    const auto next=l3::prepare_source_event_random_tape(service,next_live);require(next.status==Status::Ready,"committed pool fields did not match next recorded allocator context");
    l3::invalidate_source_event_random_tape(service);require(l3::commit_source_event_random_tape(service,next,true,6)==Status::StaleOwner&&!service.admitted(),"invalidated owner committed pending tape");
    auto invalid=profile;invalid.record_count=0;require(l3::install_source_event_random_tape(service,invalid,l3::SourceEventRandomTapeMode::RecordedOpenBiosScene)==Status::InvalidProfile&&!service.admitted(),"invalid profile enabled service");
}
void zero_call_commits(){const auto profile=fixture(32,true);require(l3::is_supported_source_event_random_tape_profile(profile),"authored zero-call prefix rejected");
    l3::SourceEventRandomTapeService service;require(l3::install_source_event_random_tape(service,profile,l3::SourceEventRandomTapeMode::RecordedOpenBiosScene)==Status::Ready,"zero-call profile install failed");
    const int owner=0;for(size_t i=0;i<32;++i){auto work=event(profile.records[i]);auto owned=scene(profile.records[i]);auto live=context(profile,work,owned,&owner);live.scratch_half=profile.records[i].scratch_half;
        const auto prepared=l3::prepare_source_event_random_tape(service,live);require(prepared.status==Status::Ready&&prepared.receipts.count==0,"recorded null/signed zero-call frame did not prepare");
        require(l3::commit_source_event_random_tape(service,prepared,false,0)==Status::AwaitingCommit&&service.cursor()==i,"held zero-call frame advanced cursor");
        const auto result=l3::consume_source_event_random_effect_80100d6c(work,owned,&prepared.receipts,&prepared.receipt_owner);
        require(result.committed&&result.committed_call_count==0&&l3::commit_source_event_random_tape(service,prepared,true,0)==Status::Ready&&service.cursor()==i+1,"completed zero-call callback failed one-record commit");
    }
    auto work=event(profile.records[0]);auto owned=scene(profile.records[0]);const auto live=context(profile,work,owned,&owner);
    require(l3::prepare_source_event_random_tape(service,live).status==Status::TapeExhausted,"exhausted capture silently generated a new value");
}
} // namespace
int main(){try{codec_guards();service_guards();zero_call_commits();std::cout<<"source_event_random_tape passed\n";return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
