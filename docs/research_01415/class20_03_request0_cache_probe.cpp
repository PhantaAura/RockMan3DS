#include "legends3ds/actor_runtime.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <set>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("expected existing cooked owner EBD");
  std::ifstream file(argv[1],std::ios::binary);
  if(!file)throw std::runtime_error("owner EBD unavailable");
  std::vector<uint8_t>bytes{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
  const auto ebd=l3::load_l3ebd_directory(bytes);
  l3::StageObjectList list;
  for(size_t i=0;i<3;++i){l3::StageObjectRecord r;r.id=1;r.object_class=0x20;
   r.raw[0]=1;r.raw[1]=0x20;r.raw[2]=3;r.words_02_to_0a[0]=3;list.records.push_back(r);}
  auto area=l3::spawn_imported_actors(l3::build_fresh_stage_runtime_area(list,ebd),ebd);
  if(area.actors.size()!=3)throw std::runtime_error("expected three real bound class03 rows");
  l3::bind_imported_actor_request0_caches(area,ebd);
  const auto&cache=*area.actors[0].request0_pose_cache;
  std::set<uint32_t>pose_offsets;size_t distinct=0;
  for(size_t i=0;i<cache.poses.size();++i){pose_offsets.insert(cache.poses[i].pose_source_offset);bool unique=true;
   for(size_t j=0;j<i;++j)if(cache.poses[i].root_translation==cache.poses[j].root_translation&&
    cache.poses[i].rotations==cache.poses[j].rotations&&cache.poses[i].base_translation==cache.poses[j].base_translation)unique=false;
   distinct+=unique;}
  for(unsigned t=1;t<=480;++t){const auto result=l3::tick_imported_actor_request0(area);
   if(result.admitted!=3||result.pose_changed!=3||result.unsupported||area.actors[0].pose->record_index!=t%8)
    throw std::runtime_error("owner cache tick mismatch");}
  const auto cache_bytes=area.decoded_pose_cache_bytes,total=area.decoded_resource_bytes;
  std::weak_ptr<const l3::ActorPoseSequenceCache>life=area.actors[0].request0_pose_cache;
  const size_t count=cache.poses.size();l3::unload_imported_actors(area);
  std::cout<<"{\"schema\":\"class20-03-request0-cache-probe/1\",\"status\":\"SOURCE_BACKED_PARTIAL\",\"instances\":3,\"unique_cache\":1,\"reachable_records\":"<<count
   <<",\"distinct_pose_windows\":"<<pose_offsets.size()<<",\"distinct_decoded_poses\":"<<distinct
   <<",\"decoded_cache_elements\":"<<cache_bytes<<",\"decoded_total_elements\":"<<total
   <<",\"ticks\":480,\"actor_steps\":1440,\"unload_released_cache\":"<<(life.expired()?"true":"false")
   <<",\"cadence\":\"CANDIDATE\",\"pause_ownership\":\"UNKNOWN\",\"allocation_first_tick\":\"UNKNOWN\",\"full_ai\":false}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
