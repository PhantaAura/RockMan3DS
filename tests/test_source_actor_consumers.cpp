#include "legends3ds/source_actor_consumers.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int64_t number(std::istream& r){int64_t n;require(bool(r>>n),"consumer fixture truncated");return n;}
void guards(){
 l3::SourceCollisionRegistry q;uint16_t contact=123;q.count=q.identities.size();
 require(l3::register_source_collision_80051bac(q,0x8009cf88,3,contact)==l3::SourceConsumerStatus::FullHeld && contact==123,"full registry mutated contact");
 require(l3::register_source_collision_80051bac(q,0,0,contact)==l3::SourceConsumerStatus::SourceBackedPartial && contact==0,"inactive source clears contact even full");
 q={};contact=9;require(l3::register_source_collision_80051bac(q,123,3,contact)==l3::SourceConsumerStatus::InvalidIdentity && !q.count && contact==9,"unaligned identity accepted");
 l3::SourceShadowQueue shadows;l3::SourceShadowWork w;w.raw={{-101,-128,202}};w.heading56=0xabcd;
 require(l3::append_source_shadow_8002c8cc(shadows,w,60,512,0,std::nullopt)==l3::SourceConsumerStatus::NeedsFloor && !shadows.count,"shadow floor invented");
 shadows.count=64;require(l3::append_source_shadow_8002c8cc(shadows,w,60,512,0,std::nullopt)==l3::SourceConsumerStatus::SourceBackedPartial && shadows.count==64,"full source shadow queried floor");
 shadows={};w.work76=-1;require(l3::append_source_shadow_8002c8cc(shadows,w,60,512,0,std::nullopt)==l3::SourceConsumerStatus::SourceBackedPartial && shadows.records[0].point[1]==-128,"source nonzero76 ignored");
}
void registry(const char* path){
 std::ifstream f(path);require(bool(f),"registry fixture missing");std::string line;std::getline(f,line);size_t cases=0;
 while(std::getline(f,line)){std::istringstream r(line);l3::SourceCollisionRegistry q;q.count=size_t(number(r));const auto initial=q.count;const auto flags=uint8_t(number(r));uint16_t contact=uint16_t(number(r));
 const auto cursor=uint32_t(number(r));const auto after=uint16_t(number(r));const auto id=uint32_t(number(r));
 require(l3::register_source_collision_80051bac(q,0x8009cf88,flags,contact)==l3::SourceConsumerStatus::SourceBackedPartial,"admitted registry held");
 require(0x9000+q.count*4==cursor && contact==after && (q.count==initial || q.identities[initial]==id),"original registry append/clear mismatch");++cases;
 }require(cases==36,"registry differential coverage");std::cout<<"original registry cases="<<cases<<'\n';
}
void shadow(const char* path){
 std::ifstream f(path);require(bool(f),"shadow fixture missing");std::string line;std::getline(f,line);size_t cases=0;
 while(std::getline(f,line)){std::istringstream r(line);l3::SourceShadowQueue q;q.count=uint32_t(number(r));const auto index=q.count;l3::SourceShadowWork w;w.raw={{-101,int16_t(number(r)),202}};w.heading56=0xabcd;
 const auto floor=int32_t(number(r)),scale=int32_t(number(r));const auto type=uint32_t(number(r)),radius=uint32_t(number(r)),expectedcount=uint32_t(number(r));
 std::array<uint16_t,6> expected{};for(auto& v:expected)v=uint16_t(number(r));
 require(l3::append_source_shadow_8002c8cc(q,w,radius,scale,type,floor)==l3::SourceConsumerStatus::SourceBackedPartial && q.count==expectedcount,"original shadow append/count mismatch");
 if(index<64){const auto& sh=q.records[index];require(std::array<uint16_t,6>{{uint16_t(sh.point[0]),uint16_t(sh.point[1]),uint16_t(sh.point[2]),sh.heading_scale,sh.radius_type,sh.height_delta}}==expected,"original shadow16bit packing mismatch");}++cases;
 }require(cases==288,"shadow differential coverage");std::cout<<"original shadow cases="<<cases<<'\n';
}
}
int main(int argc,char** argv){try{guards();if(argc>1)registry(argv[1]);if(argc>2)shadow(argv[2]);std::cout<<"PASS bounded source collision registry/shadow consumers\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
