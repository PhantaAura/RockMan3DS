#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* kPack = "/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/full-build/tutorial/legends3ds/tutorial/";
struct Point { double x, z; };
using Quad = std::array<Point, 4>;
using Triangle = std::array<Point, 3>;
using Order = std::array<unsigned, 6>;
constexpr Order kStrip{{0, 1, 2, 1, 3, 2}};
constexpr Order kFan{{0, 1, 2, 0, 2, 3}};

std::vector<uint8_t> read_authorized(const char* name) {
    std::ifstream input(std::string(kPack) + name, std::ios::binary);
    if (!input) throw std::runtime_error("authorized cooked input unavailable");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

double cross(Point a, Point b, Point c) {
    return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
}
double area(const std::vector<Point>& polygon) {
    double twice_area = 0;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const auto a = polygon[i];
        const auto b = polygon[(i + 1) % polygon.size()];
        twice_area += a.x * b.z - a.z * b.x;
    }
    return std::abs(twice_area) * 0.5;
}
std::vector<Point> unique_points(const Quad& quad) {
    std::vector<Point> points(quad.begin(), quad.end());
    std::sort(points.begin(), points.end(), [](Point a, Point b) {
        return a.x < b.x || (a.x == b.x && a.z < b.z);
    });
    points.erase(std::unique(points.begin(), points.end(), [](Point a, Point b) {
        return a.x == b.x && a.z == b.z;
    }), points.end());
    return points;
}
std::vector<Point> convex_hull(const std::vector<Point>& points) {
    if (points.size() < 3) return points;
    std::vector<Point> hull;
    for (Point p : points) {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.push_back(p);
    }
    const size_t lower = hull.size();
    for (size_t i = points.size() - 1; i-- > 0;) {
        const Point p = points[i];
        while (hull.size() > lower && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.push_back(p);
    }
    hull.pop_back();
    return hull;
}
Triangle triangle(const Quad& quad, const Order& indices, size_t first) {
    return {{quad[indices[first]], quad[indices[first + 1]], quad[indices[first + 2]]}};
}
double intersection_area(const Triangle& subject, const Triangle& clip) {
    if (cross(subject[0], subject[1], subject[2]) == 0 ||
        cross(clip[0], clip[1], clip[2]) == 0) return 0;
    std::vector<Point> polygon(subject.begin(), subject.end());
    const double winding = cross(clip[0], clip[1], clip[2]) > 0 ? 1 : -1;
    for (size_t edge = 0; edge < clip.size() && !polygon.empty(); ++edge) {
        const Point a = clip[edge], b = clip[(edge + 1) % clip.size()];
        std::vector<Point> next;
        Point previous = polygon.back();
        double previous_distance = winding * cross(a, b, previous);
        for (Point current : polygon) {
            const double distance = winding * cross(a, b, current);
            if ((previous_distance >= 0) != (distance >= 0)) {
                const double t = previous_distance / (previous_distance - distance);
                next.push_back({previous.x + t * (current.x - previous.x),
                                previous.z + t * (current.z - previous.z)});
            }
            if (distance >= 0) next.push_back(current);
            previous = current;
            previous_distance = distance;
        }
        polygon = std::move(next);
    }
    return area(polygon);
}
struct Coverage { double overlap, missing; bool valid; };
Coverage coverage(const Quad& quad, const Order& order, double hull_area) {
    const auto a = triangle(quad, order, 0), b = triangle(quad, order, 3);
    const double overlap = intersection_area(a, b);
    const double sum = (std::abs(cross(a[0], a[1], a[2])) +
                        std::abs(cross(b[0], b[1], b[2]))) * 0.5;
    const double missing = hull_area - (sum - overlap);
    const double epsilon = std::max(1.0, hull_area) * 1e-8;
    return {overlap, missing, std::abs(overlap) <= epsilon && std::abs(missing) <= epsilon};
}

struct Stats {
    size_t total = 0, degenerate = 0, rectangles = 0;
    std::array<size_t, 5> unique_counts{}, hull_counts{};
    std::array<std::array<size_t, 4>, 5> coverage_by_hull{};
    size_t strip_only = 0, fan_only = 0, both = 0, neither = 0;
    size_t strip_overlap = 0, fan_overlap = 0, strip_missing = 0, fan_missing = 0;
    double total_area = 0, strip_missing_area = 0, fan_missing_area = 0;
    void add(const Quad& quad) {
        ++total;
        const auto unique = unique_points(quad), hull = convex_hull(unique);
        ++unique_counts[unique.size()];
        ++hull_counts[hull.size()];
        const double hull_area = area(hull);
        if (hull_area == 0) { ++degenerate; return; }
        total_area += hull_area;
        if (unique.size() == 4 && hull.size() == 4) {
            size_t x_values = 0, z_values = 0;
            for (size_t i = 0; i < unique.size(); ++i) {
                bool first_x = true, first_z = true;
                for (size_t j = 0; j < i; ++j) {
                    if (unique[j].x == unique[i].x) first_x = false;
                    if (unique[j].z == unique[i].z) first_z = false;
                }
                x_values += first_x; z_values += first_z;
            }
            if (x_values == 2 && z_values == 2) ++rectangles;
        }
        const auto strip = coverage(quad, kStrip, hull_area), fan = coverage(quad, kFan, hull_area);
        size_t category = 3;
        if (strip.valid && fan.valid) { ++both; category = 2; }
        else if (strip.valid) { ++strip_only; category = 0; }
        else if (fan.valid) { ++fan_only; category = 1; }
        else ++neither;
        ++coverage_by_hull[hull.size()][category];
        const double epsilon = std::max(1.0, hull_area) * 1e-8;
        strip_overlap += strip.overlap > epsilon; fan_overlap += fan.overlap > epsilon;
        strip_missing += strip.missing > epsilon; fan_missing += fan.missing > epsilon;
        strip_missing_area += strip.missing; fan_missing_area += fan.missing;
    }
    void print(const char* label) const {
        std::printf("%s total=%zu degenerate=%zu rectangles=%zu\n", label, total, degenerate, rectangles);
        std::printf("unique_vertices counts_1_2_3_4=%zu,%zu,%zu,%zu hull_vertices=%zu,%zu,%zu,%zu\n",
            unique_counts[1], unique_counts[2], unique_counts[3], unique_counts[4],
            hull_counts[1], hull_counts[2], hull_counts[3], hull_counts[4]);
        std::printf("coverage strip_only=%zu fan_only=%zu both=%zu neither=%zu\n", strip_only, fan_only, both, neither);
        for (size_t vertices : {size_t{3}, size_t{4}}) {
            const auto& c = coverage_by_hull[vertices];
            std::printf("coverage_hull_%zu strip_only=%zu fan_only=%zu both=%zu neither=%zu\n",
                vertices, c[0], c[1], c[2], c[3]);
        }
        std::printf("invalid strip_overlap=%zu strip_missing=%zu fan_overlap=%zu fan_missing=%zu\n",
            strip_overlap, strip_missing, fan_overlap, fan_missing);
        std::printf("missing_hull_area_fraction strip=%.9f fan=%.9f\n",
            total_area ? strip_missing_area / total_area : 0, total_area ? fan_missing_area / total_area : 0);
    }
};
} // namespace

