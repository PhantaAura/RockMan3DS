#include "legends3ds/source_prop_lift.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/l3map.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <sstream>
#include <string>
using namespace l3;
namespace {bool watched=false;size_t allocations=0;}
void* operator new(size_t n){if(watched)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace {
void check(bool b,const std::string& why){if(!b){std::cerr<<why<<'\n';std::exit(1);}}
std::vector<uint8_t> file(const char* path){std::ifstream f(path,std::ios::binary);check(bool(f),"fixture open");return {std::istreambuf_iterator<char>(f),{}};}
std::vector<uint8_t> hex(const std::string& s){std::vector<uint8_t> b(s.size()/2);for(size_t i=0;i<b.size();++i)b[i]=uint8_t(std::stoul(s.substr(2*i,2),nullptr,16));return b;}
template<class R>void raw(R& r,const std::string& text){const auto b=hex(text);check(b.size()==r.raw.size(),"hex fixture width");for(size_t i=0;i<b.size();++i)r.raw[i]=b[i];}
template<class R>void put(R& r,size_t o,size_t n,uint32_t v){for(size_t i=0;i<n;++i){r.raw[o+i]=uint8_t(v>>(8*i));r.known[o+i]=1;}}
template<class R>void authored(R& r,size_t seed){for(size_t i=0;i<r.raw.size();++i)r.raw[i]=uint8_t(seed*37+i*17+11);}
uint16_t u16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
template<class P>bool samepool(const P& a,const P& b){if(a.used!=b.used)return false;for(size_t i=0;i<a.slots.size();++i)if(a.slots[i].raw!=b.slots[i].raw||a.slots[i].known!=b.slots[i].known)return false;return true;}
bool same(const SourceEventPhase5Context& a,const SourceEventPhase5Context& b){return samepool(a.pool_e0,b.pool_e0)&&samepool(a.pool40,b.pool40)&&samepool(a.pool80,b.pool80)&&a.pool80_pending_release_800988c8==b.pool80_pending_release_800988c8;}
SourcePropLiftResult tick(SourceEventPhase5Context& c,const SourceEventWork& e,uint32_t id,const SourcePropLiftInputs& in){const auto n=allocations;watched=true;const auto r=consume_source_prop_lift_80113280(c,e,id,in);watched=false;check(n==allocations,"hot callback allocated heap");return r;}
void put32(std::vector<uint8_t>& b,size_t at,uint32_t v){for(size_t i=0;i<4;++i)b[at+i]=uint8_t(v>>(8*i));}
bool rejected(const std::vector<uint8_t>& b){try{(void)load_source_prop_lift_profile(b);return false;}catch(const std::exception&){return true;}}
}
int main(int argc,char** argv){
 check(argc==2||argc==3,"arguments: authored-fixture [private-owner-L3PL]");std::ifstream f(argv[1]);check(bool(f),"fixture open");std::string line;std::getline(f,line);size_t cases=0,guards=0;
 SourcePropLiftCurveReceipt curves;curves.admitted=true;curves.source_table_identity=kSourcePropLiftCurveTable;curves.curve_identities={{0x8011c958u,0x8011c998u,0x8011c9d8u}};
 for(size_t s=0;s<3;++s)for(size_t n=0;n<64;++n)curves.windows[s][n]=uint8_t(17*s+3*n+5);
 while(std::getline(f,line)){
  if(line.empty())continue;
  std::istringstream row(line);size_t number,ps,as;unsigned state,sub,selector,frame;int bone,beforeY,child;uint32_t fixed,used0,used1,seed,out0,out1;
  std::string callText,before,after,peerBefore,peerAfter,childBefore,childAfter,reads,writes,actorReads,peerWrites,childWrites;
  row>>number>>ps>>as>>state>>sub>>selector>>frame>>bone>>beforeY>>fixed>>used0>>used1>>seed>>callText>>before>>after>>peerBefore>>peerAfter>>child>>childBefore>>childAfter>>out0>>out1>>reads>>writes>>actorReads>>peerWrites>>childWrites;
  check(bool(row),"fixture parse");(void)beforeY;(void)fixed;(void)seed;
  SourceEventPhase5Context c;auto& p=c.pool_e0.slots[ps];raw(p,before);
  check(reads.size()==80&&writes.size()==80&&actorReads.size()==1444,"fixture masks");
  for(size_t i=0;i<80;++i)p.known[i]=uint8_t(reads[i]=='1');
  for(size_t i:{size_t(0),size_t(1),size_t(2),size_t(3),size_t(6),size_t(8),size_t(9),size_t(10),size_t(56),size_t(57),size_t(58),size_t(59),size_t(60),size_t(61)})p.known[i]=1;
  auto& actor=c.pool40.slots[as];authored(actor,number+7);actor.raw[1]=uint8_t(0x40u|as);actor.raw[14]=uint8_t(selector);
  for(size_t i=0;i<1444;++i)actor.known[i]=uint8_t(actorReads[i]=='1');
  actor.known[1]=1;for(size_t i=0;i<4;++i)actor.raw[152+i]=uint8_t(0x510000u>>(8*i));actor.raw[442]=uint8_t(bone);actor.raw[443]=uint8_t(uint32_t(bone)>>8);
  auto& peer=c.pool_e0.slots[9];raw(peer,peerBefore);peer.known[1]=1;
  c.pool_e0.used[0]=(0x80000000u>>ps)|(0x80000000u>>9);c.pool40.used[0]=0x80000000u>>as;
  c.pool80.used={{used0,used1}};
  for(size_t i=0;i<64;++i){authored(c.pool80.slots[i],number+23);c.pool80.slots[i].raw[1]=uint8_t(0x80u|i);c.pool80.slots[i].known[1]=1;}
  // The probe's contiguous pool pattern uses the global byte index.
  for(size_t i=0;i<64;++i)for(size_t n=0;n<72;++n)if(n!=1)c.pool80.slots[i].raw[n]=uint8_t((number+23)*37+(i*72+n)*17+11);
  SourceEventWork event;const uint32_t id=0x800bf118u+uint32_t(ps*80),actorId=0x800c3580u+uint32_t(as*1444);
  put(event,0x24,4,actorId);put(event,0x4c,4,0x800bf118u+9*80);
  SourcePropLiftActorFrameReceipt fr;fr.admitted=true;fr.source_actor=actorId;fr.sequence_identity=0x510000;fr.source_generation=31;fr.frame=uint8_t(frame);
  SourcePropLiftRngOwner owner{&event,17};std::array<SourcePropLiftRngReceipt,2> rr{};size_t count=0;
  if(callText!="-"){std::istringstream calls(callText);std::string item;while(std::getline(calls,item,',')){const auto colon=item.find(':');auto& r=rr[count];r.owner=owner.owner;r.owner_generation=owner.generation;r.source_generation=31;r.global_ordinal=1000+count;r.callback_identity=kSourcePropLiftCallback;r.prop_identity=id;r.actor_identity=actorId;r.sequence_identity=fr.sequence_identity;r.return_address=uint32_t(std::stoul(item.substr(0,colon)));r.v0=uint32_t(std::stoul(item.substr(colon+1)));r.child_identity=count?0x800a3b40u+uint32_t(child*72):0;r.prop_before_xyz={{u16(p.raw.data()+56),u16(p.raw.data()+58),u16(p.raw.data()+60)}};r.actor_part7_y=uint16_t(bone);r.actor_selector14=uint8_t(selector);r.sequence_frame=uint8_t(frame);r.state09=uint8_t(state);r.sub10=uint8_t(sub);++count;}}
  SourcePropLiftInputs in;in.curves=&curves;in.frame=&fr;in.rng=rr.data();in.rng_count=count;in.current_rng_owner=&owner;in.source_generation=31;
  const auto original=c;const auto result=tick(c,event,id,in);const auto label="case "+std::to_string(number)+" ";
  check(result.committed,label+"held status="+std::to_string(unsigned(result.status))+" missing="+(result.missing_offset?std::to_string(*result.missing_offset):"none"));
  check(p.raw==[&]{auto r=p;raw(r,after);return r.raw;}(),label+"prop output mismatch");
  check(peer.raw==[&]{auto r=peer;raw(r,peerAfter);return r.raw;}(),label+"linked output mismatch");
  check(c.pool80.used==std::array<std::optional<uint32_t>,2>{{out0,out1}},label+"allocator bitmap mismatch");
  check(result.rng_consumed==count&&result.child_allocated==(child>=0),label+"effect/RNG metadata mismatch");
  for(size_t i=0;i<80;++i){check(p.known[i]==uint8_t(original.pool_e0.slots[ps].known[i]||writes[i]=='1'),label+"prop knowledge mismatch");check(peer.known[i]==uint8_t(original.pool_e0.slots[9].known[i]||peerWrites[i]=='1'),label+"linked knowledge mismatch");}
  if(child>=0){auto expected=c.pool80.slots[size_t(child)];raw(expected,childAfter);check(c.pool80.slots[size_t(child)].raw==expected.raw,label+"child bytes mismatch");for(size_t i=0;i<72;++i)check(c.pool80.slots[size_t(child)].known[i]==uint8_t(original.pool80.slots[size_t(child)].known[i]||childWrites[i]=='1'),label+"child known mismatch");}
  for(size_t i=0;i<80;++i)if(reads[i]=='1'){auto denied=original;denied.pool_e0.slots[ps].known[i]=0;const auto saved=denied;check(!tick(denied,event,id,in).committed&&same(denied,saved),label+"unknown prop field not atomic "+std::to_string(i));++guards;}
  for(size_t i=0;i<1444;++i)if(actorReads[i]=='1'){auto denied=original;denied.pool40.slots[as].known[i]=0;const auto saved=denied;check(!tick(denied,event,id,in).committed&&same(denied,saved),label+"unknown actor field not atomic "+std::to_string(i));++guards;}
  if(state==1&&sub==1){auto denied=original;auto bad=fr;bad.frame=64;auto args=in;args.frame=&bad;check(!tick(denied,event,id,args).committed&&same(denied,original),"out-of-window frame guard");++guards;
   denied=original;bad=fr;++bad.sequence_identity;args.frame=&bad;check(!tick(denied,event,id,args).committed&&same(denied,original),"stale sequence guard");++guards;}
  if(count){auto denied=original;auto args=in;args.rng=nullptr;check(!tick(denied,event,id,args).committed&&same(denied,original),"missing RNG atomic");++guards;
   denied=original;auto stale=rr;++stale[0].owner_generation;args=in;args.rng=stale.data();check(!tick(denied,event,id,args).committed&&same(denied,original),"stale RNG owner atomic");++guards;
   if(count==2){denied=original;stale=rr;++stale[1].child_identity;args.rng=stale.data();check(!tick(denied,event,id,args).committed&&same(denied,original),"wrong child receipt atomic");++guards;}}
  ++cases;
 }
 check(cases==202,"fixture case count");
 for(size_t n=0;n<384;++n){check(rejected(std::vector<uint8_t>(n)),"profile truncation accepted");++guards;}
 check(rejected(std::vector<uint8_t>(385)),"profile trailing bytes accepted");++guards;
 if(argc==3){const auto b=file(argv[2]);const auto profile=load_source_prop_lift_profile(b);check(profile.admitted(),"owner profile not admitted");for(size_t i=0;i<384;++i){auto bad=b;bad[i]^=1;if(i<24||i>=28)put32(bad,24,crc32(bad.data()+32,bad.size()-32));check(rejected(bad),"self-CRC profile mutation accepted");++guards;}}
 std::cout<<cases<<" unchanged-original lift/drop cases; "<<guards<<" atomic/profile guards; zero hot allocations\n";
}
