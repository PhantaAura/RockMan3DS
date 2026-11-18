#include "legends3ds/source_door_locks.h"

namespace l3 {

std::optional<StageDoorLocks> load_stage_door_locks(const std::vector<uint8_t>& b) noexcept {
    if (b.size() < 16 || b[0] != 'L' || b[1] != '3' || b[2] != 'D' || b[3] != 'L') return std::nullopt;
    const unsigned version = unsigned(b[4] | (b[5] << 8));
    if (version != 1 && version != 2) return std::nullopt;
    const size_t n = b[6], m = version == 2 ? b[7] : 0;
    if (n == 0 || b.size() != 16 + n * 8 + m * 4) return std::nullopt;
    StageDoorLocks out;
    for (size_t i = 0; i < 8; ++i) out.doors[i] = b[8 + i];
    out.rows.resize(n);
    for (size_t r = 0; r < n; ++r)
        for (size_t i = 0; i < 8; ++i) out.rows[r][i] = b[16 + r * 8 + i];
    for (size_t k = 0; k < m; ++k) {
        const size_t o = 16 + n * 8 + k * 4;
        out.overrides.emplace_back(uint16_t(b[o] | (b[o + 1] << 8)), b[o + 2]);
    }
    return out;
}

} // namespace l3
