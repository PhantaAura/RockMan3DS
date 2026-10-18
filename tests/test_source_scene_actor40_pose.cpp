#include "legends3ds/source_scene_actor40_pose.h"
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
void put16(Work& w,size_t at,int32_t value){const auto n=uint16_t(value);w.raw[at]=uint8_t(n);w.raw[at+1]=uint8_t(n>>8);}
uint8_t unhex(const std::string& s,size_t at){return uint8_t(std::stoul(s.substr(at*2,2),nullptr,16));}
}
int main(int argc,char** argv){assert(argc==2);std::ifstream file(argv[1]);assert(file);std::string line;std::getline(file,line);size_t cases=0,guards=0;
 while(std::getline(file,line)){std::istringstream row(line);std::array<int32_t,30> spec{};for(auto& n:spec)row>>n;std::string outputs;row>>outputs;assert(outputs.size()==208);
  l3::ReferenceTables refs;for(size_t i=0;i<refs.trig.size();++i){auto& t=refs.trig[i];t.sin_q12=int16_t(int32_t((i*37+size_t(spec[0])*311)&65535)-32768);t.cos_q12=int16_t(int32_t((i*19+size_t(spec[0])*137)&65535)-32768);}refs.trig[0]={0,4096};
  Work w;w.known.fill(1);w.raw[1]=0x40;w.raw[2]=5;w.raw[6]=0x23;put16(w,86,spec[2]);
  l3::SourceSceneActor40Resources resource;resource.mesh.bindings.resize(3);resource.mesh.model.parts.resize(3);resource.pose.base_translation.resize(3);resource.descriptor_copy[1]=3;
  for(size_t i=0;i<3;++i){put16(w,344+i*2,spec[3+i]);put16(w,20+i*2,spec[6+i]);resource.pose.root_translation[i]=int16_t(spec[9+i]);auto& b=resource.mesh.bindings[i];b.parent=i?int8_t(i-1):-1;b.rotation_translation_index=uint8_t(i);
   if(i){resource.descriptor_copy[i*4+1]=uint8_t(i-1);resource.descriptor_copy[i*4+2]=uint8_t(i);}
   for(size_t k=0;k<3;++k){resource.pose.rotations[i][k]=uint16_t(spec[12+i*3+k]);resource.pose.base_translation[i][k]=int16_t(spec[21+i*3+k]);}}
  std::copy(resource.descriptor_copy.begin(),resource.descriptor_copy.end(),w.raw.begin()+248);const auto saved=w;const auto n=allocations;
  const auto pose=l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(spec[1]));assert(allocations==n&&pose.part_count==3&&w.raw==saved.raw&&w.known==saved.known);
  std::array<uint8_t,104> generated{};std::copy(pose.scaled_heading_160.begin(),pose.scaled_heading_160.end(),generated.begin());
  for(size_t i=0;i<3;++i){for(size_t k=0;k<3;++k){const auto value=uint16_t(pose.parts[i].position[k]);generated[20+i*8+k*2]=uint8_t(value);generated[21+i*8+k*2]=uint8_t(value>>8);}std::copy(pose.parts[i].matrix.begin(),pose.parts[i].matrix.end(),generated.begin()+44+i*20);}
  for(size_t i=0;i<generated.size();++i){if(generated[i]!=unhex(outputs,i)){std::cerr<<"case "<<spec[0]<<" byte "<<i<<" got "<<unsigned(generated[i])<<" expected "<<unsigned(unhex(outputs,i))<<'\n';return 2;}}
  const auto before_present=allocations;const auto presented=l3::present_source_scene_actor40_integer_pose(pose);assert(presented.part_count==3&&allocations==before_present);
  using S=l3::SourceSceneActor40PoseStatus;
  assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,nullptr,uint8_t(spec[1])).status==S::NeedsReference);++guards;
  assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,std::nullopt).status==S::NeedsScratchShift);++guards;
  assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(3)).status==S::UnsupportedScratchShift);++guards;
  w.raw[6]|=0x40;assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(spec[1])).status==S::NeedsPerPartScale);++guards;w=saved;
  w.raw[172]=1;assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(spec[1])).status==S::NeedsPose);++guards;w=saved;
  w.known[86]=0;assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(spec[1])).status==S::NeedsWorkFields);++guards;w=saved;
  resource.mesh.bindings[1].parent=2;assert(l3::produce_source_scene_actor40_integer_pose_8003346c(w,resource,&refs,uint8_t(spec[1])).status==S::InvalidHierarchy);++guards;
  ++cases;
 }
 assert(cases==36);std::cout<<"class40 integer pose: "<<cases<<" original authored cases, "<<guards<<" guards, zero hot allocations\n";
}
