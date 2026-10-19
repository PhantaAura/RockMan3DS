// Player door event: 0x800514FC query, 0x80051970 gate, 0x80050C2C case 3
// matching/request lanes, event-0 arrival 0x80048898, the event-0 door
// sequence (0x800485E4: 0x80042238, state 3 0x80041B8C, input 0x8003C07C,
// walk 0x80048A9C, release 0x80042314, state 0 0x8003C7B0), the ST00 door
// object 0x8011C928 states 0..5 and the session commit.
// Synthetic map and aux data only; the values mirror the Area 1 door record.
#include "legends3ds/runtime_session.h"
#include "legends3ds/source_door_event.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace l3;
namespace {
void put(L3PlayerCompatWork& w,size_t o,int16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(uint16_t(v)>>8);}
int16_t get(const L3PlayerCompatWork& w,size_t o){return int16_t(uint16_t(w.bytes[o])|(uint16_t(w.bytes[o+1])<<8));}
int32_t get32(const L3PlayerCompatWork& w,size_t o){uint32_t v=0;for(size_t i=0;i<4;++i)v|=uint32_t(w.bytes[o+i])<<(8*i);return int32_t(v);}
int16_t lane(const std::array<uint8_t,26>& raw,size_t at){return int16_t(uint16_t(raw[at]|(uint16_t(raw[at+1])<<8)));}

ReferenceTables refs(){
    ReferenceTables r;for(auto& t:r.trig)t={0,4096};
    r.trig[0x400]={4096,0};r.trig[0x800]={0,-4096};r.trig[0xC00]={-4096,0};
    r.trig[0x900]={-1567,-3784};
    return r;
}
// One 2x2 area around the origin; every cell is IDX 1 at `origin`. Group 0 is
// a floor (y=0), group 1 the door event primitive (code, flags 0x8000).
RuntimeMap door_map(RuntimeMapIdx origin,uint16_t code=0x301,uint16_t flags=0x8000,size_t areas=1){
    RuntimeMap m;
    for(size_t i=0;i<areas;++i){RuntimeMapArea a;a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;m.areas.push_back(a);}
    m.cells={1,1,1,1};m.idx.resize(2);origin.descriptor_index=0;m.idx[1]=origin;
    RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,2});
    m.contact_groups.push_back({-512,512,-512,512,0,1});
    m.contact_groups.push_back({-129,129,-129,129,1,1});
    m.contact_primitives.push_back({0,1024,3,0});
    m.contact_primitives.push_back({-33,33,code,flags});
    return m;
}
// The same door on an 8x8 grid with a 4096-unit floor, so the forced walks
// (about 30 frames at 32 units) stay on it. `door_pair` adds the closed/open
// descriptor pair 0x61900/0x60900 that the ST00 door object swaps.
RuntimeMap walk_map(bool door_pair){
    RuntimeMap m=door_map({},0x301,0x8000,2);
    for(auto& a:m.areas){a.start_x=60;a.start_z=60;a.width=8;a.height=8;a.cell_count=64;}
    m.cells.assign(64,1);
    m.contact_groups[0]={-2048,2048,-2048,2048,0,1};
    if(door_pair){
        m.descriptors[0].word0=0x61900;
        RuntimeMapDescriptor open;open.word0=0x60900;open.default_contact_index=0;m.descriptors.push_back(open);
    }
    return m;
}
L3PlayerCompatWork player_at(int16_t x,int16_t y,int16_t z,uint16_t heading){
    L3PlayerCompatWork w{};l3_pl00_install_control_profile_800425fc(&w,0);
    put(w,0x14,x);put(w,0x16,y);put(w,0x18,z);put(w,0x56,int16_t(heading));
    w.bytes[8]=1;return w;
}
St00AuxRecord record(std::array<int16_t,12> h){
    St00AuxRecord r;r.halfwords=h;
    for(size_t i=0;i<12;++i){r.raw[i*2]=uint8_t(uint16_t(h[i]));r.raw[i*2+1]=uint8_t(uint16_t(h[i])>>8);}
    return r;
}
// Area 1 aux record 2 (0x8000 transition to stage 0 area 2) at an origin door.
St00AuxRecord area1_door(){return record({int16_t(0x8000),0,0,512,0,0,0,2048,-256,-1,6272,2048});}

