#include "legends3ds/runtime_session.h"
#include "legends3ds/actor_runtime.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
static std::vector<uint8_t> read(const std::filesystem::path& p){
    std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("missing cooked input");
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("usage: l3runtime_spawn_probe existing-cooked-tutorial-directory");
    const std::filesystem::path root=argv[1];
    auto map=l3::load_l3map(read(root/"st00.l3map"));
    auto area=l3::load_tutorial_runtime_area(read(root/"st00.l3flow"),read(root/"ebd/area_01.l3ebd"),1);
    area.conditional_spawns=l3::load_l3st00_spawn(read(root/"st00.l3spawn")).areas[1];
    L3QolSettings q{};l3_qol_settings_defaults(&q);l3::RuntimeSession s(std::move(map),std::move(area),{},q);
    // Deliberate owner-local test premise: zero story flags and a raw position
    // inside the first proved source gate. This is not an original game save.
    s.reset_player({352,-1,-8128},0,100,100);
    if(!s.conditional_spawn_controller_active())throw std::runtime_error("source controller not bound");
    s.tick({});if(!s.conditional_spawn_objects().records.empty())throw std::runtime_error("spawn happened before source state2");
    s.tick({});auto records=s.active_area().flow.objects;
    const auto& spawned=s.conditional_spawn_objects().records;
    records.records.insert(records.records.end(),spawned.begin(),spawned.end());
    auto stage=l3::build_fresh_stage_runtime_area(records,s.active_area().ebd);
    auto actors=l3::spawn_imported_actors(stage,s.active_area().ebd);
    size_t enemies=0,parts=0,triangles=0,quads=0;
    for(const auto& a:actors.actors)if(a.subtype==3){
        if(!a.pose||!a.pose_selected_by_retail_state)throw std::runtime_error("initializer pose unbound");
        ++enemies;parts+=a.resource->model.parts.size();
        for(const auto& part:a.resource->model.parts){triangles+=part.triangles.size();quads+=part.quads.size();}
    }
    if(spawned.size()!=3||enemies!=3)throw std::runtime_error("first source gate did not instantiate three real actors");
    const auto contact=s.player_contact();
    std::cout<<"conditional_records="<<spawned.size()<<" class20_03_meshes="<<enemies
      <<" parts="<<parts<<" triangles="<<triangles<<" quads="<<quads
      <<" decoded_bytes="<<actors.decoded_resource_bytes<<" shared_resources="<<actors.unique_resource_count
      <<" player_mapped="<<s.probe_player_map().in_area<<" contact_status="<<int(contact.status)
      <<" qualifier="<<int(s.conditional_spawn_qualifier())<<"\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
