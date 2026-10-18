#include "legends3ds/source_scene_actor40_pose.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("input unavailable");return {std::istreambuf_iterator<char>(f),{}};}
std::array<uint8_t,1444> unhex(const std::string& s){assert(s.size()==2888);std::array<uint8_t,1444> out{};for(size_t i=0;i<out.size();++i)out[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
int main(int argc,char** argv){assert(argc==5);const auto profile=l3::load_source_event_phase5_profile(bytes(argv[1]));auto ebd=l3::parse_ebd_directory(bytes(argv[2]),0x8016c000);const auto witness=l3::bind_source_event_phase5_resources(profile,ebd);assert(witness.admitted());const auto resource=l3::decode_source_scene_actor40_phase5_resources(profile,witness,ebd);
 const auto table=bytes(argv[3]);assert(table.size()==16384);l3::ReferenceTables refs;for(size_t i=0;i<4096;++i){const auto s=uint16_t(table[i*4])|(uint16_t(table[i*4+1])<<8);const auto c=uint16_t(table[i*4+2])|(uint16_t(table[i*4+3])<<8);refs.trig[i]={int16_t(s<32768?s:int32_t(s)-65536),int16_t(c<32768?c:int32_t(c)-65536)};}
 std::ifstream f(argv[4]);std::string line;std::getline(f,line);size_t cases=0,guarded=0;
 while(std::getline(f,line)){std::istringstream row(line);int shift;std::string before,after;row>>shift>>before>>after;l3::SourceEventPhase5Bytes<1444> w;w.known.fill(1);w.raw=unhex(before);const auto expected=unhex(after);const auto saved=w;const auto n=allocations;
  const auto pose=l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(shift));assert(allocations==n&&pose.part_count==18);
  const auto result=l3::commit_source_scene_actor40_initial_pose_800322a8(w,resource,ebd,&refs,uint8_t(shift));assert(allocations==n&&result.committed&&w.raw[8]==saved.raw[8]);
  for(size_t i=0;i<w.raw.size();++i)if(w.known[i]&&w.raw[i]!=expected[i]){std::cerr<<"shift "<<shift<<" byte "<<i<<" got "<<unsigned(w.raw[i])<<" expected "<<unsigned(expected[i])<<'\n';return 2;}
  for(size_t i=372;i<384;++i)assert(!w.known[i]);
  auto missing=saved;const auto missing_before=missing;const auto held=l3::commit_source_scene_actor40_initial_pose_800322a8(missing,resource,ebd,&refs,std::nullopt);assert(!held.committed&&missing.raw==missing_before.raw&&missing.known==missing_before.known);++guarded;
  auto wrong=ebd;wrong.entries[0].variant^=1;const auto bad=l3::commit_source_scene_actor40_initial_pose_800322a8(missing,resource,wrong,&refs,uint8_t(shift));assert(!bad.committed&&missing.raw==missing_before.raw&&missing.known==missing_before.known);++guarded;
  auto expired=ebd;expired.source_payload.clear();for(size_t i=0;i<256;++i){const auto h=l3::commit_source_scene_actor40_initial_pose_800322a8(missing,resource,expired,&refs,uint8_t(shift));assert(!h.committed&&missing.raw==missing_before.raw&&missing.known==missing_before.known);++guarded;}
  ++cases;
 }
 std::cout<<"{\"status\":\"SOURCE_BACKED_PARTIAL\",\"owner_cases\":"<<cases<<",\"part_banks_compared\":"<<cases*18<<",\"atomic_guards\":"<<guarded<<",\"hot_allocations\":0,\"work08_acknowledged\":false,\"opaque_stack_lanes_invalidated\":12,\"hardware_tested\":false,\"retail_parity\":false}\n";
}
