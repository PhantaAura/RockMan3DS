#pragma once
#include "legends3ds/l3map.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace l3 {
// Native renderer bookkeeping for IDX descriptor swaps (the ST00 door object
// writes word0 & ~0x1000 through 0x8003AC24 and restores it in state 5). The
// 3DS terrain is baked once per area; instead of rebuilding the area, the
// vertices each IDX identity contributed are recorded, the baked ones of a
// swapped identity are collapsed (saved for the restore), and the live
// descriptor is drawn from a small overlay. The original redraws from the
// live IDX each frame; this is a native cache, not a retail mechanism.
struct TerrainIdxSpan {
    uint16_t idx_index = 0;
    uint64_t buffer = 0;   // renderer-owned buffer key (material batch key)
    uint32_t first = 0;    // first vertex in that buffer
    uint32_t count = 0;    // vertices
    std::vector<uint8_t> saved; // baked vertices while collapsed
};

struct TerrainDescriptorPatchPlan {
    std::vector<uint16_t> hide;    // swapped since the bake and still drawn
    std::vector<uint16_t> restore; // collapsed, descriptor back to the baked one
    std::vector<uint16_t> overlay; // every swapped identity: draw its live descriptor
    bool empty() const noexcept { return hide.empty() && restore.empty(); }
};

class TerrainDescriptorPatch {
public:
    void reset();
    // Records the descriptor an identity was baked with. Repeated calls for an
    // aliased identity keep the first value.
    void bake(uint16_t idx_index, uint16_t descriptor_index);
    // Records vertices appended for an identity; contiguous appends to the same
    // buffer extend the previous span.
    void add_span(uint16_t idx_index, uint64_t buffer, uint32_t first, uint32_t count);
    TerrainDescriptorPatchPlan plan(const RuntimeMap& live) const;
    bool hidden(uint16_t idx_index) const;
    size_t baked_count() const noexcept { return baked_.size(); }
    size_t span_count() const noexcept { return spans_.size(); }
    size_t saved_bytes() const noexcept;

    // Applies hide/restore to the baked buffers. `resolve(buffer)` returns the
    // buffer's first byte (or nullptr when it is gone); `written(ptr, bytes)`
    // is called for each modified range (cache flush). Collapsing sets every
    // vertex of the span to its first vertex, so the triangles have no area.
    template <typename Resolve, typename Written>
    size_t apply(const TerrainDescriptorPatchPlan& plan, size_t stride, Resolve&& resolve, Written&& written) {
        size_t touched = 0;
        for (auto& s : spans_) {
            const bool hide = contains(plan.hide, s.idx_index);
            const bool restore = contains(plan.restore, s.idx_index);
            if ((!hide && !restore) || !s.count) continue;
            uint8_t* base = resolve(s.buffer);
            if (!base) continue;
            uint8_t* p = base + size_t(s.first) * stride;
            const size_t bytes = size_t(s.count) * stride;
            if (hide && s.saved.empty()) {
                s.saved.assign(p, p + bytes);
                for (uint32_t v = 1; v < s.count; ++v) std::memcpy(p + size_t(v) * stride, p, stride);
            } else if (restore && s.saved.size() == bytes) {
                std::memcpy(p, s.saved.data(), bytes);
                std::vector<uint8_t>().swap(s.saved);
            } else {
                continue;
            }
            written(p, bytes);
            ++touched;
        }
        for (const auto idx : plan.hide) if (!hidden(idx)) hidden_.push_back(idx);
        for (const auto idx : plan.restore)
            for (size_t i = 0; i < hidden_.size(); ++i)
                if (hidden_[i] == idx) { hidden_[i] = hidden_.back(); hidden_.pop_back(); break; }
        return touched;
    }

private:
    static bool contains(const std::vector<uint16_t>& v, uint16_t idx) {
        for (const auto x : v) if (x == idx) return true;
        return false;
    }
    struct Baked { uint16_t idx_index; uint16_t descriptor_index; };
    std::vector<Baked> baked_;
    std::vector<TerrainIdxSpan> spans_;
    std::vector<uint16_t> hidden_;
};
} // namespace l3
