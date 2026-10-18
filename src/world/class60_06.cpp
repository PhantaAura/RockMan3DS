#include "legends3ds/class60_06.h"
#include <algorithm>
#include <limits>
namespace l3 {
namespace {
using Work=Class60_06Work;using Status=Class60_06Status;
uint16_t u16(const Work& w,size_t o) noexcept {return uint16_t(uint16_t(w[o])|(uint16_t(w[o+1])<<8));}
int16_t s16(uint16_t v) noexcept {return v<32768u?int16_t(v):int16_t(int32_t(v)-65536);}
int16_t i16(const Work& w,size_t o) noexcept {return s16(u16(w,o));}
int8_t s8(uint8_t v) noexcept {return v<128u?int8_t(v):int8_t(int16_t(v)-256);}
uint32_t u32(const Work& w,size_t o) noexcept {uint32_t v=0;for(size_t i=0;i<4;++i)v|=uint32_t(w[o+i])<<(i*8);return v;}
int32_t s32(uint32_t v) noexcept {return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-0x100000000LL);}
void put16(Work& w,size_t o,uint16_t v) noexcept {w[o]=uint8_t(v);w[o+1]=uint8_t(v>>8);}
void put32(Work& w,size_t o,uint32_t v) noexcept {for(size_t i=0;i<4;++i)w[o+i]=uint8_t(v>>(i*8));}
int32_t asr(int32_t v,unsigned shift) noexcept {
    if(v>=0)return v/int32_t(1u<<shift);
    return int32_t(-((int64_t(-int64_t(v))+(int64_t(1)<<shift)-1)/(int64_t(1)<<shift)));
}
void mirrors(Work& w,size_t fixed,size_t raw) noexcept {
    for(size_t i=0;i<3;++i)put16(w,raw+2*i,uint16_t(reference_fixed_to_short_80030150(s32(u32(w,fixed+4*i)))));
}
void snapshot(Work& w) noexcept {for(size_t i=0;i<16;++i)w[0x34+i]=w[0x1c+i];for(size_t i=0;i<8;++i)w[0x2c+i]=w[0x14+i];}
void center(Work& w,bool subtract16) noexcept {
    put16(w,0x90,u16(w,0x14));put16(w,0x92,uint16_t(u16(w,0x16)-(subtract16?16u:0u)));put16(w,0x94,u16(w,0x18));
}
void packet(Work& w) noexcept {put32(w,0x88,0x3d48000eu);put32(w,0x8c,0x2c808080u);}
bool uv(Work& w,const Class60_06Profile* profile) noexcept {
    if(!profile)return false;
    const auto frame=s8(w[0xf]);
    if(frame<0 || frame>=6)return false;
    const auto& pair=(w[3]&0x80u)?profile->alternate_uv[size_t(frame)]:profile->normal_uv[size_t(frame)];
    w[0x96]=uint8_t(pair[0]);w[0x97]=uint8_t(pair[1]);return true;
}
Status uv_status(const Work& w,const Class60_06Profile* p) noexcept {return !p?Status::NeedsProfile:(s8(w[0xf])<0 || s8(w[0xf])>=6?Status::UnknownUvFrame:Status::SourceBackedPartial);}
bool fx_identity(uint32_t identity) noexcept {return identity==0 || (identity>=0x800a3b40u && (identity-0x800a3b40u)%72u==0 && (identity-0x800a3b40u)/72u<64u);}
std::array<uint32_t,2> pointed(const Work& w) noexcept {return {{u32(w,0x14),u32(w,0x18)}};}
Class60_06EffectIntent end_effect(const Work& w,uint32_t id) noexcept {
    Class60_06EffectIntent e;e.source_identity=id;e.work00=5;e.subtype02=128;e.parameter03=w[0xa]?2:66;e.copy_raw=true;
    for(size_t i=0;i<4;++i)e.raw_to_38[i]=i16(w,0x14+i*2);
    return e;
}
Class60_06EffectIntent contact_effect(const Work& w,uint32_t id) noexcept {
    Class60_06EffectIntent e;e.source_identity=id;e.work00=1;e.subtype02=131;e.parameter03=0;e.copy_primitive=true;
    std::copy_n(w.begin()+0x88,52,e.primitive_to_14.begin());return e;
}
bool release(Work& w,const Class60_06Inputs& in,Class60_06Result& out) noexcept {
    auto request=end_effect(w,in.effect_allocation_80031618.value_or(0));out.effect_request=request;
    if(!in.effect_allocation_80031618 || !fx_identity(*in.effect_allocation_80031618)){out.status=Status::NeedsEffectConsumer;return false;}
    if(*in.effect_allocation_80031618==0)out.effect_request.reset();
    w[0]=0;out.release_800318d8=true;return true;
}
bool contact(Work& w,const Class60_06Inputs& in,Class60_06Result& out,uint16_t& returned) noexcept {
    out.contact_request=w;
    if(!in.contact){out.status=Status::NeedsContact;return false;}
    if(in.contact->status!=Status::SourceBackedPartial){out.status=in.contact->status;return false;}
    if(in.contact->before!=w){out.status=Status::StaleContact;return false;}
    w=in.contact->after;returned=in.contact->source_return;return true;
}
bool redirected(Work& w,const Class60_06Inputs& in,Class60_06Result& out,bool state3) noexcept {
    if(!(u32(w,0x9c)&0x20000000u))return true;
    if(!in.references){out.status=Status::NeedsTrig;return false;}
    if(!in.player_heading_800b5206){out.status=Status::NeedsPlayerHeading;return false;}
    const auto& trig=in.references->trig[*in.player_heading_800b5206&4095u];
    put16(w,0x44,uint16_t(asr(s32(uint32_t(-int32_t(trig.sin_q12))<<5),4)));
    put16(w,0x46,uint16_t(-4096));
    put16(w,0x48,uint16_t(asr(s32(uint32_t(-int32_t(trig.cos_q12))<<5),4)));
    w[0xa]=1;if(state3){w[8]=1;put16(w,0x4c,0);put16(w,0x4e,512);put16(w,0x50,0);}put32(w,0x9c,0);return true;
}
bool same_proxy(const Class60_06ProxyIntent& a,const Class60_06ProxyIntent& b) noexcept {
    return a.point==b.point && a.source_class==b.source_class && a.extent==b.extent && a.mode==b.mode && a.work9c==b.work9c && a.worka0==b.worka0 && a.packet_mask==b.packet_mask && a.extra_flags==b.extra_flags;
}
std::array<int16_t,3> reflected(std::array<int16_t,3> v,uint32_t mask) noexcept {
    const auto magnitude=[](int16_t n){return n<0?-int32_t(n):int32_t(n);};const auto old=v;
    if((mask&0x300u) && (mask&0xc00u)){
        v[0]=s16(uint16_t((mask&0x100u)?-magnitude(old[2]):magnitude(old[2])));
        v[2]=s16(uint16_t((mask&0x400u)?-magnitude(old[0]):magnitude(old[0])));
    }else{
        if(mask&0x300u)v[0]=s16(uint16_t((mask&0x100u)?-magnitude(v[0]):magnitude(v[0])));
        if(mask&0xc00u)v[2]=s16(uint16_t((mask&0x400u)?-magnitude(v[2]):magnitude(v[2])));
    }
    if(mask&0x3000u)v[1]=s16(uint16_t((mask&0x1000u)?-magnitude(v[1]):magnitude(v[1])));
    return v;
}
bool neighbors(const RuntimeMap& map,size_t index,int16_t x,int16_t z) noexcept {
    if(index>=map.areas.size())return false;
    const auto& a=map.areas[index];
    const int32_t nx=(uint16_t(x)&511u)<257u?-512:512,nz=(uint16_t(z)&511u)<257u?-512:512;
    const std::array<std::array<int32_t,2>,4> offsets{{{{0,0}},{{nx,0}},{{0,nz}},{{nx,nz}}}};
    const auto shift=[](int32_t v){return v>=0?v/512:-((-v+511)/512);};
    for(const auto& d:offsets){const auto gx=shift(int32_t(x)+d[0]+32768)-a.start_x,gz=shift(int32_t(z)+d[1]+32768)-a.start_z;
        if(gx<0 || gz<0 || gx>=a.width || gz>=a.height)return false;
        const uint64_t local=uint64_t(gz)*a.width+uint64_t(gx);
        if(local>=a.cell_count || uint64_t(a.cell_first)+local>=map.cells.size())return false;}
    return true;
}
}
void initialize_class60_source_pool_8002e8d8(Class60SourcePool& pool) noexcept {
    pool={};for(size_t i=0;i<pool.slots.size();++i)pool.slots[i][1]=uint8_t(0x60u|i);
}
void class60_06_integrate_80110070(Class60_06Work& w) noexcept {
    for(size_t i=0;i<3;++i){const auto vel=s16(uint16_t(u16(w,0x44+i*2)+u16(w,0x4c+i*2)));
        const auto clamped=std::max<int32_t>(-24576,std::min<int32_t>(24576,vel));put16(w,0x44+i*2,uint16_t(clamped));
        put32(w,0x1c+i*4,u32(w,0x1c+i*4)+(uint32_t(clamped)<<8));}
}
Class60_06Result class60_06_step_8010f354(Work& original,const Class60_06Inputs& in) noexcept {
    Class60_06Result out;
    if(!(original[0]&original[6]&1u)){out.status=Status::Inactive;return out;}
    if((original[1]&0xe0u)!=0x60u){out.status=Status::WrongClass;return out;}
    if(original[2]!=6){out.status=Status::UnknownSubtype;return out;}
    if(original[8]>4){out.status=Status::UnknownState;return out;}
    auto w=original;uint16_t contact_return=0;
    switch(w[8]){
    case 0:{
        if(!in.references){out.status=Status::NeedsTrig;return out;}
        const auto& trig=in.references->trig[u16(w,0x64)&4095u];const auto scale=s32(u32(w,0x9c));
        w[0xb]=w[0xa]=w[0xc]=w[0xe]=w[0xf]=0;put16(w,0x7c,0);
        put16(w,0x44,uint16_t(uint32_t(-int64_t(trig.sin_q12)*scale)>>4));
        put16(w,0x46,uint16_t((0u-u32(w,0xa0))<<8));
        put16(w,0x48,uint16_t(uint32_t(-int64_t(trig.cos_q12)*scale)>>4));
        put16(w,0x4c,0);put16(w,0x4e,512);put16(w,0x50,0);put32(w,0x9c,0);packet(w);
        mirrors(w,0x1c,0x14);put32(w,0x34,u32(w,0x1c)-(uint32_t(int32_t(i16(w,0x44)))<<10));
        put32(w,0x38,u32(w,0x20));put32(w,0x3c,u32(w,0x24)-(uint32_t(int32_t(i16(w,0x48)))<<10));mirrors(w,0x34,0x2c);
        put32(w,0x78,0);if(!uv(w,in.profile)){out.status=uv_status(w,in.profile);return out;}
        put16(w,0x98,32);w[0x74]=1;w[0]|=2;
        if(!contact(w,in,out,contact_return))return out;
        if(contact_return){w[8]=3;w[0xd]=0;}
        ++w[8];center(w,true);w[0xe]&=0xfbu;break;
    }
    case 1:{
        if(!redirected(w,in,out,false))return out;
        const auto mask=w[0xa]?0x1c000000u:0x1a000000u;
        if(u32(w,0x9c)&mask){put32(w,0x9c,0);if(!release(w,in,out))return out;break;}
        put32(w,0x9c,0);
        if(!(w[3]&1u) && i16(w,0x7c)!=0){if(!release(w,in,out))return out;break;}
        if(w[3]&1u)put16(w,0x7c,0);
        snapshot(w);put32(w,0x8c,0x2c808080u);
        if(!uv(w,in.profile)){out.status=uv_status(w,in.profile);return out;}
        class60_06_integrate_80110070(w);mirrors(w,0x34,0x2c);mirrors(w,0x1c,0x14);
        if(!contact(w,in,out,contact_return))return out;
        if(contact_return){w[8]=3;w[0xd]=0;}
        if(w[0xc]>=150){w[8]=3;w[0xd]=0;}else ++w[0xc];center(w,true);w[0x76]=0;break;
    }
    case 2:
        packet(w);mirrors(w,0x1c,0x14);center(w,false);
        if(!uv(w,in.profile)){out.status=uv_status(w,in.profile);return out;}put16(w,0x98,32);
        if(!release(w,in,out))return out;
        break;
    case 3:{
        if(!redirected(w,in,out,true))return out;
        if(u32(w,0x9c)&(w[0xa]?0x1c000000u:0x1a000000u)){put32(w,0x9c,0);if(!release(w,in,out))return out;break;}
        put32(w,0x9c,0);
        if(u32(w,0x44)!=0 || i16(w,0x48)!=0){if(!uv(w,in.profile)){out.status=uv_status(w,in.profile);return out;}}
        snapshot(w);class60_06_integrate_80110070(w);mirrors(w,0x34,0x2c);mirrors(w,0x1c,0x14);
        if(!contact(w,in,out,contact_return))return out;
        center(w,true);
        const auto delay=s8(w[0xd]);if(delay>=30){if(!release(w,in,out))return out;break;}
        const auto sign=delay<15?1:-1;if((int32_t(delay)&sign)!=0)put32(w,0x8c,w[0x8c]==255?0x2c808080u:0x2cffffffu);
        ++w[0xd];break;
    }
    case 4:
        packet(w);put16(w,0x7c,0);w[0x76]=0;mirrors(w,0x1c,0x14);center(w,true);snapshot(w);
        if(!uv(w,in.profile)){out.status=uv_status(w,in.profile);return out;}put16(w,0x98,32);w[0]|=2;break;
    }
    if(!(w[3]&1u))++w[0xf];
    if(s8(w[0xf])==6)w[0xf]=0;
    if(!(w[3]&1u)){
        Class60_06ProxyIntent p;for(size_t i=0;i<3;++i)p.point[i]=i16(w,0x90+2*i);
        p.source_class=w[1];p.mode=w[0xa]?0x4000:0x8000;p.work9c=u32(w,0x9c);p.worka0=u32(w,0xa0);out.proxy_request=p;
        if(!in.proxy){out.status=Status::NeedsProxyConsumer;return out;}
        if(!same_proxy(p,in.proxy->before)){out.status=Status::StaleProxy;return out;}
        put32(w,0xa0,in.proxy->worka0_after);
        if(w[0]&3u){out.register_collision=true;if(!in.collision_registry_ready){out.status=Status::NeedsCollisionRegistry;return out;}}
        put16(w,0x7c,0);
    }
    out.shadow_request=w;
    if(!in.shadow_count_800987d8){out.status=Status::NeedsShadowConsumer;return out;}
    if(*in.shadow_count_800987d8<64){
        int32_t floor=i16(w,0x16);
        if(s8(w[0x76])==0){if(!in.shadow_floor_80048e6c){out.status=Status::NeedsShadowFloor;return out;}
            floor=std::max<int32_t>(*in.shadow_floor_80048e6c,floor);}
        Class60_06ShadowRecord record;record.point={{i16(w,0x14),s16(uint16_t(floor)),i16(w,0x18)}};
        record.heading_scale=uint16_t((u16(w,0x56)&4095u)|0x8000u);record.height_delta=uint16_t(floor-i16(w,0x16));out.shadow=record;
    }
    original=w;return out;
}
Class60_06Contact prepare_class60_06_contact_8010fdd4(
    const Work& original,const RuntimeMap& map,size_t area,
    std::optional<uint8_t> stage,bool audio,std::optional<uint32_t> fx) noexcept {
    Class60_06Contact out;out.before=original;out.after=original;
    if(u32(original,0x78)!=0){out.status=Status::UnsupportedBounds;return out;}
    ActorContactWork motion;motion.object_class=original[1];motion.state74=original[0x74];
    for(size_t i=0;i<4;++i){motion.raw[i]=i16(original,0x14+2*i);motion.fixed[i]=s32(u32(original,0x1c+4*i));motion.previous_raw[i]=i16(original,0x2c+2*i);motion.previous_fixed[i]=s32(u32(original,0x34+4*i));}
    for(size_t i=0;i<3;++i)motion.velocity[i]=i16(original,0x44+2*i);
    if(!neighbors(map,area,motion.raw[0],motion.raw[2])){out.status=Status::UnsupportedBoundary;return out;}
    const RetailPlayerBounds zero{0,0,0,0,0,0};const auto wall=query_actor_wall_contact_8004b6ac(map,area,motion,zero);
    if(!apply_actor_contact_result_8004b414(motion,wall)){out.status=Status::UnsupportedWall;return out;}
    out.source_wall_result=wall.result;auto& w=out.after;
    for(size_t i=0;i<4;++i){put16(w,0x14+2*i,uint16_t(motion.raw[i]));put32(w,0x1c+4*i,uint32_t(motion.fixed[i]));}
    for(size_t i=0;i<3;++i)put16(w,0x44+2*i,uint16_t(motion.velocity[i]));
    w[0x74]=motion.state74;
    if(wall.result==0){out.status=Status::SourceBackedPartial;return out;}
    if(!stage){out.status=Status::NeedsProfile;return out;}
    if(*stage==11 && i16(w,0x16)==-1){out.status=Status::UnsupportedSpecialStage;return out;}
    if(!audio){out.status=Status::NeedsContactSound;return out;}
    if(!fx || !fx_identity(*fx)){out.status=Status::NeedsEffectConsumer;return out;}
    if(uint32_t(wall.result)&0x1000u){
        const auto vy=i16(w,0x46);const auto abs=vy<0?-int32_t(vy):int32_t(vy);
        if(abs<=2560){out.source_return=1;for(size_t i=0;i<3;++i){put16(w,0x44+2*i,0);put16(w,0x4c+2*i,0);}}
        else {put16(w,0x46,uint16_t(uint32_t(abs*160)>>8));put16(w,0x44,uint16_t(asr(i16(w,0x44),1)));put16(w,0x48,uint16_t(asr(i16(w,0x48),1)));}
    }
    out.sound=179;out.sound_pointed_words=pointed(w);if(*fx)out.effect=contact_effect(w,*fx);
    auto velocities=std::array<int16_t,3>{{i16(w,0x44),i16(w,0x46),i16(w,0x48)}};velocities=reflected(velocities,uint32_t(wall.result));
    for(size_t i=0;i<3;++i)put16(w,0x44+2*i,uint16_t(velocities[i]));
    out.status=Status::SourceBackedPartial;return out;
}
std::optional<Class60_06Primitive> class60_06_primitive(const Work& w) noexcept {
    if(!(w[0]&w[6]&2u) || (w[0]&0x1cu)!=0 || (w[0]&0x20u)!=0)return std::nullopt;
    Class60_06Primitive p;for(size_t i=0;i<3;++i)p.center[i]=i16(w,0x90+i*2);
    p.half_extent=u16(w,0x98);p.clut=u16(w,0x8a);p.tpage=u16(w,0x88);p.uv={{w[0x96],w[0x97]}};p.color_command=u32(w,0x8c);p.source_fog=(w[0]&8u)==0;return p;
}
std::array<std::array<int16_t,3>,3> class60_06_camera_points(std::array<int32_t,3> c,uint16_t extent) noexcept {
    const auto x=uint32_t(c[0]),y=uint32_t(c[1]);const auto z=s16(uint16_t(c[2]));
    return {{{{s16(uint16_t(x-extent)),s16(uint16_t(y-extent)),z}},
              {{s16(uint16_t(x+extent)),s16(uint16_t(y-extent)),z}},
              {{s16(uint16_t(x)),s16(uint16_t(y)),z}}}};
}
std::array<uint32_t,4> class60_06_texture_words(const Class60_06Primitive& p) noexcept {
    const uint32_t uv=uint32_t(p.uv[0])|(uint32_t(p.uv[1])<<8);
    return {{(uint32_t(p.clut)<<16)|uv,(uint32_t(p.tpage)<<16)|(uv+31u),uv+7936u,uv+7967u}};
}
} // namespace l3
