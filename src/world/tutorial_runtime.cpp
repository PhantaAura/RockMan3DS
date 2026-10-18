#include "legends3ds/tutorial_runtime.h"
#include <stdexcept>

namespace l3 {
TutorialRuntimeArea load_tutorial_runtime_area(const std::vector<uint8_t>& l3flow_bytes,
                                               const std::vector<uint8_t>& l3ebd_bytes,
                                               size_t area_index,
                                               bool verify_crc) {
    if (area_index >= kSt00TutorialAreaCount) throw std::runtime_error("tutorial area index out of range");
    const auto flow = load_l3st00_flow(l3flow_bytes, verify_crc);
    TutorialRuntimeArea out;
    out.area_index = area_index;
    out.flow = flow.areas[area_index];
    out.source_render_scratch_75 = out.flow.setup_blob[38];
    out.ebd = load_l3ebd_directory(l3ebd_bytes, verify_crc);
    out.stage = build_fresh_stage_runtime_area(out.flow.objects, out.ebd);
    if (out.stage.instances.size() != out.flow.objects.records.size()) {
        throw std::runtime_error("tutorial runtime instance count mismatch");
    }
    return out;
}
} // namespace l3
