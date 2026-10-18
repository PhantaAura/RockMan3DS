
#ifndef MML_RUNTIME_DIAG_V2_H
#define MML_RUNTIME_DIAG_V2_H
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum{MML_PHASE_UPDATE=0,MML_PHASE_COLLISION,MML_PHASE_EVENT,MML_PHASE_RENDER,MML_PHASE_LOADING,MML_PHASE_BOTTOM_UI,MML_PHASE_COUNT}MMLDiagPhase;
typedef struct{uint32_t phase_us[MML_PHASE_COUNT],phase_peak_us[MML_PHASE_COUNT],heap_budget,draw_call_budget,triangle_budget;uint32_t frame_us,frame_budget_us,input_to_update_us,loading_spike_us,frame_allocations,heap_bytes,heap_peak,linear_bytes,linear_peak,linear_budget,vram_bytes,vram_peak,vram_budget,texture_count,texture_bytes,draw_calls,triangles,stack_reserved_bytes,boot_stage,error_code;}MMLRuntimeDiag;
void mml_diag_phase(MMLRuntimeDiag*d,MMLDiagPhase phase,uint32_t microseconds);
void mml_diag_init(MMLRuntimeDiag*d,uint32_t frame_budget,uint32_t stack_reserved);void mml_diag_frame(MMLRuntimeDiag*d,uint32_t frame_us,uint32_t allocs);void mml_diag_memory(MMLRuntimeDiag*d,uint32_t heap,uint32_t linear,uint32_t vram);bool mml_diag_budget_ok(const MMLRuntimeDiag*d);

#ifdef __cplusplus
}
#endif
#endif
