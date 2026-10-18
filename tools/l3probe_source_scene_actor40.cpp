#include "legends3ds/source_scene_actor40.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <cstdlib>
#include <new>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("input unavailable");return {std::istreambuf_iterator<char>(f),{}};}
std::array<uint8_t,1444> unhex(const std::string& s){assert(s.size()==2888);std::array<uint8_t,1444> out{};for(size_t i=0;i<out.size();++i)out[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
int main(int argc,char** argv){assert(argc==4);const auto profile=l3::load_source_event_phase5_profile(bytes(argv[1]));auto ebd=l3::parse_ebd_directory(bytes(argv[2]),0x8016c000);
 const auto witness=l3::bind_source_event_phase5_resources(profile,ebd);assert(witness.admitted());
 const auto resource=l3::decode_source_scene_actor40_phase5_resources(profile,witness,ebd);assert(resource.mesh.bindings.size()==18);
 size_t vertices=0,triangles=0,quads=0;std::set<uint32_t> materials;for(size_t i=0;i<resource.mesh.model.parts.size();++i){const auto& p=resource.mesh.model.parts[i];vertices+=p.vertices.size();triangles+=p.triangles.size();quads+=p.quads.size();materials.insert(resource.mesh.bindings[i].material_word);}
 std::ifstream f(argv[3]);std::string line;std::getline(f,line);size_t cases=0,holds=0;while(std::getline(f,line)){std::istringstream row(line);int seed;std::string before,after;row>>seed>>before>>after;
  l3::SourceEventPhase5Bytes<1444> w;w.known.fill(1);w.raw=unhex(before);const auto expected=unhex(after);const auto n=allocations;
  const auto result=l3::initialize_source_scene_actor40_prefix_8010c930(w,resource,ebd);assert(allocations==n);assert(result.committed);
  if(w.raw!=expected){for(size_t i=0;i<w.raw.size();++i)if(w.raw[i]!=expected[i])std::cerr<<"case "<<seed<<" offset "<<i<<" got "<<unsigned(w.raw[i])<<" expected "<<unsigned(expected[i])<<'\n';return 2;}
  const auto m=l3::build_source_scene_actor40_pose_matrices(w,resource);assert(m.part_count==18);assert(allocations==n);++cases;
  const auto saved=w;auto changed=ebd;changed.entries[0].subtype^=1;
  const auto wrong_metadata=l3::initialize_source_scene_actor40_prefix_8010c930(w,resource,changed);assert(!wrong_metadata.committed&&w.raw==saved.raw&&w.known==saved.known);
  auto missing=w;missing.known[4]=0;const auto missing_before=missing;const auto missing_field=l3::initialize_source_scene_actor40_prefix_8010c930(missing,resource,ebd);assert(!missing_field.committed&&missing.raw==missing_before.raw&&missing.known==missing_before.known);
  auto wrong=w;wrong.raw[4]=0;const auto wrong_before=wrong;const auto wrong_route=l3::initialize_source_scene_actor40_prefix_8010c930(wrong,resource,ebd);assert(!wrong_route.committed&&wrong.raw==wrong_before.raw&&wrong.known==wrong_before.known);
  auto expired=ebd;expired.source_payload.clear();for(size_t i=0;i<256;++i){const auto held=l3::initialize_source_scene_actor40_prefix_8010c930(w,resource,expired);assert(!held.committed&&w.raw==saved.raw&&w.known==saved.known);++holds;}
 }
 std::cout<<"{\"status\":\"SOURCE_BACKED_PREFIX\",\"owner_prefix_byte_comparisons\":"<<cases<<",\"expired_owner_holds\":"<<holds<<",\"parts\":18,\"vertices\":"<<vertices<<",\"triangles\":"<<triangles<<",\"quads\":"<<quads<<",\"material_words\":"<<materials.size()<<",\"decoded_mesh_bytes\":"<<resource.mesh.footprint.decoded_bytes<<",\"hot_allocations\":0,\"hardware_tested\":false,\"retail_parity\":false}\n";
}