void query_800514fc(){
    const auto r=refs();auto m=door_map({});
    // Heading 0x800 faces +z: the tested point is 64 ahead, z -100+64 = -36.
    auto w=player_at(0,-1,-100,0x800);
    auto q=runtime_player_event_primitive_800514fc(m,0,w,&r);
    assert(q.status==RuntimeEventQueryStatus::Hit&&q.sample&&q.candidate_cells==1);
    assert(q.sample->x==0&&q.sample->y==0&&q.sample->z==0);
    assert(q.sample->type==3&&q.sample->flags==0x8000&&q.sample->heading==0x800&&q.sample->primitive_index==1);
    // Flag 0x8000 tests the front point: z=-200 hits via -136, while the same
    // pose without the flag tests the player itself and misses.
    auto plain=door_map({},0x301,0);
    put(w,0x18,-200);
    assert(runtime_player_event_primitive_800514fc(m,0,w,&r).status==RuntimeEventQueryStatus::Hit);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Miss);
    put(w,0x18,-240);
    assert(runtime_player_event_primitive_800514fc(m,0,w,&r).status==RuntimeEventQueryStatus::Miss);
    put(w,0x18,-160);q=runtime_player_event_primitive_800514fc(plain,0,w,&r);
    assert(q.status==RuntimeEventQueryStatus::Hit);
    // Strict expanded rectangle: x0 - 32 = -161 misses, -160 hits.
    put(w,0x18,0);put(w,0x14,-161);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Miss);
    put(w,0x14,-160);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Hit);
    // Y: bounds are subtracted twice: lo = -33 - 0, hi = 33 + 320 (y0 = -160).
    put(w,0x16,-33);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Miss);
    put(w,0x16,-32);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Hit);
    put(w,0x16,352);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Hit);
    put(w,0x16,353);
    assert(runtime_player_event_primitive_800514fc(plain,0,w,&r).status==RuntimeEventQueryStatus::Miss);
    // Centres use the IDX origin and halfword arithmetic.
    auto shifted=door_map({-256,0,256,0,0});
    auto at=player_at(-256,-1,200,0x800);
    q=runtime_player_event_primitive_800514fc(shifted,0,at,&r);
    assert(q.status==RuntimeEventQueryStatus::Hit&&q.sample->x==-256&&q.sample->y==0&&q.sample->z==256);
    // Unported shapes and missing trig hold instead of reporting a miss.
    assert(runtime_player_event_primitive_800514fc(m,0,player_at(0,-1,-100,0x800),nullptr).status==RuntimeEventQueryStatus::NeedsTrig);
    auto family2=door_map({},0x321,0);
    q=runtime_player_event_primitive_800514fc(family2,0,player_at(0,-1,0,0),&r);
    assert(q.status==RuntimeEventQueryStatus::Unsupported&&q.unsupported_code==0x321);
    auto slope=door_map({},0x308,0);
    assert(runtime_player_event_primitive_800514fc(slope,0,player_at(0,-1,0,0),&r).status==RuntimeEventQueryStatus::Unsupported);
    auto none=door_map({},0x001,0);
    assert(runtime_player_event_primitive_800514fc(none,0,player_at(0,-1,0,0),&r).status==RuntimeEventQueryStatus::Miss);
}

