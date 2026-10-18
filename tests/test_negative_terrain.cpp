#include "legends3ds/render/negative_terrain.h"
#include "legends3ds/render/positive_terrain.h"
#include "legends3ds/tile_transform.h"
#include <cassert>
#include <functional>
#include <stdexcept>

namespace {
void rejects(const std::function<void()>& fn) {
    bool failed = false;
    try { fn(); } catch (const std::runtime_error&) { failed = true; }
    assert(failed);
}
void exercise_area_selection() {
    l3::RuntimeMap map;
    map.areas={{63,63,1,1,0,1,0,0}, {63,63,3,1,1,3,0,0},
               {63,63,1,1,4,1,0,0}, {63,63,1,1,5,1,0,0}};
    map.cells={0,1,1,int16_t(0x8000),int16_t(0x8080),0};
    map.descriptors.resize(1); map.idx.resize(2);
    map.idx[1].descriptor_index=0; map.idx[1].raw6=1;
    l3::St00RenderSource render;
    render.descriptors.resize(1); render.descriptors[0].record_indices[1]=0;
    render.default_negative_descriptor=0; render.special_negative_descriptors[0]=0;
    render.records.resize(1); render.records[0].materials={0};
    render.records[0].stream_indices={0}; render.streams.resize(1);
    render.streams[0].primitives.resize(1);
    assert(l3::count_area_combined_mid_primitives(map,render,0,0)==0);
    // Positive aliases draw once; the default cell contributes independently.
    assert(l3::count_area_combined_mid_primitives(map,render,1,2)==2);
    assert(l3::count_area_combined_mid_primitives(map,render,2,1)==1);
    assert(l3::count_area_combined_mid_primitives(map,render,3,0)==0);
    assert(l3::next_area_with_mid_terrain(map,render,0,2)==1); // Empty first.
    assert(l3::next_area_with_mid_terrain(map,render,1,2)==1); // Inclusive.
    assert(l3::next_area_with_mid_terrain(map,render,2,2)==2); // Special geometry.
    assert(l3::next_area_with_mid_terrain(map,render,3,2)==1); // Empty last, wrap.
    rejects([&]{(void)l3::count_area_combined_mid_primitives(map,render,1,1);});
    rejects([&]{(void)l3::next_area_with_mid_terrain(map,render,0,1);});
    rejects([&]{(void)l3::next_area_with_mid_terrain(map,render,0,0);});
    rejects([&]{(void)l3::next_area_with_mid_terrain(map,render,4,2);});
    rejects([&]{(void)l3::count_area_combined_mid_primitives(map,render,4,2);});
    // Unknown identities cannot manufacture eligibility or texture bindings.
    map.idx[1].descriptor_index=0xFFFFu;
    render.default_negative_descriptor=0xFFFFu;
    render.special_negative_descriptors[0]=0xFFFFu;
    assert(!l3::next_area_with_mid_terrain(map,render,0,0));
    assert(!l3::next_area_with_mid_terrain(map,render,3,0));
    render.special_negative_descriptors[0]=0;
    assert(l3::next_area_with_mid_terrain(map,render,3,1)==2);
    render.descriptors[0].record_indices[1]=0xFFFFu;
    assert(!l3::next_area_with_mid_terrain(map,render,0,0)); // Identity, absent Mid.
    render.descriptors[0].record_indices[1]=1;
    rejects([&]{(void)l3::next_area_with_mid_terrain(map,render,0,UINT64_MAX);});
    l3::RuntimeMap empty;
    assert(!l3::next_area_with_mid_terrain(empty,{},0,0));
}
}

