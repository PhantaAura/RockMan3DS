#include "legends3ds/source_player_proxy.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { bool count_allocations=false;std::size_t allocations=0; }
void* operator new(std::size_t n){if(count_allocations)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

namespace {
using namespace l3;
constexpr std::array<std::size_t,16> changed{{8,9,10,11,12,13,14,15,16,17,18,19,24,25,26,27}};
SourceGteRtpsState seeded(){
    SourceGteRtpsState state;
    for(std::size_t i=0;i<32;++i){state.data[i]=0x13579bdfU+static_cast<std::uint32_t>(i)*0x1111111U;state.control[i]=0xa5a50000U+static_cast<std::uint32_t>(i);}
    state.projection_controls_bound=true;return state;
}
std::array<std::uint32_t,8> camera(){return {{4096,0,4096,0,4096,0,0,2000}};}
std::uint32_t token(std::istringstream& stream){std::uint64_t value=0;if(!(stream>>value)||value>0xffffffffULL)throw std::runtime_error("invalid player-proxy fixture scalar");return static_cast<std::uint32_t>(value);}
void fixture(const char* filename){
    std::ifstream file(filename);if(!file)throw std::runtime_error("missing original player-proxy fixture");
    std::string line;std::size_t cases=0,commands=0;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::istringstream input(line);SourcePlayerLeaseInputs lease;
        std::array<std::uint32_t,8> scratch{};for(auto& w:scratch)w=token(input);lease.scratch_camera_1f800008=scratch;
        auto state=seeded();for(std::size_t i=24;i<29;++i)state.control[i]=token(input);
        const auto before=state;lease.mask_80098798=token(input);const auto alias=token(input);
        std::array<SourceHitProxyOwnerPoint,24> owners{};
        for(std::size_t i=0;i<24;++i){owners[i].raw_bc_words={{token(input),token(input)}};lease.owners_800be478[i]=&owners[alias?0:i];}
        std::array<std::uint32_t,16> expected{};for(auto& x:expected)x=token(input);
        const auto expected_flag=token(input);
        std::array<std::array<std::uint32_t,2>,24> expected_owners{};
        for(auto& owner:expected_owners)owner={{token(input),token(input)}};
        const auto projected=token(input);std::string extra;assert(!(input>>extra));
        count_allocations=true;const auto result=source_player_consume_leases_80035508(state,lease);count_allocations=false;
        assert(result.status==SourcePlayerProxyStatus::SourceBackedPartial);
        assert(result.projected==projected && result.consumed_mask==(*lease.mask_80098798&0xffffff00U));
        for(std::size_t i=0;i<changed.size();++i)if(state.data[changed[i]]!=expected[i]){
            std::cerr<<"player-proxy case "<<cases<<" data"<<changed[i]<<" got="<<state.data[changed[i]]<<" expected="<<expected[i]<<'\n';
            throw std::runtime_error("original/native GTE lease mismatch");
        }
        assert(state.control[31]==expected_flag);
        for(std::size_t i=0;i<24;++i)assert(lease.owners_800be478[i]->raw_bc_words==expected_owners[i]);
        for(std::size_t i=8;i<31;++i)assert(state.control[i]==before.control[i]);
        for(std::size_t i:{2U,3U,4U,5U,6U,7U,20U,21U,22U,23U,28U,29U,30U,31U})assert(state.data[i]==before.data[i]);
        ++cases;commands+=projected;
    }
    assert(cases>1000);std::cout<<"original80035508/native authored cases="<<cases<<" RTPS="<<commands<<'\n';
}
void holds(){
    auto state=seeded();const auto before=state;
    SourcePlayerLeaseInputs input;SourceHitProxyOwnerPoint owner;owner.raw_bc_words={{0x12345678,0x9abcdef0}};
    assert(source_player_consume_leases_80035508(state,input).status==SourcePlayerProxyStatus::NeedsMatrixTranslation);
    input.scratch_camera_1f800008=camera();
    assert(source_player_consume_leases_80035508(state,input).status==SourcePlayerProxyStatus::NeedsMask);
    input.mask_80098798=0x80000100;
    assert(source_player_consume_leases_80035508(state,input).status==SourcePlayerProxyStatus::NeedsOwnerPoint);
    input.owners_800be478[0]=&owner;state.projection_controls_bound=false;
    assert(source_player_consume_leases_80035508(state,input).status==SourcePlayerProxyStatus::NeedsProjectionControls);
    state.projection_controls_bound=true;
    // Valid first owner, unavailable final owner: no early lease clear or GTE commit.
    const auto held=source_player_consume_leases_80035508(state,input);
    assert(held.status==SourcePlayerProxyStatus::NeedsOwnerPoint && held.projected==0 && held.consumed_mask==0);
    assert(owner.raw_bc_words[0]==0x12345678 && owner.raw_bc_words[1]==0x9abcdef0);
    assert(state.data==before.data && state.control==before.control && !state.matrix_translation_bound);
    input.owners_800be478[23]=&owner;
    (*input.scratch_camera_1f800008)[5]=0x7fffffff; // first translation plus positive product exceeds44-bit domain
    owner.raw_bc_words={{32767,0}};
    assert(source_player_consume_leases_80035508(state,input).status==SourcePlayerProxyStatus::NeedsBoundedArithmetic);
    assert(state.data==before.data && state.control==before.control && owner.raw_bc_words[0]==32767);
    input.mask_80098798=255;
    const auto empty=source_player_consume_leases_80035508(state,input);
    assert(empty.status==SourcePlayerProxyStatus::SourceBackedPartial && empty.projected==0);
    assert(state.matrix_translation_bound && state.data==before.data);
    assert(state.control[5]==0x7fffffff); // camera load occurs even when no RTPS is reached
}
void divider_and_ticks(){
    auto reads=seeded();reads.data[1]=0xa5a5ffff;reads.data[19]=0xa5a50001;
    assert(source_gte_read_data(reads,15)==reads.data[14] && reads.data[15]!=reads.data[14]);
    assert(source_gte_read_data(reads,1)==0xffffffffU && source_gte_read_data(reads,19)==1);
    assert(!source_gte_read_data(reads,28) && !source_gte_read_data(reads,29) && !source_gte_read_data(reads,32));
    bool overflow=false;
    assert(source_gte_unr_divide(0,0,overflow)==0x1ffff && overflow);
    assert(source_gte_unr_divide(0,1,overflow)==0 && !overflow);
    assert(source_gte_unr_divide(1,1,overflow)==0x10000 && !overflow);
    assert(source_gte_unr_divide(2,1,overflow)==0x1ffff && overflow);
    // Hardware reciprocal clips these rounded approximations without divide overflow.
    assert(source_gte_unr_divide(0xfe3f,0x7f20,overflow)==0x1ffff && !overflow);
    assert(source_gte_unr_divide(0xf015,0x780b,overflow)==0x1ffff && !overflow);
    SourcePlayerLeaseInputs input;input.scratch_camera_1f800008=camera();input.mask_80098798=0xffffff00;
    std::array<SourceHitProxyOwnerPoint,24> owners{};
    for(std::size_t i=0;i<24;++i)input.owners_800be478[i]=&owners[i];
    auto state=seeded();state.control[24]=160U<<16;state.control[25]=120U<<16;state.control[26]=320;state.control[27]=0;state.control[28]=0;
    count_allocations=true;
    for(std::size_t tick=0;tick<4096;++tick){
        for(std::size_t i=0;i<24;++i)owners[i].raw_bc_words={{static_cast<std::uint32_t>(i*31),123}};
        const auto result=source_player_consume_leases_80035508(state,input);
        assert(result.status==SourcePlayerProxyStatus::SourceBackedPartial && result.projected==24);
        for(const auto& owner:owners)assert(owner.raw_bc_words[0]==0 && owner.raw_bc_words[1]==0);
    }
    count_allocations=false;assert(allocations==0);
}
int reference_fixture(const char* filename){
    std::ifstream file(filename);if(!file)throw std::runtime_error("missing captured player lease fixture");
    std::string line;std::size_t cases=0,matched=0,held=0,projections=0;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::istringstream input(line);const auto ordinal=token(input);
        SourcePlayerLeaseInputs lease;std::array<std::uint32_t,8> scratch{};
        for(auto& x:scratch)x=token(input);lease.scratch_camera_1f800008=scratch;lease.mask_80098798=token(input);
        SourceGteRtpsState state;for(auto& x:state.control)x=token(input);for(auto& x:state.data)x=token(input);
        state.matrix_translation_bound=true;state.projection_controls_bound=true;
        std::array<std::uint32_t,24> physical{};for(auto& x:physical)x=token(input);
        std::array<SourceHitProxyOwnerPoint,24> owners{};
        for(std::size_t i=0;i<24;++i)owners[i].raw_bc_words={{token(input),token(input)}};
        for(std::size_t i=0;i<24;++i){
            if(!(*lease.mask_80098798&(0x80000000U>>i)))continue;
            std::size_t owner=i;for(std::size_t j=0;j<i;++j)if(physical[j]==physical[i] && (*lease.mask_80098798&(0x80000000U>>j))){owner=j;break;}
            if(physical[i])lease.owners_800be478[i]=&owners[owner];
        }
        std::array<std::uint32_t,32> expected_c{},expected_d{};
        for(auto& x:expected_c)x=token(input);for(auto& x:expected_d)x=token(input);
        std::array<std::array<std::uint32_t,2>,24> expected_owners{};
        for(auto& x:expected_owners)x={{token(input),token(input)}};
        std::string extra;assert(!(input>>extra));
        const auto result=source_player_consume_leases_80035508(state,lease);
        ++cases;projections+=result.projected;
        if(result.status!=SourcePlayerProxyStatus::SourceBackedPartial){
            ++held;std::cout<<"{\"ordinal\":"<<ordinal<<",\"status\":\"UNKNOWN\",\"hold\":"<<unsigned(result.status)<<"}\n";continue;
        }
        std::size_t differences=0;
        auto difference=[&](const char* space,std::size_t index,std::uint32_t actual,std::uint32_t expected){
            if(actual==expected)return;
            ++differences;std::cout<<"{\"ordinal\":"<<ordinal<<",\"mismatch\":\""<<space<<"\",\"index\":"<<index<<",\"actual\":"<<actual<<",\"expected\":"<<expected<<"}\n";
        };
        for(std::size_t i=0;i<32;++i){difference("CP2C",i,state.control[i],expected_c[i]);difference("CP2D",i,state.data[i],expected_d[i]);}
        for(std::size_t i=0;i<24;++i)if(lease.owners_800be478[i])for(std::size_t word=0;word<2;++word)
            difference("owner_bc_c0",i*2+word,lease.owners_800be478[i]->raw_bc_words[word],expected_owners[i][word]);
        if(!differences)++matched;
        std::cout<<"{\"ordinal\":"<<ordinal<<",\"status\":\""<<(differences?"DIVERGENT":"MATCHES_MEASURED_FIELDS")<<"\",\"projections\":"<<result.projected<<",\"differences\":"<<differences<<"}\n";
    }
    std::cout<<"{\"schema\":\"mml-source-player-lease-native-comparison/1\",\"cases\":"<<cases<<",\"matched\":"<<matched<<",\"held\":"<<held<<",\"projections\":"<<projections<<",\"retail_parity\":false,\"status\":\""<<(cases && matched==cases?"MATCHES_MEASURED_FIELDS":"UNKNOWN")<<"\"}\n";
    return !cases ? 3 : matched==cases ? 0 : 1;
}
} // namespace
int main(int argc,char** argv){
    if(argc==3 && std::string(argv[1])=="--reference")return reference_fixture(argv[2]);
    if(argc!=2)return 2;
    fixture(argv[1]);holds();divider_and_ticks();assert(allocations==0);
    std::cout<<"source player proxy ownership/alias/UNR guards and4096 allocation-free24-owner ticks PASS\n";
}
