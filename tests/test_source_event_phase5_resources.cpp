#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_event_pipeline.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Status=l3::SourceEventPhase5ResourceStatus;
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
void put(uint8_t* data,uint32_t value){for(size_t i=0;i<4;++i)data[i]=uint8_t(value>>(i*8));}
std::array<uint8_t,32> digest(const std::string& text){
    require(text.size()==64,"authored digest malformed");std::array<uint8_t,32> out{};
    const auto digit=[](char c){return uint8_t(c>='a'?c-'a'+10:c-'0');};
    for(size_t i=0;i<32;++i)out[i]=uint8_t(digit(text[i*2])*16+digit(text[i*2+1]));return out;
}
struct Vector{size_t bytes;const char* sha;};
// Python hashlib independently computed these digests over authored bytes.
constexpr Vector vectors[]={{232,"a5cc2dc41d7325d8e23aca0856bf6f486a81c19316e55bc37424bb49c3078207"},{255,"07ecaefa4fe3291db23ca6aa16eec5b1f1bb96459f7d8123e586cc996090ebeb"},{256,"8c497499ca3c031680951fde6d185712ed0aee3d05563a0e38153364f152c45c"},{511,"f7de038a7deeff8e8dd38a666189a769dddeba3096786ee663c229ab9afffaef"},{512,"922fc4d6ca51424c8d83680c6de1be739214a9da3efa07ebd713bf6eef3e2e3f"},{513,"e9d1bd04b707a43406a9df8c11f655d7ed392cfca29ae7890dda607490bde6c7"},{113808,"e8aa11efbb96d6044415024dbd5b60fa8336d66b4b65f3aaa62c58b3fd151c2d"}};
std::vector<uint8_t> bytes(size_t count){
    std::vector<uint8_t> out(count);for(size_t i=0;i<count;++i)out[i]=uint8_t(i*7+3);
    put(out.data(),14);
    for(size_t i=0;i<14;++i){auto* row=out.data()+4+i*16;row[0]=0xe0;row[1]=3;row[2]=uint8_t(i%4);row[3]=0;
        put(row+4,0x8016c000u+228u+uint32_t(i%4));put(row+8,0);put(row+12,0);}
    return out;
}
l3::SourceEventPhase5Profile profile(const char* sha){
    l3::SourceEventPhase5Profile out;out.initialized=true;out.directory_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    out.ebd_source_digest=digest(sha);
    for(size_t i=0;i<16;++i){auto& record=out.records[i];record.raw[0]=uint8_t(i);record.raw[1]=i?3:5;record.raw[3]=i?0xe0:0x40;
        if(i){record.resource_variant=uint8_t((i-1)%4);record.resource_directory_index=record.resource_variant;
            record.resource_token=0x8016c000u+228u+record.resource_variant;}}
    return out;
}
l3::SourceEventWork work(){l3::SourceEventWork out;out.known.fill(1);out.raw[0]=1;out.raw[1]=1;out.raw[4]=5;return out;}
l3::SourceEventPhase5Context context(){l3::SourceEventPhase5Context out;
    l3::initialize_source_event_phase5_pools_8002e88c(out);
    out.initial_eye_800a39a0=std::array<uint32_t,3>{};out.initial_rotation_800a3964=std::array<uint32_t,3>{};return out;}
