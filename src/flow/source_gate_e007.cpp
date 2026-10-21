#include "legends3ds/source_gate_e007.h"
#include "legends3ds/source_door_event.h"

namespace l3 {

std::optional<SourceE007Object> source_e007_init_80115a88(const RuntimeMap& map, size_t area,
    std::array<int16_t,3> position, uint8_t parameter, bool flag_607) {
    if (flag_607) return std::nullopt;           // 0x80031A4C: already used
    SourceE007Object o;
    o.position = position;
    if (parameter != 0) { o.state = 5; return o; }
    o.gate = true; o.state = 1; o.anim_20 = 0;
    const auto cell = runtime_map_find_cell(map, area, position[0], position[2]);
    if (!cell || cell->packed_cell <= 0 || size_t(cell->packed_cell) >= map.idx.size()) return std::nullopt;
    const auto closed = source_descriptor_by_word0_8003ac24(map, 0x40500u);
    if (!closed) return std::nullopt;
    o.idx_cell = uint16_t(cell->packed_cell);
    o.open_descriptor = map.idx[o.idx_cell].descriptor_index;
    o.closed_descriptor = *closed;
    return o;
}

SourceE007Step source_e007_step(SourceE007Object& o, bool f607, bool f61f,
                                std::array<int16_t,3> p) noexcept {
    SourceE007Step out;
    if (o.released) return out;
    switch (o.state) {
    case 1:                                                     // 0x80115B64
        if (f607) { o.timer = 8; ++o.state; out.sound = 0x119; out.descriptor_write = o.closed_descriptor; }
        break;
    case 2:                                                     // 0x80115BD8
        if (--o.timer == 0) { ++o.state; out.shake_frames = 0x1E; out.sound = 0x118; }
        o.anim_20 = uint16_t((8u - o.timer) * 0x200u);
        break;
    case 3:                                                     // 0x80115C4C
        if (f61f) { o.timer = 0x40; ++o.state; out.set_60a = true; out.sound = 0x119; }
        break;
    case 4:                                                     // 0x80115CA4
        if (--o.timer == 0) { out.bgm_8001b2f0 = true; o.released = true; }
        else {
            if (o.timer == 0x10) out.descriptor_write = o.open_descriptor;
            o.anim_20 = uint16_t(uint16_t(o.timer) << 6);
        }
        break;
    case 5: {                                                   // 0x80115D38
        const uint32_t dx = uint32_t(int32_t(p[0]) - o.position[0] + 0x3C);
        const uint32_t dz = uint32_t(int32_t(p[2]) - o.position[2] + 0x3C);
        const uint32_t dy = uint32_t(int32_t(p[1]) - o.position[1] + 10);
        if (dx < 0x79u && dz < 0x79u && dy < 0xABu) {
            out.set_607 = true; out.sound = 0x11E; o.timer = 0x10; ++o.state;
        }
        break;
    }
    case 6:                                                     // 0x80115DF4
        if (--o.timer == 0) ++o.state;
        o.anim_20 = uint16_t(uint16_t(o.timer) << 8);
        break;
    case 7: ++o.state; break;                                   // 0x80115E34
    default: break;                                             // 8: 0x80115E48 idle
    }
    return out;
}

} // namespace l3
