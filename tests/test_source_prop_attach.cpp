#include "legends3ds/source_prop_attach.h"
#include "legends3ds/source_event_pipeline.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
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
void require(bool v,const std::string& message){if(!v){std::cerr<<message<<'\n';std::exit(1);}}
template<size_t N>std::array<uint8_t,N> unhex(const std::string& text){require(text.size()==N*2,"fixture width");std::array<uint8_t,N> out{};for(size_t i=0;i<N;++i)out[i]=uint8_t(std::stoul(text.substr(2*i,2),nullptr,16));return out;}
bool same(const SourceEventPhase5Bytes<80>& a,const SourceEventPhase5Bytes<80>& b){return a.raw==b.raw&&a.known==b.known;}
}
int main(int argc,char** argv){
 require(argc==2,"supply original authored attachment fixture");std::ifstream f(argv[1]);require(bool(f),"fixture open");std::string line;std::getline(f,line);size_t cases=0,guards=0;
 while(std::getline(f,line)){
  if(line.empty())continue;std::istringstream row(line);size_t slot,actor_slot;unsigned state;int zs,zc,xs,xc;uint32_t pending_address,used_address,pending_before,used_word,pending_after;std::string before,actor_bytes,after,writes,reads;
  row>>slot>>actor_slot>>state>>zs>>zc>>xs>>xc>>pending_address>>used_address>>pending_before>>used_word>>pending_after>>before>>actor_bytes>>after>>writes>>reads;require(bool(row),"fixture parse");
  SourceEventPhase5Bytes<80> prop;prop.raw=unhex<80>(before);prop.known.fill(1);auto original=prop;
  SourceEventPhase5Pool<4,1444> actors;actors.used[0]=0x80000000u>>actor_slot;auto& actor=actors.slots[actor_slot];actor.raw=unhex<1444>(actor_bytes);actor.known.fill(0);
  require(reads.size()==1444&&writes.size()==80,"fixture masks");for(size_t i=0;i<1444;++i)actor.known[i]=uint8_t(reads[i]=='1');actor.known[1]=actor.known[2]=1;
  // The source helper's last LW includes two matrix padding bytes which do
  // not enter a GTE coefficient. Their validity must not be fabricated.
  actor.known[690]=actor.known[691]=0;
  ReferenceTables refs;refs.trig[0]={int16_t(zs),int16_t(zc)};refs.trig[1024]={int16_t(xs),int16_t(xc)};
  SourceEventWork event;const uint32_t token=0x800c3580u+uint32_t(actor_slot*1444);for(size_t i=0;i<4;++i){event.raw[0x24+i]=uint8_t(token>>(8*i));event.known[0x24+i]=1;}
  const uint32_t identity=0x800bf118u+uint32_t(slot*80);SourcePropForeignReleaseOwner release;release.prop_identity=identity;release.pending_address=pending_address;release.used_address=used_address;release.pending_word=pending_before;release.used_word=used_word;release.admitted=true;
  SourcePropAttachInputs in;in.references=&refs;in.event_work=&event;in.actors=&actors;in.release_owner=&release;
  const size_t before_allocations=allocations;const auto result=update_source_prop_attach_80112d14(prop,identity,in);const bool heap_stable=allocations==before_allocations;
  const std::string label="case "+std::to_string(cases)+" ";require(heap_stable,label+"heap allocation");require(result.committed,label+"held "+std::to_string(unsigned(result.status)));
  const auto expected=unhex<80>(after);require(prop.raw==expected,label+"source scalar mismatch");require(*release.pending_word==pending_after,label+"foreign release mismatch");require(*release.used_word==used_word,label+"used owner was modified");
  require(result.released==(state==1&&original.raw[12]!=0),label+"release metadata");require(result.invalidated_opaque_lanes==(state==1?12:0),label+"opaque metadata");
  for(size_t i=0;i<80;++i)require(prop.known[i]==uint8_t(!(state==1&&i>=44&&i<56)),label+"known lane "+std::to_string(i));
  for(size_t i=0;i<1444;++i)if(actor.known[i]){
   auto local=original;auto denied=actors;denied.slots[actor_slot].known[i]=0;auto altered=in;altered.actors=&denied;auto owner=release;owner.pending_word=pending_before;altered.release_owner=&owner;
   const auto held=update_source_prop_attach_80112d14(local,identity,altered);require(!held.committed&&same(local,original)&&owner.pending_word==std::optional<uint32_t>(pending_before),label+"unknown actor lane atomic "+std::to_string(i));++guards;
  }
  for(size_t off:{size_t(0),size_t(1),size_t(2),size_t(3),size_t(6),size_t(9)}){
   auto local=original;local.known[off]=0;const auto saved=local;const auto held=update_source_prop_attach_80112d14(local,identity,in);require(!held.committed&&same(local,saved),label+"unknown prop field atomic");++guards;
  }
  {auto local=original;auto altered=in;altered.references=nullptr;const auto held=update_source_prop_attach_80112d14(local,identity,altered);require(!held.committed&&same(local,original),"missing references atomic");++guards;}
  {auto local=original;auto stale=event;stale.raw[0x24]^=1;auto altered=in;altered.event_work=&stale;const auto held=update_source_prop_attach_80112d14(local,identity,altered);require(!held.committed&&same(local,original),"stale actor identity atomic");++guards;}
  {auto local=original;auto stale=actors;stale.used[0]=0;auto altered=in;altered.actors=&stale;const auto held=update_source_prop_attach_80112d14(local,identity,altered);require(!held.committed&&same(local,original),"unallocated actor atomic");++guards;}
  if(state==1&&original.raw[12]){
   auto local=original;auto owner=release;owner.pending_word.reset();auto altered=in;altered.release_owner=&owner;const auto held=update_source_prop_attach_80112d14(local,identity,altered);require(held.status==SourcePropAttachStatus::NeedsForeignReleaseOwner&&!held.committed&&same(local,original)&&!owner.pending_word,"unknown foreign release atomic");++guards;
   owner=release;owner.pending_word=pending_before;owner.pending_address^=4;local=original;const auto mismatch=update_source_prop_attach_80112d14(local,identity,altered);require(mismatch.status==SourcePropAttachStatus::ForeignReleaseMismatch&&!mismatch.committed&&same(local,original)&&owner.pending_word==std::optional<uint32_t>(pending_before),"wrong foreign release key atomic");++guards;
  }
  ++cases;
 }
 require(cases==180,"fixture count");std::cout<<cases<<" original attachment cases; "<<guards<<" atomic guards passed; zero callback allocations\n";
}
