#include "legends3ds/source_loop_snapshot.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

static size_t allocations=0;
void* operator new(std::size_t size){++allocations;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceLoopSnapshotStatus;
void require(bool valid,const char* why){if(!valid)throw std::runtime_error(why);}
l3::SourceLoopSnapshotDigest digest(const char* text){
    l3::SourceLoopSnapshotDigest result{};const auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
    for(size_t i=0;i<result.size();++i)result[i]=uint8_t((nibble(text[2*i])<<4)|nibble(text[2*i+1]));
    return result;
}
// Authored protocol fixture; none of the recorded owner-local checkpoint bytes
// are read or embedded in this test/source tree.
l3::SourceLoopSnapshot fixture(uint32_t loops=37){
    l3::SourceLoopSnapshot snapshot;snapshot.pc=l3::kSourceLoopSnapshotConstructorPc;
    snapshot.loops=loops;snapshot.scratch_half=uint16_t(loops-1u);snapshot.cadence=2;
    snapshot.event_id=1;snapshot.event_phase=5;snapshot.main_phase=3;snapshot.subphase=2;
    snapshot.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    snapshot.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    snapshot.ebd_sha256=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");
    snapshot.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    return snapshot;
}
bool same(const l3::SourceLoopSnapshot& a,const l3::SourceLoopSnapshot& b){
    return a.scope==b.scope&&a.pc==b.pc&&a.loops==b.loops&&a.scratch_half==b.scratch_half&&
        a.cadence==b.cadence&&a.event_id==b.event_id&&a.event_phase==b.event_phase&&
        a.main_phase==b.main_phase&&a.subphase==b.subphase&&a.scratch0==b.scratch0&&
        a.exe_sha256==b.exe_sha256&&a.overlay_sha256==b.overlay_sha256&&a.ebd_sha256==b.ebd_sha256&&a.bios_sha256==b.bios_sha256;
}
void fix_crc(l3::SourceLoopSnapshotBytes& bytes){
    uint32_t crc=0xffffffffu;for(size_t i=16;i<bytes.size();++i){crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
    crc=~crc;for(size_t i=0;i<4;++i)bytes[12+i]=uint8_t(crc>>(8*i));
}
void codec(){
    for(uint32_t loops:{1u,2u,37u,65535u,65536u,65537u,0x7fffffffu,0x80000000u,0xffffffffu}){
        const auto snapshot=fixture(loops);l3::SourceLoopSnapshotBytes bytes;l3::SourceLoopSnapshot decoded;
        const auto before=allocations;
        require(l3::is_supported_source_loop_snapshot(snapshot)&&l3::encode_l3source_loop_snapshot(snapshot,bytes)==Status::Ready,
            "supported authored snapshot did not encode");
        require(l3::load_l3source_loop_snapshot(bytes.data(),bytes.size(),decoded)==Status::Ready&&same(decoded,snapshot)&&allocations==before,
            "canonical loop snapshot round trip failed or allocated");
    }
    const auto snapshot=fixture();l3::SourceLoopSnapshotBytes bytes;
    require(l3::encode_l3source_loop_snapshot(snapshot,bytes)==Status::Ready,"fixture did not encode");
    auto decoded=fixture(7);const auto before=decoded;
    require(l3::load_l3source_loop_snapshot(nullptr,bytes.size(),decoded)==Status::InvalidSnapshot&&same(decoded,before),"null input published");
    for(size_t size=0;size<bytes.size();++size)require(l3::load_l3source_loop_snapshot(bytes.data(),size,decoded)==Status::InvalidSnapshot&&same(decoded,before),"truncated input admitted");
    require(l3::load_l3source_loop_snapshot(bytes.data(),bytes.size()+1,decoded)==Status::InvalidSnapshot&&same(decoded,before),"oversized input admitted");
    for(size_t offset=0;offset<bytes.size();++offset){
        // Changing only loops' high16 remains a valid low16 relation. The
        // cooker separately pins actual observation/profile provenance.
        if(offset==22||offset==23)continue;
        auto bad=bytes;bad[offset]^=1;
        if(offset>=16)fix_crc(bad);
        require(l3::load_l3source_loop_snapshot(bad.data(),bad.size(),decoded)==Status::InvalidSnapshot&&same(decoded,before),"re-CRC tampered identity/boundary/reserved input admitted");
    }
    for(unsigned test=0;test<12;++test){
        auto bad=snapshot;
        switch(test){
        case 0:bad.scope=static_cast<l3::SourceLoopSnapshotScope>(0);break;
        case 1:bad.loops=0;bad.scratch_half=65535;break;
        case 2:++bad.scratch_half;break;case 3:++bad.pc;break;
        case 4:bad.cadence=0;break;case 5:bad.event_id=0;break;case 6:bad.event_phase=6;break;
        case 7:bad.main_phase=2;break;case 8:bad.subphase=3;break;case 9:bad.scratch0=1;break;
        case 10:bad.bios_sha256.fill(0);break;default:bad.ebd_sha256[4]^=1;break;
        }
        auto encoded=bytes;require(!l3::is_supported_source_loop_snapshot(bad)&&
            l3::encode_l3source_loop_snapshot(bad,encoded)==Status::InvalidSnapshot&&encoded==bytes,"invalid object published by encoder");
    }
}
l3::SourceLoopSnapshotBoundary boundary(const l3::SourceLoopSnapshot& snapshot,const void* owner){
    l3::SourceLoopSnapshotBoundary out;out.recorded_replay_selected=true;out.constructor_consumed=true;
    out.source_pc=snapshot.pc;out.event_active=1;out.event_id=snapshot.event_id;out.event_phase=snapshot.event_phase;
    out.main_phase=snapshot.main_phase;out.subphase=snapshot.subphase;out.cadence=snapshot.cadence;out.scratch0=snapshot.scratch0;
    out.exe_sha256=snapshot.exe_sha256;out.overlay_sha256=snapshot.overlay_sha256;out.ebd_sha256=snapshot.ebd_sha256;out.bios_sha256=snapshot.bios_sha256;
    out.owner=out.current_owner=owner;out.generation=out.current_generation=19;return out;
}
void admission(){
    const auto snapshot=fixture();const int owner=0,other=0;const auto live=boundary(snapshot,&owner);
    const auto allocated=allocations;
    for(unsigned iteration=0;iteration<4096;++iteration)require(l3::evaluate_source_loop_snapshot_admission(&snapshot,live)==Status::Ready,"valid witnessed constructor boundary rejected");
    require(allocations==allocated,"admission allocated");
    l3::SourceLoopSnapshotBoundary disabled;require(l3::evaluate_source_loop_snapshot_admission(nullptr,disabled)==Status::Disabled,"optional replay default enabled");
    require(l3::evaluate_source_loop_snapshot_admission(nullptr,live)==Status::NeedsSnapshot,"absent snapshot fabricated initial value");
    auto invalid=snapshot;invalid.loops=0;require(l3::evaluate_source_loop_snapshot_admission(&invalid,live)==Status::InvalidSnapshot,"invalid bound object admitted");
    auto context=live;context.constructor_consumed=false;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::NeedsConstructorReceipt,"pre-constructor admission accepted");
    context=live;context.already_admitted=true;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::AlreadyAdmitted,"duplicate scene admission accepted");
    for(unsigned field=0;field<8;++field){context=live;
        switch(field){case 0:context.source_pc.reset();break;case 1:context.event_active.reset();break;case 2:context.event_id.reset();break;
        case 3:context.event_phase.reset();break;case 4:context.main_phase.reset();break;case 5:context.subphase.reset();break;
        case 6:context.cadence.reset();break;default:context.scratch0.reset();break;}
        require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::NeedsBoundary,"missing source/replay boundary owner admitted");
    }
    for(unsigned field=0;field<8;++field){context=live;
        switch(field){case 0:context.source_pc=0x8010102c;break;case 1:context.event_active=0;break;case 2:context.event_id=2;break;
        case 3:context.event_phase=6;break;case 4:context.main_phase=2;break;case 5:context.subphase=3;break;
        case 6:context.cadence=1;break;default:context.scratch0=1;break;}
        require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::WrongBoundary,"wrong source/replay boundary admitted");
    }
    for(unsigned field=0;field<4;++field){context=live;
        switch(field){case 0:context.exe_sha256[0]^=1;break;case 1:context.overlay_sha256[0]^=1;break;
        case 2:context.ebd_sha256[0]^=1;break;default:context.bios_sha256[0]^=1;break;}
        require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::OwnerMismatch,"different resident owner admitted");
    }
    context=live;context.owner=nullptr;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::NeedsOwner,"ownerless constructor admitted");
    context=live;context.generation=0;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::NeedsOwner,"zero generation admitted");
    context=live;context.current_generation++;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::StaleOwner,"accepted lifecycle change retained admission");
    context=live;context.current_owner=&other;require(l3::evaluate_source_loop_snapshot_admission(&snapshot,context)==Status::StaleOwner,"replacement owner retained admission");
}
} // namespace
int main(){try{codec();admission();std::cout<<"source_loop_snapshot passed\n";return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