int main() {
    exercise_area_selection();
    l3::RuntimeMap map;
    map.areas.push_back({63,63,3,2,0,6,0,0});
    map.cells = {int16_t(0x8000),int16_t(0x8101),0,int16_t(0x8282),1,int16_t(0x8303)};
    l3::St00RenderSource render;
    render.descriptors.resize(1);
    auto unknown = l3::collect_area_negative_placement(map,render,0);
    assert(unknown.instances.empty() && unknown.default_cells_unbound==3 && unknown.special_cells_unbound==1);
    render.default_negative_descriptor=0;
    render.descriptors[0].record_indices[1]=0;
    render.records.resize(1);
    render.records[0].materials={0};
    render.records[0].stream_indices={0};
    render.streams.resize(1);
    render.streams[0].base_shift=2;
    l3::SourceRenderPrimitive p;
    // Retained legacy storage of consecutive XYZ triplets. Y bytes are ignored
    // by the negative flat path, even when they would alter a positive record.
    p.local_x={{-64,64,23,29}};
    p.local_z={{17,-64,64,64}};
    p.ignored_raw={{-64,19,31,64}};
    render.streams[0].primitives={p};
    const auto placement=l3::collect_area_negative_placement(map,render,0);
    assert(placement.instances.size()==3 && placement.special_cells_unbound==1);
    assert(placement.instances[0].origin.x==-256 && placement.instances[0].origin.z==-256);
    assert(placement.instances[1].origin.y==-128 && placement.instances[2].origin.y==-384);
    assert(l3::count_area_negative_mid_primitives(render,placement,3)==3);
    rejects([&]{(void)l3::count_area_negative_mid_primitives(render,placement,2);});
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0,2);});
    l3::NegativeMaterialBinding binding{0x10001d,12,true};
    for (unsigned rotation=0;rotation<4;++rotation) {
        auto instance=placement.instances[0];instance.packed_cell=uint16_t(0x8000u|rotation);
        const auto expanded=l3::expand_negative_terrain_mid(render,instance,&binding);
        assert(expanded.size()==1 && expanded[0].texture_binding_known);
        assert(expanded[0].material_word==binding.material_word);
        assert(expanded[0].uv==l3::negative_material_quad_uv(binding));
        // The proven tile transform rotates (x,z) as (z,-x) for turn1.
        const int32_t expected_x[4]={-512,-512,0,0};
        const int32_t expected_z[4]={-512,0,0,-512};
        assert(expanded[0].world_x[0]==expected_x[rotation]);
        assert(expanded[0].world_z[0]==expected_z[rotation]);
        const auto local=l3::positive_terrain_consecutive_xyz(p);
        for (size_t v=0;v<4;++v) {
            const auto expected=legends_tile_transform_xz(-256,-256,local.x[v],local.z[v],2,
                                                         instance.packed_cell);
            assert(expanded[0].world_x[v]==expected.x && expanded[0].world_z[v]==expected.z);
        }
        for (int32_t y:expanded[0].world_y) assert(y==instance.origin.y);
    }
    auto no_palette=l3::expand_negative_terrain_mid(render,placement.instances[0],nullptr);
    assert(!no_palette[0].texture_binding_known && no_palette[0].material_word==0);
    // Each source selector is independent of height and the low rotation bits;
    // repeated references remain separate cell instances.
    render.descriptors.resize(3);
    render.descriptors[1].record_indices[1]=0;
    render.descriptors[2].record_indices[1]=0;
    render.special_negative_descriptors[0]=1;
    render.special_negative_descriptors[31]=2;
    map.areas[0]={63,63,3,2,0,6,0,0};
    map.cells={int16_t(0x8080),int16_t(0x83ff),int16_t(0x8082),int16_t(0x8084),
               int16_t(0x8500),0};
    const auto mixed=l3::collect_area_negative_placement(map,render,0);
    assert(mixed.instances.size()==4 && mixed.special_cells_unbound==1 && mixed.default_cells_unbound==0);
    assert(mixed.instances[0].descriptor_index==1 && mixed.instances[1].descriptor_index==2);
    assert(mixed.instances[2].descriptor_index==1 && mixed.instances[3].descriptor_index==0);
    assert(mixed.instances[1].origin.y==-384 && mixed.instances[3].origin.y==-640);
    assert(l3::count_area_negative_mid_primitives(render,mixed,4)==4);
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0,3);});
    rejects([&]{(void)l3::count_area_negative_mid_primitives(render,mixed,3);});
    for (size_t i=0;i<3;++i) {
        const auto special=l3::expand_negative_terrain_mid(render,mixed.instances[i],&binding);
        assert(special.size()==1 && !special[0].texture_binding_known && special[0].material_word==0);
        assert(special[0].uv==p.uv);
        for (int32_t y:special[0].world_y) assert(y==mixed.instances[i].origin.y);
    }
    render.default_negative_descriptor=0xFFFFu;
    const auto unknown_default=l3::collect_area_negative_placement(map,render,0);
    assert(unknown_default.instances.size()==3 && unknown_default.default_cells_unbound==1 &&
           unknown_default.special_cells_unbound==1);
    render.default_negative_descriptor=0;
    auto invalid_instance=mixed.instances[0];invalid_instance.packed_cell=0;
    rejects([&]{(void)l3::expand_negative_terrain_mid(render,invalid_instance,&binding);});
    render.special_negative_descriptors[1]=3;
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0);});
    render.special_negative_descriptors[1]=0xFFFFu;
    render.records[0].stream_indices.push_back(0);
    assert(l3::count_area_negative_mid_primitives(render,mixed,8)==8);
    rejects([&]{(void)l3::count_area_negative_mid_primitives(render,mixed,7);});
    render.records[0].stream_indices.pop_back();
    render.descriptors[1].record_indices[1]=0xFFFFu;
    assert(l3::expand_negative_terrain_mid(render,mixed.instances[0],nullptr).empty());
    assert(l3::count_area_negative_mid_primitives(render,mixed,2)==2);
    render.descriptors[1].record_indices[1]=0;
    render.records[0].stream_indices[0]=1;
    rejects([&]{(void)l3::count_area_negative_mid_primitives(render,mixed,UINT64_MAX);});
    rejects([&]{(void)l3::expand_negative_terrain_mid(render,mixed.instances[0],nullptr);});
    render.records[0].stream_indices[0]=0;
    render.streams[0].base_shift=16;
    rejects([&]{(void)l3::expand_negative_terrain_mid(render,placement.instances[0],&binding);});
    rejects([&]{(void)l3::count_area_negative_mid_primitives(render,mixed,UINT64_MAX);});
    render.streams[0].base_shift=2;
    map.areas[0].start_x=256;
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0);});
    map.areas[0].start_x=63;
    map.areas[0].width=4;
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0);});
    map.areas[0].width=3;map.areas[0].cell_first=UINT32_MAX;
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,0);});
    rejects([&]{(void)l3::collect_area_negative_placement(map,render,1);});
}
