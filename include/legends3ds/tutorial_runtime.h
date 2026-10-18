#pragma once
#include "legends3ds/st00_flow.h"
#include "legends3ds/st00_spawn.h"
#include "legends3ds/stage_runtime.h"
#include "legends3ds/ebd_directory.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

// A fully pointer-free tutorial-area bootstrap assembled only from cooked runtime files.
// This is the same boundary the 3DS backend consumes: no PS1 pointers and no original
// container/EXE access is required after the computer-side cooker has run.
struct TutorialRuntimeArea {
    size_t area_index = 0;
    St00FlowArea flow;
    EbdDirectory ebd;
    StageRuntimeArea stage;
    std::optional<St00SpawnArea> conditional_spawns;
    // Original area setup+38 -> renderer scratch1F800075. Generic/unvalidated
    // areas leave this unknown; no default mode or room-number substitution.
    std::optional<uint8_t> source_render_scratch_75;
};

TutorialRuntimeArea load_tutorial_runtime_area(const std::vector<uint8_t>& l3flow_bytes,
                                               const std::vector<uint8_t>& l3ebd_bytes,
                                               size_t area_index,
                                               bool verify_crc = true);

} // namespace l3
