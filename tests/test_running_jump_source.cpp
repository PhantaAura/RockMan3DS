#include "legends3ds/runtime_session.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
namespace {
l3::RuntimeSession airborne_session(unsigned phase,int vertical,unsigned held,int control){
    l3::RuntimeMap m;l3::RuntimeMapArea area;area.start_x=56;area.start_z=56;
    area.width=16;area.height=16;area.cell_count=256;m.areas.push_back(area);m.cells.assign(256,int16_t(0x8000));
    l3::TutorialRuntimeArea runtime;L3QolSettings q{};l3_qol_settings_defaults(&q);
    l3::RuntimeSession s(std::move(m),std::move(runtime),{},q);
    l3::ReferenceTables r;for(auto& t:r.trig)t={0,4096};s.set_reference_tables(std::move(r));
    l3::RuntimePlayerReplaySeed seed;seed.position={0,-1000,0};seed.fixed_position={0,-1000*65536,0};
    seed.state={1,8,uint8_t(phase)};seed.velocity_work={512,int16_t(vertical),0};seed.contact_state=1;
    seed.animation_request=20;seed.animation_signed_control=int8_t(control);
    assert(s.seed_player_replay_diagnostic(seed));
    l3::RuntimeInputFrame input;input.held_psx_mask=uint16_t(held);s.tick(input);return s;
}
int value(const L3PlayerCompatWork& w,unsigned offset,unsigned n,bool signed_half){
    if(n==1)return w.bytes[offset];
    const auto v=uint16_t(w.bytes[offset]|uint16_t(w.bytes[offset+1])<<8);
    return signed_half?int(int16_t(v)):int(v);
}
}
int main(int argc,char** argv){
    assert(argc==2);std::ifstream in(argv[1]);assert(in);std::string line;unsigned cases=0;
    const std::array<unsigned,10> offsets{{9,10,11,0xAC,0x44,0x46,0x48,0x74,0x116,0x175}};
    while(std::getline(in,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);unsigned phase,held;int vertical,control;row>>phase>>vertical>>held>>control;
        std::array<int,10> expected{};for(auto& v:expected)row>>v;assert(row);
        const auto s=airborne_session(phase,vertical,held,control);
        for(size_t i=0;i<offsets.size();++i){
            const auto o=offsets[i];const unsigned n=(o==0x44||o==0x46||o==0x48||o==0x116)?2:1;
            const int actual=value(s.compat_player(),o,n,n==2);
            if(actual!=expected[i]){
                std::cerr<<"Source scalar mismatch case "<<cases<<" phase "<<phase<<" velocity "<<vertical
                         <<" held "<<held<<" control "<<control<<" offset "<<o<<": "<<actual<<" expected "<<expected[i]<<'\n';return 1;
            }
        }
        ++cases;
    }
    assert(cases==672);std::cout<<"PASS 672 owner-source action8 scalar cases; integration/audio excluded; PHYSICS_1TO1=false\n";
}
