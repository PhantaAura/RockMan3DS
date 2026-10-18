#include "legends3ds/source_scene_actor40_tail.h"
#include <algorithm>
#include <cassert>
#include <utility>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>

static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
using Work=l3::SourceEventPhase5Bytes<1444>;
using Status=l3::SourceSceneActor40TailStatus;
constexpr uint32_t base=0x8016c000u;
static void put(std::vector<uint8_t>& b,size_t at,uint32_t value){for(size_t i=0;i<4;++i)b[at+i]=uint8_t(value>>(i*8));}
static void work32(Work& w,size_t at,uint32_t value){for(size_t i=0;i<4;++i)w.raw[at+i]=uint8_t(value>>(i*8));}
static uint32_t crc(const uint8_t* p,size_t n){uint32_t value=0xffffffffu;for(size_t i=0;i<n;++i){value^=p[i];for(unsigned j=0;j<8;++j)value=(value>>1)^((value&1u)?0xedb88320u:0u);}return ~value;}
struct Fixture {
    std::vector<uint8_t> bytes=std::vector<uint8_t>(65536);
    l3::EbdDirectory ebd;
    l3::SourceSceneActor40Resources resource;
    Work work;
    l3::SourceSceneActor40PlayerMirrorReceipt receipt;
    Fixture(unsigned count=1){
        ebd.load_base=base;ebd.entries.resize(14);ebd.entries[0].object_class=0x40;
        ebd.entries[0].subtype=5;ebd.entries[0].resource_offset[0]=0x100;
        resource.load_base=base;resource.descriptor_token=base+0x100;
        std::copy(ebd.entries.begin(),ebd.entries.end(),resource.directory_entries.begin());
        put(bytes,0x100+112,base+0x200);
        for(size_t pair=0;pair<2;++pair){const size_t part=16+pair*2,dest=0x200+part*20+20;
            put(bytes,dest,count|(count<<8));
            for(size_t kind=0;kind<2;++kind){const size_t lane=pair*4+kind*2;
                put(bytes,dest+4+kind*4,base+0x800+lane*4096);
                put(bytes,dest+24+kind*4,base+0x1800+lane*4096);
            }
        }
        bind();work.known.fill(1);work.raw[1]=0x40;work.raw[2]=5;
        work.raw[4]=1;work.raw[6]=0x23;work.raw[8]=1;work32(work,136,resource.descriptor_token);
        receipt.admitted=true;receipt.source_tag=work.raw[1];
    }
    void bind(){ebd.source_payload.assign(bytes.begin(),bytes.end());
        resource.payload=ebd.source_payload.lifetime();resource.payload_identity=ebd.source_payload.storage_identity();}
    l3::SourceSceneActor40TailMirrors result(){
        const auto before=work;const auto count=allocations;
        auto out=l3::admit_source_scene_actor40_final_mirrors_8010d824(work,resource,ebd,receipt);
        assert(allocations==count&&work.raw==before.raw&&work.known==before.known);return out;
    }
    void expect(Status status){const auto out=result();static size_t check=0;++check;
        if(out.status!=status)std::cerr<<"tail check "<<check<<" actual "<<unsigned(out.status)<<" expected "<<unsigned(status)<<"\n";assert(out.status==status);
        if(status==Status::SourceBackedPartial){assert(out.admitted&&out.player_mirrors&&out.presentation_pending&&out.shadow_pending&&out.material_cues_pending);}
        else assert(!out.admitted&&!out.player_mirrors&&!out.presentation_pending);
    }
};
int main(int argc,char** argv){
    assert(argc==2||argc==3);std::ifstream file(argv[1]);assert(file);std::string line;std::getline(file,line);size_t cases=0;
    while(std::getline(file,line)){
        std::istringstream row(line);unsigned seed,count;uint32_t expected;row>>seed>>count;Fixture f(count);
        for(auto& value:f.receipt.previous_2c)row>>value;
        for(auto& value:f.receipt.primary_14)row>>value;row>>expected;assert(row);
        for(size_t axis=0;axis<3;++axis){const auto value=f.receipt.primary_14[axis];f.work.raw[20+axis*2]=uint8_t(value);f.work.raw[21+axis*2]=uint8_t(value>>8);}
        f.expect(Status::SourceBackedPartial);const auto result=f.result();
        L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known{};
        for(size_t i=0;i<known.size();++i){player.bytes[i]=uint8_t(seed*37+i*17);known[i]=uint8_t((seed+i)%3==0);}
        const auto prior=player;const auto prior_known=known;const auto allocations_before=allocations;
        assert(l3::apply_source_scene_actor40_player_mirrors_8010d824(*result.player_mirrors,player,known));
        assert(allocations_before==allocations&&crc(player.bytes,sizeof(player.bytes))==expected);
        for(size_t i=0;i<known.size();++i){const bool written=(i>=20&&i<26)||(i>=44&&i<50);
            if(written)assert(known[i]);else assert(known[i]==prior_known[i]&&player.bytes[i]==prior.bytes[i]);}
        ++cases;
    }
    assert(cases==7);size_t guards=0;
    for(const auto window:{std::pair<size_t,size_t>{1,2},{4,1},{6,1},{8,1},{20,6},{136,4}})
        for(size_t i=0;i<window.second;++i){Fixture f;f.work.known[window.first+i]=0;f.expect(Status::NeedsWorkFields);++guards;}
    for(size_t offset:{size_t(1),size_t(2),size_t(4),size_t(6),size_t(8)}){Fixture f;f.work.raw[offset]=0;f.expect(Status::UnsupportedActor);++guards;}
    for(size_t axis=0;axis<3;++axis){Fixture f;f.receipt.primary_14[axis]=1;f.expect(Status::StaleStateReceipt);++guards;}
    {Fixture f;f.receipt.admitted=false;f.expect(Status::StaleStateReceipt);++guards;}
    {Fixture f;f.receipt.source_tag=0x41;f.expect(Status::StaleStateReceipt);++guards;}
    {Fixture f;f.ebd.source_payload.clear();f.expect(Status::NeedsResourceOwnership);++guards;}
    {Fixture f;f.resource.payload_identity=nullptr;f.expect(Status::NeedsResourceOwnership);++guards;}
    {Fixture f;f.resource.load_base=base+4;f.expect(Status::NeedsResourceOwnership);++guards;}
    {Fixture f;f.ebd.entries.pop_back();f.expect(Status::NeedsResourceOwnership);++guards;}
    for(size_t row=0;row<14;++row)for(size_t member=0;member<7;++member){Fixture f;auto& entry=f.ebd.entries[row];
        if(member==0)++entry.object_class;else if(member==1)++entry.subtype;else if(member==2)++entry.variant;else if(member==3)++entry.key_flags;else ++entry.resource_offset[member-4];
        f.expect(Status::NeedsResourceOwnership);++guards;}
    {Fixture f;work32(f.work,136,base+0x104);f.expect(Status::NeedsResourceOwnership);++guards;}
    for(uint32_t bad:{base-4,base+65536-112,base+0x101}){Fixture f;f.resource.descriptor_token=bad;work32(f.work,136,bad);f.expect(Status::UnsafeMaterialDestination);++guards;}
    for(uint32_t bad:{base-4,base+65536-12,0xfffffff0u,base+0x201}){Fixture f;put(f.bytes,0x100+112,bad);f.bind();f.expect(Status::UnsafeMaterialDestination);++guards;}
    for(size_t part:{size_t(16),size_t(18)})for(size_t kind=0;kind<2;++kind)for(size_t source=0;source<2;++source)
        for(uint32_t bad:{base-4,base+65536-8,0x800c3594u,0xffffffffu,base+0x801}){
            Fixture f;put(f.bytes,0x200+part*20+24+kind*4+source*20,bad);f.bind();f.expect(Status::UnsafeMaterialDestination);++guards;
        }
    // With zero counts the original loads pointer words but never dereferences
    // them. Invalid primitive pointers therefore cannot alter actor positions.
    {Fixture f(0);for(size_t part:{size_t(16),size_t(18)})for(size_t i=0;i<4;++i)put(f.bytes,0x200+part*20+24+(i%2)*4+(i/2)*20,0x800c3594u);
        f.bind();f.expect(Status::SourceBackedPartial);++guards;}
    for(uint32_t badbase:{0x800c3580u,0x800c3000u,0x800c4c0fu,0xfffffff0u}){Fixture f;
        f.ebd.load_base=f.resource.load_base=badbase;f.resource.descriptor_token=badbase+0x100;work32(f.work,136,f.resource.descriptor_token);
        f.expect(Status::UnsafeMaterialDestination);++guards;}
    if(argc==3){std::ifstream raw(argv[2],std::ios::binary);assert(raw);
        std::vector<uint8_t> owner((std::istreambuf_iterator<char>(raw)),std::istreambuf_iterator<char>());
        Fixture f;f.ebd=l3::parse_ebd_directory(owner,base);
        const auto rows=l3::find_ebd_entries(f.ebd,0x40,5);assert(!rows.empty());
        f.resource.load_base=base;f.resource.descriptor_token=base+f.ebd.entries[rows.front()].resource_offset[0];
        f.resource.payload=f.ebd.source_payload.lifetime();f.resource.payload_identity=f.ebd.source_payload.storage_identity();
        std::copy(f.ebd.entries.begin(),f.ebd.entries.end(),f.resource.directory_entries.begin());
        work32(f.work,136,f.resource.descriptor_token);f.expect(Status::SourceBackedPartial);
    }
    std::cout<<"class40 tail: "<<cases<<" original authored mirror cases, "<<guards<<" work/receipt/ownership/pointer guards, unchanged actor bytes and allocation-free admission\n";
}
