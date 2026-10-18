#include "legends3ds/source_effect2.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <limits>
#include <sstream>
#include <string>
namespace { size_t allocations=0; }
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
using namespace l3;
namespace {
void require(bool value,const std::string& message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
SourceEffectWork unhex(const std::string& text){require(text.size()==144,"fixture width");SourceEffectWork w{};for(size_t i=0;i<72;++i)w[i]=uint8_t(std::stoul(text.substr(2*i,2),nullptr,16));return w;}
SourceEventPhase5Context cold(){SourceEventPhase5Context c;for(size_t i=0;i<64;++i){c.pool80.slots[i].raw[1]=uint8_t(128|i);c.pool80.slots[i].known[1]=1;}c.pool80.used={0u,0u};c.pool80_pending_release_800988c8={0u,0u};return c;}
bool same(const SourceEventPhase5Context& a,const SourceEventPhase5Context& b){for(size_t i=0;i<64;++i)if(a.pool80.slots[i].raw!=b.pool80.slots[i].raw || a.pool80.slots[i].known!=b.pool80.slots[i].known)return false;return a.pool80.used==b.pool80.used && a.pool80_pending_release_800988c8==b.pool80_pending_release_800988c8;}
uint16_t u16(const SourceEffectWork& w,size_t o){return uint16_t(w[o]|(uint16_t(w[o+1])<<8));}
void receipts(std::array<SourceEffect2RngReceipt,3>& rr,size_t slot,size_t child,const SourceEffectWork& w){for(size_t i=0;i<3;++i){rr[i].callback_identity=source_effect2_callback;rr[i].parent_identity=source_effect_base+uint32_t(slot*72);rr[i].child_identity=source_effect_base+uint32_t(child*72);rr[i].return_address=std::array<uint32_t,3>{{0x80116f60u,0x80116f98u,0x80116fe4u}}[i];rr[i].source_generation=17;rr[i].global_ordinal=1000+uint32_t(i);rr[i].parent_clock11=w[11];rr[i].parent_parameter03=w[3];rr[i].parent_mode0d=w[13];rr[i].parent_tag0e=w[14];rr[i].parent_xyz1c={{u16(w,28),u16(w,30),u16(w,32)}};}}
}
int main(int argc,char** argv){
 require(argc==2,"supply original authored fixture");std::ifstream file(argv[1]);require(bool(file),"fixture open");std::string line;std::getline(file,line);size_t cases=0,guards=0;
 while(std::getline(file,line)){
  if(line.empty())continue;std::istringstream row(line);size_t slot;int child;std::array<uint32_t,2> used{},pending{},usedAfter{},pendingAfter{};std::array<SourceEffect2RngReceipt,3> rr{};SourceEffect2ColorReceipt colors;colors.source_table_identity=source_effect2_color_table;colors.admitted=true;
  std::string before,childBefore,after,childAfter,reads,writes,childWrites;
  row>>slot>>child>>used[0]>>used[1]>>pending[0]>>pending[1];for(auto& receipt:rr)row>>receipt.v0;for(auto& word:colors.words)row>>word;
  row>>before>>childBefore>>after>>childAfter>>usedAfter[0]>>usedAfter[1]>>pendingAfter[0]>>pendingAfter[1]>>reads>>writes>>childWrites;require(bool(row),"fixture parse");
  auto context=cold();context.pool80.used={used[0],used[1]};context.pool80_pending_release_800988c8={pending[0],pending[1]};auto& parent=context.pool80.slots[slot];parent.raw=unhex(before);
  require(reads.size()==72&&writes.size()==72&&childWrites.size()==72,"fixture masks");for(size_t i=0;i<72;++i)parent.known[i]=uint8_t(reads[i]=='1');for(size_t i:{size_t(0),size_t(1),size_t(2),size_t(6)})parent.known[i]=1;
  if(child>=0){context.pool80.slots[size_t(child)].raw=unhex(childBefore);context.pool80.slots[size_t(child)].known={};context.pool80.slots[size_t(child)].known[1]=1;}
  receipts(rr,slot,child>=0?size_t(child):0,parent.raw);SourceEffect2Inputs inputs;inputs.rng=rr.data();inputs.rng_count=3;inputs.source_generation=17;inputs.colors=&colors;
  const auto original=context;const auto allocations_before=allocations;const auto result=tick_source_effect2_80116e68(context,source_effect_base+uint32_t(slot*72),inputs);
  const bool heap_stable=allocations==allocations_before;require(heap_stable,"callback C++ heap allocation");
  const std::string label="case "+std::to_string(cases)+" ";require(result.committed,label+"held status="+std::to_string(unsigned(result.status))+"missing="+(result.missing_offset?std::to_string(*result.missing_offset):"none"));
  require(context.pool80.slots[slot].raw==unhex(after),label+"parent raw mismatch");require(context.pool80.used==std::array<std::optional<uint32_t>,2>{{usedAfter[0],usedAfter[1]}},label+"allocation mismatch");require(context.pool80_pending_release_800988c8==std::array<std::optional<uint32_t>,2>{{pendingAfter[0],pendingAfter[1]}},label+"release mismatch");
  for(size_t i=0;i<72;++i)require(context.pool80.slots[slot].known[i]==uint8_t(original.pool80.slots[slot].known[i]||writes[i]=='1'),label+"parent known lane "+std::to_string(i));
  require(result.child_allocated==(child>=0),label+"child allocation metadata");if(child>=0){require(context.pool80.slots[size_t(child)].raw==unhex(childAfter),label+"child raw mismatch");for(size_t i=0;i<72;++i)require(context.pool80.slots[size_t(child)].known[i]==uint8_t(original.pool80.slots[size_t(child)].known[i]||childWrites[i]=='1'),label+"child known lane "+std::to_string(i));}
  // Remove each actual parent read lane. Unknowns must hold without any
  // mutation; gate short circuit may make a lane unnecessary for inactive
  // records, but these authored fixtures are all eligible source callbacks.
  for(size_t i=0;i<72;++i)if(reads[i]=='1'){
   auto denied=original;denied.pool80.slots[slot].known[i]=0;auto saved=denied;const auto held=tick_source_effect2_80116e68(denied,source_effect_base+uint32_t(slot*72),inputs);require(!held.committed&&same(denied,saved),label+"unknown lane not atomic "+std::to_string(i));++guards;
  }
  if(result.rng_consumed){
   for(size_t j=0;j<3;++j){auto denied=original;auto stale=rr;stale[j].global_ordinal+=1;auto in=inputs;in.rng=stale.data();const auto held=tick_source_effect2_80116e68(denied,source_effect_base+uint32_t(slot*72),in);require(!held.committed&&same(denied,original),"stale RNG ordinal atomic");++guards;}
   const auto maximum=std::numeric_limits<uint64_t>::max();
   const std::array<std::array<uint64_t,3>,3> invalid{{{{0,1,2}},{{maximum-1u,maximum,0}},{{maximum,0,1}}}};
   for(const auto& ordinals:invalid){
    auto denied=original;auto stale=rr;for(size_t j=0;j<3;++j)stale[j].global_ordinal=ordinals[j];auto in=inputs;in.rng=stale.data();
    const auto held=tick_source_effect2_80116e68(denied,source_effect_base+uint32_t(slot*72),in);
    require(held.status==SourceEffect2Status::RngContextMismatch&&!held.committed&&same(denied,original),"zero or wrapping RNG ordinal atomic");++guards;
   }
   auto admitted=original;auto last=rr;for(size_t j=0;j<3;++j)last[j].global_ordinal=maximum-2u+uint64_t(j);auto in=inputs;in.rng=last.data();
   const auto accepted=tick_source_effect2_80116e68(admitted,source_effect_base+uint32_t(slot*72),in);
   require(accepted.committed&&same(admitted,context),"last nonwrapping RNG triple preserves scalar result");++guards;
  }
  ++cases;
 }
 require(cases>300,"fixture case count");
 // Exhaustion/unknown masks/source identity all hold with no low-address write.
 auto context=cold();auto& parent=context.pool80.slots[0];parent.known.fill(1);parent.raw[0]=1;parent.raw[2]=2;parent.raw[6]=3;context.pool80.used={0xffffffffu,0xffffffffu};auto saved=context;auto result=tick_source_effect2_80116e68(context,source_effect_base);require(result.status==SourceEffect2Status::SourceAllocationExhausted&&same(context,saved),"exhaustion atomic");++guards;
 context=saved;context.pool80.used[1].reset();saved=context;result=tick_source_effect2_80116e68(context,source_effect_base);require(result.status==SourceEffect2Status::NeedsPoolBitmap&&same(context,saved),"unknown allocation bank atomic");++guards;
 result=tick_source_effect2_80116e68(context,source_effect_base+1);require(result.status==SourceEffect2Status::InvalidIdentity&&same(context,saved),"identity alignment atomic");++guards;
 std::cout<<cases<<" original scalar cases; "<<guards<<" atomic guards passed\n";
}