void gate_80051970(){
    auto w=player_at(0,-1,0,0);
    assert(!source_door_gate_80051970(w));                 // nothing pressed
    put(w,0x11E,0x2000);assert(source_door_gate_80051970(w)); // Circle = +0x136
    put(w,0x11E,0x4000);assert(!source_door_gate_80051970(w));
    put(w,0x11E,0x2000);
    w.bytes[9]=1;assert(source_door_gate_80051970(w));
    w.bytes[9]=2;assert(!source_door_gate_80051970(w));
    w.bytes[9]=4;assert(source_door_gate_80051970(w));
    w.bytes[9]=0;w.bytes[0x187]=1;assert(!source_door_gate_80051970(w));
    w.bytes[0x187]=0;w.bytes[0x174]=0x40;assert(!source_door_gate_80051970(w));
    w.bytes[0x174]=0;w.bytes[0x177]=1;assert(!source_door_gate_80051970(w));
}

SourceDoorResult evaluate(const L3PlayerCompatWork& w,const St00AuxList& aux,const uint8_t* flags=nullptr,
                          uint16_t code=0x301){
    RuntimeEventPrimitiveQuery q;q.status=RuntimeEventQueryStatus::Hit;
    RuntimeEventPrimitiveSample s;s.heading=uint16_t(get(w,0x56));s.type=uint16_t(code>>8);s.flags=0x8000;q.sample=s;
    static uint8_t clear[kL3StoryFlagBytes]{};
    return evaluate_source_door_event_80050c2c(q,w,aux,flags?flags:clear,kL3StoryFlagBytes);
}
void case3_80050c2c(){
    St00AuxList aux;
    aux.records.push_back(record({0,0,0,0,0,0,512,0,0,0,0,0}));   // other position
    aux.records.push_back(area1_door());
    auto w=player_at(0,-1,-33,0x800);put(w,0x11E,0x2000);
    auto r=evaluate(w,aux);
    assert(r.status==SourceDoorStatus::TransitionRequested&&r.aux_record_index==1u&&r.flag_set==0x7B0);
    assert(r.request&&!r.locked_message);
    const auto& q=*r.request;
    // 3558/355C: sample x,y,z and facing; 3560/3564: hw8..11; 3568 hw7; 356A hw11.
    assert(lane(q.raw,0)==0&&lane(q.raw,2)==0&&lane(q.raw,4)==0&&lane(q.raw,6)==0x800);
    assert(lane(q.raw,8)==-256&&lane(q.raw,10)==-1&&lane(q.raw,12)==6272&&lane(q.raw,14)==2048);
    assert(lane(q.raw,16)==2048&&lane(q.raw,18)==2048);
    assert(!q.known[20]);                                       // 356C is not written
    assert(q.known[21]&&q.raw[21]==0&&q.raw[22]==0&&q.raw[23]==2&&q.raw[24]==2&&q.raw[25]==10);
    for(size_t i=0;i<26;++i)if(i!=20)assert(q.known[i]);
    assert(q.sound_id==0x56&&q.sound_argument==5&&q.music_check_8001f6c4);
    const auto t=source_door_transition_intent(q,7);
    assert(t.source_function==0x80050C2Cu&&t.source_generation==7&&t.destination_area==2&&t.source_stage==0);
    assert(t.spawn[0]==-256&&t.spawn[1]==-1&&t.spawn[2]==6272&&!t.known[12]&&t.raw[16]==2&&t.raw[17]==10);

    // Bit 0x400 gate: the far side (heading 0, player beyond) also requests.
    auto far=player_at(0,-1,40,0);put(far,0x11E,0x2000);
    assert(evaluate(far,aux).status==SourceDoorStatus::TransitionRequested);
    // Side test: facing +z (quadrant 2) from beyond the door fails.
    auto beyond=player_at(0,-1,40,0x800);put(beyond,0x11E,0x2000);
    assert(evaluate(beyond,aux).status==SourceDoorStatus::NoAuxMatch);
    // Sideways heading fails the window.
    auto side=player_at(0,-1,-33,0x400);put(side,0x11E,0x2000);
    assert(evaluate(side,aux).status==SourceDoorStatus::NoAuxMatch);
    // Signed heading -0x700: divide truncates toward zero, quadrant 3 (not 2):
    // x <= sample passes although z is beyond the door.
    auto neg=player_at(-1,-1,40,uint16_t(int16_t(-0x700)));put(neg,0x11E,0x2000);
    assert(evaluate(neg,aux).status==SourceDoorStatus::TransitionRequested);
    put(neg,0x14,1);assert(evaluate(neg,aux).status==SourceDoorStatus::NoAuxMatch);

    // 0x800519E4 locks and the message request.
    uint8_t flags[kL3StoryFlagBytes]{};
    flags[0x7E0>>3]|=uint8_t(1u<<(0x7E0&7));
    r=evaluate(w,aux,flags);
    assert(r.status==SourceDoorStatus::Locked&&r.locked_message==0&&!r.request&&!r.flag_set);
    flags[0x7C0>>3]|=uint8_t(1u<<(0x7C0&7));
    r=evaluate(w,aux,flags);assert(r.status==SourceDoorStatus::Locked&&!r.locked_message);
    St00AuxList keyed;keyed.records.push_back(record({int16_t(0x8003),0,77,512,0,0,0,2048,-256,-1,6272,2048}));
    uint8_t id_flags[kL3StoryFlagBytes]{};id_flags[0x7E3>>3]|=uint8_t(1u<<(0x7E3&7));
    r=evaluate(w,keyed,id_flags);assert(r.status==SourceDoorStatus::Locked&&r.locked_message==77);
    // Halfword 0 bit 15 clear: local door, flag 0x7B1 only.
    St00AuxList local;local.records.push_back(record({0,0,0,0,0,0,0,2048,0,0,0,0}));
    r=evaluate(w,local);assert(r.status==SourceDoorStatus::LocalDoorOpened&&r.flag_set==0x7B1&&!r.request);

    // Prefix checks.
    auto busy=w;busy.bytes[0xBD]=1;assert(evaluate(busy,aux).status==SourceDoorStatus::PlayerBusy);
    auto owner=w;owner.bytes[1]=0x10;assert(evaluate(owner,aux).status==SourceDoorStatus::GateClosed);
    auto idle=w;put(idle,0x11E,0);assert(evaluate(idle,aux).status==SourceDoorStatus::GateClosed);
    assert(evaluate(w,aux,nullptr,0x201).status==SourceDoorStatus::UnsupportedType);
    RuntimeEventPrimitiveQuery miss;miss.status=RuntimeEventQueryStatus::Miss;
    assert(evaluate_source_door_event_80050c2c(miss,w,aux,nullptr,0).status==SourceDoorStatus::NoEvent);
}

