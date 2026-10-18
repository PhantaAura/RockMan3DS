#include "legends3ds/source_scene_actor40_init.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("input unavailable");return {std::istreambuf_iterator<char>(f),{}};}
std::vector<uint8_t> unhex(const std::string& s){std::vector<uint8_t> out(s.size()/2);for(size_t i=0;i<out.size();++i)out[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
int main(int argc,char** argv){assert(argc==5||argc==6);const auto profile=l3::load_source_event_phase5_profile(bytes(argv[1]));auto ebd=l3::parse_ebd_directory(bytes(argv[2]),0x8016c000);const auto witness=l3::bind_source_event_phase5_resources(profile,ebd);assert(witness.admitted());const auto resource=l3::decode_source_scene_actor40_phase5_resources(profile,witness,ebd);
 const auto table=bytes(argv[3]);assert(table.size()==16384);l3::ReferenceTables refs;for(size_t i=0;i<4096;++i){const auto s=uint16_t(table[i*4])|(uint16_t(table[i*4+1])<<8);const auto c=uint16_t(table[i*4+2])|(uint16_t(table[i*4+3])<<8);refs.trig[i]={int16_t(s<32768?s:int32_t(s)-65536),int16_t(c<32768?c:int32_t(c)-65536)};}
 std::ifstream f(argv[4]);assert(f);std::string line;std::getline(f,line);size_t cases=0,guards=0;
 while(std::getline(f,line)){std::istringstream row(line);unsigned seed,shift;uint32_t before70,before74;std::string before,after,player_before,player_after;row>>seed>>shift>>before70>>before74>>before>>after>>player_before>>player_after;
  const auto in=unhex(before),expected=unhex(after),pin=unhex(player_before),pexpected=unhex(player_after);assert(in.size()==1444&&pin.size()==L3_PLAYER_COMPAT_WORK_BYTES);
  l3::SourceEventPhase5Bytes<1444> w;w.known.fill(1);std::copy(in.begin(),in.end(),w.raw.begin());L3PlayerCompatWork player;std::copy(pin.begin(),pin.end(),std::begin(player.bytes));l3::SourceEventPlayerKnown known;for(size_t i=0;i<known.size();++i)known[i]=uint8_t((seed+i)%3==0);const auto before_known=known;
  const auto saved=w;l3::SourceSceneActor40InitializerGlobals globals;globals.word_80122470=before70;globals.word_80122474=before74;const auto n=allocations;
  const auto result=l3::initialize_source_scene_actor40_wrapper_8010d824(w,resource,ebd,&refs,uint8_t(shift),globals);assert(result.committed&&result.player_mirrors&&w.raw[8]==1&&allocations==n);
  assert(globals.word_80122470==0x8011c338&&globals.word_80122474==0x8011c3a4);
  for(size_t i=0;i<w.raw.size();++i)if(w.known[i]&&w.raw[i]!=expected[i]){std::cerr<<"case "<<seed<<" byte "<<i<<" got "<<unsigned(w.raw[i])<<" expected "<<unsigned(expected[i])<<'\n';return 2;}
  for(size_t i=372;i<384;++i)assert(!w.known[i]);
  assert(l3::apply_source_scene_actor40_player_mirrors_8010d824(*result.player_mirrors,player,known));assert(std::equal(pexpected.begin(),pexpected.end(),std::begin(player.bytes))&&allocations==n);
  for(size_t i=0;i<known.size();++i){const bool written=(i>=0x14&&i<0x1a)||(i>=0x2c&&i<0x32);assert(known[i]==(written?1:before_known[i]));}
  const auto completed=w;const auto again=l3::initialize_source_scene_actor40_wrapper_8010d824(w,resource,ebd,&refs,uint8_t(shift),globals);assert(!again.committed&&!again.player_mirrors&&w.raw==completed.raw&&w.known==completed.known);++guards;
  for(unsigned kind=0;kind<7;++kind){auto bad=saved;auto changed=ebd;l3::SourceSceneActor40InitializerGlobals g;g.word_80122470=before70;g.word_80122474=before74;const l3::ReferenceTables* r=&refs;std::optional<uint8_t> sh=uint8_t(shift);
   if(kind==0)r=nullptr;else if(kind==1)sh.reset();else if(kind==2)changed.source_payload.clear();else if(kind==3)changed.entries[0].subtype^=1;else if(kind==4)bad.known[20]=0;else if(kind==5)bad.raw[6]|=0x40;else sh=3;
   const auto before_bad=bad;const auto h=l3::initialize_source_scene_actor40_wrapper_8010d824(bad,resource,changed,r,sh,g);assert(!h.committed&&!h.player_mirrors&&bad.raw==before_bad.raw&&bad.known==before_bad.known&&g.word_80122470==before70&&g.word_80122474==before74);++guards;
  }
  ++cases;
 }
 size_t later_cases=0;
 if(argc==6){std::ifstream later(argv[5]);assert(later);std::getline(later,line);
  while(std::getline(later,line)){std::istringstream row(line);unsigned tick,shift;std::string before,after;row>>tick>>shift>>before>>after;
   const auto in=unhex(before),expected=unhex(after);assert(in.size()==1444&&expected.size()==1444);
   l3::SourceEventPhase5Bytes<1444> w;w.known.fill(1);std::copy(in.begin(),in.end(),w.raw.begin());
   const auto n=allocations;const auto result=l3::advance_source_scene_actor40_pose_80032488(w,resource,ebd,&refs,uint8_t(shift));
   if(!result.committed||allocations!=n){std::cerr<<"later tick "<<tick<<" held status "<<unsigned(result.status)<<'\n';return 3;}
   const auto check=[&](size_t at,size_t count){
    for(size_t i=at;i<at+count;++i){
     if(w.raw[i]!=expected[i]){
      std::cerr<<"later tick "<<tick<<" source-owned byte "<<i<<" got "<<unsigned(w.raw[i])<<" expected "<<unsigned(expected[i])<<'\n';
      return false;
     }
    }
    return true;
   };
   if(!check(152,4)||!check(164,4)||!check(352,20))return 4;
   for(size_t part=0;part<result.part_count;++part)if(!check(384+part*8,6)||!check(576+part*32,20))return 4;
   ++later_cases;
  }
 }
 std::cout<<"{\"status\":\"SOURCE_BACKED_PARTIAL\",\"complete_wrapper_cases\":"<<cases<<",\"part_banks_compared\":"<<cases*18<<",\"later_sequence_cases\":"<<later_cases<<",\"atomic_guards\":"<<guards<<",\"hot_allocations\":0,\"work08_acknowledged_after_complete_initializer\":true,\"player_bytes_written\":12,\"fixed_heading_state_writes\":0,\"opaque_stack_lanes_invalidated\":12,\"hardware_tested\":false,\"retail_parity\":false}\n";
}
