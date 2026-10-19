#include "legends3ds/render/terrain_descriptor_patch.h"

namespace l3 {
void TerrainDescriptorPatch::reset() {
    std::vector<Baked>().swap(baked_);
    std::vector<TerrainIdxSpan>().swap(spans_);
    std::vector<uint16_t>().swap(hidden_);
}

void TerrainDescriptorPatch::bake(uint16_t idx_index, uint16_t descriptor_index) {
    for (const auto& b : baked_) if (b.idx_index == idx_index) return;
    baked_.push_back({idx_index, descriptor_index});
}

void TerrainDescriptorPatch::add_span(uint16_t idx_index, uint64_t buffer, uint32_t first, uint32_t count) {
    if (!count) return;
    if (!spans_.empty()) {
        auto& last = spans_.back();
        if (last.idx_index == idx_index && last.buffer == buffer && last.first + last.count == first) {
            last.count += count;
            return;
        }
    }
    TerrainIdxSpan span;
    span.idx_index = idx_index; span.buffer = buffer; span.first = first; span.count = count;
    spans_.push_back(std::move(span));
}

TerrainDescriptorPatchPlan TerrainDescriptorPatch::plan(const RuntimeMap& live) const {
    TerrainDescriptorPatchPlan out;
    for (const auto& b : baked_) {
        const uint16_t now = b.idx_index < live.idx.size() ? live.idx[b.idx_index].descriptor_index : b.descriptor_index;
        const bool swapped = now != b.descriptor_index;
        const bool is_hidden = hidden(b.idx_index);
        if (swapped && !is_hidden) out.hide.push_back(b.idx_index);
        if (!swapped && is_hidden) out.restore.push_back(b.idx_index);
        if (swapped) out.overlay.push_back(b.idx_index);
    }
    return out;
}

bool TerrainDescriptorPatch::hidden(uint16_t idx_index) const { return contains(hidden_, idx_index); }

size_t TerrainDescriptorPatch::saved_bytes() const noexcept {
    size_t n = 0;
    for (const auto& s : spans_) n += s.saved.size();
    return n;
}
} // namespace l3
