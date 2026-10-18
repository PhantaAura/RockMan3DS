#include "legends3ds/render/positive_terrain.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int8_t signed_byte(uint8_t byte) {
    return static_cast<int8_t>(int(byte) - (byte > 127u ? 256 : 0));
}
l3::SourceRenderPrimitive retained(const std::array<uint8_t, 12>& raw) {
    l3::SourceRenderPrimitive p;
    p.local_x = {signed_byte(raw[0]), signed_byte(raw[3]), signed_byte(raw[5]), signed_byte(raw[10])};
    p.local_z = {signed_byte(raw[1]), signed_byte(raw[6]), signed_byte(raw[8]), signed_byte(raw[11])};
    p.ignored_raw = {signed_byte(raw[2]), signed_byte(raw[4]), signed_byte(raw[7]), signed_byte(raw[9])};
    p.prefix = 0x12808080u;
    p.uv = {3, 5, 7, 11, 13, 17, 19, 23};
    return p;
}
template <typename Fn> void rejects(Fn&& fn, const char* message) {
    bool rejected = false;
    try { fn(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, message);
}
l3::St00RenderSource fixture(const l3::SourceRenderPrimitive& p) {
    l3::St00RenderSource source;
    source.descriptors.resize(1);
    // All six slots deliberately differ, catching implicit primary/near use.
    for (uint16_t slot = 0; slot < 6u; ++slot) {
        source.descriptors[0].record_indices[slot] = slot;
        l3::SourceRenderRecord record;
        record.materials = {0xABC00000u + slot};
        record.stream_indices = {slot};
        source.records.push_back(record);
        l3::SourceRenderStream stream;
        stream.base_shift = static_cast<uint8_t>(slot);
        stream.code = static_cast<uint8_t>(slot << 4u);
        stream.primitives = {p};
        source.streams.push_back(stream);
    }
    return source;
}
void test_byte_roles_and_origin() {
    // X/Z have opposite signed extremes; Y crosses both signed boundaries.
    const auto p = retained({128, 0, 127, 127, 127, 128, 1, 128, 255, 255, 255, 1});
    const auto xyz = l3::positive_terrain_consecutive_xyz(p);
    require(xyz.x == std::array<int8_t, 4>{-128, 127, 1, -1}, "positive X byte roles");
    require(xyz.y == std::array<uint8_t, 4>{0, 127, 128, 255}, "positive Y must be unsigned");
    require(xyz.z == std::array<int8_t, 4>{127, -128, -1, 1}, "positive Z byte roles");
    // Values outside int16 prove the helper does not truncate supplied origins.
    const l3::St00CellOrigin origin{40000, -200, -50000};
    const auto e = l3::expand_positive_terrain_primitive(p, 0x12345678u, origin, 2);
    require(e.world_x == std::array<int32_t, 4>{39488, 40508, 40004, 39996}, "positive X origin/addition");
    require(e.world_y == std::array<int32_t, 4>{-200, -708, -712, -1220}, "positive Y subtraction");
    require(e.world_z == std::array<int32_t, 4>{-49492, -50512, -50004, -49996}, "positive Z origin/addition");
    require(e.prefix == p.prefix && e.material_word == 0x12345678u, "raw prefix/material retained");
    require(e.uv == p.uv, "UV/vertex association retained");
    const auto high = l3::expand_positive_terrain_primitive(p, 0, {}, 15);
    require(high.world_x[0] == -4194304 && high.world_x[1] == 4161536, "highest supported signed X shift");
    require(high.world_y[3] == -8355840, "highest supported unsigned Y shift");
}
void test_bank_lod_material_and_scaling() {
    const auto p = retained({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    auto source = fixture(p);
    const l3::St00CellOrigin origin{100, -200, 300};
    for (uint8_t bank = 0; bank < 2u; ++bank) {
        for (uint8_t lod = 0; lod < 3u; ++lod) {
            const uint8_t slot = static_cast<uint8_t>(bank * 3u + lod);
            const auto expanded = l3::expand_positive_terrain_descriptor_lod(source, 0,
                static_cast<l3::St00RenderBank>(bank), static_cast<l3::St00Lod>(lod), origin);
            require(expanded.size() == 1u, "selected bank/LOD primitive count");
            require(expanded[0].material_word == 0xABC00000u + slot, "selected bank/LOD material");
            const int32_t scale = int32_t{1} << slot;
            require(expanded[0].world_x[0] == 100 + scale, "selected bank/LOD local scale");
            require(expanded[0].world_y[0] == -200 - 2 * scale, "selected bank/LOD unsigned Y");
            require(expanded[0].uv == p.uv, "bank/LOD preserves UV");
        }
    }
    const auto scaled = l3::expand_positive_terrain_descriptor_lod(source, 0,
        l3::St00RenderBank::Secondary, l3::St00Lod::Near, origin, 1);
    require(scaled[0].world_x[0] == 216 && scaled[0].world_y[0] == -432 &&
        scaled[0].world_z[0] == 648, "global shift scales IDX XYZ and local stream coordinates");

    // Reused streams resolve their material in the selected record, never in
    // the first record which happens to reference the stream.
    source.records[1].stream_indices = {0};
    source.records[1].materials = {0x98765432u};
    const auto shared = l3::expand_positive_terrain_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin);
    require(shared[0].material_word == 0x98765432u && shared[0].world_x[0] == 101,
        "shared stream selected-record material");
    source.descriptors[0].record_indices[2] = 0xFFFFu;
    require(l3::expand_positive_terrain_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Far, origin).empty(), "absent slot stays empty");
    source.records[0].stream_indices.clear();
    require(l3::expand_positive_terrain_descriptor_lod(source, 0,
        l3::St00RenderBank::Primary, l3::St00Lod::Near, origin).empty(), "valid empty record stays empty");
}
void test_failures() {
    const auto p = retained({128, 0, 127, 127, 127, 128, 1, 128, 255, 255, 255, 1});
    const auto hi = std::numeric_limits<int32_t>::max();
    const auto lo = std::numeric_limits<int32_t>::min();
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {}, 16); }, "invalid primitive shift rejected");
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {hi, 0, 0}, 0); }, "X positive overflow rejected");
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {lo, 0, 0}, 0); }, "X negative overflow rejected");
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {0, lo, 0}, 0); }, "Y subtraction overflow rejected");
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {0, 0, hi}, 0); }, "Z positive overflow rejected");
    rejects([&] { l3::expand_positive_terrain_primitive(p, 0, {0, 0, lo}, 0); }, "Z negative overflow rejected");
    // Extreme zero-shift origins remain valid when there is no local offset.
    const auto zero = retained({});
    const auto exact = l3::expand_positive_terrain_primitive(zero, 0, {hi, hi, lo}, 0);
    require(exact.world_x[0] == hi && exact.world_y[0] == hi && exact.world_z[0] == lo,
        "valid extreme origins retained");

    const auto pristine = fixture(p);
    auto call = [](const l3::St00RenderSource& source, const l3::St00CellOrigin& origin,
                   uint8_t shift = 0) {
        return l3::expand_positive_terrain_descriptor_lod(source, 0,
            l3::St00RenderBank::Primary, l3::St00Lod::Near, origin, shift);
    };
    rejects([&] { l3::expand_positive_terrain_descriptor_lod(pristine, 1,
        l3::St00RenderBank::Primary, l3::St00Lod::Near, {}); }, "invalid descriptor rejected");
    rejects([&] { l3::expand_positive_terrain_descriptor_lod(pristine, 0,
        static_cast<l3::St00RenderBank>(2), l3::St00Lod::Near, {}); }, "invalid bank rejected");
    rejects([&] { l3::expand_positive_terrain_descriptor_lod(pristine, 0,
        l3::St00RenderBank::Primary, static_cast<l3::St00Lod>(3), {}); }, "invalid LOD rejected");
    rejects([&] { call(pristine, {}, 16); }, "invalid global shift rejected");
    for (const l3::St00CellOrigin origin : {l3::St00CellOrigin{hi, 0, 0},
            l3::St00CellOrigin{0, lo, 0}, l3::St00CellOrigin{0, 0, hi}})
        rejects([&] { call(pristine, origin, 1); }, "scaled IDX origin overflow rejected");
    auto malformed = pristine;
    malformed.descriptors[0].record_indices[0] = 6;
    rejects([&] { call(malformed, {}); }, "missing record rejected");
    malformed = pristine; malformed.records[0].stream_indices = {6};
    rejects([&] { call(malformed, {}); }, "missing stream rejected");
    malformed = pristine; malformed.streams[0].material_slot = 1;
    rejects([&] { call(malformed, {}); }, "material slot out of range rejected");
    malformed = pristine; malformed.records[0].materials.clear();
    rejects([&] { call(malformed, {}); }, "missing material bank rejected");
    malformed = pristine; malformed.streams[0].base_shift = 16;
    rejects([&] { call(malformed, {}); }, "invalid stream shift rejected");
    malformed = pristine; malformed.streams[0].base_shift = 15;
    rejects([&] { call(malformed, {}, 1); }, "overflowing combined shift rejected");
}
} // namespace

int main() {
    try {
        test_byte_roles_and_origin();
        test_bank_lod_material_and_scaling();
        test_failures();
        std::cout << "positive terrain source-backed coordinate and malformed-input tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "positive terrain test failed: " << e.what() << '\n';
        return 1;
    }
}
