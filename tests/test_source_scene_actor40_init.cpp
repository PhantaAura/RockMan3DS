#include "legends3ds/source_scene_actor40_init.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
using Work=l3::SourceEventPhase5Bytes<1444>;
int main(int argc,char** argv){assert(argc==2);std::ifstream f(argv[1]);assert(f);std::string line;std::getline(f,line);size_t cases=0;
 while(std::getline(f,line)){std::istringstream row(line);unsigned seed;std::array<uint16_t,3> previous{},primary{};uint32_t expected;row>>seed;for(auto& n:previous)row>>n;for(auto& n:primary)row>>n;row>>expected;
  L3PlayerCompatWork player; l3::SourceEventPlayerKnown known;for(size_t i=0;i<known.size();++i){player.bytes[i]=uint8_t(seed*37+i*17);known[i]=uint8_t((seed+i)%3==0);}
  const auto before=player;const auto known_before=known;Work w;w.known.fill(1);for(size_t i=0;i<3;++i){w.raw[20+i*2]=uint8_t(previous[i]);w.raw[21+i*2]=uint8_t(previous[i]>>8);}
  const auto n=allocations;const auto capture=l3::capture_source_scene_actor40_previous_8010d824(w);assert(capture&&*capture==previous&&allocations==n);
  l3::SourceSceneActor40PlayerMirrorReceipt receipt;receipt.previous_2c=*capture;receipt.primary_14=primary;receipt.admitted=true;receipt.source_tag=0x40;
  assert(l3::apply_source_scene_actor40_player_mirrors_8010d824(receipt,player,known)&&allocations==n);
  assert(l3::crc32(player.bytes,sizeof(player.bytes))==expected);
  for(size_t i=0;i<known.size();++i){const bool written=(i>=0x14&&i<0x1a)||(i>=0x2c&&i<0x32);if(written)assert(known[i]==1);else assert(known[i]==known_before[i]&&player.bytes[i]==before.bytes[i]);}
  w.known[24]=0;assert(!l3::capture_source_scene_actor40_previous_8010d824(w));
  receipt.admitted=false;const auto saved=player;const auto saved_known=known;assert(!l3::apply_source_scene_actor40_player_mirrors_8010d824(receipt,player,known));assert(std::equal(std::begin(saved.bytes),std::end(saved.bytes),std::begin(player.bytes))&&known==saved_known);
  ++cases;
 }
 assert(cases==32);
 L3PlayerCompatWork gate_player{};l3::SourceEventPlayerKnown gate_known{};
 gate_known[0]=gate_known[6]=1;const auto gate_allocations=allocations;
 for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){
  gate_player.bytes[0]=uint8_t(a);gate_player.bytes[6]=uint8_t(b);
  const auto gate=l3::source_player_update_gate_8003c07c(gate_player,gate_known);
  assert(gate&&*gate==((a&b&1u)!=0)&&allocations==gate_allocations);
 }
 for(unsigned missing=0;missing<2;++missing){gate_known[missing?6:0]=0;
  assert(!l3::source_player_update_gate_8003c07c(gate_player,gate_known));
  gate_known[missing?6:0]=1;
 }
 Work w;w.known.fill(1);w.raw[1]=0x40;w.raw[2]=5;w.raw[4]=1;l3::SourceSceneActor40Resources resource;l3::EbdDirectory ebd;l3::SourceSceneActor40InitializerGlobals globals;globals.word_80122470=123;globals.word_80122474=456;
 const auto saved=w;const auto n=allocations;const auto result=l3::initialize_source_scene_actor40_wrapper_8010d824(w,resource,ebd,nullptr,std::nullopt,globals);assert(!result.committed&&!result.player_mirrors&&w.raw==saved.raw&&w.known==saved.known&&globals.word_80122470==123&&globals.word_80122474==456&&allocations==n);
 w.raw[8]=1;const auto later=w;const auto held=l3::initialize_source_scene_actor40_wrapper_8010d824(w,resource,ebd,nullptr,std::nullopt,globals);assert(!held.committed&&held.status==l3::SourceSceneActor40InitializerStatus::NeedsFreshInitializer&&w.raw==later.raw&&w.known==later.known);
 std::cout<<"class40 init:32 original halfword-copy cases,65536 dispatch masks, exact player byte/knowledge isolation, atomic resource/later holds\n";
}
