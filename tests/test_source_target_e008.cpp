// ST00 E0/08 targets/doors (0x80115E70) and the action check 0x800411DC on an
// authored map. Trig values are authored, not the retail table.
#include "legends3ds/source_target_e008.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
using namespace l3;
RuntimeMap map2x2(){
    RuntimeMap m;RuntimeMapArea a{};a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;
    m.areas.push_back(a);m.cells.assign(4,int16_t(1));m.idx.resize(2);m.idx[1].descriptor_index=0;
    m.descriptors.resize(3);m.descriptors[0].word0=0x70400;m.descriptors[1].word0=0x71400;m.descriptors[2].word0=0xF0900;return m;
}
ReferenceTables refs(){
    ReferenceTables r;
    for(size_t i=0;i<r.scalar_quarter.size();++i)r.scalar_quarter[i]=int16_t(std::lround(std::sin(double(i)*1.5707963267948966/1024.0)*4096));
    r.scalar_quarter_known=true;return r;
}
}

int main(){
    const auto m=map2x2();
    // Targets: 0x60 -> 0x608, 0x61 -> 0x609; already-set flag releases at init.
    assert(!source_e008_init_80115eac(m,0,{{0,-384,0}},0x60,true,false));
    auto t=*source_e008_init_80115eac(m,0,{{0,-576,0}},0x60,false,false);
    assert(t.target&&t.state==1&&t.hp_0a==100&&t.position[1]==-608&&t.heading_30==0xC00&&t.check_box);
    SourceE008HitRecord rec;SourceE008Inputs in;
    auto s=source_e008_step(t,rec,in);
    assert(s.register_proxy&&s.proxy_point[1]==-648&&s.sound==0);
    rec.packet=0x08000028u;s=source_e008_step(t,rec,in);           // 40 damage
    assert(t.hp_0a==60&&rec.packet==0&&s.sound==0xA0);
    // The action check on this target sets 0x60E and clears its check data.
    in.checked_by_player=true;s=source_e008_step(t,rec,in);
    assert(s.set_60e&&s.clear_player_check&&!t.check_box);in.checked_by_player=false;
    rec.packet=0x08000040u;s=source_e008_step(t,rec,in);           // 64 > 60: destroyed
    assert(s.set_608&&s.sound==0xA2&&t.state==2&&!s.set_609);
    int frames=0;bool debris=false,burst=false;
    while(!t.released&&frames<64){s=source_e008_step(t,rec,in);debris|=s.debris_effects==3;burst|=s.burst_effect;++frames;}
    assert(t.released&&debris&&burst&&frames==16+8+1+1);
    // A strong hit destroys at once; 0x61 raises 0x609.
    auto u=*source_e008_init_80115eac(m,0,{{0,-576,0}},0x61,false,false);SourceE008HitRecord r1;
    r1.packet=0x28000001u;s=source_e008_step(u,r1,in);assert(s.set_609&&u.state==2);

    // Doors: 0xE0 owns its cell (key 0x71400), opens once both flags are set.
    assert(!source_e008_init_80115eac(m,0,{{0,-384,0}},0xE0,true,true));
    auto d=*source_e008_init_80115eac(m,0,{{0,-384,0}},0xE0,false,false);
    assert(d.state==5&&d.idx_cell==uint16_t(1)&&d.closed_descriptor==0&&d.open_descriptor==1);
    auto v=*source_e008_init_80115eac(m,0,{{0,-384,0}},0xA0,false,false);assert(!v.idx_cell);
    SourceE008HitRecord none;in.flag_608=true;s=source_e008_step(d,none,in);
    assert(!s.descriptor_write&&d.state==5);
    in.flag_609=true;s=source_e008_step(d,none,in);
    assert(s.descriptor_write==uint16_t(1)&&d.state==6);
    for(int i=0;i<8;++i)(void)source_e008_step(d,none,in);
    (void)source_e008_step(d,none,in);assert(d.released);

    // Action check: a target 0x80 ahead (heading 0 faces -z) is found; behind it is not.
    const auto rf=refs();
    std::vector<SourceActionCheckCandidate> c{{{{0,-608,-200}},{-80,80,-64,64,-96,32}}};
    auto hit=source_action_check_800411dc(rf,{{0,-576,0}},0,c);
    assert(hit.index==size_t(0)&&hit.distance_182==200);
    assert(!source_action_check_800411dc(rf,{{0,-576,0}},0x800,c).index);   // facing away
    std::vector<SourceActionCheckCandidate> far{{{{0,-608,-400}},{-80,80,-64,64,-96,32}}};
    assert(!source_action_check_800411dc(rf,{{0,-576,0}},0,far).index);      // beyond the probes
    std::cout<<"PASS: E0/08 targets (hits, check 0x60E, destroy 0x608/0x609, break-up), doors (IDX open), action check\n";
}
