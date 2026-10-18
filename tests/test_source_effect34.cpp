#include "legends3ds/source_effect34.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
size_t allocation_count=0;bool tracking=false;
void check(bool v,const char* why){if(!v){std::cerr<<why<<'\n';std::exit(1);}}
constexpr uint32_t base=0x800a3b40u;
using Record=l3::SourceEventPhase5Bytes<72>;
uint16_t u16(const Record& r,size_t o){return uint16_t(r.raw[o]|(uint16_t(r.raw[o+1])<<8));}
uint32_t bit(size_t s){return 0x80000000u>>(s&31u);}
void init(l3::SourceEventPhase5Context& c,uint32_t seed,uint8_t subtype=3){
    c.pool80.used={{0x80000000u,0u}};c.pool80_pending_release_800988c8={{0u,0u}};
    for(size_t s=0;s<64;++s){for(size_t i=0;i<72;++i){c.pool80.slots[s].raw[i]=uint8_t(seed*37u+(s*72u+i)*17u+11u);c.pool80.slots[s].known[i]=1;}c.pool80.slots[s].raw[1]=uint8_t(0x80u|s);}
    auto& p=c.pool80.slots[0];p.raw[0]=3;p.raw[2]=subtype;p.raw[3]=0;p.raw[6]=3;p.raw[8]=0;p.raw[9]=0;p.raw[10]=0;p.raw[13]=0;p.raw[14]=0;p.raw[15]=0;
}
bool same(const l3::SourceEventPhase5Context& a,const l3::SourceEventPhase5Context& b){
    if(a.pool80.used!=b.pool80.used||a.pool80_pending_release_800988c8!=b.pool80_pending_release_800988c8)return false;
    for(size_t s=0;s<64;++s)if(a.pool80.slots[s].raw!=b.pool80.slots[s].raw||a.pool80.slots[s].known!=b.pool80.slots[s].known)return false;
    return true;
}
std::vector<std::string> split(const std::string& s,char delimiter){std::vector<std::string> result;std::stringstream in(s);std::string item;while(std::getline(in,item,delimiter))result.push_back(item);return result;}
l3::SourceEffect34ControlReceipt authored_controls(){
    l3::SourceEffect34ControlReceipt c;c.admitted=true;c.sequence3_identity=l3::source_effect3_sequence;c.rows4_identity=l3::source_effect4_rows;
    for(size_t i=0;i<10;++i)c.sequence3[i]=uint16_t(11u+i*7u);c.sequence3[10]=0xffffu;
    for(size_t i=0;i<4;++i)c.rows4[i]={{uint8_t(20+i*3),uint8_t(250+i),2,uint8_t(1+i)}};
    return c;
}
l3::SourceEffect34RngReceipt receipt(const l3::SourceEventPhase5Context& c,uint32_t callback,uint32_t child,uint32_t ra,uint64_t ordinal){
    const auto& p=c.pool80.slots[0];l3::SourceEffect34RngReceipt r;r.callback_identity=callback;r.parent_identity=base;r.child_identity=child;r.return_address=ra;r.v0=uint32_t(ordinal*199u);r.global_ordinal=ordinal;r.source_generation=1;r.parent_xyz1c={{u16(p,28),u16(p,30),u16(p,32)}};r.parent_clock09=p.raw[9];r.parent_parameter03=p.raw[3];r.parent_mode0d=p.raw[13];r.parent_tag0f=p.raw[15];return r;
}
l3::SourceEffect34Result invoke(l3::SourceEventPhase5Context& c,uint32_t id,uint32_t callback,const l3::SourceEffect34Inputs& in){
    allocation_count=0;tracking=true;const auto r=l3::tick_source_effect34(c,id,callback,in);tracking=false;check(allocation_count==0,"hot callback heap allocation");return r;
}
size_t guards(){
    auto controls=authored_controls();l3::SourceEffect34Inputs in;in.controls=&controls;in.source_generation=1;
    l3::SourceEventPhase5Context c;init(c,0);const auto original=c;
    check(!invoke(c,base+1,l3::source_effect3_callback,in).committed&&same(c,original),"identity atomic guard");
    auto plan=invoke(c,base,l3::source_effect3_callback,in);check(plan.status==l3::SourceEffect34Status::NeedsRngReceipts&&plan.rng_required==2&&same(c,original),"source3 plan must hold");
    std::array<l3::SourceEffect34RngReceipt,120> r{};for(size_t i=0;i<2;++i)r[i]=receipt(c,l3::source_effect3_callback,base+72,plan.rng_return_addresses[i],1001+i);
    in.rng=r.data();in.rng_count=2;r[0].global_ordinal=0;check(!invoke(c,base,l3::source_effect3_callback,in).committed&&same(c,original),"zero ordinal guard");r[0].global_ordinal=1001;
    r[0].global_ordinal=std::numeric_limits<uint64_t>::max();r[1].global_ordinal=0;
    check(!invoke(c,base,l3::source_effect3_callback,in).committed&&same(c,original),"wrapped scope ordinal atomic guard");r[0].global_ordinal=1001;r[1].global_ordinal=1002;
    r[1].parent_identity=base+72;check(!invoke(c,base,l3::source_effect3_callback,in).committed&&same(c,original),"parent guard");r[1].parent_identity=base;
    r[1].source_generation=2;check(!invoke(c,base,l3::source_effect3_callback,in).committed&&same(c,original),"generation guard");r[1].source_generation=1;
    auto bad=c;bad.pool80.slots[1].known[1]=0;check(!invoke(bad,base,l3::source_effect3_callback,in).committed,"unknown childheader");
    controls.admitted=false;check(!invoke(c,base,l3::source_effect3_callback,in).committed&&same(c,original),"table admission guard");controls.admitted=true;
    const auto good=invoke(c,base,l3::source_effect3_callback,in);check(good.committed&&good.rng_consumed==2&&good.allocated_count==1,"source3 spawn");
    init(c,2,4);in.rng=nullptr;in.rng_count=0;const auto four=invoke(c,base,l3::source_effect4_callback,in);check(four.rng_required==120,"source4 bounded120");
    for(size_t i=0;i<120;++i)r[i]=receipt(c,l3::source_effect4_callback,four.child_identities[i/5],four.rng_return_addresses[i],1001+i);
    in.rng=r.data();in.rng_count=120;check(invoke(c,base,l3::source_effect4_callback,in).allocated_count==24,"source4 24 children");
    size_t count=12;
    for(uint8_t sub:{uint8_t(3),uint8_t(4)}){init(c,0,sub);c.pool80.used={{0xffffffffu,0xffffffffu}};const auto before=c;in.rng=nullptr;in.rng_count=0;
        auto full=invoke(c,base,sub==3?l3::source_effect3_callback:l3::source_effect4_callback,in);check(full.committed&&full.source_null_allocation&&full.rng_consumed==0,"original null path");
        check(c.pool80.slots[0].raw[9]==(sub==3?before.pool80.slots[0].raw[9]:uint8_t(before.pool80.slots[0].raw[9]+1)),"subtype-specific null clock");++count;}
    for(uint8_t sub:{uint8_t(3),uint8_t(4)}){init(c,0,sub);auto& p=c.pool80.slots[0];p.raw[13]=1;p.raw[8]=1;
        auto before=c;c.pool80_pending_release_800988c8[0]=std::nullopt;auto held=c;check(!invoke(c,base,sub==3?l3::source_effect3_callback:l3::source_effect4_callback,in).committed&&same(c,held),"unknown release guard");c=before;
        check(invoke(c,base,sub==3?l3::source_effect3_callback:l3::source_effect4_callback,in).released&&*c.pool80.used[0]==bit(0)&&*c.pool80_pending_release_800988c8[0]==bit(0),"deferred release preserves allocation");count+=2;}
    // Unknown lanes retain their raw values and remain unknown if not read/write.
    init(c,0);c.pool80.slots[0].raw[13]=1;c.pool80.slots[0].raw[8]=0;c.pool80.slots[0].raw[10]=9;c.pool80.slots[0].known[40]=0;
    check(invoke(c,base,l3::source_effect3_callback,in).committed&&c.pool80.slots[0].raw[8]==1&&!c.pool80.slots[0].known[40],"sequence termination/known lanes");++count;
    // Every consumed lane is necessary; losing any one must retain all owners.
    init(c,0);in.rng=r.data();in.rng_count=2;
    plan=invoke(c,base,l3::source_effect3_callback,l3::SourceEffect34Inputs{&controls,nullptr,0,1});
    for(size_t i=0;i<2;++i)r[i]=receipt(c,l3::source_effect3_callback,base+72,plan.rng_return_addresses[i],1001+i);
    for(size_t off:{size_t(0),size_t(2),size_t(3),size_t(6),size_t(9),size_t(13),size_t(15),size_t(28),size_t(29),size_t(30),size_t(31),size_t(32),size_t(33)}){
        auto held=c;held.pool80.slots[0].known[off]=0;const auto before=held;
        check(!invoke(held,base,l3::source_effect3_callback,in).committed&&same(held,before),"unknown consumed lane atomic guard");++count;
    }
    for(size_t changed=0;changed<2;++changed){const auto original_r=r[changed];r[changed].parent_tag0f=1;auto held=c;
        check(!invoke(held,base,l3::source_effect3_callback,in).committed&&same(held,c),"source3 tag binding");r[changed]=original_r;++count;}
    controls.sequence3[10]=0;auto held=c;check(!invoke(held,base,l3::source_effect3_callback,in).committed&&same(held,c),"terminator control identity");controls.sequence3[10]=0xffffu;++count;
    init(c,0,4);in.rng=nullptr;in.rng_count=0;plan=invoke(c,base,l3::source_effect4_callback,in);
    for(size_t i=0;i<120;++i)r[i]=receipt(c,l3::source_effect4_callback,plan.child_identities[i/5],plan.rng_return_addresses[i],1001+i);
    in.rng=r.data();in.rng_count=120;r[119].return_address^=4;held=c;
    check(!invoke(held,base,l3::source_effect4_callback,in).committed&&same(held,c),"last of120 receipt rollback");++count;
    r[119].return_address^=4;const auto first=std::numeric_limits<uint64_t>::max()-118u;
    for(size_t i=0;i<120;++i)r[i].global_ordinal=first+i;
    held=c;check(!invoke(held,base,l3::source_effect4_callback,in).committed&&same(held,c),"120 result overflowing scope atomic guard");++count;
    // Release identity and pending-mask semantics work in the high bank too.
    init(c,0,4);c.pool80.slots[63]=c.pool80.slots[0];c.pool80.slots[63].raw[1]=0xbf;c.pool80.slots[63].raw[13]=1;c.pool80.slots[63].raw[8]=1;
    c.pool80.used={{0u,1u}};in.rng=nullptr;in.rng_count=0;
    check(invoke(c,base+63u*72u,l3::source_effect4_callback,in).released&&*c.pool80_pending_release_800988c8[1]==1u,"high bank source release");++count;
    return count;
}
size_t oracle(const char* controls_path,const char* fixture_path){
    auto controls=authored_controls();std::ifstream cb(controls_path,std::ios::binary);std::array<uint8_t,42> data{};cb.read(reinterpret_cast<char*>(data.data()),42);check(cb.gcount()==42&&cb.peek()==EOF,"control carrier length");
    for(size_t i=0;i<11;++i)controls.sequence3[i]=uint16_t(data[i*2]|(uint16_t(data[i*2+1])<<8));for(size_t i=0;i<5;++i)for(size_t j=0;j<4;++j)controls.rows4[i][j]=data[22+i*4+j];
    std::ifstream f(fixture_path);check(bool(f),"fixture read");std::string line;std::getline(f,line);size_t cases=0;
    while(std::getline(f,line)){std::istringstream input(line);uint32_t id,sub,mode,param,state,clock,phase,tag,dir,used0,used1,seed,out0,out1,pending0,pending1,crc;std::string rr,changes;
        check(bool(input>>id>>sub>>mode>>param>>state>>clock>>phase>>tag>>dir>>used0>>used1>>seed>>rr>>changes>>out0>>out1>>pending0>>pending1>>crc),"fixture row");(void)seed;(void)crc;
        l3::SourceEventPhase5Context c;init(c,id,uint8_t(sub));auto& p=c.pool80.slots[0];p.raw[3]=uint8_t(param);p.raw[8]=uint8_t(state);p.raw[9]=uint8_t(clock);p.raw[10]=uint8_t(phase);p.raw[13]=uint8_t(mode);p.raw[14]=uint8_t(dir);p.raw[15]=uint8_t(tag);c.pool80.used={{used0,used1}};auto expected=c;
        if(changes!="-")for(const auto& t:split(changes,',')){const auto q=split(t,':');check(q.size()==2,"fixture patch");const auto offset=std::stoul(q[0]);expected.pool80.slots[offset/72].raw[offset%72]=uint8_t(std::stoul(q[1]));}
        std::array<l3::SourceEffect34RngReceipt,120> rng{};size_t n=0;const auto callback=sub==3?l3::source_effect3_callback:l3::source_effect4_callback;
        if(rr!="-")for(const auto& t:split(rr,',')){const auto q=split(t,':');check(q.size()==4&&n<120,"fixture receipt");rng[n]=receipt(c,callback,uint32_t(std::stoul(q[2])),uint32_t(std::stoul(q[0])),std::stoull(q[3]));rng[n++].v0=uint32_t(std::stoul(q[1]));}
        l3::SourceEffect34Inputs in;in.controls=&controls;in.rng=rng.data();in.rng_count=n;in.source_generation=1;
        const auto got=invoke(c,base,callback,in);if(!got.committed){std::cerr<<"case"<<id<<" status"<<unsigned(got.status)<<'\n';std::exit(1);}
        check(got.rng_consumed==n&&*c.pool80.used[0]==out0&&*c.pool80.used[1]==out1&&*c.pool80_pending_release_800988c8[0]==pending0&&*c.pool80_pending_release_800988c8[1]==pending1,"oracle scalar owners");
        for(size_t s=0;s<64;++s)if(c.pool80.slots[s].raw!=expected.pool80.slots[s].raw){std::cerr<<"case"<<id<<" slot"<<s<<" raw mismatch\n";std::exit(1);}++cases;
    }return cases;
}
}
void* operator new(size_t n){if(tracking)++allocation_count;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
int main(int argc,char** argv){const auto count=guards();size_t original=0;if(argc==3)original=oracle(argv[1],argv[2]);else check(argc==1,"usage controls fixture");std::cout<<"source_effect34 "<<count<<" guards, "<<original<<" original cases; zero callback allocations\n";}
