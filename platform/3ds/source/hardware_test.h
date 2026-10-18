#pragma once
#include <3ds.h>
#include "mml_renderer_recovery.h"
#include "mml_runtime_diag_v2.h"
#include "legends3ds/runtime_session.h"

// Diagnostic-only UI and telemetry. No game rules or asset decoding live here.
bool hardware_test_launch(MMLRendererGate& gate, bool& xyz_candidate);
void hardware_test_begin(bool xyz_candidate);
void hardware_test_line(const char* format, ...);
// Full diagnostic records go to the existing buffered owner-local log.
void hardware_test_trace(const char* format, ...);
void hardware_test_geometry_event(bool xyz_candidate,size_t area,size_t primitives,int32_t min_y,int32_t max_y);
void hardware_test_visibility_event(size_t area, size_t negative_bound, size_t default_unbound,
    size_t special_unbound, size_t material_unknown, size_t visible_primitives,
    size_t inspection_storage, const char* candidate_source);
// Live allocator/free-space samples are separate from parser admission estimates.
void hardware_test_area_event(const char* phase, size_t area, u64 elapsed_us,
                             size_t package_bytes, size_t decoded_estimate, size_t peak_estimate);
void hardware_test_sample(const l3::RuntimeSession& session, const MMLRuntimeDiag& diag,
                          u64 frame_start, u32 held, u32 pressed, size_t area);
void hardware_test_hud(const MMLRuntimeDiag& diag, const l3::RuntimeSession& session);
void hardware_test_end();

#include "legends3ds/actor_runtime.h"
#include "legends3ds/gameplay_clock.h"
void hardware_test_actor_event(size_t area,const l3::ActorAreaRuntime& actors);
void hardware_test_clock_event(const l3::GameplayClock& clock);
void hardware_test_actor_target_event(const l3::RuntimeSession& session,
    const l3::ActorAreaRuntime& actors, const l3::SourceAngleSteps* steps);

void hardware_test_actor_state_event(const l3::RuntimeSession& session,
    const l3::ActorAreaRuntime& actors);

void hardware_test_actor_action2_probe(const l3::ActorAreaRuntime& actors,
    const l3::RuntimeMap& map,size_t area,const l3::ReferenceTables& refs);
void hardware_test_actor_action4_probe(const l3::ActorAreaRuntime& actors,
    const l3::RuntimeMap& map,size_t area,const l3::ReferenceTables& refs);
