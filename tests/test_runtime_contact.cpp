#include "legends3ds/runtime_session.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>

// The active tick must not allocate merely to gather collision candidates.
// Reserve-free movement/contact/camera with no projectiles or dialogue is the
// measured scope; this does not claim every subsystem is allocation-free.
static size_t allocation_count=0;
void* operator new(std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){++allocation_count;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

namespace {
void put(L3PlayerCompatWork& w,size_t o,int16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(uint16_t(v)>>8);}
int16_t get(const L3PlayerCompatWork& w,size_t o){return int16_t(uint16_t(w.bytes[o])|(uint16_t(w.bytes[o+1])<<8));}
l3::RuntimeMap map_with_floor(uint16_t code=3,int16_t floor=0){
    l3::RuntimeMap m;
    l3::RuntimeMapArea a;a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;m.areas.push_back(a);
    m.cells={1,1,1,1};m.idx.resize(2);m.idx[1].descriptor_index=0;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;m.descriptors.push_back(d);
    m.contact_lists.push_back({0,1});
    m.contact_groups.push_back({-512,512,-512,512,0,1});
    m.contact_primitives.push_back({floor,1024,code,0});return m;
}
l3::ReferenceTables refs(){
    l3::ReferenceTables r;for(auto& t:r.trig)t={0,4096};
    r.trig[0x400]={4096,0};r.trig[0x800]={0,-4096};r.trig[0xC00]={-4096,0};return r;
}
l3::RuntimeSession session(l3::RuntimeMap m){
    l3::TutorialRuntimeArea a;a.area_index=0;L3QolSettings q{};l3_qol_settings_defaults(&q);
    l3::RuntimeSession s(std::move(m),std::move(a),{},q);s.set_reference_tables(refs());s.reset_player({},0,100,100);return s;
}
// 0x8004BDC8 cases 8/9: a grounded walk up a slope primitive follows its
// surface to the top (raw y0) without an Unsupported status.
void slope_climb(uint16_t code,int16_t start_z,int16_t step){
    auto m=map_with_floor(code,-192);m.contact_primitives[0].y1=0;
    int16_t x=0,y=0,z=start_z;uint8_t cs=0;int16_t top=0;
    for(int f=0;f<60;++f){
        L3PlayerCompatWork w{};
        put(w,0x2C,x);put(w,0x2E,y);put(w,0x30,z);
        put(w,0x14,x);put(w,0x16,int16_t(y+8));put(w,0x18,int16_t(z+step));w.bytes[0x74]=cs;
        const auto c=l3::runtime_player_contact_step(m,0,w);
        assert(c.status==l3::RuntimeContactStatus::SourceBackedPartial&&c.unsupported_code==0);
        x=c.scratch.x;y=c.scratch.y;z=c.scratch.z;cs=c.scratch.contact_state;
        assert(y<=0&&y>=-192);
        if(y<top)top=y;
        if((step>0&&z>=400)||(step<0&&z<=-400))break;
    }
    assert(top<=-180&&cs==0);   // reached the raised end, still grounded
}
void contact_response_and_bounds(){
    auto m=map_with_floor();L3PlayerCompatWork w{};
    put(w,0x16,10);put(w,0x2E,-10);w.bytes[0x74]=1;
    const auto hit=l3::runtime_player_contact_step(m,0,w);
    assert(hit.status==l3::RuntimeContactStatus::SourceBackedPartial);
    assert(hit.scratch.y==0&&hit.scratch.contact_state==0&&hit.grounded==true&&hit.landed);
    assert(hit.floor_y==0&&hit.floor_primitive_index==0&&hit.candidate_cells==1);
    assert((hit.result&L3_CONTACT_RESULT_AXIS_Y_MASK)==0x1000);
    assert(hit.iterations==2);
    // Retail strict boundary and halfword contact result remain observable.
    put(w,0x16,0);put(w,0x2E,0);w.bytes[0x74]=0;
    const auto rest=l3::runtime_player_contact_step(m,0,w);
    assert(rest.result==0&&rest.grounded==true);
    auto wall=map_with_floor(1,-1024);
    wall.contact_groups[0]={0,100,-512,512,0,1};
    put(w,0x14,-20);put(w,0x2C,-40);put(w,0x16,0);put(w,0x2E,0);
    const auto wh=l3::runtime_player_contact_step(wall,0,w);
    assert(wh.scratch.x==-32&&wh.wall_contact);
    assert((wh.result&L3_CONTACT_RESULT_AXIS_X_MASK)==0x100);
    auto ceiling=map_with_floor(15,-512);ceiling.contact_primitives[0].y1=-160;
    put(w,0x14,0);put(w,0x2C,0);put(w,0x16,-10);put(w,0x2E,10);w.bytes[0x74]=1;
    const auto ch=l3::runtime_player_contact_step(ceiling,0,w);
    assert(ch.scratch.y==0&&ch.ceiling_contact&&(ch.result&0x2000));
}
void floor_adhesion_precedes_geometry_response(){
    auto map=map_with_floor();
    // A narrow overhead block intersects the head while the feet remain above
    // the floor. Original 0x80048E6C selects its interval at the requested XZ;
    // 0x8003C610 does not snap down, then 0x8004B414 rejects horizontal entry.
    // Querying again only after the wall push used to select the lower floor
    // and publish a Y snap that the original earlier query never requested.
    map.contact_lists[0].group_count=2;
    map.contact_groups.push_back({0,100,-512,512,1,1});
    map.contact_primitives.push_back({-512,-161,1,0});
    auto s=session(std::move(map));s.reset_player({-40,-8,0},0xC00,100,100);
    l3::RuntimeInputFrame forward;forward.held_psx_mask=0x10;s.tick(forward);
    assert(s.player_contact().wall_contact);
    assert(s.player_position().x==-32&&s.player_position().y==-8);
    assert(get(s.compat_player(),0x16)==-8);
    assert(s.player_contact().floor_y==0&&!s.player_contact().grounded.value_or(false));
}
void diagonal_response_does_not_publish_false_adhesion(){
    auto map=map_with_floor();
    map.contact_lists[0].group_count=2;
    map.contact_groups.push_back({-512,512,-512,512,1,1});
    map.contact_primitives.push_back({-8,1024,0x31,0});
    auto s=session(std::move(map));s.reset_player({0,-16,0},0,100,100);s.tick({});
    assert(s.player_contact().status==l3::RuntimeContactStatus::SourceBackedPartial);
    assert(!s.player_contact().grounded.value_or(false));
}
void diagonal_wall_source_vectors(){
    struct Case{uint16_t code;int16_t x,z,dx,dz,out_x,out_z;};
    constexpr Case cases[]={{0x21,-24,-80,16,16,-16,-88},{0x31,-80,16,16,16,-84,12},
                            {0x41,-24,-24,16,16,36,36},{0x51,-80,-24,16,16,-88,-16}};
    for(const auto& c:cases){
        auto map=map_with_floor(c.code,-512);map.contact_primitives[0].y1=512;
        map.contact_groups[0]={-64,64,-64,64,0,1};L3PlayerCompatWork w{};
        put(w,0x14,c.x);put(w,0x16,0);put(w,0x18,c.z);
        put(w,0x2c,int16_t(c.x-c.dx));put(w,0x2e,0);put(w,0x30,int16_t(c.z-c.dz));
        const auto hit=l3::runtime_player_contact_step(map,0,w);
        assert(hit.status==l3::RuntimeContactStatus::SourceBackedPartial&&hit.wall_contact);
        assert(hit.scratch.x==c.out_x&&hit.scratch.z==c.out_z&&hit.unsupported_primitives==0);
    }
}
void iteration_failure_consumes_source_rollback(){
    auto map=map_with_floor();
    // Mutually overlapping authored floor/ceiling volumes exercise the exact
    // bounded solver failure contract. They are intentionally malformed world
    // data, not a proposed retail surface: failed resolution must not admit the
    // pending XZ displacement or leave airborne work/fixed mirrors incoherent.
    map.contact_lists[0].group_count=2;
    map.contact_groups.push_back({-512,512,-512,512,1,1});
    map.contact_primitives.push_back({-512,-144,15,0});
    auto s=session(std::move(map));s.reset_player({0,1,0},0,100,100);
    l3::RuntimeInputFrame forward;forward.held_psx_mask=0x10;s.tick(forward);
    assert(s.player_contact().status==l3::RuntimeContactStatus::IterationLimit);
    assert(s.player_contact().result<0&&s.player_contact().iterations==9);
    assert(s.player_position().x==0&&s.player_position().z==0);
    assert(get(s.compat_player(),0x44)==0&&get(s.compat_player(),0x48)==0);
    assert(get(s.compat_player(),0x14)==get(s.compat_player(),0x2C));
    assert(get(s.compat_player(),0x18)==get(s.compat_player(),0x30));
}
void runtime_jump_is_animation_gated(){
    auto s=session(map_with_floor());s.tick({});assert(s.player_contact().grounded==true);
    l3::RuntimeInputFrame jump;jump.held_psx_mask=0x4000;
    s.tick(jump);assert(s.compat_player().bytes[0x09]==7&&s.compat_player().bytes[0x0A]==0);
    s.tick(jump);assert(s.compat_player().bytes[0xAC]==16&&s.compat_player().bytes[0x0A]==1);
    assert(!l3_pl00_shot_is_enabled(&s.compat_player()));
    s.tick(jump);assert(s.player_position().y==0&&get(s.compat_player(),0x46)==0);
    assert(!s.submit_player_animation_record(0,0,0,0,-1));
    assert(s.submit_player_animation_record(16,0,1,0,-1));
    s.tick(jump);
    assert(get(s.compat_player(),0x46)==-0x1C0&&s.player_position().y==-28);
    assert(s.compat_player().bytes[0xAC]==17&&s.player_contact().grounded==false);
    assert(l3_pl00_shot_is_enabled(&s.compat_player()));
    // Retail gravity acts after position integration, preserving frame order.
    s.tick(jump);assert(s.player_position().y==-56&&get(s.compat_player(),0x46)==-0x190);
    // Released-jump branch first changes substate, then truncates the ascent
    // through the source +0x240/zero branch; no generic spring/gravity tuning.
    s.tick({});assert(s.compat_player().bytes[0x0A]==3);
    s.tick({});assert(get(s.compat_player(),0x46)==0&&s.compat_player().bytes[0x0A]==5);
    bool landed=false;
    for(unsigned i=0;i<32;++i){s.tick({});if(s.player_contact().landed)landed=true;}
    assert(landed&&s.player_position().y==0&&s.player_contact().grounded==true);
    assert(s.compat_player().bytes[0x09]==8&&s.compat_player().bytes[0x0A]==6);
    assert(s.submit_player_animation_record(19,0,1,0,-1));
    s.tick({});assert(s.compat_player().bytes[0xAC]==42&&s.compat_player().bytes[0x0A]==7);
    assert(s.submit_player_animation_record(42,0,1,0,-1));
    s.tick({});assert(s.compat_player().bytes[0x09]==0&&l3_pl00_shot_is_enabled(&s.compat_player()));
}
void ordinary_running_jump_and_land(){
    auto m=map_with_floor();m.areas[0].start_x=56;m.areas[0].start_z=56;
    m.areas[0].width=16;m.areas[0].height=16;m.areas[0].cell_count=256;
    m.cells.assign(256,int16_t(0x8000));auto s=session(std::move(m));s.tick({});
    assert(s.player_contact().grounded==true&&s.player_position().y==-1);
    l3::RuntimeInputFrame forward;forward.held_psx_mask=0x10;
    l3::RuntimeInputFrame jump;jump.held_psx_mask=0x4010;
    s.tick(forward);s.tick(jump);
    assert(s.compat_player().bytes[9]==8&&s.compat_player().bytes[10]==0);
    const auto before=s.player_position();s.tick(jump);
    assert(s.compat_player().bytes[10]==2&&s.compat_player().bytes[0xAC]==20);
    assert(s.player_position().y==before.y-28&&s.player_position().z<before.z);
    assert(get(s.compat_player(),0x44)==552&&get(s.compat_player(),0x46)==-448);
    assert(l3_pl00_shot_is_enabled(&s.compat_player())&&s.player_contact().grounded==false);
    s.tick(jump);assert(get(s.compat_player(),0x46)==-400);
    s.tick(forward);assert(s.compat_player().bytes[10]==3);
    s.tick(forward);assert(s.compat_player().bytes[10]==5&&get(s.compat_player(),0x46)==0);
    bool landed=false;
    for(unsigned i=0;i<80&&s.compat_player().bytes[10]!=6;++i){
        s.tick(forward);landed=landed||s.player_contact().landed;
    }
    assert(landed&&s.player_position().y==-1&&s.player_contact().grounded==true);
    assert(s.compat_player().bytes[9]==8&&s.compat_player().bytes[10]==6&&s.compat_player().bytes[0xAC]==22);
    assert(s.submit_player_animation_record(22,0,1,0,-1));s.tick(forward);
    assert(s.compat_player().bytes[9]==2&&s.compat_player().bytes[10]==2);
    assert(s.compat_player().bytes[0xAC]==23&&get(s.compat_player(),0x46)==0);
    assert(!l3_pl00_shot_is_enabled(&s.compat_player()));
    assert(s.submit_player_animation_record(23,0,1,0,-1));const auto landing_z=s.player_position().z;s.tick(forward);
    assert(s.compat_player().bytes[0xAC]==1&&s.player_position().z==landing_z-32);
    assert(l3_pl00_shot_is_enabled(&s.compat_player()));
    // A platform tap can begin and end between two 30Hz updates. Preserve its
    // source +11E edge while keeping +11C released, then consume it only once.
    auto tapped=session(map_with_floor());tapped.tick({});
    l3::RuntimeInputFrame tap;tap.pressed_psx_mask=0x4000;tapped.tick(tap);
    assert(tapped.compat_player().bytes[9]==7&&get(tapped.compat_player(),0x11C)==0);
    assert(get(tapped.compat_player(),0x11E)==0x4000);
    tapped.tick({});assert(get(tapped.compat_player(),0x11E)==0);
}
void adhesion_lifecycle_and_unknowns(){
    auto s=session(map_with_floor());s.reset_player({0,-16,0},0,100,100);s.tick({});
    assert(s.player_position().y==0&&s.player_contact().grounded==true);
    auto diagonal=session(map_with_floor(0x31));diagonal.reset_player({0,16,0},0,100,100);diagonal.tick({});
    assert(diagonal.player_contact().status==l3::RuntimeContactStatus::SourceBackedPartial);
    assert(!diagonal.player_contact().grounded.value_or(false));
    auto empty=map_with_floor();empty.cells={0,0,0,0};auto missing=session(std::move(empty));
    missing.tick({});assert(missing.player_contact().status==l3::RuntimeContactStatus::NoData);
    // 0.0.15.0: 0x8003C4EC uses the no-floor sentinel 32767, so a player with
    // no floor below selects the fall branch (action7/4, +74=1) instead of
    // standing on air. No floor-at-zero launch is invented either.
    assert(missing.player_contact().grounded==false);
    assert(missing.compat_player().bytes[0x09]==7&&missing.compat_player().bytes[0x0A]==4);
    assert(missing.compat_player().bytes[0x74]==1);
    l3::RuntimeInputFrame jump;jump.held_psx_mask=0x4000;missing.tick(jump);
    assert(missing.compat_player().bytes[0x09]==7&&missing.compat_player().bytes[0x0A]!=1);
    auto malformed=map_with_floor();malformed.contact_groups[0].primitive_first=0xFFFFFFFFu;
    auto rejected=session(std::move(malformed));rejected.tick({});
    assert(rejected.player_contact().status==l3::RuntimeContactStatus::Unsupported);
    assert(!rejected.player_contact().grounded.has_value()&&!rejected.player_contact().floor_y.has_value());
    auto neg=map_with_floor();neg.cells={-32768,-32768,-32768,-32768};auto packed=session(std::move(neg));
    packed.tick({});assert(packed.player_position().y==-1&&packed.player_contact().grounded==true);
    auto saved=s.make_checkpoint(1);s.reset_player({0,-200,0},0,100,100);s.restore_checkpoint(saved);
    assert(!s.player_contact().grounded.has_value());s.tick({});assert(s.player_contact().grounded==true);
    auto ledge_map=map_with_floor();ledge_map.contact_groups[0].z0=-32;ledge_map.contact_groups[0].z1=32;
    auto ledge=session(std::move(ledge_map));ledge.tick({});assert(ledge.player_contact().grounded==true);
    l3::RuntimeInputFrame forward;forward.held_psx_mask=0x0010;
    for(unsigned i=0;i<14;++i)ledge.tick(forward);
    assert(ledge.compat_player().bytes[0x09]==7&&ledge.compat_player().bytes[0x74]==1);
    const auto before=ledge.player_position().y;ledge.tick({});ledge.tick({});
    assert(ledge.player_position().y>before&&!ledge.player_contact().floor_y.has_value());
}
void zero_allocation_contact_ticks(){
    auto s=session(map_with_floor());s.tick({});const size_t before=allocation_count;
    for(unsigned i=0;i<240;++i)s.tick({});
    assert(allocation_count==before);
    assert(s.player_contact().candidate_cells<=4&&s.player_contact().iterations<=9);
}
void ordinary_locomotion_source_and_retail_window(){
    auto m=map_with_floor();m.areas[0].start_z=62;m.areas[0].height=4;m.areas[0].cell_count=8;
    m.cells.assign(8,static_cast<int16_t>(0x8000));
    auto s=session(std::move(m));s.reset_player({0,-1,0},0,100,100);
    l3::RuntimeInputFrame up;up.held_psx_mask=0x10;
    // Retail FLAT_ACCEL's first 25 observations: immediate +44=512 and
    // -32 world units/update; A7 completion changes animation, not speed.
    for(unsigned frame=0;frame<25;++frame){
        if(frame==5)assert(s.submit_player_animation_record(4,4,1,0,-1));
        s.tick(up);
        assert(s.player_position().x==0&&s.player_position().y==-1);
        assert(s.player_position().z==-32*int(frame+1));
        assert(get(s.compat_player(),0x44)==512&&s.compat_player().bytes[0x09]==2);
        assert(s.compat_player().bytes[0x0A]==(frame<5?2:4));
        assert(s.compat_player().bytes[0xAC]==(frame<5?4:1));
    }
    const auto z=s.player_position().z;s.tick({});
    assert(s.player_position().z==z&&get(s.compat_player(),0x44)==0);
    assert(s.compat_player().bytes[0x09]==0&&s.compat_player().bytes[0x0A]==1&&s.compat_player().bytes[0xAC]==0);

    auto reverse=session(map_with_floor());l3::RuntimeInputFrame down;down.held_psx_mask=0x40;
    reverse.tick(down);
    assert(reverse.player_position().z==32&&get(reverse.compat_player(),0x44)==512);
    assert(reverse.compat_player().bytes[0x112]==2&&get(reverse.compat_player(),0x116)==0x800);
    assert(reverse.compat_player().bytes[0x09]==2&&reverse.compat_player().bytes[0x0A]==2);
    assert(reverse.compat_player().bytes[0xAC]==9&&!l3_pl00_shot_is_enabled(&reverse.compat_player()));
    assert(!reverse.submit_player_animation_record(4,0,1,0,-1));
    assert(reverse.submit_player_animation_record(9,0,1,0,-1));reverse.tick(down);
    assert(reverse.player_position().z==64&&reverse.compat_player().bytes[0xAC]==1&&reverse.compat_player().bytes[0x0A]==4);
    reverse.tick({});
    assert(reverse.player_position().z==96&&reverse.compat_player().bytes[0xAC]==10&&reverse.compat_player().bytes[0x0A]==6);
    assert(get(reverse.compat_player(),0x4C)==0x30);
    reverse.tick({});assert(reverse.player_position().z==96);
    assert(reverse.submit_player_animation_record(10,0,1,1,0));reverse.tick({});
    assert(reverse.compat_player().bytes[0x09]==0&&get(reverse.compat_player(),0x44)==0&&get(reverse.compat_player(),0x116)==0);

    auto turn=session(map_with_floor());l3::RuntimeInputFrame right;right.held_psx_mask=0x800;
    turn.tick(right);assert(turn.player_yaw()==4&&turn.compat_player().bytes[0x09]==4&&turn.compat_player().bytes[0xAC]==13);
    turn.tick(right);assert(turn.player_yaw()==12&&get(turn.compat_player(),0xB4)==8);
    l3::RuntimeInputFrame both;both.held_psx_mask=0xC00;turn.tick(both);
    assert(turn.player_yaw()==12&&get(turn.compat_player(),0xB4)==0&&turn.compat_player().bytes[0xAC]==0);
    turn.tick({});assert(turn.compat_player().bytes[0x09]==0);
    auto slow=session(map_with_floor());up.held_psx_mask=0x2010;slow.tick(up);
    assert(slow.player_position().z==-5&&slow.compat_player().bytes[0x09]==1&&slow.compat_player().bytes[0xAC]==12);
}
void bounded_diagnostic_replay_seed(){
    auto s=session(map_with_floor());l3::RuntimePlayerReplaySeed seed;
    seed.position={10,-1,20};seed.fixed_position={{10*65536+123,-65536,20*65536}};
    seed.animation_pose=22;
    assert(s.seed_player_replay_diagnostic(seed));
    assert(s.player_position().x==10&&s.player_position().y==-1&&s.compat_player().bytes[0x0A]==1);
    assert(s.compat_player().bytes[0xA4]==22&&s.frame_counter()==0);
    auto invalid=seed;invalid.fixed_position[0]=11*65536;
    assert(!s.seed_player_replay_diagnostic(invalid)&&s.player_position().x==10);
    invalid=seed;invalid.state[1]=20;
    assert(!s.seed_player_replay_diagnostic(invalid)&&s.compat_player().bytes[0x09]==0);
}
void source_floor_interval_and_unknown_movement(){
    l3::RuntimeMapIdx idx{};
    l3::RuntimeContactGroup group{-256,256,-512,0,0,1};
    l3::RuntimeContactPrimitive primitive{-129,383,8,0};
    // Independently recorded outputs from the original 0x8004A060, with the
    // source classifier's expanded rectangle and original PL00 bounds.
    const std::array<int16_t,4> expected{{256,53,53,256}};
    for(uint16_t code=8;code<=11;++code){
        primitive.code=code;
        const auto interval=l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,{},-128,-384);
        assert(interval && interval->y0==expected[code-8] && interval->y1==543);
    }
    primitive.code=8;
    assert(!l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,{},-288,-384));
    assert(!l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,{},-128,32));
    primitive.code=0x10; // The unimplemented ceiling-slope branch stays unknown.
    assert(!l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,{},-128,-384));
    primitive.code=8;group.z1=group.z0;l3::RetailPlayerBounds zero{0,0,0,0,0,0};
    assert(!l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,zero,0,-512));

    auto map=map_with_floor(8,-129);
    map.contact_groups[0]={-256,256,-512,0,0,1};map.contact_primitives[0].y1=383;
    L3PlayerCompatWork player{};put(player,0x18,-256);put(player,0x30,-256);
    put(player,0x16,155);put(player,0x2E,155);
    const auto contact=l3::runtime_player_contact_step(map,0,player);
    assert(contact.floor_y==155);
    // The 0x8004BDC8 slope cases now answer the movement step too.
    assert(contact.status==l3::RuntimeContactStatus::SourceBackedPartial && contact.unsupported_code==0 &&
           contact.grounded.has_value());
}
void source_floor_fixture(const char* path){
    std::ifstream in(path);assert(in);std::string line;unsigned cases=0;
    while(std::getline(in,line)){
        if(line.empty() || line[0]=='#')continue;
        std::array<int,17> values{};std::istringstream row(line);
        for(auto& value:values)row>>value;assert(row);
        const l3::RuntimeMapIdx idx{};
        const l3::RuntimeContactGroup group{int16_t(values[1]),int16_t(values[2]),int16_t(values[3]),int16_t(values[4]),0,1};
        const l3::RuntimeContactPrimitive primitive{int16_t(values[5]),int16_t(values[6]),uint16_t(values[0]),0};
        const l3::RetailPlayerBounds bounds{int16_t(values[9]),int16_t(values[10]),int16_t(values[11]),int16_t(values[12]),int16_t(values[13]),int16_t(values[14])};
        const auto interval=l3::runtime_contact_floor_interval_8004a060(idx,group,primitive,bounds,int16_t(values[7]),int16_t(values[8]));
        if(!interval || interval->y0!=values[15] || interval->y1!=values[16]){
            std::cerr<<"Original floor interval mismatch in fixture case "<<cases<<'\n';std::abort();
        }
        ++cases;
    }
    assert(cases==240);
    std::cout<<"PASS: 240 original source floor interval scalar cases; slope movement remains UNKNOWN\n";
}
} // namespace
// 0x80050400 vectors, hand-derived from the original R3000 instructions
// (docs/research_01503/DEEP_OVERLAP_80050400.md). Synthetic geometry checks
// the branch tree; it is not a retail movement trace.
L3PlayerCompatWork pose(int16_t x,int16_t y,int16_t z,int16_t bx,int16_t by,int16_t bz,uint8_t state){
    L3PlayerCompatWork w{};put(w,0x14,x);put(w,0x16,y);put(w,0x18,z);
    put(w,0x2C,bx);put(w,0x2E,by);put(w,0x30,bz);w.bytes[0x74]=state;return w;
}
void deep_overlap_recovery_80050400(){
    using l3::RuntimeContactStatus;
    // Deep wall penetration: no correction fits the 4-unit delta. Nearest X
    // face (-44) beats the tied Z faces (544); Y (-1024) is larger.
    auto wall=map_with_floor(1,-1024);
    auto h=l3::runtime_player_contact_step(wall,0,pose(-500,0,0,-504,0,0,0));
    assert(h.status==RuntimeContactStatus::SourceBackedPartial&&h.unsupported_primitives==0);
    assert(h.deep_overlap_recoveries==1&&h.wall_contact&&!h.ceiling_contact);
    assert(h.scratch.x==-544&&h.scratch.y==0&&h.scratch.z==0&&h.iterations==2);
    assert((h.result&L3_CONTACT_RESULT_AXIS_X_MASK)!=0);
    // Repeated resolution is stable: the corrected pose is on the strict face.
    auto again=l3::runtime_player_contact_step(wall,0,pose(-544,0,0,-548,0,0,0));
    assert(again.deep_overlap_recoveries==0&&again.scratch.x==-544&&again.result==0);
    // Floor/wall intersection: shallow Y overlap (-20) beats tied horizontal
    // faces; a nonnegative Y delta publishes the mode-1 grounded state.
    auto block=map_with_floor(1,0);
    h=l3::runtime_player_contact_step(block,0,pose(0,20,0,0,20,0,1));
    assert(h.deep_overlap_recoveries==1&&h.scratch.y==0&&h.scratch.contact_state==0);
    assert(h.grounded==true&&h.landed&&!h.wall_contact);
    // Same recovery while moving up (dy<0): LAB_8004C144 leaves +74 airborne.
    h=l3::runtime_player_contact_step(block,0,pose(0,20,0,0,24,0,1));
    assert(h.deep_overlap_recoveries==1&&h.scratch.y==0&&h.scratch.contact_state==1);
    assert(!h.grounded.value_or(false));
    // Underside overlap pushes down to y1 and reports the ceiling.
    h=l3::runtime_player_contact_step(block,0,pose(0,1180,0,0,1180,0,1));
    assert(h.deep_overlap_recoveries==1&&h.scratch.y==1184&&h.ceiling_contact);
    // Corner / multiple contacts: group 0 pushes X (blocked=2). Group 1 then
    // needs deep recovery with equal X/Z magnitudes (24); a tie normally
    // takes X, but the X block selects Z. The recovered pose leaves both.
    auto corner=map_with_floor(1,-1024);
    corner.contact_lists[0].group_count=2;
    corner.contact_groups[0]={100,200,-512,512,0,1};
    corner.contact_groups.push_back({-100,60,-300,-8,1,1});
    corner.contact_primitives.push_back({-1024,1024,1,0});
    h=l3::runtime_player_contact_step(corner,0,pose(80,0,0,64,0,0,0));
    assert(h.status==RuntimeContactStatus::SourceBackedPartial&&h.deep_overlap_recoveries==1);
    assert(h.scratch.x==68&&h.scratch.z==24&&h.iterations==3);
    // Without the earlier X block the same tie takes X.
    auto single=corner;single.contact_groups[0]={1000,1001,-512,512,0,1};
    h=l3::runtime_player_contact_step(single,0,pose(68,0,0,64,0,0,0));
    assert(h.deep_overlap_recoveries==1&&h.scratch.x==92&&h.scratch.z==0);
    // 0x8004B6AC correction history in a corner. Pass 1: wall A with both
    // horizontal corrections eligible pushes X by -4 and records h.x=+4.
    // Pass 2: wall B pushes Z only (no h, and 0x80050BB4 does not clear u.x).
    // Restart 2 adds u.x, undoing the X push, so the player slides to
    // (100,95). The 0.0.15.2 solver, without the history, stopped at (96,95).
    auto history=map_with_floor(1,-1024);
    history.contact_lists[0].group_count=2;
    history.contact_groups[0]={128,400,127,400,0,1};
    history.contact_groups.push_back({82,118,127,400,1,1});
    history.contact_primitives.push_back({-1024,1024,1,0});
    h=l3::runtime_player_contact_step(history,0,pose(100,0,100,84,0,84,0));
    assert(h.status==RuntimeContactStatus::SourceBackedPartial&&h.deep_overlap_recoveries==0);
    assert(h.scratch.x==100&&h.scratch.z==95&&h.iterations==3&&h.wall_contact);
    // Airborne-upward code 3 (dispatcher mode 3, previously held): the Y
    // target is always the top face, and the grounded state is not published.
    auto oneway=map_with_floor(3,0);
    h=l3::runtime_player_contact_step(oneway,0,pose(0,500,0,0,504,0,1));
    assert(h.status==RuntimeContactStatus::SourceBackedPartial&&h.unsupported_primitives==0);
    assert(h.deep_overlap_recoveries==1&&h.scratch.y==0&&h.scratch.contact_state==1);
    // Runtime: a player embedded in a wall block is recovered out of it
    // instead of the 0.0.15.2 pre-move hold.
    auto map=map_with_floor();
    map.contact_lists[0].group_count=2;
    map.contact_groups.push_back({0,400,-512,512,1,1});
    map.contact_primitives.push_back({-512,-1,1,0});
    auto s=session(std::move(map));s.reset_player({20,0,0},0,100,100);s.tick({});
    assert(s.player_contact().status==RuntimeContactStatus::SourceBackedPartial);
    assert(s.player_contact().deep_overlap_recoveries==1&&s.player_position().x==-32);
}
int main(int argc,char** argv){deep_overlap_recovery_80050400();slope_climb(8,-400,24);slope_climb(9,400,-24);contact_response_and_bounds();floor_adhesion_precedes_geometry_response();diagonal_response_does_not_publish_false_adhesion();diagonal_wall_source_vectors();iteration_failure_consumes_source_rollback();runtime_jump_is_animation_gated();ordinary_running_jump_and_land();adhesion_lifecycle_and_unknowns();zero_allocation_contact_ticks();ordinary_locomotion_source_and_retail_window();bounded_diagnostic_replay_seed();source_floor_interval_and_unknown_movement();
    assert(argc==1 || argc==2);if(argc==2)source_floor_fixture(argv[1]);
    std::cout<<"PASS: source-backed contact subset, ordinary action2 forward/reverse/release/turn, bounded retail scalar window, diagnostic seed, animation-gated jump and zero-allocation ticks; PHYSICS_1TO1=false\n";
}
