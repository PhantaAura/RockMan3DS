// Player door event: 0x800514FC query, 0x80051970 gate, 0x80050C2C case 3
// matching/request lanes, event-0 arrival 0x80048898 and the session commit.
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
    RuntimeSession s(door_map({},0x301,0x8000,2),a0,{},qol);s.set_reference_tables(refs());
    s.reset_player({0,-1,-100},0x800,100,100);
    RuntimeInputFrame idle{};s.tick(idle);
    assert(s.source_door_result().status==SourceDoorStatus::GateClosed&&!s.source_door_transition());
    // Story flag 0x200 suppresses the call entirely.
    s.set_story_flag(0x200,true);
    RuntimeInputFrame press{};press.held_psx_mask=0x2000;press.pressed_psx_mask=0x2000;
    s.tick(press);assert(!s.source_door_transition());
    s.set_story_flag(0x200,false);
    s.tick(idle);s.tick(press);
    assert(s.source_door_result().status==SourceDoorStatus::TransitionRequested);
    assert(s.story_flag(0x7B0)&&s.source_door_transition()&&s.source_door_transition()->destination_area==1);
    // The destination must be registered before anything changes.
    assert(!s.can_commit_source_door_transition()&&!s.commit_source_door_transition()&&s.map_area_index()==0);
    TutorialRuntimeArea a1;a1.area_index=1;s.register_area(1,a1);
    // A pending request is not re-evaluated.
    s.tick(press);assert(s.source_door_transition()&&s.map_area_index()==0);
    assert(s.can_commit_source_door_transition()&&s.commit_source_door_transition());
    const auto p=s.player_position();
    assert(s.map_area_index()==1&&p.x==0&&p.y==-2&&p.z==36&&s.player_yaw()==2048);
    assert(!s.story_flag(0x7B0)&&s.story_flag(0x380)&&!s.source_door_transition());
    const auto& w=s.compat_player();
    assert(w.bytes[8]==1&&w.bytes[9]==0&&w.bytes[10]==0&&w.bytes[11]==0);
    assert(get(w,0x2C)==0&&get(w,0x2E)==-2&&get(w,0x30)==36);
    // The synthetic floor stands at y=0: one idle update lands the arrival.
    s.tick(idle);
    assert(s.player_position().y==0&&s.player_contact().grounded.value_or(false));
}
} // namespace

int main(){
    query_800514fc();
    gate_80051970();
    case3_80050c2c();
    arrival_80048898();
    session_commit();
    std::puts("source_door_event: PASS");
    return 0;
}
