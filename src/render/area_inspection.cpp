#include "legends3ds/render/area_inspection.h"
#include "legends3ds/render/area_placement.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
constexpr unsigned kHeadingCandidates = 64;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int32_t wrap_retail_halfword(int32_t value) {
    const uint16_t bits = static_cast<uint16_t>(value);
    return bits < 0x8000u ? int32_t(bits) : int32_t(bits) - 65536;
}
} // namespace

AreaInspectionGeometry::AreaInspectionGeometry(size_t max_primitives)
    : max_primitives_(max_primitives) {
    require(max_primitives <= std::numeric_limits<size_t>::max() / sizeof(Primitive),
            "inspection geometry budget overflows");
}

void AreaInspectionGeometry::add(const ExpandedRenderPrimitive& primitive) {
    require(primitives_.size() < max_primitives_, "inspection geometry budget exceeded");
    if (primitives_.size() == primitives_.capacity()) {
        // Cap growth explicitly instead of permitting vector's default growth to
        // request more than the caller's admission policy near a non-power-of-two
        // limit. Any allocation occurs while loading/inspecting the area.
        const size_t requested = std::min(max_primitives_,
            std::max<size_t>(64, primitives_.capacity() * 2));
        primitives_.reserve(requested);
    }
    Primitive record;
    for (size_t vertex = 0; vertex < 4; ++vertex) {
        const std::array<int32_t, 3> xyz{{primitive.world_x[vertex],
            primitive.world_y[vertex], primitive.world_z[vertex]}};
        for (size_t axis = 0; axis < 3; ++axis) {
            record.xyz[vertex * 3 + axis] = xyz[axis];
            if (!bounds_.valid) bounds_.min[axis] = bounds_.max[axis] = xyz[axis];
            else {
                bounds_.min[axis] = std::min(bounds_.min[axis], xyz[axis]);
                bounds_.max[axis] = std::max(bounds_.max[axis], xyz[axis]);
            }
        }
        bounds_.valid = true;
    }
    primitives_.push_back(record);
}

size_t AreaInspectionGeometry::primitive_count() const noexcept { return primitives_.size(); }
size_t AreaInspectionGeometry::storage_bytes() const noexcept {
    return primitives_.capacity() * sizeof(Primitive);
}
const AreaInspectionBounds& AreaInspectionGeometry::bounds() const noexcept { return bounds_; }
void AreaInspectionGeometry::release() noexcept {
    std::vector<Primitive>().swap(primitives_);
    bounds_ = {};
}

size_t AreaInspectionGeometry::primitives_with_visible_vertex(St00CellOrigin origin,
    uint16_t yaw, const FrontendViewSettings& settings) const {
    require(valid_frontend_view(settings), "invalid inspection view settings");
    const auto camera=frontend_follow_camera(frontend_world_point(float(origin.x),
        float(origin.y),float(origin.z)),yaw,settings);
    size_t admitted=0;
    for(const auto& primitive:primitives_) {
        for(size_t vertex=0;vertex<4;++vertex) {
            const auto point=frontend_world_point(float(primitive.xyz[vertex*3]),
                float(primitive.xyz[vertex*3+1]),float(primitive.xyz[vertex*3+2]));
            if(project_frontend_point(point,camera,settings).visible){++admitted;break;}
        }
    }
    return admitted;
}

AreaInspectionEntry choose_area_inspection_entry(const RuntimeMap& map, size_t area_index,
                                                const AreaInspectionGeometry& geometry, const FrontendViewSettings& settings) {
    // The shared collector validates dimensions/ranges/IDX identities before any
    // candidate reads. Storage stays bounded by the addressable positive IDX set.
    const auto placement = collect_area_positive_placement(map, area_index);
    AreaInspectionEntry out;
    out.bounds = geometry.bounds();
    out.primitive_count = geometry.primitive_count();
    if (!out.bounds.valid || out.primitive_count == 0) return out;
    const double center_x = (double(out.bounds.min[0]) + out.bounds.max[0]) * 0.5;
    const double center_z = (double(out.bounds.min[2]) + out.bounds.max[2]) * 0.5;
    double best_distance = std::numeric_limits<double>::infinity();
    auto consider = [&](St00CellOrigin origin, AreaInspectionCandidate source) {
        // Authored instances can be referenced by alias cells. A diagnostic entry
        // must still lie inside this area's contact-grid domain.
        if (!runtime_map_find_cell(map, area_index, origin.x, origin.z)) return;
        const double dx = origin.x - center_x, dz = origin.z - center_z;
        const double distance = dx * dx + dz * dz;
        if (distance < best_distance) {
            best_distance = distance;
            out.valid = true;
            out.origin = origin;
            out.candidate_source = source;
        }
    };
    for (const auto& instance : placement.instances) {
        if (instance.descriptor_index == 0xFFFFu) continue;
        consider(instance.origin, AreaInspectionCandidate::PositiveIdx);
    }
    const auto& area = map.areas[area_index];
    for (uint32_t local = 0; local < area.cell_count; ++local) {
        const int16_t packed = map.cells[size_t(area.cell_first) + local];
        if (packed >= 0) continue;
        const int32_t gx = int32_t(area.start_x) + int32_t(local % area.width);
        const int32_t gz = int32_t(area.start_z) + int32_t(local / area.width);
        const uint16_t word = static_cast<uint16_t>(packed);
        // Only the bound default branch and original byte-grid domain are
        // eligible. Match renderer halfword storage; wrapped coordinates that
        // leave the active map domain are rejected by consider().
        if ((word & 0x80u) || gx < 0 || gx > 255 || gz < 0 || gz > 255) continue;
        consider({wrap_retail_halfword(gx * 0x200 - 0x7F00),
                  -int32_t((word & 0x1F00u) >> 1u),
                  wrap_retail_halfword(gz * 0x200 - 0x7F00)},
                 AreaInspectionCandidate::NegativeGrid);
    }
    if (!out.valid) return out;
    for (unsigned heading = 0; heading < kHeadingCandidates; ++heading) {
        const uint16_t yaw = static_cast<uint16_t>(heading * (4096u / kHeadingCandidates));
        const size_t visible = geometry.primitives_with_visible_vertex(out.origin, yaw, settings);
        if (visible > out.primitives_with_visible_vertex) {
            out.primitives_with_visible_vertex = visible;
            out.yaw = yaw;
        }
    }
    return out;
}

const char* area_inspection_candidate_name(AreaInspectionCandidate source) noexcept {
    switch (source) {
        case AreaInspectionCandidate::PositiveIdx: return "POSITIVE_IDX";
        case AreaInspectionCandidate::NegativeGrid: return "NEGATIVE_GRID";
        case AreaInspectionCandidate::None: return "NONE";
    }
    return "NONE";
}
} // namespace l3
