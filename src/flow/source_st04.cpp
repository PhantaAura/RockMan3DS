#include "legends3ds/source_st04.h"

namespace l3 {
namespace {
const uint8_t* bytes_in(const std::vector<StageDataBlock>& blocks, uint32_t addr, size_t n) {
    for (const auto& d : blocks)
        if (addr >= d.token && addr - d.token + n <= d.bytes.size()) return d.bytes.data() + (addr - d.token);
    return nullptr;
}
uint32_t u32(const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
} // namespace

bool bind_field_profile(const std::vector<StageDataBlock>& blocks, const FieldProfileSpec& spec, St03Event8Profile& out) {
    out = {};
    const auto* ws = bytes_in(blocks, spec.walk_speeds, 112);
    const auto* it = bytes_in(blocks, spec.idle_timers, 64);
    const auto* wt = bytes_in(blocks, spec.walk_tables, 112);
    const auto* cm = bytes_in(blocks, spec.class_mask, 96);
    if (!ws || !it || !wt || !cm) return false;
    for (size_t i = 0; i < 64; ++i) out.idle_timers[i] = it[i];
    out.idle_timers_loaded = true;
    for (size_t i = 0; i < 112; ++i) { out.walk_speeds[i] = ws[i]; out.walk_tables[i] = wt[i]; }
    out.walk_loaded = true;
    St03GenericClasses mask{};
    for (size_t c = 0; c < 96; ++c) mask[c] = cm[c] != 0;
    out.generic_classes = mask;
    for (uint8_t n = 0; n < spec.set_count && n < out.object_sets.size(); ++n) {
        const auto* t = bytes_in(blocks, spec.set_table + 8u * n, 8);
        if (!t) continue;
        const uint32_t count = u32(t), at = u32(t + 4);
        if (count > kSt03ObjectSlots) continue;
        const auto* r = bytes_in(blocks, at, size_t(count) * 20u);
        if (!r) continue;
        for (uint32_t i = 0; i < count; ++i, r += 20) {
            St03ObjectRecord o;
            o.slot = r[0]; o.object_class = r[1]; o.subtype = r[2]; o.family = r[3]; o.byte_04 = r[4];
            o.word_08 = {r[8], r[9], r[10], r[11]};
            o.position = {int16_t(r[12] | (r[13] << 8)), int16_t(r[14] | (r[15] << 8)), int16_t(r[16] | (r[17] << 8))};
            o.heading = uint16_t(r[18] | (r[19] << 8));
            out.object_sets[n].push_back(o);
        }
    }
    return true;
}

} // namespace l3
