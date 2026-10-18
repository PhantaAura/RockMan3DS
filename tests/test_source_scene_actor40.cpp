#include "legends3ds/source_scene_actor40.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using Work=l3::SourceEventPhase5Bytes<1444>;
namespace {
constexpr uint32_t base=0x8016c000;
void put16(std::vector<uint8_t>& b,size_t at,uint16_t n){b.at(at)=uint8_t(n);b.at(at+1)=uint8_t(n>>8);}
void work16(Work& w,size_t at,uint16_t n){w.raw[at]=uint8_t(n);w.raw[at+1]=uint8_t(n>>8);w.known[at]=w.known[at+1]=1;}
std::array<uint8_t,1444> unhex(const std::string& s){assert(s.size()==2888);std::array<uint8_t,1444> out{};for(size_t i=0;i<out.size();++i)out[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
template<class F>void rejects(F f){bool fail=false;try{f();}catch(const std::runtime_error&){fail=true;}assert(fail);}
std::vector<uint8_t> fixture(size_t parts=18){
 std::vector<uint8_t>b(0x1800);l3::patch_u32(b,0,1);b[4]=0x40;b[5]=5;
 l3::patch_u32(b,8,base+0x40);l3::patch_u32(b,12,base+0x1000);l3::patch_u32(b,16,base+0x1100);
 b[0x51]=uint8_t(parts);
 for(size_t i=0;i<parts;++i){const auto d=0x50+i*4;b[d]=uint8_t(i);if(i){b[d+1]=0;b[d+2]=uint8_t(i);}const auto p=0x214+i*20;b[p]=1;b[p+2]=3;l3::patch_u32(b,p+4,base+0x800);l3::patch_u32(b,p+12,0x03410004);l3::patch_u32(b,p+16,base+0x900);}
 for(size_t i=0;i<3;++i)l3::patch_u32(b,0xb0+i*4,base+0x200);
 b[0x800]=11;b[0x801]=12;b[0x808]=0;b[0x809]=1;b[0x80a]=2;put16(b,0x900,16);put16(b,0x908+2,uint16_t(-8));put16(b,0x910+4,24);
 l3::patch_u32(b,0x1000,base+0x1200);l3::patch_u32(b,0x1004,base+0x1320);l3::patch_u32(b,0x1100,base+0x1400);l3::patch_u32(b,0x1320,base+0x1480);
 b[0x1401]=3;b[0x1403]=255;for(size_t i=1;i<24;++i)put16(b,0x1200+i*8,uint16_t(i*16));return b;
}
}
int main(int argc,char** argv){assert(argc==2);std::ifstream f(argv[1]);assert(f);std::string line;std::getline(f,line);size_t cases=0;
 while(std::getline(f,line)){std::istringstream row(line);int seed;std::string packed,before,after;row>>seed>>packed>>before>>after;std::vector<uint16_t> words;std::istringstream parser(packed);std::string n;while(std::getline(parser,n,','))words.push_back(uint16_t(std::stoul(n)));Work w;w.known.fill(1);w.raw=unhex(before);
  const auto r=l3::consume_source_scene_actor40_script_80014bcc(w,words.data(),words.size());assert(r.committed&&w.raw==unhex(after));++cases;}
 assert(cases==28);
 Work held;held.known.fill(1);work16(held,844,0);const auto saved=held;const uint16_t loop[]={26,0};const uint16_t truncated[]={19,1};const uint16_t unknown[]={28,0,1};
 for(const auto spec:{std::pair<const uint16_t*,size_t>{loop,2},{truncated,2},{unknown,3}}){assert(!l3::consume_source_scene_actor40_script_80014bcc(held,spec.first,spec.second).committed);assert(held.raw==saved.raw&&held.known==saved.known);}
 held.known[844]=0;assert(!l3::consume_source_scene_actor40_script_80014bcc(held,loop,2).committed);
 for(const size_t count:{size_t(18),size_t(24)}){auto raw=fixture(count);auto e=l3::parse_ebd_directory(raw,base);l3::SourceSceneActor40Resources r;r.mesh=l3::decode_ebd_scene_actor40_mesh(e,0);r.pose=l3::decode_ebd_scene_actor40_pose(e,0,r.mesh,0,0);assert(r.mesh.model.parts.size()==count);
  const auto single=l3::decode_ebd_scene_actor40_pose_sequence(e,0,r.mesh,0,256*1024);
  assert(single.poses.size()==1&&single.records.size()==1&&single.poses[0].record_index==0);
  assert(single.records[0][3]==255&&single.decoded_bytes<=256*1024);
  rejects([&]{(void)l3::decode_ebd_scene_actor40_pose_sequence(e,0,r.mesh,0,single.decoded_bytes-1);});
  rejects([&]{(void)l3::decode_ebd_actor_pose_sequence(e,0,r.mesh,0,256*1024);});
  std::array<uint8_t,96> descriptor{};
  std::copy(raw.begin()+0x50,raw.begin()+0xb0,descriptor.begin());
  auto changed_descriptor=descriptor;changed_descriptor[16]=2;changed_descriptor[28]=3;
  auto changed_mesh=l3::decode_ebd_scene_actor40_mesh_variant(e,0,changed_descriptor,l3::ActorMeshTier::NEAR,256*1024);
  assert(changed_mesh.bindings[4].mesh_index==2&&changed_mesh.bindings[7].mesh_index==3);
  assert(changed_mesh.bindings[4].source_part_offset==r.mesh.bindings[2].source_part_offset);
  auto bad_descriptor=changed_descriptor;bad_descriptor[17]^=1;
  rejects([&]{(void)l3::decode_ebd_scene_actor40_mesh_variant(e,0,bad_descriptor,l3::ActorMeshTier::NEAR,256*1024);});
  rejects([&]{(void)l3::decode_ebd_scene_actor40_mesh_variant(e,0,changed_descriptor,l3::ActorMeshTier::NEAR,1);});
  r.descriptor_copy=descriptor;r.mesh_variant=l3::SourceSceneActor40MeshVariant{std::move(changed_mesh),changed_descriptor};
  Work draw;draw.known.fill(1);std::copy(descriptor.begin(),descriptor.end(),draw.raw.begin()+248);
  assert(l3::select_source_scene_actor40_mesh(draw,r)==&r.mesh);
  std::copy(changed_descriptor.begin(),changed_descriptor.end(),draw.raw.begin()+248);
  assert(l3::select_source_scene_actor40_mesh(draw,r)==&r.mesh_variant->mesh);
  draw.known[248]=0;assert(!l3::select_source_scene_actor40_mesh(draw,r));draw.known[248]=1;
  draw.raw[248]^=1;assert(!l3::select_source_scene_actor40_mesh(draw,r));
  auto sequence_raw=fixture(count);sequence_raw[0x1403]=0;
  sequence_raw[0x1408]=1;sequence_raw[0x1409]=4;sequence_raw[0x140b]=255;
  l3::patch_u32(sequence_raw,0x1324,base+0x1500);sequence_raw[0x1500]=32;
  const auto sequence_ebd=l3::parse_ebd_directory(sequence_raw,base);
  const auto two=l3::decode_ebd_scene_actor40_pose_sequence(sequence_ebd,0,r.mesh,0,256*1024);
  assert(two.poses.size()==2&&two.records.size()==2&&two.poses[1].record_index==1);
  assert(two.poses[1].frame_index==1&&two.poses[1].countdown==4&&two.poses[1].root_translation[0]==32);
  assert(two.poses[1].sequence_source_offset==0x1408&&two.poses[1].pose_source_offset==0x1500);
  auto bank_raw=sequence_raw;
  // Two explicit requests may legitimately select the same source window;
  // retain each request identity rather than substituting request zero.
  l3::patch_u32(bank_raw,0x1008,base+0x1320);
  l3::patch_u32(bank_raw,0x1104,base+0x1400);
  const auto bank_ebd=l3::parse_ebd_directory(bank_raw,base);
  const uint8_t requests[]={0,1};
  const auto bank=l3::decode_ebd_scene_actor40_animation_bank(bank_ebd,0,r.mesh,requests,2,256*1024);
  assert(bank.requests.size()==2&&bank.requests[1].request==1&&bank.requests[1].sequence.poses[1].request==1);
  assert(l3::find_actor_animation_request(bank,1)==&bank.requests[1]&&!l3::find_actor_animation_request(bank,2));
  rejects([&]{(void)l3::decode_ebd_scene_actor40_animation_bank(bank_ebd,0,r.mesh,requests,2,bank.decoded_bytes-1);});
  rejects([&]{(void)l3::decode_ebd_actor_animation_bank(bank_ebd,0,r.mesh,requests,2,256*1024);});
  const uint8_t duplicate[]={1,1};
  rejects([&]{(void)l3::decode_ebd_scene_actor40_animation_bank(bank_ebd,0,r.mesh,duplicate,2,256*1024);});
  rejects([&]{(void)l3::decode_ebd_scene_actor40_pose_sequence(sequence_ebd,0,r.mesh,0,two.decoded_bytes-1);});
  sequence_raw[0x140b]=0x84; // unsupported forward skip from record1 to record4.
  rejects([&]{(void)l3::decode_ebd_scene_actor40_pose_sequence(l3::parse_ebd_directory(sequence_raw,base),0,r.mesh,0,256*1024);});
  for(const auto& p:r.mesh.model.parts){assert(p.triangles[0].uv[0]==11&&p.triangles[0].uv[1]==12&&p.triangles[0].clut==0x341&&p.triangles[0].tpage==0x24);}
  Work w;w.known.fill(1);work16(w,344,4096);work16(w,346,4096);work16(w,348,4096);const auto m=l3::build_source_scene_actor40_pose_matrices(w,r);assert(m.part_count==count);assert(std::fabs(m.world[count-1].m[3]-float((count-1)*16))<0.001f);
  w.raw[6]=0x40;assert(l3::build_source_scene_actor40_pose_matrices(w,r).status==l3::SourceSceneActor40Status::NeedsPerPartScale);
  w.raw[6]=0;w.known[86]=0;assert(l3::build_source_scene_actor40_pose_matrices(w,r).status==l3::SourceSceneActor40Status::NeedsWorkFields);
  rejects([&]{l3::decode_ebd_actor_mesh(e,0);});rejects([&]{l3::decode_ebd_scene_actor40_mesh(e,0,l3::ActorMeshTier::NEAR,1);});
  raw[0x51]=25;e=l3::parse_ebd_directory(raw,base);rejects([&]{l3::decode_ebd_scene_actor40_mesh(e,0);});
  raw=fixture(count);raw[0x55]=2;e=l3::parse_ebd_directory(raw,base);rejects([&]{l3::decode_ebd_scene_actor40_mesh(e,0);});
  raw=fixture(count);raw[0x80a]=3;e=l3::parse_ebd_directory(raw,base);rejects([&]{l3::decode_ebd_scene_actor40_mesh(e,0);});
 }
 rejects([]{l3::load_source_scene_actor40_bootstrap(std::vector<uint8_t>(130));});
 std::cout<<"source class40:28 original script differential cases,18/24part decode and hierarchy,atomic guards passed\n";
}