void arrival_80048898(){
    SourceDoorRequest q;
    const auto lanes=[&](size_t at,int16_t v){q.raw[at]=uint8_t(uint16_t(v));q.raw[at+1]=uint8_t(uint16_t(v)>>8);q.known[at]=q.known[at+1]=1;};
    lanes(8,-256);lanes(10,-1);lanes(12,6272);lanes(14,2048);lanes(18,2048);
    auto t=source_door_transition_intent(q,1);
    auto w=player_at(10,-1,20,0);w.bytes[0x1A]=0x5A;w.bytes[0x28]=0x6B;
    const auto r=refs();
    assert(apply_source_door_arrival_80048898(t,r,w));
    // 6272 + (rcos(0x800)*64 >> 12) = 6208; y is 3562 - 1.
    assert(get(w,0x14)==-256&&get(w,0x16)==-2&&get(w,0x18)==6208&&get(w,0x56)==2048);
    assert(get32(w,0x1C)==int32_t(uint32_t(-256)<<16)&&get32(w,0x20)==-2*65536&&get32(w,0x24)==6208*65536);
    assert(get(w,0x2C)==-256&&get(w,0x2E)==-2&&get(w,0x30)==6208&&get(w,0x5E)==2048);
    assert(get(w,0x438)==-256&&get(w,0x442)==2048&&w.bytes[0x448]==48&&get(w,0x54)==0&&get(w,0x58)==0);
    assert(w.bytes[0x1A]==0x5A&&w.bytes[0x28]==0x6B);
    // A non-axis heading uses the arithmetic shift (floor) of rsin/rcos*64.
    lanes(18,0x900);t=source_door_transition_intent(q,1);
    assert(apply_source_door_arrival_80048898(t,r,w));
    assert(get(w,0x14)==-256+((-1567*64)>>12)&&get(w,0x14)==-281&&get(w,0x18)==6272-60);
    SourceDoorRequest missing;assert(!apply_source_door_arrival_80048898(source_door_transition_intent(missing,1),r,w));
}

