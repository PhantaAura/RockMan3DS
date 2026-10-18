
#include "mml_renderer_recovery.h"
#include <string.h>
void mml_renderer_gate_init(MMLRendererGate *g){
    if(!g) return;
    memset(g,0,sizeof(*g));
    g->requested=MML_RENDER_0143_FALLBACK; g->active=MML_RENDER_0143_FALLBACK;
    g->true_0143_fallback=true;
}
bool mml_renderer_gate_set(MMLRendererGate *g,MMLRendererStage s){
    if(!g||s<MML_RENDER_0143_FALLBACK||s>MML_RENDER_ANIMATION)return false;
    g->requested=s; g->active=s;
    g->true_0143_fallback=(s==MML_RENDER_0143_FALLBACK);
    g->uv_enabled=s>=MML_RENDER_UV_INPUT;
    g->batching_enabled=s>=MML_RENDER_BATCHING_TEXTURES_OFF;
    g->textures_enabled=s>=MML_RENDER_ONE_TERRAIN_TEXTURE;
    g->alpha_enabled=s>=MML_RENDER_ALPHA;
    g->animation_enabled=s>=MML_RENDER_ANIMATION;
    g->feature_mask=(g->uv_enabled?MML_RF_UV:0u)|(g->batching_enabled?MML_RF_BATCHING:0u)|
        (g->textures_enabled?MML_RF_TERRAIN:0u)|(s>=MML_RENDER_ALL_TERRAIN_TEXTURES?MML_RF_ALL_TERRAIN:0u)|
        (s>=MML_RENDER_PLAYER_TEXTURES?MML_RF_PLAYER:0u)|(g->alpha_enabled?MML_RF_ALPHA:0u)|
        (g->animation_enabled?MML_RF_ANIMATION:0u);
    return true;
}
bool mml_renderer_gate_mask(MMLRendererGate *g,uint32_t mask){
    if(!g||(mask&~MML_RF_ALL))return false;
    if((mask&(MML_RF_BATCHING|MML_RF_TERRAIN|MML_RF_PLAYER|MML_RF_ALPHA))&&!(mask&MML_RF_UV))return false;
    if((mask&MML_RF_TERRAIN)&&!(mask&MML_RF_BATCHING))return false;
    if((mask&MML_RF_ALL_TERRAIN)&&!(mask&MML_RF_TERRAIN))return false;
    g->feature_mask=mask;g->uv_enabled=(mask&MML_RF_UV)!=0;
    g->true_0143_fallback=!g->uv_enabled;g->batching_enabled=(mask&MML_RF_BATCHING)!=0;
    g->textures_enabled=(mask&MML_RF_TERRAIN)!=0;g->alpha_enabled=(mask&MML_RF_ALPHA)!=0;
    g->animation_enabled=(mask&MML_RF_ANIMATION)!=0;
    return true;
}
void mml_boot_diag_init(MMLBootDiag *d,uint32_t stack){if(d){memset(d,0,sizeof(*d));d->stage=MML_BOOT_START;d->stack_reserved_bytes=stack;}}
void mml_boot_diag_advance(MMLBootDiag *d,MMLBootStage s){if(d&&d->stage!=MML_BOOT_FAILED)d->stage=s;}
void mml_boot_diag_fail(MMLBootDiag *d,uint32_t c){if(d){d->stage=MML_BOOT_FAILED;d->error_code=c;}}
