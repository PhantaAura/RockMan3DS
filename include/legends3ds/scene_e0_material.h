#pragma once
#include "legends3ds/material_cache.h"
#include "legends3ds/scene_e0.h"
#include <array>

namespace l3 {
// The original E0 packet consumer retains modulationRGB24/ABE25 and installs
// flat24/2C. ABE is not a visibility flag. These two passes classify retained
// RGBA alpha0/128/255, independently of PS1 depth/order/raster equivalence.
enum class SceneE0TexelPass : uint8_t { Nonzero,OpaqueOnly,StpOnly };
enum class SceneE0BlendEquation : uint8_t { Add,DestinationMinusSource };
enum class SceneE0BlendFactor : uint8_t { Zero,One,Half,Quarter };
struct SceneE0MaterialPass {
    SceneE0TexelPass texels=SceneE0TexelPass::Nonzero;
    SceneE0BlendEquation equation=SceneE0BlendEquation::Add;
    SceneE0BlendFactor source=SceneE0BlendFactor::One,destination=SceneE0BlendFactor::Zero;
    uint8_t alpha_tev_scale=2;
};
enum class SceneE0MaterialPlanStatus : uint8_t {
    Ready,AlphaGateDisabled,NeedsRetainedStpMask,NeedsNearestSampling
};
struct SceneE0MaterialPlan {
    SceneE0MaterialPlanStatus status=SceneE0MaterialPlanStatus::Ready;
    bool semi_transparent=false;
    uint8_t abr=0,pass_count=0;
    uint32_t triangle_rgb_code=0,quad_rgb_code=0;
    std::array<SceneE0MaterialPass,2> passes{};
};
// STP availability means the current admitted owner texture was uploaded from
// material_page_rgba8 without changing its0/128/255 alpha encoding. NEAREST is
// mandatory for the exact mask partition; interpolation must not be mistaken
// for new source STP values. Both are explicit backend receipts, not defaults.
SceneE0MaterialPlan plan_scene_e0_material(uint32_t modulation_word,uint32_t material_word,
    bool alpha_gate,bool retained_stp_mask,bool nearest_sampling) noexcept;
bool scene_e0_texel_in_pass(SceneE0TexelPass,uint8_t retained_alpha) noexcept;

constexpr size_t kSceneE0MaterialCountLimit=45;
constexpr uint64_t kSceneE0MaterialDecodedLimit=2u*1024u*1024u;
// Load-time collection only. Keep the exact E0 material, including ABR; the
// unrelated class20 TPAGE OR20 rule must never be applied to this consumer.
std::vector<uint32_t> collect_scene_e0_material_words(const SourceEventPhase5Profile& profile,
    const SceneE0Resources& resources);
// L3SM1 binds an initial-upload-batch snapshot to the retained EBD identity.
// It does not claim live palette chronology. Resolved entries require their
// own complete CLUT; unresolved entries remain empty, never substituted.
std::vector<uint8_t> cook_scene_e0_material_snapshot(const std::array<uint8_t,32>& ebd_digest,
    const MaterialCache& cache);
MaterialCache load_scene_e0_material_snapshot(const std::vector<uint8_t>& bytes,
    const std::array<uint8_t,32>& expected_ebd_digest,const std::vector<uint32_t>& expected_words,
    uint64_t decoded_limit=kSceneE0MaterialDecodedLimit);
} // namespace l3
