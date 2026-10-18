#include "legends3ds/material_cache.h"
#include "legends3ds/render/ps1_geometry.h"
#include "legends3ds/render/ps1_texture_modulation.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {

constexpr double kEpsilon = 1e-9;

struct Point {
    double x;
    double y;
};

using Quad = std::array<Point, 4>;
using Triangle = std::array<Point, 3>;
using Indices = std::array<unsigned, 6>;

double cross(Point a, Point b, Point c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double polygon_area(const std::vector<Point>& polygon) {
    double twice_area = 0.0;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1) % polygon.size()];
        twice_area += a.x * b.y - a.y * b.x;
    }
    return std::abs(twice_area) * 0.5;
}

Triangle triangle(const Quad& quad, const Indices& indices, size_t first) {
    for (size_t i = first; i < first + 3; ++i) assert(indices[i] < quad.size());
    return {{quad[indices[first]], quad[indices[first + 1]], quad[indices[first + 2]]}};
}

// Clip one triangle against the other. Measuring their intersection catches an
// overlap and a matching hole that a summed-triangle-area assertion would miss.
double intersection_area(const Triangle& subject, const Triangle& clip) {
    std::vector<Point> polygon(subject.begin(), subject.end());
    const double winding = cross(clip[0], clip[1], clip[2]) > 0.0 ? 1.0 : -1.0;
    for (size_t edge = 0; edge < clip.size() && !polygon.empty(); ++edge) {
        const Point a = clip[edge];
        const Point b = clip[(edge + 1) % clip.size()];
        std::vector<Point> next;
        Point previous = polygon.back();
        double previous_distance = winding * cross(a, b, previous);
        for (Point current : polygon) {
            const double current_distance = winding * cross(a, b, current);
            const bool previous_inside = previous_distance >= 0.0;
            const bool current_inside = current_distance >= 0.0;
            if (previous_inside != current_inside) {
                const double t = previous_distance / (previous_distance - current_distance);
                next.push_back({previous.x + t * (current.x - previous.x),
                                previous.y + t * (current.y - previous.y)});
            }
            if (current_inside) next.push_back(current);
            previous = current;
            previous_distance = current_distance;
        }
        polygon = std::move(next);
    }
    return polygon_area(polygon);
}

struct Coverage {
    double overlap_area;
    double union_area;
};

Coverage coverage(const Quad& quad, const Indices& indices) {
    const auto a = triangle(quad, indices, 0);
    const auto b = triangle(quad, indices, 3);
    const double overlap = intersection_area(a, b);
    const double total = (std::abs(cross(a[0], a[1], a[2])) +
                          std::abs(cross(b[0], b[1], b[2]))) * 0.5;
    return {overlap, total - overlap};
}

bool covers_once(Coverage result, double expected_area) {
    return std::abs(result.overlap_area) < kEpsilon &&
           std::abs(result.union_area - expected_area) < kEpsilon;
}

// Distinct corner UVs from a known affine mapping give an independent expected
// interpolated value for both the square and the trapezoid.
Point expected_uv(Point position) {
    return {0.10 + 0.08 * position.x + 0.03 * position.y,
            0.20 + 0.02 * position.x + 0.15 * position.y};
}

bool sample_uv(const Quad& positions, const Quad& uv, const Indices& indices,
               size_t first, Point sample, Point& result) {
    const auto p = triangle(positions, indices, first);
    const double denominator = cross(p[0], p[1], p[2]);
    assert(std::abs(denominator) > kEpsilon);
    const double w0 = cross(sample, p[1], p[2]) / denominator;
    const double w1 = cross(sample, p[2], p[0]) / denominator;
    const double w2 = 1.0 - w0 - w1;
    if (w0 < -kEpsilon || w1 < -kEpsilon || w2 < -kEpsilon) return false;
    const auto t = triangle(uv, indices, first);
    result = {w0 * t[0].x + w1 * t[1].x + w2 * t[2].x,
              w0 * t[0].y + w1 * t[1].y + w2 * t[2].y};
    return true;
}

void assert_uv_sample(const Quad& positions, const Quad& uv, Point sample) {
    bool sampled = false;
    const auto expected = expected_uv(sample);
    for (size_t first : {size_t{0}, size_t{3}}) {
        Point actual{};
        if (!sample_uv(positions, uv, l3::kPs1QuadTriangleIndices, first, sample, actual)) continue;
        sampled = true;
        assert(std::abs(actual.x - expected.x) < kEpsilon);
        assert(std::abs(actual.y - expected.y) < kEpsilon);
    }
    assert(sampled);
}

