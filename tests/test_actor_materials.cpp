#include "legends3ds/actor_material.h"
#include "legends3ds/render/ps1_geometry.h"
#include <cassert>
#include <cmath>
#include <stdexcept>

template<class F>void rejects(F&& f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    l3::ActorMeshResource mesh;mesh.model.parts.resize(2);mesh.bindings.resize(2);
    for(size_t i=0;i<2;++i){mesh.bindings[i].material_word=0x3cc00007;
        l3::PbdTri tri;tri.clut=0x3cc0;tri.tpage=0x27;tri.uv={{0,0,255,127,89,128}};
        l3::PbdQuad quad;quad.clut=0x3cc0;quad.tpage=0x27;quad.uv={{1,2,3,4,5,6,7,8}};
        mesh.model.parts[i].triangles.push_back(tri);mesh.model.parts[i].quads.push_back(quad);}
    const auto words=l3::collect_actor_material_words(mesh);assert(words.size()==1&&words[0]==0x3cc00027);
    mesh.model.parts[1].quads[0].clut=0x3cc1;rejects([&]{(void)l3::collect_actor_material_words(mesh);});
    mesh.model.parts[1].quads[0].clut=0x3cc0;mesh.bindings.pop_back();rejects([&]{(void)l3::collect_actor_material_words(mesh);});
    const auto uv=l3::actor_render_uv(255,128);assert(uv[0]==255.5f/256.f&&uv[1]==1.f-128.5f/256.f);
    assert(uv[1]!=l3::actor_render_uv(255,127)[1]); // Do not hide missing row128.
    const auto& q=mesh.model.parts[0].quads[0];
    const std::array<unsigned,6> expected{{0,1,2,1,3,2}};
    assert(l3::kPs1QuadTriangleIndices==expected);
    for(size_t i=0;i<6;++i){const auto corner=l3::kPs1QuadTriangleIndices[i];
        const auto mapped=l3::actor_render_uv(q.uv[2*corner],q.uv[2*corner+1]);
        assert(mapped[0]==(q.uv[2*expected[i]]+.5f)/256.f);}
    // An authored half-page upload fully owns its CLUT, with row128 unknown.
    l3::IndexedResource resource;resource.texture.bpp=4;resource.texture.width_words=64;
    resource.texture.width_pixels=256;resource.texture.height=128;resource.texture.image_x_words=448;
    resource.texture.clut_y=243;resource.texture.palette_entries=16;resource.texture.palette_rows=1;
    resource.texture.packed_indices.assign(64*2*128,0x11);resource.texture.palette_ps1.assign(16,0x1234);
    std::vector<l3::MaterialVramDiagnostics> coverage;
    auto cache=l3::build_material_cache({0x3cc00027},{resource},&coverage);
    assert(cache.entries[0].resolved&&coverage[0].image_words_written==8192&&coverage[0].palette_words_written==16);
    assert(cache.entries[0].indices[127*256]==1&&cache.entries[0].indices[128*256]==0);
    auto bytes=l3::cook_l3material_cache(cache);auto loaded=l3::load_actor_material_cache(bytes);
    assert(loaded.entries[0].indices==cache.entries[0].indices);
    rejects([&]{(void)l3::load_actor_material_cache(bytes,1);});
    bytes.back()^=1;rejects([&]{(void)l3::load_actor_material_cache(bytes);});
    const auto missing=l3::build_material_cache({0x3cc00027},{});
    const auto unresolved=l3::load_actor_material_cache(l3::cook_l3material_cache(missing));
    assert(!unresolved.entries[0].resolved&&unresolved.entries[0].indices.empty());
    std::vector<uint32_t> excess;for(size_t i=0;i<l3::kActorMaterialEntryLimit+1;++i)excess.push_back(i);
    const auto over=l3::build_material_cache(excess,{});
    rejects([&]{(void)l3::load_actor_material_cache(l3::cook_l3material_cache(over));});
}
