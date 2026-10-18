
#ifndef MML_RENDERER_RECOVERY_H
#define MML_RENDERER_RECOVERY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MML_RENDER_0143_FALLBACK=0,
    MML_RENDER_LOGGING_ONLY,
    MML_RENDER_UV_INPUT,
    MML_RENDER_BATCHING_TEXTURES_OFF,
    MML_RENDER_ONE_TERRAIN_TEXTURE,
    MML_RENDER_ALL_TERRAIN_TEXTURES,
    MML_RENDER_PLAYER_TEXTURES,
    MML_RENDER_ALPHA,
    MML_RENDER_ANIMATION
} MMLRendererStage;

typedef enum {
    MML_BOOT_START=0, MML_BOOT_GFX, MML_BOOT_C3D, MML_BOOT_SHADER,
    MML_BOOT_DATA, MML_BOOT_TERRAIN_VBO, MML_BOOT_FIRST_TEXTURE,
    MML_BOOT_PLAYER, MML_BOOT_FIRST_FRAME, MML_BOOT_READY, MML_BOOT_FAILED
} MMLBootStage;

enum {
    MML_RF_UV=1u, MML_RF_BATCHING=2u, MML_RF_TERRAIN=4u,
    MML_RF_ALL_TERRAIN=8u, MML_RF_PLAYER=16u, MML_RF_ALPHA=32u,
    MML_RF_ANIMATION=64u, MML_RF_ALL=127u
};
typedef struct {
    uint32_t feature_mask;
    MMLRendererStage requested;
    MMLRendererStage active;
    bool true_0143_fallback;
    bool textures_enabled;
    bool uv_enabled;
    bool batching_enabled;
    bool alpha_enabled;
    bool animation_enabled;
} MMLRendererGate;

typedef struct {
    MMLBootStage stage;
    uint32_t error_code;
    uint32_t stack_reserved_bytes;
    uint32_t linear_bytes;
    uint32_t vram_bytes;
} MMLBootDiag;

void mml_renderer_gate_init(MMLRendererGate *g);
bool mml_renderer_gate_set(MMLRendererGate *g, MMLRendererStage stage);
/* Returns false without changing the gate when GPU dependencies are missing. */
bool mml_renderer_gate_mask(MMLRendererGate *g, uint32_t mask);
void mml_boot_diag_init(MMLBootDiag *d, uint32_t stack_reserved_bytes);
void mml_boot_diag_advance(MMLBootDiag *d, MMLBootStage stage);
void mml_boot_diag_fail(MMLBootDiag *d, uint32_t code);

#ifdef __cplusplus
}
#endif
#endif
