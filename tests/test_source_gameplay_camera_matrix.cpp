// Matrix half of the original gameplay camera producer 0x80037FB4 and the
// lock-on selection 0x80034EB8 it feeds.
// 1. Differential against unchanged R3000 code: docs/research_01510/
//    gameplay_camera_matrix_probe.py ran 0x80037FB4 on 640 authored states
//    (authored trig tables, no retail values) and recorded 0x800A3A10,
//    0x1F800008..24 and 0x800A39F0. Every native word must match.
// 2. L3RF v1/v2 loading and rsin/rcos branch structure.
// 3. 0x80034EB8 with camera words from the producer: an enemy proxy straight
//    ahead is selected; to the side, behind or too deep it is not. The
//    retained pad 0x1F8000F2 never changes the result.
#include "legends3ds/source_gameplay_camera.h"
#include "legends3ds/source_camera.h"
#include "legends3ds/source_player_buster.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace l3;
void require(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
std::vector<std::vector<long long>> read_rows(const char* path){
    std::ifstream file(path);require(static_cast<bool>(file),"fixture unavailable");
    std::vector<std::vector<long long>> rows;std::string line;
    while(std::getline(file,line)){std::istringstream s(line);std::vector<long long> row;long long v;while(s>>v)row.push_back(v);rows.push_back(row);}
    require(!rows.empty(),"empty fixture");return rows;
}

void differential(const char* matrix_path,const char* quarter_path){
    const auto quarter=read_rows(quarter_path);
    require(quarter.size()==1&&quarter[0].size()==kReferenceQuarterEntries,"quarter fixture width");
    ReferenceTables refs;refs.scalar_quarter_known=true;
    for(size_t i=0;i<kReferenceQuarterEntries;++i)refs.scalar_quarter[i]=int16_t(quarter[0][i]);
    size_t number=0;
    for(const auto& row:read_rows(matrix_path)){
        require(row.size()==28,"matrix fixture width");
        const std::array<int32_t,3> target{int32_t(row[0]),int32_t(row[1]),int32_t(row[2])};
        const auto d=int32_t(row[3]),yaw=int32_t(row[4]),pitch=int32_t(row[5]);
        const auto shift=uint8_t(row[6]);const auto pad=uint16_t(row[7]);
        const std::array<size_t,3> idx{(0u-uint32_t(pitch))&4095u,uint32_t(yaw)&4095u,0};
        for(size_t i=0;i<3;++i)refs.trig[idx[i]]={int16_t(row[8+2*i]),int16_t(row[9+2*i])};
        const auto f=source_gameplay_camera_80037fb4(refs,target,d,yaw,pitch,shift,pad);
        require(!f.packed_trig_substituted,"quarter table in use");
        for(size_t i=0;i<3;++i)require(f.rotation_800a3a10[i]==row[14+i],"rotation 0x800A3A10");
        for(size_t i=0;i<8;++i)if(f.camera_1f800008[i]!=uint32_t(row[17+i])){
            std::cerr<<"row "<<number<<" word "<<i<<" got "<<f.camera_1f800008[i]<<" expected "<<row[17+i]<<'\n';std::exit(1);}
        for(size_t i=0;i<3;++i)require(f.eye_800a39f0[i]==row[25+i],"eye 0x800A39F0");
        ++number;
    }
    require(number==640,"all differential rows");
}

std::vector<uint8_t> l3rf(uint16_t version,const ReferenceTables& r){
    std::vector<uint8_t> b(32);
    auto p16=[&](size_t at,uint16_t v){b[at]=uint8_t(v);b[at+1]=uint8_t(v>>8);};
    auto p32=[&](size_t at,uint32_t v){p16(at,uint16_t(v));p16(at+2,uint16_t(v>>16));};
    b[0]='L';b[1]='3';b[2]='R';b[3]='F';p16(4,version);p16(6,32);p32(8,version==1?1:3);p32(12,4096);p32(16,32);
    for(const auto& e:r.trig){b.push_back(uint8_t(e.sin_q12));b.push_back(uint8_t(uint16_t(e.sin_q12)>>8));
        b.push_back(uint8_t(e.cos_q12));b.push_back(uint8_t(uint16_t(e.cos_q12)>>8));}
    if(version==2){p32(28,uint32_t(b.size()));
        for(auto v:r.scalar_quarter){b.push_back(uint8_t(v));b.push_back(uint8_t(uint16_t(v)>>8));}
        b.push_back(0);b.push_back(0);}
    p32(20,uint32_t(b.size()));p32(24,crc32(b.data()+32,b.size()-32));return b;
}
ReferenceTables canonical(){
    ReferenceTables r;r.scalar_quarter_known=true;
    for(size_t i=0;i<r.trig.size();++i){const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096.0)),int16_t(std::lround(std::cos(a)*4096.0))};}
    for(size_t i=0;i<kReferenceQuarterEntries;++i)r.scalar_quarter[i]=int16_t(std::lround(std::sin(double(i)*6.283185307179586/4096.0)*4096.0));
    return r;
}
void reference_format(){
    auto r=canonical();
    // Make the two tables disagree so a mix-up is visible.
    r.scalar_quarter[100]=1234;
    const auto v2=load_l3reference(l3rf(2,r));
    require(v2.scalar_quarter_known&&v2.scalar_quarter[100]==1234&&v2.trig[100].sin_q12==r.trig[100].sin_q12,"L3RF v2 load");
    const auto v1=load_l3reference(l3rf(1,r));
    require(!v1.scalar_quarter_known&&v1.trig[7].cos_q12==r.trig[7].cos_q12,"L3RF v1 still loads");
    auto bad=l3rf(2,r);bad[bad.size()-1]=1;bool threw=false;try{(void)load_l3reference(bad,false);}catch(...){threw=true;}
    require(threw,"L3RF v2 pad rejected");
    // rsin/rcos 0x8007A060/0x8007A12C branches, including negative angles.
    require(reference_rsin_8007a060(v2,100)==1234&&reference_rsin_8007a060(v2,1948)==1234,"rsin quarter mirror");
    require(reference_rsin_8007a060(v2,2148)==-1234&&reference_rsin_8007a060(v2,3996)==-1234,"rsin lower half");
    require(reference_rsin_8007a060(v2,-100)==-1234&&reference_rsin_8007a060(v2,4196)==1234,"rsin sign and wrap");
    require(reference_rcos_8007a12c(v2,924)==1234&&reference_rcos_8007a12c(v2,-924)==1234,"rcos even");
    require(reference_rcos_8007a12c(v2,1124)==-1234&&reference_rcos_8007a12c(v2,2972)==-1234&&reference_rcos_8007a12c(v2,3172)==1234,"rcos quadrants");
    // Without the quarter table the producer counts a packed substitution.
    const auto f=source_gameplay_camera_80037fb4(v1,{0,-150,0},0x200,0x800,0x30,0,0);
    require(f.packed_trig_substituted,"v1 substitution flagged");
}