void session_commit(){
    TutorialRuntimeArea a0;a0.area_index=0;a0.flow.aux.records.push_back(
        record({int16_t(0x8000),0,0,0x0100,0,0,0,2048,0,-1,100,2048}));
    L3QolSettings qol{};l3_qol_settings_defaults(&qol);
    RuntimeSession s(walk_map(false),a0,{},qol);s.set_reference_tables(refs());
    s.reset_player({0,-1,-100},0x800,100,100);
    RuntimeInputFrame idle{};s.tick(idle);
    assert(s.source_door_result().status==SourceDoorStatus::GateClosed&&!s.source_door_transition());
    // Story flag 0x200 suppresses the call entirely.
    s.set_story_flag(0x200,true);
    RuntimeInputFrame press{};press.held_psx_mask=0x2000;press.pressed_psx_mask=0x2000;
    s.tick(press);assert(!s.source_door_transition()&&!s.source_door_event0().active);
    s.set_story_flag(0x200,false);
    s.tick(idle);s.tick(press);
    assert(s.source_door_result().status==SourceDoorStatus::TransitionRequested);
    // Event 0 starts at once; the load is not offered before substate 6.
    assert(s.story_flag(0x7B0)&&s.source_door_event0().active&&s.source_door_event0().phase==1);
    assert(!s.source_door_transition()&&s.compat_player().bytes[0xBD]==0x40);
    // This synthetic cell has no 0x1000 door pair: 0x7B1 opens nothing, so
    // 0x7B2 never holds substates 2 and 5.
    int frames=0;
    while(!s.source_door_transition()&&frames<100){s.tick(press);++frames;}
    const auto& e=s.source_door_event0();
    assert(s.source_door_transition()&&e.substate==6&&e.status_356c==1&&e.counter==40);
    assert(s.source_door_transition()->destination_area==1&&s.story_flag(0x7B0)&&!s.story_flag(0x7B1));
    // The destination must be registered before anything changes.
    assert(!s.can_commit_source_door_transition()&&!s.commit_source_door_transition()&&s.map_area_index()==0);
    TutorialRuntimeArea a1;a1.area_index=1;s.register_area(1,a1);
    // Substate 6 keeps waiting; a pending request is not re-evaluated.
    s.tick(press);assert(s.source_door_transition()&&s.map_area_index()==0&&e.counter==41);
    assert(s.can_commit_source_door_transition()&&s.commit_source_door_transition());
    const auto p=s.player_position();
    assert(s.map_area_index()==1&&p.x==0&&p.y==-2&&p.z==36&&s.player_yaw()==2048);
    assert(e.active&&e.substate==9&&e.status_356c==0&&e.counter==44&&!s.source_door_transition());
    const auto& w=s.compat_player();
    assert(w.bytes[8]==3&&w.bytes[9]==1);
    assert(get(w,0x2C)==0&&get(w,0x2E)==-2&&get(w,0x30)==36);
    // Second walk until counter > 108, then phase 2 returns control.
    frames=0;while(e.active&&frames<200){s.tick(idle);++frames;}
    assert(!e.active&&e.counter==112);
    assert(!s.story_flag(0x7B0)&&!s.story_flag(0x7B1)&&!s.story_flag(0x7B3)&&s.story_flag(0x380));
    assert(w.bytes[8]==0&&w.bytes[0xBD]==8);
    // Player state 0 (0x8003C7B0) then normal updates: +0xBD bit 8 clears.
    s.tick(idle);assert(w.bytes[8]==1&&w.bytes[9]==0);
    s.tick(idle);assert(w.bytes[0xBD]==0);
    assert(s.player_position().z>36&&s.player_position().y==0&&s.player_contact().grounded.value_or(false));
}

