// ST00 E0/07 floor switch and gate (0x80115A4C) on an authored 2x2 map:
// the gate cell's open descriptor 0 and the word0 0x40500 closed one.
#include "legends3ds/source_gate_e007.h"
#include <cassert>
#include <iostream>

namespace {
using namespace l3;
RuntimeMap map2x2(){
    RuntimeMap m;RuntimeMapArea a{};a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;
    m.areas.push_back(a);m.cells.assign(4,int16_t(1));m.idx.resize(2);m.idx[1].descriptor_index=0;
    m.descriptors.resize(2);m.descriptors[0].word0=0x10;m.descriptors[1].word0=0x40500;return m;
}
}

int main(){
    const auto m=map2x2();
    // Already used: both release at init.
    assert(!source_e007_init_80115a88(m,0,{{0,-384,0}},0,true));
    auto gate=*source_e007_init_80115a88(m,0,{{0,-384,0}},0,false);
    auto sw=*source_e007_init_80115a88(m,0,{{0,-1,300}},1,false);
    assert(gate.gate&&gate.state==1&&gate.idx_cell==1&&gate.open_descriptor==0&&gate.closed_descriptor==1);
    assert(!sw.gate&&sw.state==5);
    // Switch box: dx,dz within +-60, dy -10..160 of the switch.
    assert(!source_e007_step(sw,false,false,{{61,-1,300}}).set_607);
    assert(!source_e007_step(sw,false,false,{{0,-12,300}}).set_607);
    auto s=source_e007_step(sw,false,false,{{60,-1,240}});
    assert(s.set_607&&s.sound==0x11E&&sw.state==6&&sw.timer==16);
    for(int i=0;i<16;++i)(void)source_e007_step(sw,true,false,{{0,0,0}});
    assert(sw.state==7);(void)source_e007_step(sw,true,false,{{0,0,0}});assert(sw.state==8);
    // Gate: 0x607 closes (closed descriptor), 8 frames to the shake.
    s=source_e007_step(gate,false,false,{{0,0,0}});assert(!s.descriptor_write&&gate.state==1);
    s=source_e007_step(gate,true,false,{{0,0,0}});
    assert(s.descriptor_write==uint16_t(1)&&s.sound==0x119&&gate.state==2&&gate.timer==8);
    for(int i=0;i<7;++i)(void)source_e007_step(gate,true,false,{{0,0,0}});
    s=source_e007_step(gate,true,false,{{0,0,0}});
    assert(gate.state==3&&s.shake_frames==0x1E&&s.sound==0x118&&gate.anim_20==8*0x200);
    // Holds until the room is clear (0x61F), then 0x60A and a 64-frame reopen.
    assert(!source_e007_step(gate,true,false,{{0,0,0}}).set_60a);
    s=source_e007_step(gate,true,true,{{0,0,0}});assert(s.set_60a&&gate.state==4&&gate.timer==0x40);
    bool restored=false,bgm=false;
    for(int i=0;i<0x40;++i){s=source_e007_step(gate,true,true,{{0,0,0}});
        if(s.descriptor_write){assert(*s.descriptor_write==0&&gate.timer==0x10);restored=true;}
        if(s.bgm_8001b2f0)bgm=true;}
    assert(restored&&bgm&&gate.released);
    std::cout<<"PASS: E0/07 init release, switch box/press, gate close/shake/0x61F->0x60A/reopen/release\n";
}
