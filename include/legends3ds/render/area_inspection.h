#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/render/frontend_view.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// Diagnostic admission/framing only. Neither the selected entry nor the
// frontend's existing fixed follow-camera offsets are a recovered retail spawn
// or camera rule. Retail entry and floor/contact behavior remain UNKNOWN.
enum class AreaInspectionCandidate : uint8_t { None, PositiveIdx, NegativeGrid };

struct AreaInspectionBounds {
    bool valid = false;
    std::array<int32_t, 3> min{};
    std::array<int32_t, 3> max{};
};

class AreaInspectionGeometry {
public:
    // Caller policy, not a measured hardware limit. Storage is at most
    // max_primitives * 48 bytes, apart from vector bookkeeping/allocator overhead.
    explicit AreaInspectionGeometry(size_t max_primitives);
    void add(const ExpandedRenderPrimitive& primitive);
    size_t primitive_count() const noexcept;
    size_t storage_bytes() const noexcept;
    const AreaInspectionBounds& bounds() const noexcept;
    // Tests existing native camera/frustum against original admitted vertices.
    // A nonzero count proves a vertex is admitted, not successful GPU rasterization.
    size_t primitives_with_visible_vertex(St00CellOrigin origin, uint16_t yaw,
        const FrontendViewSettings& settings = {}) const;
    void release() noexcept;
private:
    struct Primitive { std::array<int32_t, 12> xyz{}; };
    size_t max_primitives_ = 0;
    AreaInspectionBounds bounds_{};
    std::vector<Primitive> primitives_;
};

struct AreaInspectionEntry {
    bool valid = false;
    St00CellOrigin origin{};
    uint16_t yaw = 0;
    AreaInspectionCandidate candidate_source = AreaInspectionCandidate::None;
    AreaInspectionBounds bounds{};
    size_t primitive_count = 0;
    size_t primitives_with_visible_vertex = 0;
};

// Selects an in-area source-backed coordinate candidate nearest admitted
// geometry's XZ bounds center, then an inspection heading from 64 directions.
// Positive coordinates come from IDX XYZ; negative coordinates/Y use the proved
// packed-cell renderer convention. This does not perform a floor test or snap,
// and it does not assign object/transition records retail spawn semantics.
AreaInspectionEntry choose_area_inspection_entry(const RuntimeMap& map,
                                                size_t area_index,
                                                const AreaInspectionGeometry& geometry,
                                                const FrontendViewSettings& settings = {});
const char* area_inspection_candidate_name(AreaInspectionCandidate source) noexcept;
} // namespace l3