void event0_player_routines(){
    assert(source_event0_walk_limit_8008aa04(0)==108&&source_event0_walk_limit_8008aa04(19)==0x70&&
           source_event0_walk_limit_8008aa04(1)==0&&source_event0_walk_limit_8008aa04(40)==0);
    // 0x80042238.
    auto w=player_at(0,-1,0,0);w.bytes[0]=1;put(w,0xB4,77);
    auto r=source_player_event_ready_80042238(w,std::nullopt);
    assert(r&&*r&&w.bytes[0x449]==0xFF&&w.bytes[0xBD]==0x40&&get(w,0xB4)==0);
    w.bytes[0x174]=0x40;put(w,0xB4,77);r=source_player_event_ready_80042238(w,std::nullopt);
    assert(r&&!*r&&get(w,0xB4)==77);
    w.bytes[9]=0x0D;w.bytes[10]=0x02;r=source_player_event_ready_80042238(w,std::nullopt);
    assert(r&&!*r&&w.bytes[10]==6);
    w.bytes[9]=14;assert(!source_player_event_ready_80042238(w,std::nullopt));
    r=source_player_event_ready_80042238(w,uint8_t(0));assert(r&&!*r);   // byte 0: mask path only
    w.bytes[0x174]=0;r=source_player_event_ready_80042238(w,uint8_t(1));assert(r&&*r);
    w.bytes[8]=3;w.bytes[9]=5;r=source_player_event_ready_80042238(w,std::nullopt);assert(r&&*r);
    w.bytes[8]=2;r=source_player_event_ready_80042238(w,std::nullopt);assert(r&&!*r);
    w.bytes[0]=0;r=source_player_event_ready_80042238(w,std::nullopt);assert(r&&*r);

    // 0x80041B8C state 3: substate 1 -> 2 pins the pose to the targets.
    auto p=player_at(5,-1,7,0x123);p.bytes[8]=3;p.bytes[9]=1;p.bytes[0xAC]=9;p.bytes[0xA4]=4;
    put(p,0x438,-256);put(p,0x43A,-2);put(p,0x43C,6208);put(p,0x442,2048);p.bytes[0x448]=48;
    for(size_t i=0;i<4;++i){p.bytes[0x428+i]=0x11;p.bytes[0x42C+i]=0x22;p.bytes[0x430+i]=0x33;}
    source_player_state3_80041b8c(p);
    assert(p.bytes[9]==2&&p.bytes[0xBD]==0x40&&p.bytes[0x104]==9&&p.bytes[0x100]==4);
    assert(get(p,0x14)==-256&&get(p,0x16)==-2&&get(p,0x18)==6208&&get(p,0x56)==2048);
    assert(get(p,0x2C)==5&&get(p,0x30)==7&&get(p,0x5E)==0x123&&p.bytes[0x1C]==0x11&&p.bytes[0x24]==0x33);
    assert(p.bytes[0xAC]==48&&p.bytes[0x448]==0xFF);
    source_player_state3_80041b8c(p);assert(get(p,0x2C)==-256&&p.bytes[9]==2);
    p.bytes[9]=3;source_player_state3_80041b8c(p);
    assert(p.bytes[8]==1&&p.bytes[9]==0&&p.bytes[0xBD]==8&&(p.bytes[0x175]&1));

    // 0x8003C07C input selection.
    auto in=player_at(0,0,0,0);put(in,0x11C,0x2010);put(in,0x11E,0x2000);
    assert(!source_player_input_select_8003c07c(in,0x10)&&get(in,0x11C)==0x2010);
    in.bytes[0xBD]=0x20;assert(source_player_input_select_8003c07c(in,0x10)&&get(in,0x11C)==0x10&&get(in,0x11E)==0);
    in.bytes[0xBD]=0x60;put(in,0x11C,0x10);assert(source_player_input_select_8003c07c(in,0x10)&&get(in,0x11C)==0);

    // 0x80048A9C: first walk until counter >= 37, second while counter <= +11.
    SourceDoorEvent0 e;e.substate=3;auto wk=player_at(0,0,0,0);wk.bytes[0xBD]=8;e.counter=36;
    source_event0_forced_walk_80048a9c(e,wk,0);
    assert(e.walk_state==1&&e.walk_limit==108&&e.forced_pad==0x10&&wk.bytes[0xBD]==0x28&&e.substate==3);
    e.counter=37;source_event0_forced_walk_80048a9c(e,wk,0);
    assert(e.walk_state==2&&e.substate==4&&e.forced_pad==0&&wk.bytes[0xBD]==0x48);
    e.substate=11;wk.bytes[0xBD]=0;e.counter=108;source_event0_forced_walk_80048a9c(e,wk,0);
    assert(e.walk_state==3&&e.forced_pad==0x10&&wk.bytes[0xBD]==0x20&&e.substate==11);
    e.counter=109;source_event0_forced_walk_80048a9c(e,wk,0);
    assert(e.walk_state==4&&e.substate==12&&wk.bytes[0xBD]==0x40);
    e.counter=1;source_event0_forced_walk_80048a9c(e,wk,0);assert(e.substate==12&&e.forced_pad==0);

    // 0x80042314 and 0x8003C7B0.
    auto rel=player_at(0,0,0,0);rel.bytes[0]=1;rel.bytes[8]=1;rel.bytes[0x449]=0xFF;rel.bytes[0x190]=4;
    source_player_event_release_80042314(rel);
    assert(rel.bytes[0]==3&&rel.bytes[6]==3&&rel.bytes[0xBD]==8&&rel.bytes[0x449]==0&&rel.bytes[0x190]==0);
    rel.bytes[8]=3;rel.bytes[0xBD]=0;source_player_event_release_80042314(rel);
    assert(rel.bytes[9]==3&&rel.bytes[0xBD]==0);
    rel.bytes[8]=0;put(rel,0x44,90);rel.bytes[0xAC]=5;source_player_state0_8003c7b0(rel);
    assert(get32(rel,8)==1&&get(rel,0x6C)==60&&get(rel,0x44)==0&&rel.bytes[0x175]==7&&rel.bytes[0xAC]==0);
}

