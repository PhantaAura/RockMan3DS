
#include "mml_runtime_diag_v2.h"
#include <string.h>
void mml_diag_init(MMLRuntimeDiag*d,uint32_t f,uint32_t s){if(d){memset(d,0,sizeof(*d));d->frame_budget_us=f;d->stack_reserved_bytes=s;}}
void mml_diag_frame(MMLRuntimeDiag*d,uint32_t f,uint32_t a){if(d){d->frame_us=f;d->frame_allocations=a;if(f>d->loading_spike_us)d->loading_spike_us=f;}}
void mml_diag_memory(MMLRuntimeDiag*d,uint32_t h,uint32_t l,uint32_t v){if(!d)return;d->heap_bytes=h;d->linear_bytes=l;d->vram_bytes=v;if(h>d->heap_peak)d->heap_peak=h;if(l>d->linear_peak)d->linear_peak=l;if(v>d->vram_peak)d->vram_peak=v;}
void mml_diag_phase(MMLRuntimeDiag*d,MMLDiagPhase p,uint32_t us){if(!d||(unsigned)p>=MML_PHASE_COUNT)return;d->phase_us[p]=us;if(us>d->phase_peak_us[p])d->phase_peak_us[p]=us;}
bool mml_diag_budget_ok(const MMLRuntimeDiag*d){if(!d)return false;if(d->frame_budget_us&&d->frame_us>d->frame_budget_us)return false;if(d->frame_allocations)return false;if(d->heap_budget&&d->heap_bytes>d->heap_budget)return false;if(d->draw_call_budget&&d->draw_calls>d->draw_call_budget)return false;if(d->triangle_budget&&d->triangles>d->triangle_budget)return false;if(d->linear_budget&&d->linear_bytes>d->linear_budget)return false;if(d->vram_budget&&d->vram_bytes>d->vram_budget)return false;return true;}
