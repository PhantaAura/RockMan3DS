#include "legends3ds/source_stage_load.h"

namespace l3 {

SourceStageRequest source_stage_request_from_lanes(const std::array<uint8_t,18>& raw,
                                                   uint32_t source_function, uint64_t generation) noexcept {
    const auto s16 = [&](size_t at) { return int16_t(uint16_t(raw[at] | (uint16_t(raw[at + 1]) << 8))); };
    SourceStageRequest r;
    r.stage = raw[14]; r.area = raw[15];
    r.spawn = {s16(0), s16(2), s16(4)};
    r.heading = uint16_t(s16(10));
    r.source_function = source_function; r.generation = generation;
    return r;
}

SourceStageArrival source_stage_arrival(const SourceStageRequest& request) noexcept {
    SourceStageArrival a;
    a.stage = request.stage; a.area = request.area;
    a.position = request.spawn;
    a.heading = request.heading;
    return a;
}

std::string stage_asset_path(uint8_t stage, const std::string& rel) {
    if (stage == 0) return rel;
    static constexpr const char* hex = "0123456789abcdef";
    std::string dir = "stages/st";
    dir += hex[stage >> 4]; dir += hex[stage & 15]; dir += '/';
    static const std::pair<const char*, const char*> renamed[] = {
        {"st00.l3map", "map.l3map"}, {"st00.l3render", "render.l3render"},
        {"st00.l3init", "init.l3init"}, {"st00.l3flow", "flow.l3flow"},
    };
    for (const auto& [from, to] : renamed) if (rel == from) return dir + to;
    return dir + rel;
}

} // namespace l3