void test_quad(const Quad& positions, double expected_area) {
    const auto a = triangle(positions, l3::kPs1QuadTriangleIndices, 0);
    const auto b = triangle(positions, l3::kPs1QuadTriangleIndices, 3);
    assert(cross(a[0], a[1], a[2]) * cross(b[0], b[1], b[2]) > 0.0);
    assert(covers_once(coverage(positions, l3::kPs1QuadTriangleIndices), expected_area));

    constexpr Indices old_fan{{0, 1, 2, 0, 2, 3}};
    const auto old = coverage(positions, old_fan);
    assert(!covers_once(old, expected_area));
    assert(old.overlap_area > kEpsilon);
    assert(old.union_area < expected_area - kEpsilon);

    Quad uv{};
    Point center{};
    for (size_t i = 0; i < positions.size(); ++i) {
        uv[i] = expected_uv(positions[i]);
        center.x += positions[i].x * 0.25;
        center.y += positions[i].y * 0.25;
    }
    for (Point corner : positions) {
        assert_uv_sample(positions, uv, corner);
        assert_uv_sample(positions, uv, {0.95 * corner.x + 0.05 * center.x,
                                       0.95 * corner.y + 0.05 * center.y});
    }
    assert_uv_sample(positions, uv, center);
}

void test_texture_modulation() {
    for (float texel : {0.0f, 0.25f, 0.75f, 1.0f}) {
        for (unsigned color : {0u, 64u, 128u, 192u, 255u}) {
            const float input = l3::ps1_texture_color_input(0x2cu, static_cast<uint8_t>(color));
            const float actual = std::fmin(1.0f, texel * input * 2.0f);
            const float expected = std::fmin(1.0f, texel * float(color) / 128.0f);
            assert(std::abs(actual - expected) < 1e-6f);
            assert(input >= 0.0f && input <= 1.0f);
            // Raw texture ignores vertex color, including black and full white.
            const float raw = l3::ps1_texture_color_input(0x2du, static_cast<uint8_t>(color));
            assert(std::abs(texel * raw * 2.0f - texel) < 1e-6f);
        }
    }
}

void test_palette_alpha() {
    l3::MaterialCacheEntry entry{};
    entry.material_word = 0; // Synthetic 4bpp TPAGE with no external resources.
    entry.bpp = 4;
    entry.palette_entries = 16;
    entry.source_width = 6;
    entry.source_height = 1;
    entry.resolved = true;
    entry.indices.resize(size_t(l3::kL3MaterialTextureWidth) * l3::kL3MaterialTextureHeight);
    entry.palette_ps1.resize(16);
    constexpr std::array<uint16_t, 6> colors{{0x0000, 0x8000, 0x001F, 0x801F, 0x7FFF, 0xFFFF}};
    constexpr std::array<std::array<uint8_t, 4>, 6> expected{{
        {{0, 0, 0, 0}}, {{0, 0, 0, 128}}, {{255, 0, 0, 255}},
        {{255, 0, 0, 128}}, {{255, 255, 255, 255}}, {{255, 255, 255, 128}}
    }};
    for (size_t i = 0; i < colors.size(); ++i) {
        // A nonzero index for transparent zero verifies palette-word semantics.
        entry.indices[i] = static_cast<uint8_t>(i + 1);
        entry.palette_ps1[i + 1] = colors[i];
    }
    const auto palette_before = entry.palette_ps1;
    const auto rgba = l3::material_page_rgba8(entry);
    assert(rgba.size() == entry.indices.size() * 4);
    for (size_t i = 0; i < expected.size(); ++i)
        for (size_t channel = 0; channel < 4; ++channel)
            assert(rgba[i * 4 + channel] == expected[i][channel]);
    assert(entry.palette_ps1 == palette_before);
    // All supported texel classes under triangle/quad/raw/modulated opcodes.
    // In particular STP black is visible, zero is discarded and opaque heads
    // do not become translucent because their CLUT sets the high bit.
    for (uint8_t opcode : {0x24, 0x25, 0x26, 0x27, 0x2c, 0x2d, 0x2e, 0x2f}) {
        const bool semi = (opcode & 2u) != 0;
        assert(l3::ps1_textured_semitransparency_enabled(opcode) == semi);
        for (size_t i = 0; i < colors.size(); ++i) {
            const float actual = std::fmin(1.0f, float(rgba[i*4+3]) / 255.0f *
                l3::ps1_texture_alpha_scale(opcode));
            const float expected_alpha = colors[i] == 0 ? 0.0f :
                (semi && (colors[i] & 0x8000u) ? 128.0f/255.0f : 1.0f);
            assert(std::abs(actual - expected_alpha) < 1e-6f);
        }
    }
}

} // namespace

int main() {
    test_quad(Quad{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}, 1.0);
    test_quad(Quad{{{-1, 0}, {3, 0}, {0, 2}, {2, 2}}}, 6.0);
    test_palette_alpha();
    test_texture_modulation();
}
