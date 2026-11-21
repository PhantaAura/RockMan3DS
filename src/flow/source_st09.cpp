#include "legends3ds/source_st09.h"

namespace l3 {

bool bind_st09_profile(const std::vector<StageDataBlock>& blocks, St03Event8Profile& out) {
    out = {};
    for (const auto& d : blocks) {
        if (d.token != kSt09Set0Record || d.bytes.size() < 20) continue;
        const auto& r = d.bytes;
        const auto s16 = [&](size_t o) { return int16_t(uint16_t(r[o] | (r[o + 1] << 8))); };
        St03ObjectRecord g;
        g.slot = r[0]; g.object_class = r[1]; g.subtype = r[2]; g.family = r[3]; g.byte_04 = r[4];
        g.word_08 = {r[8], r[9], r[10], r[11]};
        g.position = {s16(12), s16(14), s16(16)};
        g.heading = uint16_t(r[18] | (r[19] << 8));
        out.object_sets[0] = {g};
        out.generic_classes = St03GenericClasses{};   // no generic NPCs in ST09's set 0
        return true;
    }
    return false;
}

} // namespace l3
