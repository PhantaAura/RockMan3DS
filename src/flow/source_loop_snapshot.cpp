#include "legends3ds/source_loop_snapshot.h"

namespace l3 {
namespace {
constexpr uint8_t nibble(char c){return uint8_t(c<='9'?c-'0':c-'a'+10);}
constexpr SourceLoopSnapshotDigest digest(const char* value){
    SourceLoopSnapshotDigest out{};
    for(size_t i=0;i<out.size();++i)out[i]=uint8_t((nibble(value[2*i])<<4)|nibble(value[2*i+1]));
    return out;
}
constexpr auto kExe=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
constexpr auto kOverlay=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
constexpr auto kEbd=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");
constexpr auto kBios=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
uint32_t word(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
void put(uint8_t* p,uint32_t value,size_t count=4) noexcept {
    for(size_t i=0;i<count;++i)p[i]=uint8_t(value>>(8*i));
}
uint32_t crc32(const uint8_t* bytes,size_t count) noexcept {
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<count;++i){crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}
bool valid(const SourceLoopSnapshot& snapshot) noexcept {
    return snapshot.scope==SourceLoopSnapshotScope::RecordedOpenBiosConstructor&&
        snapshot.pc==kSourceLoopSnapshotConstructorPc&&snapshot.loops>0&&
        uint16_t(snapshot.loops-1u)==snapshot.scratch_half&&snapshot.cadence==2&&
        snapshot.event_id==1&&snapshot.event_phase==5&&snapshot.main_phase==3&&
        snapshot.subphase==2&&snapshot.scratch0==0&&snapshot.exe_sha256==kExe&&
        snapshot.overlay_sha256==kOverlay&&snapshot.ebd_sha256==kEbd&&snapshot.bios_sha256==kBios;
}
} // namespace
bool is_supported_source_loop_snapshot(const SourceLoopSnapshot& snapshot) noexcept {return valid(snapshot);}
SourceLoopSnapshotStatus load_l3source_loop_snapshot(const uint8_t* bytes,size_t size,
    SourceLoopSnapshot& result) noexcept {
    if(!bytes||size!=kSourceLoopSnapshotBytes||bytes[0]!='L'||bytes[1]!='3'||bytes[2]!='L'||bytes[3]!='S'||
        bytes[4]!=1||bytes[5]!=0||bytes[6]!=176||bytes[7]!=0||word(bytes+8)!=1||
        word(bytes+12)!=crc32(bytes+16,size-16))return SourceLoopSnapshotStatus::InvalidSnapshot;
    for(size_t i=160;i<size;++i)if(bytes[i]!=0)return SourceLoopSnapshotStatus::InvalidSnapshot;
    SourceLoopSnapshot out;out.scope=static_cast<SourceLoopSnapshotScope>(word(bytes+8));
    out.pc=word(bytes+16);out.loops=word(bytes+20);
    out.scratch_half=uint16_t(uint16_t(bytes[24])|(uint16_t(bytes[25])<<8));
    out.cadence=bytes[26];out.event_id=bytes[27];out.event_phase=bytes[28];
    out.main_phase=bytes[29];out.subphase=bytes[30];out.scratch0=bytes[31];
    SourceLoopSnapshotDigest* digests[]={&out.exe_sha256,&out.overlay_sha256,&out.ebd_sha256,&out.bios_sha256};
    for(size_t i=0;i<4;++i)for(size_t j=0;j<32;++j)(*digests[i])[j]=bytes[32+i*32+j];
    if(!valid(out))return SourceLoopSnapshotStatus::InvalidSnapshot;
    result=out;return SourceLoopSnapshotStatus::Ready;
}
SourceLoopSnapshotStatus encode_l3source_loop_snapshot(const SourceLoopSnapshot& snapshot,
    SourceLoopSnapshotBytes& result) noexcept {
    if(!valid(snapshot))return SourceLoopSnapshotStatus::InvalidSnapshot;
    SourceLoopSnapshotBytes out{};out[0]='L';out[1]='3';out[2]='L';out[3]='S';out[4]=1;out[6]=176;
    put(out.data()+8,uint32_t(snapshot.scope));put(out.data()+16,snapshot.pc);put(out.data()+20,snapshot.loops);
    put(out.data()+24,snapshot.scratch_half,2);out[26]=snapshot.cadence;out[27]=snapshot.event_id;
    out[28]=snapshot.event_phase;out[29]=snapshot.main_phase;out[30]=snapshot.subphase;out[31]=snapshot.scratch0;
    const SourceLoopSnapshotDigest* digests[]={&snapshot.exe_sha256,&snapshot.overlay_sha256,&snapshot.ebd_sha256,&snapshot.bios_sha256};
    for(size_t i=0;i<4;++i)for(size_t j=0;j<32;++j)out[32+i*32+j]=(*digests[i])[j];
    put(out.data()+12,crc32(out.data()+16,out.size()-16));result=out;return SourceLoopSnapshotStatus::Ready;
}
SourceLoopSnapshotStatus evaluate_source_loop_snapshot_admission(const SourceLoopSnapshot* snapshot,
    const SourceLoopSnapshotBoundary& boundary) noexcept {
    using Status=SourceLoopSnapshotStatus;
    if(!boundary.recorded_replay_selected)return Status::Disabled;
    if(!snapshot)return Status::NeedsSnapshot;
    if(!valid(*snapshot))return Status::InvalidSnapshot;
    if(boundary.already_admitted)return Status::AlreadyAdmitted;
    if(!boundary.constructor_consumed)return Status::NeedsConstructorReceipt;
    if(!boundary.source_pc||!boundary.event_active||!boundary.event_id||!boundary.event_phase||
        !boundary.main_phase||!boundary.subphase||!boundary.cadence||!boundary.scratch0)return Status::NeedsBoundary;
    if(*boundary.source_pc!=snapshot->pc||*boundary.event_active!=1||*boundary.event_id!=snapshot->event_id||
        *boundary.event_phase!=snapshot->event_phase||*boundary.main_phase!=snapshot->main_phase||
        *boundary.subphase!=snapshot->subphase||*boundary.cadence!=snapshot->cadence||
        *boundary.scratch0!=snapshot->scratch0)return Status::WrongBoundary;
    if(!boundary.owner||!boundary.current_owner||boundary.generation==0||boundary.current_generation==0)return Status::NeedsOwner;
    if(boundary.owner!=boundary.current_owner||boundary.generation!=boundary.current_generation)return Status::StaleOwner;
    if(boundary.exe_sha256!=snapshot->exe_sha256||boundary.overlay_sha256!=snapshot->overlay_sha256||
        boundary.ebd_sha256!=snapshot->ebd_sha256||boundary.bios_sha256!=snapshot->bios_sha256)return Status::OwnerMismatch;
    return Status::Ready;
}
} // namespace l3