// Lock-on: player at the origin facing -z (heading 0), camera behind per the
// follow path (yaw = heading + 0x800, pitch 0x30, distance 0x200, target y
// -0x96), boot projection OFX/OFY 160/120, H 384 (0x8007A4F0 + 0x80011F48).
struct Scene {
    ChildRuntime runtime;std::array<SourceHitProxyTarget,1> targets{};
    SourceBusterPlayer player;SourceGteRtpsState gte;std::optional<uint32_t> mask=0;
};
SourcePlayerBusterResult select(const ReferenceTables& refs,std::array<int16_t,3> enemy,uint16_t pad,uint8_t fire,uint8_t mode=0x49){
    const auto owned=std::make_unique<Scene>();auto& s=*owned;
    s.player.class_01=2;s.player.heading_56=0;
    for(auto& lane:s.player.lanes){lane.selection_170_or_171=0;lane.muzzle_3c8_or_3e0=std::array<uint16_t,3>{{0,uint16_t(-80),0}};lane.subtype_80089a8b=3;lane.counter_4fa=10;}
    SourceProjectionControls controls;source_projection_init_geom_8007a4f0(controls);
    source_projection_offset_8007b118(controls,160,120);source_projection_h_8007b138(controls,384);
    require(bind_source_projection_controls(s.gte,controls),"projection bind");
    // One class 20/03-shaped proxy (0x80102308: extent 80>>2 | 0x8000, mode 4).
    auto& bank=s.runtime.hit_proxy_bank();bank.count=1;
    s.targets[0].source_address=0x8009cf88u;bank.records[0].target=&s.targets[0];
    bank.records[0].encoded_identity=s.targets[0].source_address&0x007fffffu;
    bank.records[0].encoded_extent=uint16_t((80u>>2)|0x8000u);bank.records[0].point=enemy;
    const auto f=source_gameplay_camera_80037fb4(refs,{0,-0x96,0},0x200,0x800,0x30,0,pad);
    SourcePlayerTargetProfile window{1280,130,190,0,240,std::nullopt};
    SourcePlayerBusterInputs in;in.active=true;in.target_mode_49e=mode;in.fire_flags_174=fire;
    in.player_point_14=std::array<int16_t,3>{{0,0,0}};in.target_profile_8008994c=window;
    in.camera_1f800008=f.camera_1f800008;
    return source_player_select_and_spawn_80034eb8(s.runtime,s.gte,s.player,s.mask,in);
}
void lock_on(){
    const auto refs=canonical();
    const auto ahead=select(refs,{0,-80,-400},0,0);
    require(ahead.status==SourcePlayerBusterStatus::SourceBackedPartial&&ahead.projected_proxies==1,"ahead projects");
    require(ahead.selected_proxy[0]==0&&ahead.selected_proxy[1]==0,"ahead selected in both lanes");
    std::cout<<"ahead: player depth+50="<<ahead.player_depth_plus_50<<'\n';
    require(select(refs,{0,-80,-400},0xffff,0).selected_proxy==ahead.selected_proxy,"pad never reaches the GTE");
    const auto side=select(refs,{600,-80,-400},0,0);
    require(side.projected_proxies==1&&side.selected_proxy[0]==-1&&side.selected_proxy[1]==-1,"side out of window");
    const auto behind=select(refs,{0,-80,300},0,0);
    require(behind.projected_proxies==1&&behind.selected_proxy[0]==-1&&behind.selected_proxy[1]==-1,"behind rejected by depth");
    const auto far=select(refs,{0,-80,-1200},0,0);
    require(far.projected_proxies==1&&far.selected_proxy[0]==-1&&far.selected_proxy[1]==-1,"beyond depth 0x500 rejected");
    // Bit 0x10 spawns lane 1 toward the selected proxy.
    const auto fired=select(refs,{0,-80,-400},0,0x10);
    require(fired.status==SourcePlayerBusterStatus::SourceBackedPartial&&fired.spawn_attempted[1]&&!fired.spawn_attempted[0],"lane 1 spawn");
    require(fired.spawns[1].status==SourceBusterStatus::SourceBackedPartial,"lane 1 spawn admitted");
    // Target mode GP+0x49E gates by proxy mode: (0x49 << 4) has bit 7 set,
    // (0x01 << 4) does not, so that proxy is never projected or selected.
    const auto gated=select(refs,{0,-80,-400},0,0,0x01);
    require(gated.projected_proxies==0&&gated.selected_proxy[0]==-1&&gated.selected_proxy[1]==-1,"mode gate");
}
} // namespace

int main(int argc,char** argv){
    require(argc==3,"usage: test_source_gameplay_camera_matrix MATRIX.tsv QUARTER.tsv");
    differential(argv[1],argv[2]);reference_format();lock_on();
    std::cout<<"PASS: 0x80037FB4 matrix words (640 original-code rows), L3RF v2 rsin/rcos, 0x80034EB8 lock-on window\n";
}