void authored(){
    for(const auto& vector:vectors){const auto payload=bytes(vector.bytes);const auto owner=l3::parse_ebd_directory(payload,0x8016c000);
        const auto p=profile(vector.sha);const auto before=allocations;const auto witness=l3::bind_source_event_phase5_resources(p,owner);
        require(witness.status()==Status::Admitted&&witness.binding_count()==15&&witness.matches(p)&&witness.resident_in(owner)&&allocations==before,
            "authored SHA/padding/ownership admission failed or allocated");
        auto state=context();auto event=work();const auto call_allocations=allocations;
        const auto result=l3::consume_source_event1_phase5_80100efc(event,state,p,&witness);
        require(result.committed&&event.phase04()==6&&result.resource_bindings==15&&allocations==call_allocations,"owned constructor held or allocated");
    }
    const auto p=profile(vectors[6].sha);const auto payload=bytes(vectors[6].bytes);
    const auto source=l3::parse_ebd_directory(payload,0x8016c000);
    const auto admitted=l3::bind_source_event_phase5_resources(p,source);
    for(unsigned i=0;i<11;++i){auto candidate=p;auto owner=source;Status expected;
        switch(i){case 0:candidate.initialized=false;expected=Status::NeedsProfile;break;
            case 1:candidate.records[1].raw[3]=0x20;expected=Status::UnsupportedProfile;break;
            case 2:owner.source_payload.clear();expected=Status::NeedsPayload;break;
            case 3:owner.source_payload.resize(262145);expected=Status::PayloadBudgetExceeded;break;
            case 4:candidate.ebd_source_digest[0]^=1;expected=Status::PayloadDigestMismatch;break;
            case 5:owner.entries[0].key_flags^=1;expected=Status::SourceDirectoryMismatch;break;
            case 6:candidate.records[1].resource_variant=1;expected=Status::RowKeyMismatch;break;
            case 7:candidate.records[1].resource_directory_index=4;expected=Status::NotFirstMatch;break;
            case 8:candidate.records[1].resource_token+=1;expected=Status::ResourceOffsetMismatch;break;
            case 9:owner.entries[0].resource_offset[0]+=1;expected=Status::SourceDirectoryMismatch;break;
            default:{auto changed=payload;changed.back()^=1;owner.source_payload.assign(changed.begin(),changed.end());expected=Status::PayloadDigestMismatch;break;}}
        const auto rejected=l3::bind_source_event_phase5_resources(candidate,owner);
        require(rejected.status()==expected&&!rejected.admitted()&&rejected.binding_count()==0,"malformed cross-package identity admitted");
    }
    {
        const auto small=l3::parse_ebd_directory(bytes(vectors[0].bytes),0x8016c000);
        auto candidate=profile(vectors[0].sha);candidate.records[1].resource_token+=100;
        require(l3::bind_source_event_phase5_resources(candidate,small).status()==Status::ResourceOutsidePayload,
            "outside resource window admitted");
        auto changed_directory=source;changed_directory.entries[0].subtype^=1;
        require(!admitted.resident_in(changed_directory),"mutated active directory retained admission");
    }
    for(unsigned i=0;i<4;++i){auto candidate=p;const l3::SourceEventPhase5ResourceBinding* witness=&admitted;
        l3::SourceEventPhase5ResourceBinding empty;
        if(i==0)witness=nullptr;else if(i==1)witness=&empty;else if(i==2)candidate.records[7].raw[12]^=1;else candidate.ebd_source_digest[1]^=1;
        auto state=context();auto event=work();const auto before_work=event.raw;const auto allocated=allocations;
        const auto result=l3::consume_source_event1_phase5_80100efc(event,state,candidate,witness);
        require(!result.committed&&result.status==l3::SourceEventPhase5Status::NeedsResourceOwnership&&event.raw==before_work&&
            state.pool_e0.used[0]==0&&allocations==allocated,"missing/stale/mutated ownership did not roll back atomically");
    }
    l3::SourceEventPhase5ResourceBinding lease;
    {auto original=l3::parse_ebd_directory(payload,0x8016c000);auto shared=original;
        lease=l3::bind_source_event_phase5_resources(p,original);original.source_payload.clear();
        require(lease.admitted()&&!lease.resident_in(original)&&lease.resident_in(shared),"shared owner/replacement identity incorrect");
        shared.source_payload.clear();require(!lease.admitted()&&!lease.matches(p),"unload did not expire borrowed resource lease");}
    auto state=context();auto event=work();const auto allocated=allocations;bool held=true;
    for(unsigned i=0;i<4096;++i){const auto result=l3::consume_source_event1_phase5_80100efc(event,state,p,&lease);held&=!result.committed;}
    require(held&&allocations==allocated&&event.phase04()==5,"expired ownership hold allocated or advanced");
    std::cout<<"7 independent SHA/padding vectors; 12 malformed joins; constructor rollback/shared/unload/4096 allocation-free holds PASS; binding_bytes="
        <<sizeof(l3::SourceEventPhase5ResourceBinding)<<'\n';
}
std::vector<uint8_t> read(const char* path){std::ifstream input(path,std::ios::binary);require(bool(input),"private owner file missing");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());}
}
int main(int argc,char** argv){try{
    if(argc==3){const auto p=l3::load_source_event_phase5_profile(read(argv[1]));const auto owner=l3::load_l3ebd_directory(read(argv[2]));
        const auto witness=l3::bind_source_event_phase5_resources(p,owner);require(witness.admitted(),"private owner resource identity rejected");
        auto state=context();auto event=work();const auto result=l3::consume_source_event1_phase5_80100efc(event,state,p,&witness);
        require(result.committed&&result.resource_bindings==15,"private owner constructor rejected");
        std::cout<<"resident owner SHA256 + 15 exact source pointer/index/variant/first-match joins + constructor PASS\n";
    }else{require(argc==1,"usage: test_source_event_phase5_resources [owner.l3p5 owner.l3ebd]");authored();}return 0;
}catch(const std::exception& failure){std::cerr<<failure.what()<<'\n';return 1;}}
