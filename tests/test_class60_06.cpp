#include "legends3ds/class60_06.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using Work=l3::Class60_06Work;using Status=l3::Class60_06Status;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int64_t number(std::istream& r){int64_t n;require(bool(r>>n),"truncated child fixture");return n;}
Work bytes(std::istream& r){Work w;for(auto& b:w)b=static_cast<uint8_t>(number(r));return w;}
void put32(Work& w,size_t o,uint32_t v){for(size_t i=0;i<4;++i)w[o+i]=uint8_t(v>>(i*8));}
l3::Class60_06Profile profile(){l3::Class60_06Profile p;for(size_t b=0;b<2;++b)for(size_t f=0;f<6;++f){auto& pair=b?p.alternate_uv[f]:p.normal_uv[f];pair[0]=uint16_t(0x1200+b*49+f*5);pair[1]=uint16_t(0x3400+b*31+f*7);}return p;}
void guards(){
 l3::Class60SourcePool pool;pool.allocated_bitmap=0xffffffff;pool.pending_release_bitmap=123;l3::initialize_class60_source_pool_8002e8d8(pool);
 require(pool.allocated_bitmap==0 && pool.pending_release_bitmap==0,"pool boot bitmap ownership");for(size_t i=0;i<32;++i)for(size_t o=0;o<268;++o)require(pool.slots[i][o]==(o==1?uint8_t(0x60u|i):0),"pool boot clear/class identity");
 const auto identity=l3::class60_allocate_80031518(pool);auto w=pool.slots[*l3::class60_source_slot(identity)];w[0]=1;w[2]=6;put32(w,0x9c,32);put32(w,0xa0,16);const auto before=w;
 l3::Class60_06Inputs in;require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsTrig && w==before,"missing trig changed child");l3::ReferenceTables refs;refs.trig.fill({0,4096});in.references=&refs;
 require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsProfile && w==before,"UV profile invented");auto p=profile();in.profile=&p;
 auto r=l3::class60_06_step_8010f354(w,in);require(r.status==Status::NeedsContact && r.contact_request && w==before,"contact boundary not held");
 l3::Class60_06Contact c;c.status=Status::SourceBackedPartial;c.before=*r.contact_request;c.after=c.before;in.contact=c;
 r=l3::class60_06_step_8010f354(w,in);require(r.status==Status::NeedsProxyConsumer && r.proxy_request && w==before,"proxy readiness fabricated");
 l3::Class60_06ProxyCommit proxy;proxy.before=*r.proxy_request;proxy.worka0_after=128;in.proxy=proxy;
 require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsCollisionRegistry && w==before,"registry queue skipped");in.collision_registry_ready=true;
 require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsShadowConsumer && w==before,"shadow count invented");in.shadow_count_800987d8=0;
 require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsShadowFloor && w==before,"shadow floor invented");in.shadow_floor_80048e6c=26;
 r=l3::class60_06_step_8010f354(w,in);require(r.status==Status::SourceBackedPartial && w[8]==1 && w[0xf]==1 && w[0]==3 && r.register_collision && r.shadow,"admitted original initializer/callback subset");
 const auto primitive=l3::class60_06_primitive(w);require(primitive && primitive->half_extent==32 && primitive->clut==0x3d48 && primitive->tpage==14 && primitive->uv==std::array<uint8_t,2>{{0,0}},"original primitive packing contract");
 w=before;auto stale=*in.contact;++stale.before[267];in.contact=stale;require(l3::class60_06_step_8010f354(w,in).status==Status::StaleContact && w==before,"stale fourth/tail field admitted");in.contact=c;
 auto wrong=proxy;++wrong.before.point[2];in.proxy=wrong;require(l3::class60_06_step_8010f354(w,in).status==Status::StaleProxy && w==before,"stale proxy point admitted");in.proxy=proxy;
 const auto count=allocations;bool ok=true;for(int i=0;i<640;++i){auto copy=before;ok&=l3::class60_06_step_8010f354(copy,in).status==Status::SourceBackedPartial;}
 require(ok && allocations==count,"child hot-loop allocated");
 w={};w[0]=1;w[1]=96;w[2]=6;w[6]=3;w[8]=1;put32(w,0x9c,0x20000000);require(l3::class60_06_step_8010f354(w,in).status==Status::NeedsPlayerHeading,"player heading invented");
 w[8]=5;require(l3::class60_06_step_8010f354(w,in).status==Status::UnknownState,"unknown state ran");w[8]=4;w[0xf]=255;require(l3::class60_06_step_8010f354(w,in).status==Status::UnknownUvFrame,"negative UVframe guessed");
 w[0]=0;require(l3::class60_06_step_8010f354(w,in).status==Status::Inactive,"inactive child ran");
}
void callbacks(const char* path){
 std::ifstream f(path);require(bool(f),"child callback fixture missing");std::string line;std::getline(f,line);size_t count=0,releases=0,effects=0,contacts=0,redirects=0;
 auto p=profile();while(std::getline(f,line)){if(line.empty())continue;std::istringstream row(line);l3::Class60_06Inputs in;l3::ReferenceTables refs;
 const auto sin=static_cast<int16_t>(number(row)),cos=static_cast<int16_t>(number(row));refs.trig.fill({sin,cos});in.references=&refs;in.profile=&p;in.player_heading_800b5206=static_cast<uint16_t>(number(row));in.effect_allocation_80031618=static_cast<uint32_t>(number(row));
 const auto returned=static_cast<uint16_t>(number(row));const auto hascontact=number(row);l3::Class60_06Contact c;c.status=Status::SourceBackedPartial;c.before=bytes(row);c.after=bytes(row);c.source_return=returned;if(hascontact)in.contact=c;
 const auto hasproxy=number(row);l3::Class60_06ProxyCommit proxy;for(auto& v:proxy.before.point)v=static_cast<int16_t>(number(row));proxy.before.source_class=static_cast<uint8_t>(number(row));proxy.before.extent=static_cast<uint16_t>(number(row));proxy.before.mode=static_cast<uint16_t>(number(row));proxy.before.work9c=static_cast<uint32_t>(number(row));proxy.before.worka0=static_cast<uint32_t>(number(row));proxy.worka0_after=static_cast<uint32_t>(number(row));if(hasproxy)in.proxy=proxy;
 in.shadow_floor_80048e6c=static_cast<int32_t>(number(row));in.shadow_count_800987d8=0;in.collision_registry_ready=true;
 auto w=bytes(row);const auto original=w,expected=bytes(row);const auto release=number(row),effect=number(row);std::array<uint8_t,72> fx{};for(auto& v:fx)v=static_cast<uint8_t>(number(row));std::array<uint16_t,6> shadow{};for(auto& v:shadow)v=static_cast<uint16_t>(number(row));std::string extra;require(!(row>>extra),"extra callback fixture fields");
 const auto result=l3::class60_06_step_8010f354(w,in);
 if(result.status!=Status::SourceBackedPartial || w!=expected){std::cerr<<"child case "<<count<<" source state "<<int(original[8])<<" status "<<int(result.status)<<'\n';for(size_t i=0;i<w.size();++i)if(w[i]!=expected[i])std::cerr<<std::hex<<i<<std::dec<<": got "<<int(w[i])<<" expected "<<int(expected[i])<<'\n';throw std::runtime_error("original child callback mismatch");}
 require(result.release_800318d8==bool(release) && bool(result.effect_request)==bool(effect),"source child effect/release intent mismatch");
 if(effect){const auto& e=*result.effect_request;if(fx[0]!=e.work00 || fx[2]!=e.subtype02 || fx[3]!=e.parameter03)std::cerr<<"effect case "<<count<<": source "<<int(fx[0])<<' '<<int(fx[2])<<' '<<int(fx[3])<<" native "<<int(e.work00)<<' '<<int(e.subtype02)<<' '<<int(e.parameter03)<<'\n';require(fx[0]==e.work00 && fx[2]==e.subtype02 && fx[3]==e.parameter03 && e.copy_raw && !e.copy_primitive,"source128 effect stores mismatch");for(size_t i=0;i<4;++i)require(uint16_t(uint16_t(fx[56+2*i])|(uint16_t(fx[57+2*i])<<8))==uint16_t(e.raw_to_38[i]),"source128 effect pointed raw mismatch");}
 require(result.shadow.has_value(),"original shadow producer omitted");const auto& sh=*result.shadow;require(std::array<uint16_t,6>{{uint16_t(sh.point[0]),uint16_t(sh.point[1]),uint16_t(sh.point[2]),sh.heading_scale,sh.radius_type,sh.height_delta}}==shadow,"original shadow record mismatch");
 ++count;releases+=release;effects+=effect;contacts+=hascontact;redirects+=(original[0x9f]&0x20u)!=0;
 }
 require(count>2000 && releases>200 && effects>100 && contacts>500 && redirects>100,"insufficient child lifecycle coverage");std::cout<<"original child callbacks="<<count<<" releases="<<releases<<" effects="<<effects<<" contacts="<<contacts<<'\n';
}
void motion(const char* path){std::ifstream f(path);require(bool(f),"child motion fixture missing");std::string line;std::getline(f,line);size_t count=0;while(std::getline(f,line)){if(line.empty())continue;std::istringstream r(line);auto w=bytes(r);const auto want=bytes(r);l3::class60_06_integrate_80110070(w);require(w==want,"source velocity wrap/clamp/fixedaccumulator mismatch");++count;}require(count==192,"motion edgecases missing");std::cout<<"original child motion cases="<<count<<'\n';}
void primitive(const char* path){
 std::ifstream f(path);require(bool(f),"child primitive fixture missing");std::string line;std::getline(f,line);size_t count=0;
 while(std::getline(f,line)){std::istringstream r(line);std::array<int32_t,3> center{};for(auto& v:center)v=int32_t(number(r));const auto extent=uint16_t(number(r));std::array<std::array<int16_t,3>,3> expected{};
 for(auto& point:expected){for(auto& v:point)v=int16_t(number(r));require(number(r)==0,"authored primitive padding");}
 std::array<uint32_t,4> words{};for(auto& v:words)v=uint32_t(number(r));
 require(l3::class60_06_camera_points(center,extent)==expected,"original camera point cornerwrap mismatch");l3::Class60_06Primitive p;p.clut=0x3456;p.tpage=14;p.uv={{0x34,0x12}};
 require(l3::class60_06_texture_words(p)==words,"original packedUV/CLUT/tpage mismatch");++count;
 }require(count==9,"primitive fixture coverage");std::cout<<"original child primitive packing cases="<<count<<'\n';
}
}
int main(int argc,char** argv){try{guards();if(argc>1)callbacks(argv[1]);if(argc>2)motion(argv[2]);if(argc>3)primitive(argv[3]);std::cout<<"PASS source child lifecycle and strong consumer holds\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
