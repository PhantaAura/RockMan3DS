#include "legends3ds/class60_06.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {
using Work=l3::Class60_06Work;using Status=l3::Class60_06Status;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int64_t number(std::istream& r){int64_t n;require(bool(r>>n),"child contact fixture truncated");return n;}
Work bytes(std::istream& r){Work w;for(auto& v:w)v=uint8_t(number(r));return w;}
l3::RuntimeMap rectangle(uint16_t code=3,uint16_t flags=0,int16_t raw6=0){
 l3::RuntimeMap m;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;m.areas.push_back(a);m.cells.assign(16,1);m.idx.resize(2);m.idx[1].descriptor_index=0;m.idx[1].raw6=raw6;
 l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;m.descriptors.push_back(d);m.contact_lists.push_back({0,1});m.contact_groups.push_back({-100,100,-100,100,0,1});m.contact_primitives.push_back({-128,128,code,flags});return m;
}
void guards(){
 Work w{};w[1]=97;w[0x74]=1;auto map=rectangle();
 w[0x78]=1;require(l3::prepare_class60_06_contact_8010fdd4(w,map,0,0,true,0).status==Status::UnsupportedBounds,"unknown bounds admitted");w[0x78]=0;
 map.cells.clear();require(l3::prepare_class60_06_contact_8010fdd4(w,map,0,0,true,0).status==Status::UnsupportedBoundary,"missing collision area admitted");
}
void fixtures(const char* path){
 std::ifstream f(path);require(bool(f),"child contact fixture missing");std::string line;std::getline(f,line);size_t admitted=0,held=0,corrected=0,effects=0,sounds=0;
 while(std::getline(f,line)){std::istringstream r(line);const auto code=uint16_t(number(r)),flags=uint16_t(number(r));const auto raw6=int16_t(number(r));number(r);const auto allocation=uint32_t(number(r));
 const auto before=bytes(r),expected=bytes(r);const auto wall=uint32_t(number(r));const auto returned=uint16_t(number(r));const auto hassound=number(r);std::array<uint32_t,3> cue{};for(auto& v:cue)v=uint32_t(number(r));const auto haseffect=number(r);std::array<uint8_t,72> fx{};for(auto& v:fx)v=uint8_t(number(r));
 const auto map=rectangle(code,flags,raw6);const auto got=l3::prepare_class60_06_contact_8010fdd4(before,map,0,0,true,allocation);
 if(got.status!=Status::SourceBackedPartial){++held;require(got.status==Status::UnsupportedWall,"unexpected admitted fixture hold");continue;}
 if(got.after!=expected || uint32_t(got.source_wall_result)!=wall || got.source_return!=returned){std::cerr<<"contact case "<<admitted+held<<" code "<<code<<" nativewall "<<uint32_t(got.source_wall_result)<<" sourcewall "<<wall<<" returned "<<got.source_return<<" want "<<returned<<'\n';for(size_t i=0;i<268;++i)if(got.after[i]!=expected[i])std::cerr<<std::hex<<i<<std::dec<<" got "<<int(got.after[i])<<" source "<<int(expected[i])<<'\n';throw std::runtime_error("original child contact mismatch");}
 require(bool(got.sound)==bool(hassound) && bool(got.effect)==bool(haseffect),"original child contact sideeffect presence mismatch");
 if(hassound){require(got.sound==179 && got.sound_pointed_words && cue[0]==(179u<<16|1u) && (*got.sound_pointed_words)[0]==cue[1] && (*got.sound_pointed_words)[1]==cue[2],"original positionalcue179 mismatch");++sounds;
  require(l3::prepare_class60_06_contact_8010fdd4(before,map,0,std::nullopt,true,allocation).status==Status::NeedsProfile,"missing stage guessed");
  require(l3::prepare_class60_06_contact_8010fdd4(before,map,0,0,false,allocation).status==Status::NeedsContactSound,"missing cue consumer bypassed");
  require(l3::prepare_class60_06_contact_8010fdd4(before,map,0,0,true,std::nullopt).status==Status::NeedsEffectConsumer,"missing effect consumer bypassed");
 }
 if(haseffect){const auto& e=*got.effect;require(e.work00==fx[0] && e.subtype02==fx[2] && e.parameter03==fx[3] && e.copy_primitive && !e.copy_raw,"original contact131 identity mismatch");for(size_t i=0;i<52;++i)require(e.primitive_to_14[i]==fx[20+i],"original contact52B primitivecopy mismatch");++effects;}
 ++admitted;corrected+=wall!=0;
 }
 require(admitted>400 && held>0 && corrected>50 && effects>20 && sounds>50,"insufficient child contact coverage");std::cout<<"original child contact admitted="<<admitted<<" held="<<held<<" corrected="<<corrected<<" effects="<<effects<<" sounds="<<sounds<<'\n';
}
}
int main(int argc,char** argv){try{guards();if(argc>1)fixtures(argv[1]);std::cout<<"PASS original child contact boundaries\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
