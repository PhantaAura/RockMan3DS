#include "legends3ds/st00_source.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/visual_bridge.h"
#include <array>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 10) {
        std::cerr << "usage: l3visual_probe MDT IDX STG HED BODY.PBD HEAD.PBD RIGHT.PBD LEFT.PBD FEET.PBD\n";
        return 2;
    }
    try {
        auto mdt=l3::read_file(argv[1]), idx=l3::read_file(argv[2]), stg=l3::read_file(argv[3]), hed=l3::read_file(argv[4]);
        const auto stage=l3::parse_st00(mdt,idx,stg,hed); const auto render=l3::parse_st00_render(stage,stg);
        std::array<l3::PbdModel,l3::kPlayerComponentSlots> components;
        for(size_t i=0;i<components.size();++i) components[i]=l3::parse_player_pbd_mesh(l3::read_file(argv[5+i]));
        const auto composition=l3::build_baseline_player_composition(components);
        l3::PlayerVisualStats ps; const auto player=l3::build_player_visuals(components,composition,&ps);
        std::cout<<"player_parts="<<ps.parts<<" player_primitives="<<ps.primitives<<" triangles="<<ps.triangles
                 <<" quads="<<ps.quads<<" vertex_refs="<<ps.vertex_references<<"\n";
        uint32_t total_cells=0,total_prims=0;
        for(size_t ai=0;ai<stage.areas.size();++ai){
            // A constant mid-range depth is a structural bridge probe, not a claim about an actual camera frame.
            std::vector<uint32_t> depths(stage.areas[ai].cells.size(),0x1000u);
            l3::St00AreaVisualStats st; const auto v=l3::build_st00_area_primary_visuals(stage,render,ai,depths,0,&st);
            std::cout<<"area="<<ai<<" positive_cells="<<st.positive_cells<<" mid_cells="<<st.mid_cells<<" primitives="<<v.size()<<"\n";
            total_cells+=st.positive_cells; total_prims+=st.primitives;
        }
        std::cout<<"all_areas_positive_cells="<<total_cells<<" all_areas_mid_lod_primitives="<<total_prims<<"\n";
        return player.empty()?1:0;
    } catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}
}
