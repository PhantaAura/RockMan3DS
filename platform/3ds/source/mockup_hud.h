#pragma once
#include "legends3ds/mockup_ui.h"
#include "mml_runtime_diag_v2.h"

// Call once after the boot console has finished, before drawing gameplay frames.
// Only the bottom framebuffer is reconfigured. Restore consoleInit(GFX_BOTTOM)
// before using an error console after this point.
void mockup_hud_init();
// Invalidates both buffer entries after external framebuffer writes or in-place
// changes to borrowed map cells. New immutable maps are also keyed by identity.
void mockup_hud_invalidate();
// Draws/flushes only a changed bottom framebuffer, with no heap allocation.
// Caller owns presentation: swap GFX_BOTTOM once after this draw, at the chosen
// frame boundary. Never use gfxSwapBuffers here because top belongs to Citro3D.
void mockup_hud_draw(const l3::MockupUiState& state,
                     const l3::BottomScreenModel& status,
                     const l3::MockupMapView& map,
                     const MMLRuntimeDiag* diagnostics = nullptr);
// Raster entry point shared by the native backend and optional host screenshots.
// Buffer is a rotated 240x320 BGR8 framebuffer of at least 320*240*3 bytes.
void mockup_hud_raster(unsigned char* framebuffer,
                       const l3::MockupUiState& state,
                       const l3::BottomScreenModel& status,
                       const l3::MockupMapView& map,
                       const MMLRuntimeDiag* diagnostics = nullptr);
// Same raster behind the native two-buffer cache; true means pixels were written.
// Cell pointers identify immutable borrowed storage; call invalidate before an
// in-place edit. The cache never retains/dereferences source cells after a draw.
bool mockup_hud_raster_cached(unsigned char* framebuffer,
                              const l3::MockupUiState& state,
                              const l3::BottomScreenModel& status,
                              const l3::MockupMapView& map,
                              const MMLRuntimeDiag* diagnostics = nullptr);

// Tutorial message box on the bottom screen. Text is ASCII (decoded glyphs and
// '\n' line breaks); options > 1 shows the YES/NO cursor at choice.
void mockup_hud_set_dialogue(bool active, const char* text, int options, int choice, bool waiting_next);
