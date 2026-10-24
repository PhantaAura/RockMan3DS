#include "legends3ds/actor_material.h"
#include "legends3ds/area_package.h"
#include "legends3ds/scene_e0.h"
#include <set>
#include <stdexcept>

namespace l3 {
uint32_t actor_material_word(const PbdTri& face) {
    return (uint32_t(face.clut)<<16)|face.tpage;
}
uint32_t actor_material_word(const PbdQuad& face) {
    return (uint32_t(face.clut)<<16)|face.tpage;
}
std::vector<uint32_t> collect_actor_material_words(const ActorMeshResource& resource) {
    if(resource.bindings.size()!=resource.model.parts.size())
        throw std::runtime_error("actor material mesh/binding count mismatch");
    std::set<uint32_t> words;
    for(size_t i=0;i<resource.bindings.size();++i){
        const auto raw=resource.bindings[i].material_word;
        const uint32_t expected=(raw&0xffff0000u)|uint16_t(raw|0x20u);
        const auto& p=resource.model.parts[i];
        const auto collect=[&](const auto& faces){for(const auto& f:faces){
            const auto word=actor_material_word(f);
            if(word!=expected)throw std::runtime_error("actor material face/binding mismatch");
            words.insert(word);
            if(words.size()>kActorMaterialEntryLimit)
                throw std::runtime_error("actor material count policy exceeded");
        }};
        collect(p.triangles);collect(p.quads);
    }
    return {words.begin(),words.end()};
}
std::vector<uint32_t> collect_admitted_actor_material_words(const EbdDirectory& ebd) {
    std::set<uint32_t> words;
    for(size_t i=0;i<ebd.entries.size();++i){
        const auto& row=ebd.entries[i];
        if(row.object_class!=0x20||(row.subtype!=3&&row.subtype!=0x35))continue;
        const auto mesh=decode_ebd_actor_mesh(ebd,i);
        const auto row_words=collect_actor_material_words(mesh);
        words.insert(row_words.begin(),row_words.end());
        if(words.size()>kActorMaterialEntryLimit)
            throw std::runtime_error("actor material area count policy exceeded");
    }
    return {words.begin(),words.end()};
}
std::vector<uint32_t> collect_admitted_e0_object_material_words(const EbdDirectory& ebd) {
    std::set<uint32_t> words;
    for(const uint8_t subtype:{uint8_t(5),uint8_t(8)})
        for(const auto row:find_ebd_entries(ebd,0xE0,subtype))
            for(const auto tier:{SceneE0MeshTier::Near,SceneE0MeshTier::Middle,SceneE0MeshTier::Far})
                words.insert(decode_scene_e0_mesh(ebd,row,tier).material_word);
    if(words.size()>kActorMaterialEntryLimit)
        throw std::runtime_error("E0 object material count policy exceeded");
    return {words.begin(),words.end()};
}
MaterialCache load_actor_material_cache(const std::vector<uint8_t>& bytes,uint64_t limit) {
    const auto header=inspect_l3material_cache(bytes,true);
    if(header.material_count>kActorMaterialEntryLimit)
        throw std::runtime_error("actor material cache count policy exceeded");
    (void)preflight_area_material_cache(bytes,limit);
    return load_l3material_cache(bytes,false);
}
std::array<float,2> actor_render_uv(uint8_t u,uint8_t v) {
    return {{(float(u)+0.5f)/256.f,1.f-(float(v)+0.5f)/256.f}};
}
} // namespace l3
