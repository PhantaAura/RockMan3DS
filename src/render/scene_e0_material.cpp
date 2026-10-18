#include "legends3ds/scene_e0_material.h"
#include "legends3ds/area_package.h"
#include "legends3ds/l3map.h"
#include "legends3ds/render/source_depth_cue.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace l3 {
SceneE0MaterialPlan plan_scene_e0_material(uint32_t modulation,uint32_t material,
    bool alpha_gate,bool retained_stp,bool nearest) noexcept {
    SceneE0MaterialPlan out;
    out.triangle_rgb_code=source_flat_textured_color_02ffffff(modulation,false);
    out.quad_rgb_code=source_flat_textured_color_02ffffff(modulation,true);
    out.semi_transparent=(out.triangle_rgb_code&0x02000000u)!=0;
    out.abr=uint8_t((material>>5u)&3u);
    if(!out.semi_transparent){out.pass_count=1;return out;}
    if(!alpha_gate){out.status=SceneE0MaterialPlanStatus::AlphaGateDisabled;return out;}
    if(!retained_stp){out.status=SceneE0MaterialPlanStatus::NeedsRetainedStpMask;return out;}
    if(!nearest){out.status=SceneE0MaterialPlanStatus::NeedsNearestSampling;return out;}
    // The opaque STP0 pixels must not inherit the semitransparent primitive's
    // blend. Keep mask alpha unchanged in both passes (never double128 to255).
    out.pass_count=2;out.passes[0].texels=SceneE0TexelPass::OpaqueOnly;
    out.passes[0].alpha_tev_scale=1;
    auto& semi=out.passes[1];semi.texels=SceneE0TexelPass::StpOnly;semi.alpha_tev_scale=1;
    semi.destination=SceneE0BlendFactor::One;
    switch(out.abr){
    case 0:semi.source=SceneE0BlendFactor::Half;semi.destination=SceneE0BlendFactor::Half;break;
    case 1:semi.source=SceneE0BlendFactor::One;break;
    case 2:semi.source=SceneE0BlendFactor::One;semi.equation=SceneE0BlendEquation::DestinationMinusSource;break;
    case 3:semi.source=SceneE0BlendFactor::Quarter;break;
    }
    return out;
}
bool scene_e0_texel_in_pass(SceneE0TexelPass pass,uint8_t alpha) noexcept {
    // Only the three retained source classes are admitted. Values from linear
    // filtering/another alpha codec do not become synthesized STP evidence.
    if(alpha!=0&&alpha!=128&&alpha!=255)return false;
    switch(pass){
    case SceneE0TexelPass::Nonzero:return alpha!=0;
    case SceneE0TexelPass::OpaqueOnly:return alpha==255;
    case SceneE0TexelPass::StpOnly:return alpha==128;
    }
    return false;
}
namespace {
constexpr size_t header_bytes=64;
uint32_t word(const uint8_t* p) noexcept {
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
void put(std::vector<uint8_t>& bytes,size_t at,uint32_t value) {
    for(size_t i=0;i<4;++i)bytes[at+i]=uint8_t(value>>(i*8));
}
void require(bool condition,const char* reason) {if(!condition)throw std::runtime_error(reason);}
std::vector<uint32_t> canonical(std::vector<uint32_t> words) {
    std::sort(words.begin(),words.end());words.erase(std::unique(words.begin(),words.end()),words.end());
    require(!words.empty()&&words.size()<=kSceneE0MaterialCountLimit,"scene material count rejected");
    return words;
}
void digest_known(const std::array<uint8_t,32>& digest) {
    require(std::any_of(digest.begin(),digest.end(),[](uint8_t v){return v!=0;}),"scene material EBD identity missing");
}
}
std::vector<uint32_t> collect_scene_e0_material_words(const SourceEventPhase5Profile& profile,
    const SceneE0Resources& resources) {
    require(profile.initialized,"scene material profile missing");
    std::vector<uint32_t> words;words.reserve(kSceneE0MaterialCountLimit);
    for(size_t i=1;i<profile.records.size();++i)for(size_t tier=0;tier<3;++tier) {
        const auto& record=profile.records[i];
        const auto* mesh=resources.find(record.resource_token,record.raw[4],static_cast<SceneE0MeshTier>(tier));
        require(mesh!=nullptr,"scene material resource binding missing");
        words.push_back(mesh->material_word);
    }
    return canonical(std::move(words));
}
std::vector<uint8_t> cook_scene_e0_material_snapshot(const std::array<uint8_t,32>& digest,
    const MaterialCache& cache) {
    digest_known(digest);
    require(!cache.entries.empty()&&cache.entries.size()<=kSceneE0MaterialCountLimit,"scene material count rejected");
    std::vector<uint32_t> words;for(const auto& entry:cache.entries)words.push_back(entry.material_word);
    require(canonical(words).size()==words.size(),"duplicate scene material rejected");
    const auto payload=cook_l3material_cache(cache);
    require(payload.size()<=std::numeric_limits<uint32_t>::max()-header_bytes,"scene material byte budget rejected");
    (void)preflight_area_material_cache(payload,kSceneE0MaterialDecodedLimit);
    std::vector<uint8_t> out(header_bytes+payload.size(),0);
    out[0]='L';out[1]='3';out[2]='S';out[3]='M';put(out,4,0x00400001u);
    put(out,8,1); // INITIAL_UPLOAD_BATCH only; no live palette-state claim.
    put(out,12,uint32_t(out.size()));put(out,16,uint32_t(header_bytes));
    put(out,20,crc32(payload.data(),payload.size()));
    std::copy(digest.begin(),digest.end(),out.begin()+24);
    std::copy(payload.begin(),payload.end(),out.begin()+header_bytes);
    return out;
}
MaterialCache load_scene_e0_material_snapshot(const std::vector<uint8_t>& bytes,
    const std::array<uint8_t,32>& digest,const std::vector<uint32_t>& expected,uint64_t limit) {
    digest_known(digest);const auto words=canonical(expected);
    require(bytes.size()>=header_bytes&&bytes.size()<=kSceneE0MaterialDecodedLimit+4096,"scene material byte budget rejected");
    const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='S'&&p[3]=='M'&&word(p+4)==0x00400001u,"scene material format rejected");
    require(word(p+8)==1&&word(p+12)==bytes.size()&&word(p+16)==header_bytes&&word(p+56)==0&&word(p+60)==0,"scene material header rejected");
    require(std::equal(digest.begin(),digest.end(),p+24),"scene material EBD identity rejected");
    require(word(p+20)==crc32(p+header_bytes,bytes.size()-header_bytes),"scene material CRC rejected");
    const std::vector<uint8_t> payload(bytes.begin()+header_bytes,bytes.end());
    const auto header=inspect_l3material_cache(payload,true);
    require(header.material_count==words.size(),"scene material set count rejected");
    (void)preflight_area_material_cache(payload,std::min(limit,kSceneE0MaterialDecodedLimit));
    auto cache=load_l3material_cache(payload,false);std::vector<uint32_t> actual;
    for(const auto& entry:cache.entries)actual.push_back(entry.material_word);
    require(canonical(actual)==words&&actual.size()==words.size(),"scene material resource set rejected");
    return cache;
}
} // namespace l3