void event0_door_object(){
    auto m=walk_map(true);
    assert(!source_door_object_init_801124b0(walk_map(false),1)&&!source_door_object_init_801124b0(m,0));
    auto o=*source_door_object_init_801124b0(m,1);
    assert(o.state==1&&o.closed_descriptor==0&&o.open_descriptor==1);
    // State 1: 0x7B1 alone or the player elsewhere opens nothing.
    assert(!source_door_object_step(o,true,false,int16_t(1),m).flag_7b2&&o.state==1);
    assert(!source_door_object_step(o,true,true,int16_t(2),m).flag_7b2&&o.state==1);
    auto st=source_door_object_step(o,false,true,int16_t(1),m);
    assert(o.state==2&&o.frames==16&&st.flag_7b2==true&&st.sound==0x105&&!st.descriptor_write);
    for(int i=0;i<15;++i){st=source_door_object_step(o,true,true,int16_t(1),m);assert(!st.flag_7b2&&o.state==2);}
    st=source_door_object_step(o,true,true,int16_t(1),m);
    assert(o.state==3&&st.flag_7b2==false&&st.descriptor_write==1&&st.sound==0x185);
    // Open: 0x7B0 holds it while 0x7B1 is set; clearing 0x7B1 closes it.
    assert(!source_door_object_step(o,true,true,int16_t(2),m).flag_7b2&&o.state==3);
    st=source_door_object_step(o,true,false,int16_t(2),m);
    assert(o.state==4&&o.frames==16&&st.flag_7b2==true&&!st.flag_7b1&&st.sound==0x106);
    for(int i=0;i<15;++i)assert(!source_door_object_step(o,true,false,int16_t(2),m).flag_7b2);
    st=source_door_object_step(o,true,false,int16_t(2),m);assert(o.state==5&&st.flag_7b2==false&&st.sound==0x186);
    st=source_door_object_step(o,true,false,int16_t(2),m);assert(o.state==0&&st.descriptor_write==0);
    source_door_object_step(o,true,false,int16_t(2),m);assert(o.state==1&&o.open_descriptor==1);
    // Without 0x7B0 (local door), leaving the cell closes it and clears 0x7B1.
    o.state=3;st=source_door_object_step(o,false,true,int16_t(1),m);assert(o.state==3&&!st.flag_7b2);
    st=source_door_object_step(o,false,true,int16_t(2),m);assert(o.state==4&&st.flag_7b1==false&&st.flag_7b2==true);

    // Session: the door opens (descriptor 0 -> 1 in the active map) before the
    // first walk, holds substate 2 for 16 frames, and is restored after it.
    TutorialRuntimeArea a0;a0.area_index=0;a0.flow.aux.records.push_back(
        record({int16_t(0x8000),0,0,0x0100,0,0,0,2048,0,-1,100,2048}));
    TutorialRuntimeArea a1=a0;a1.area_index=1;
    L3QolSettings qol{};l3_qol_settings_defaults(&qol);
    RuntimeSession s(walk_map(true),a0,{},qol);s.set_reference_tables(refs());s.register_area(1,a1);
    s.reset_player({0,-1,-100},0x800,100,100);
    RuntimeInputFrame idle{},press{};press.held_psx_mask=0x2000;press.pressed_psx_mask=0x2000;
    s.tick(idle);s.tick(press);
    const auto& e=s.source_door_event0();
    assert(e.active&&e.phase==1&&e.substate==0);   // phase 0 handed over at once
    s.tick(idle);   // substate 0: targets; player enters state 3 next frame
    s.tick(idle);   // substate 1: 0x7B1; the object opens in the same frame
    assert(s.story_flag(0x7B1)&&s.story_flag(0x7B2)&&s.source_door_objects().size()==1&&e.substate==2);
    assert(s.active_map().idx[1].descriptor_index==0);
    int held=0;while(s.story_flag(0x7B2)){s.tick(idle);++held;}
    assert(held==16&&s.active_map().idx[1].descriptor_index==1&&e.substate==2);
    // Pinned in front of the door: 0x800 heading, 64 back from the sample.
    assert(s.player_position().x==0&&s.player_position().z==-64);
    while(!s.source_door_transition())s.tick(idle);
    assert(s.active_map().idx[1].descriptor_index==0&&!s.story_flag(0x7B2)&&s.player_position().z>-64);
    assert(s.can_commit_source_door_transition()&&s.commit_source_door_transition());
    assert(s.source_door_objects().empty()&&s.active_map().idx[1].descriptor_index==0);
    s.tick(idle);assert(s.source_door_objects().size()==1&&s.story_flag(0x7B2));
    while(e.active)s.tick(idle);
    assert(s.active_map().idx[1].descriptor_index==0&&s.story_flag(0x380)&&!s.story_flag(0x7B0));
}
} // namespace

int main(){
    query_800514fc();
    gate_80051970();
    case3_80050c2c();
    arrival_80048898();
    session_commit();
    event0_player_routines();
    event0_door_object();
    std::puts("source_door_event: PASS");
    return 0;
}
