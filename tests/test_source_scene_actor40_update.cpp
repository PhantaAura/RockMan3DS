#include "legends3ds/source_scene_actor40_update.h"
#include "legends3ds/source_event_pipeline.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
namespace {
using Work=l3::SourceEventPhase5Bytes<1444>;
std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<uint8_t> unhex(const std::string& s){assert(s.size()%2==0);std::vector<uint8_t> out(s.size()/2);for(size_t i=0;i<out.size();++i)out[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
void put32(Work& w,size_t at,uint32_t value){for(size_t i=0;i<4;++i){w.raw[at+i]=uint8_t(value>>(i*8));w.known[at+i]=1;}}
uint16_t get16(const Work& w,size_t at){return uint16_t(w.raw[at])|(uint16_t(w.raw[at+1])<<8);}
template<size_t Count,size_t N>void fill_pool(l3::SourceEventPhase5Pool<Count,N>& pool,const std::vector<uint8_t>& raw){assert(raw.size()==Count*N);for(size_t i=0;i<Count;++i){std::copy(raw.begin()+i*N,raw.begin()+(i+1)*N,pool.slots[i].raw.begin());pool.slots[i].known.fill(1);}}
template<size_t Count,size_t N>bool check_pool(const l3::SourceEventPhase5Pool<Count,N>& pool,const std::vector<uint8_t>& raw){for(size_t i=0;i<Count;++i)if(!std::equal(raw.begin()+i*N,raw.begin()+(i+1)*N,pool.slots[i].raw.begin()))return false;return true;}
}
int main(int argc,char** argv){
 if(argc==1){Work w;w.raw.fill(0xa7);const auto untouched=w;assert(!l3::project_source_scene_actor40_fixed_position_80030150(w));assert(w.raw==untouched.raw&&w.known==untouched.known);
  for(uint32_t high:{0u,1u,0x7fffu,0x8000u,0xffffu})for(uint32_t low:{0u,0x7fffu,0x8000u,0xffffu}){
   for(size_t i=0;i<3;++i)put32(w,28+i*4,(high<<16)|low);const auto before=allocations;
   assert(l3::project_source_scene_actor40_fixed_position_80030150(w));for(size_t i=0;i<3;++i)assert(get16(w,20+i*2)==uint16_t(high+(low>>15)));assert(allocations==before);}
  std::cout<<"class40 fixed-position guard and signed-round boundaries passed\n";return 0;}
 assert(argc==6);const auto profile=l3::load_source_event_phase5_profile(bytes(argv[1]));auto ebd=l3::parse_ebd_directory(bytes(argv[2]),0x8016c000);const auto witness=l3::bind_source_event_phase5_resources(profile,ebd);assert(witness.admitted());auto resource=l3::decode_source_scene_actor40_phase5_resources(profile,witness,ebd);
 const uint8_t requests[]={16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,34,48};l3::prepare_source_scene_actor40_request_bank(resource,ebd,requests,sizeof(requests),512*1024);
 const auto table=bytes(argv[3]);assert(table.size()==16384);l3::ReferenceTables refs;for(size_t i=0;i<4096;++i){const auto s=uint16_t(table[i*4])|(uint16_t(table[i*4+1])<<8);const auto c=uint16_t(table[i*4+2])|(uint16_t(table[i*4+3])<<8);refs.trig[i]={int16_t(s<32768?s:int32_t(s)-65536),int16_t(c<32768?c:int32_t(c)-65536)};}
 std::ifstream f(argv[4]);assert(f);std::string line;std::getline(f,line);size_t cases=0,guards=0;const bool guard=std::string(argv[5])=="guards";
 while(std::getline(f,line)){std::istringstream row(line);unsigned seed,state,sub,request,frame;uint32_t used80a,used80b,usede0,bpa,bpb,apa,apb;std::string event,before,beforee0,before80,after,aftere0,after80;
  row>>seed>>state>>sub>>request>>frame>>used80a>>used80b>>usede0>>event>>before>>beforee0>>before80>>bpa>>bpb>>after>>aftere0>>after80>>apa>>apb;assert(row);
  const auto in=unhex(before),expected=unhex(after),ein=unhex(event),e0in=unhex(beforee0),e0out=unhex(aftere0),p80in=unhex(before80),p80out=unhex(after80);assert(in.size()==1444&&expected.size()==1444&&ein.size()==168);
  Work w;w.known.fill(1);std::copy(in.begin(),in.end(),w.raw.begin());l3::SourceEventWork event_work;event_work.known.fill(1);std::copy(ein.begin(),ein.end(),event_work.raw.begin());
  l3::SourceEventPhase5Context scene;fill_pool(scene.pool_e0,e0in);fill_pool(scene.pool80,p80in);scene.pool_e0.used[0]=usede0;scene.pool80.used={{used80a,used80b}};scene.pool80_pending_release_800988c8={{bpa,bpb}};
  const auto saved=w;const auto saved_scene=scene;const auto n=allocations;
  const auto result=l3::update_source_scene_actor40_prefix_8010d824(w,resource,ebd,&refs,uint8_t(0),&event_work,&scene);
  if(!result.committed||allocations!=n){std::cerr<<"case "<<seed<<" state "<<state<<"/"<<sub<<" held "<<unsigned(result.status)<<'\n';return 2;}
  for(size_t i=0;i<w.raw.size();++i)if(w.known[i]&&w.raw[i]!=expected[i]){std::cerr<<"case "<<seed<<" state "<<state<<"/"<<sub<<" work byte "<<i<<" got "<<unsigned(w.raw[i])<<" expected "<<unsigned(expected[i])<<'\n';return 3;}
  if(!check_pool(scene.pool_e0,e0out)||!check_pool(scene.pool80,p80out)||scene.pool80_pending_release_800988c8[0]!=apa||scene.pool80_pending_release_800988c8[1]!=apb){std::cerr<<"case "<<seed<<" external journal mismatch\n";return 4;}
  if(guard){
   for(unsigned kind=0;kind<7;++kind){if(kind>=5&&!result.request_reset&&!result.sequence_advanced)continue;auto bad=saved;auto badscene=saved_scene;auto badevent=event_work;auto changed=ebd;const l3::ReferenceTables* rp=&refs;std::optional<uint8_t> sh=uint8_t(0);
    if(kind==0)bad.known[28]=0;else if(kind==1)bad.raw[9]=255;else if(kind==2)changed.source_payload.clear();else if(kind==3)bad.known[9]=0;else if(kind==4)bad.raw[8]=0;else if(kind==5)bad.raw[144]^=1;else bad.raw[160]^=1;
    const auto b=bad;const auto guard_allocations=allocations;const auto h=l3::update_source_scene_actor40_prefix_8010d824(bad,resource,changed,rp,sh,&badevent,&badscene);assert(!h.committed&&bad.raw==b.raw&&bad.known==b.known&&check_pool(badscene.pool_e0,e0in)&&check_pool(badscene.pool80,p80in)&&badscene.pool80_pending_release_800988c8==saved_scene.pool80_pending_release_800988c8&&allocations==guard_allocations);++guards;
   }
   if(result.external_stores){auto bad=saved;auto badscene=saved_scene;auto badevent=event_work;badevent.known.fill(0);const auto guard_allocations=allocations;const auto h=l3::update_source_scene_actor40_prefix_8010d824(bad,resource,ebd,&refs,uint8_t(0),&badevent,&badscene);assert(!h.committed&&bad.raw==saved.raw&&bad.known==saved.known&&check_pool(badscene.pool_e0,e0in)&&check_pool(badscene.pool80,p80in)&&allocations==guard_allocations);++guards;}
  }
  ++cases;
 }
 assert(cases>50);std::cout<<"{\"status\":\"SOURCE_BACKED_PARTIAL\",\"original_prefix_cases\":"<<cases<<",\"atomic_guards\":"<<guards<<",\"hot_allocations\":0,\"retail_parity\":false}\n";
}
