#include "legends3ds/runtime_session.h"
#include "legends3ds/st00_spawn.h"
#include <cassert>
#include <iostream>
using namespace l3;
static StageObjectRecord object(uint8_t cls,uint8_t sub,uint8_t id){
    StageObjectRecord r;r.raw[0]=id;r.raw[1]=cls;r.raw[2]=sub;
    r.id=id;r.object_class=cls;r.words_02_to_0a[0]=sub;return r;
}
static void put32(std::vector<uint8_t>& b,uint32_t a,uint32_t v){for(int i=0;i<4;++i)b[a-0x80100000u+i]=uint8_t(v>>(8*i));}
// Stage-generic E0/00 table: slots[stage] -> per-area descriptor lists.
static void stage_spawn_catalog(){
    std::vector<uint8_t> ov(0x400,0);
    const uint32_t slots=0x80100000u,table=0x80100100u,desc=0x80100140u,recs=0x80100200u;
    put32(ov,slots+4*9,table);
    put32(ov,table+0,desc);put32(ov,table+4,0);put32(ov,table+8,desc+12*2);
    // area 0: one region (id 0, qualifier 0, x 32..35, z 7..9) with two records; area 2 reuses it.
    ov[desc-0x80100000u+4]=32;ov[desc-0x80100000u+5]=7;ov[desc-0x80100000u+6]=4;ov[desc-0x80100000u+7]=3;
    put32(ov,desc+8,recs);ov[desc-0x80100000u+12]=0xFF;ov[desc-0x80100000u+13]=0xFF;
    ov[desc+24-0x80100000u+4]=1;ov[desc+24-0x80100000u+6]=1;ov[desc+24-0x80100000u+7]=1;put32(ov,desc+24+8,recs+24);
    ov[desc+36-0x80100000u]=0xFF;ov[desc+36-0x80100000u+1]=0xFF;
    for(int i=0;i<2;++i){auto o=recs-0x80100000u+24*i;ov[o]=1;ov[o+1]=0x20;ov[o+2]=0x19;put32(ov,recs+24*i+20,0x619u+i);}
    ov[recs-0x80100000u+48]=0xFF;
    const auto catalog=parse_stage_spawn_us(ov,slots,9,3);
    assert(catalog.areas.size()==3&&catalog.areas[0].regions.size()==1&&catalog.areas[1].regions.empty());
    assert(catalog.areas[0].regions[0].records.size()==2&&catalog.areas[0].regions[0].records[1].predicate==0x61A);
    assert(catalog.areas[2].regions[0].records.size()==1&&catalog.areas[0].regions[0].min_x==32);
    const auto loaded=load_l3st00_spawn(cook_l3st00_spawn(catalog),true);
    assert(loaded.areas.size()==3&&loaded.areas[0].regions[0].records[0].object.raw[2]==0x19);
    assert(st00_spawn_region_contains(loaded.areas[0].regions[0],int16_t(17085),int16_t(4342)));
}
int main(){
    stage_spawn_catalog();
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