int main() {
    const Quad strip_square{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}};
    const Quad fan_square{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
    for (const auto& square : {strip_square, fan_square}) {
        const auto hull = convex_hull(unique_points(square));
        if (hull.size() != 4 || area(hull) != 1.0)
            throw std::runtime_error("synthetic hull self-check failed");
    }
    if (!coverage(strip_square, kStrip, 1.0).valid || coverage(strip_square, kFan, 1.0).valid ||
        !coverage(fan_square, kFan, 1.0).valid || coverage(fan_square, kStrip, 1.0).valid)
        throw std::runtime_error("synthetic topology self-check failed");
    const auto render = l3::load_l3render(read_authorized("st00.l3render"), true);
    const auto map = l3::load_l3map(read_authorized("st00.l3map"), true);
    Stats source_stats;
    for (const auto& stream : render.streams) for (const auto& primitive : stream.primitives) {
        Quad quad{};
        for (size_t i = 0; i < quad.size(); ++i)
            quad[i] = {double(primitive.local_x[i]), double(primitive.local_z[i])};
        source_stats.add(quad);
    }
    source_stats.print("all_unique_source_primitives");
    Stats runtime_stats;
    for (size_t area_index = 0; area_index < std::min<size_t>(5, map.areas.size()); ++area_index) {
        const auto& area = map.areas[area_index];
        Stats area_stats;
        for (uint32_t local = 0; local < area.cell_count; ++local) {
            const int16_t packed = map.cells.at(area.cell_first + local);
            if (packed <= 0) continue;
            const auto& idx = map.idx.at(static_cast<uint16_t>(packed));
            if (idx.descriptor_index == 0xFFFFu || idx.descriptor_index >= render.descriptors.size()) continue;
            const int32_t gx = area.start_x + int32_t(local % area.width);
            const int32_t gz = area.start_z + int32_t(local / area.width);
            const l3::St00CellOrigin origin{gx * 512 - 0x7F00, idx.y, gz * 512 - 0x7F00};
            const auto primitives = l3::expand_st00_descriptor_lod(render, idx.descriptor_index,
                l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin, static_cast<uint16_t>(packed), 0);
            for (const auto& primitive : primitives) {
                Quad quad{};
                for (size_t i = 0; i < quad.size(); ++i)
                    quad[i] = {double(primitive.world_x[i]), double(primitive.world_z[i])};
                runtime_stats.add(quad); area_stats.add(quad);
            }
        }
        char label[48];
        std::snprintf(label, sizeof(label), "native_primary_mid_area_%zu", area_index);
        area_stats.print(label);
    }
    runtime_stats.print("native_primary_mid_all_five_areas");
}
