#include "legends3ds/runtime_session.h"
#include <cassert>
#include <iostream>
using namespace l3;
static StageObjectRecord object(uint8_t cls,uint8_t sub,uint8_t id){
    StageObjectRecord r;r.raw[0]=id;r.raw[1]=cls;r.raw[2]=sub;
    r.id=id;r.object_class=cls;r.words_02_to_0a[0]=sub;return r;
}
int main(){
    RuntimeMap map;RuntimeMapArea ma;ma.start_x=63;ma.start_z=63;ma.width=2;ma.height=2;ma.cell_count=4;
    map.areas.push_back(ma);map.cells.assign(4,int16_t(0x8000));
    TutorialRuntimeArea area;area.flow.objects.records.push_back(object(0xC0,0x21,1));
    area.stage=build_fresh_stage_runtime_area(area.flow.objects,area.ebd);
    St00SpawnRegion region;region.id=7;region.qualifier=1;region.min_x=-1;region.min_z=-1;region.width=2;region.depth=2;
    region.records={{object(0x20,3,0),1599},{object(0x20,3,1),1600},{object(0x20,3,2),1601}};
    area.conditional_spawns=St00SpawnArea{{region}};
    L3QolSettings q{};l3_qol_settings_defaults(&q);RuntimeSession s(map,area,{},q);
    assert(s.conditional_spawn_controller_active());s.reset_player({0,-1,0},0,100,100);
    s.set_story_flag(1600,true);auto revision=s.conditional_spawn_revision();
    s.tick({});assert(s.conditional_spawn_objects().records.empty());assert(s.conditional_spawn_revision()==revision+1);
    assert(s.conditional_spawn_qualifier()==-1);
    s.tick({});assert(s.conditional_spawn_objects().records.size()==2);assert(s.conditional_spawn_qualifier()==1);
    assert(s.conditional_spawn_objects().records[0].id==0&&s.conditional_spawn_objects().records[1].id==2);
    revision=s.conditional_spawn_revision();s.tick({});assert(s.conditional_spawn_revision()==revision);
    s.set_story_flag(1600,false);s.tick({});assert(s.conditional_spawn_objects().records.size()==2); // same qualifier never respawns.
    s.register_area(0,area);assert(s.transition_to_area(0,{0,-1,0},0));
    assert(s.conditional_spawn_objects().records.empty()&&s.conditional_spawn_qualifier()==-1);
    s.tick({});s.tick({});assert(s.conditional_spawn_objects().records.size()==3);
    auto save=s.make_save_state(0);s.apply_save_state(save);assert(s.conditional_spawn_objects().records.empty());
    TutorialRuntimeArea unbound;unbound.conditional_spawns=area.conditional_spawns;
    s.set_area(0,unbound);assert(!s.conditional_spawn_controller_active());
    s.tick({});s.tick({});assert(s.conditional_spawn_objects().records.empty());
    std::cout<<"runtime conditional spawn: two-tick gate, predicates, qualifier, cleanup, controller binding PASS\n";
}
