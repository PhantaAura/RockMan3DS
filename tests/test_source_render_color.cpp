#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/st00_source.h"
#include "legends3ds/render/ps1_texture_modulation.h"
#include "legends3ds/render/source_depth_cue.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool pass, const char* reason) { if (!pass) throw std::runtime_error(reason); }
template<class Function> void rejects(Function function, const char* reason) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, reason);
}
uint32_t number(std::istringstream& stream) {
    uint64_t value = 0;
    require(bool(stream >> value) && value <= 0xffffffffu, "invalid source color fixture number");
    return static_cast<uint32_t>(value);
}

l3::PbdModel authored_model() {
    l3::PbdModel model;
    l3::PbdPart part;
    part.part_id = 7;
    part.vertices = {{10,20,30,0}, {40,50,60,0}, {70,80,90,0}, {100,110,120,0}};
    l3::PbdTri tri;
    tri.index = {{0,1,2}}; tri.uv = {{0,1,2,3,4,5}};
    tri.clut = 0x2345; tri.tpage = 0x67; tri.r = 11; tri.g = 128; tri.b = 255; tri.code = 0x24;
    l3::PbdQuad quad;
    quad.index = {{0,1,2,3}}; quad.uv = {{6,7,8,9,10,11,12,13}};
    quad.clut = 0x3456; quad.tpage = 0x78; quad.r = 255; quad.g = 64; quad.b = 0; quad.code = 0x2c;
    part.triangles.push_back(tri); part.quads.push_back(quad);
    model.parts.push_back(part);
    return model;
}

void model_color_contract() {
    auto model = authored_model();
    for (uint8_t tri = 0x20; tri <= 0x27; ++tri) {
        for (uint8_t quad = 0x28; quad <= 0x2f; ++quad) {
            model.parts[0].triangles[0].code = tri;
            model.parts[0].quads[0].code = quad;
            const auto loaded = l3::load_l3model(l3::cook_l3model(model));
            const auto& a = loaded.parts[0].triangles[0];
            const auto& b = loaded.parts[0].quads[0];
            require(a.code == tri && a.r == 11 && a.g == 128 && a.b == 255,
                "flat triangle RGB/opcode changed");
            require(b.code == quad && b.r == 255 && b.g == 64 && b.b == 0,
                "flat quad RGB/opcode changed");
        }
    }
    model = authored_model();
    const auto bytes = l3::cook_l3model(model);
    const auto header = l3::inspect_l3model(bytes);
    for (uint8_t code : {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x28,0x60}) {
        auto invalid_model = model;
        invalid_model.parts[0].triangles[0].code = code;
        rejects([&] { (void)l3::cook_l3model(invalid_model); }, "unsupported triangle cooked");
        auto malformed = bytes;
        malformed[header.off_triangles + 19] = code;
        l3::patch_u32(malformed, 48, l3::crc32(malformed.data() + 64, malformed.size() - 64));
        rejects([&] { (void)l3::load_l3model(malformed); }, "unsupported triangle loaded");
        rejects([&] { (void)l3::load_l3model(malformed, false); }, "unchecked triangle loaded");
    }
    for (uint8_t code : {0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x24,0x60}) {
        auto invalid_model = model;
        invalid_model.parts[0].quads[0].code = code;
        rejects([&] { (void)l3::cook_l3model(invalid_model); }, "unsupported quad cooked");
        auto malformed = bytes;
        malformed[header.off_quads + 23] = code;
        l3::patch_u32(malformed, 48, l3::crc32(malformed.data() + 64, malformed.size() - 64));
        rejects([&] { (void)l3::load_l3model(malformed); }, "unsupported quad loaded");
        rejects([&] { (void)l3::load_l3model(malformed, false); }, "unchecked quad loaded");
    }
}

void scalar_color_contract() {
    const std::array<uint32_t,3> rgb{{0x01234567u, 0x89abcdefu, 0xffffffffu}};
    const std::array<uint32_t,3> controls{{0x12345670u, 0x9abcdef0u, 0xfffffff0u}};
    require(l3::source_far_color_8007b0f8(rgb).control21_23 == controls, "far leaf shift changed");
    require(l3::source_back_color_8007b0d8(rgb).control13_15 == controls, "back leaf shift changed");
    require(l3::source_flat_textured_color_02ffffff(0xfd123456u, false) == 0x24123456u,
        "source metadata misread as raw triangle opcode");
    require(l3::source_flat_textured_color_02ffffff(0xfe123456u, true) == 0x2e123456u,
        "source ABE bit lost");

    for (unsigned color : {0u,1u,64u,128u,254u,255u})
        for (unsigned fc : {0u,1u,16u,64u,128u,255u})
            for (unsigned ir0 : {0u,1u,17u,2047u,2048u,4095u,4096u}) {
                const auto far = l3::source_far_color_8007b0f8({{fc,fc,fc}});
                l3::SourceDpcsColor result;
                const uint32_t rgba = 0x2c000000u | color | color << 8u | color << 16u;
                require(l3::source_dpcs_0780010(rgba, far, uint16_t(ir0), result), "owned color held");
                // Independently weighted endpoint form; division is floor for
                // this nonnegative input domain, including descending fades.
                const unsigned byte = (color * (4096u - ir0) + fc * ir0) / 4096u;
                const uint32_t expected = 0x2c000000u | byte | byte << 8u | byte << 16u;
                require(result.rgb_code == expected, "DPCS interpolation/rounding changed");
                for (size_t i = 0; i < 3; ++i)
                    require(result.mac[i] == result.ir[i] && result.mac[i] >= 0 && result.mac[i] <= 4080,
                        "admitted DPCS escaped MAC/IR bounds");
                // Textured DPCS outputs are modulation colors, not a fog RGB
                // to blend after sampling. Neutral128 remains unity gain.
                const float input = l3::ps1_texture_color_input(0x2c, uint8_t(byte));
                require(std::abs(input * 2.0f - float(byte) / 128.0f) < 1e-6f,
                    "depth-cue modulation gain changed");
            }
    l3::SourceDpcsColor untouched;
    untouched.rgb_code = 0x12345678u; untouched.mac = {{11,22,33}}; untouched.ir = {{44,55,66}};
    const auto before = untouched;
    for (const auto far : {l3::SourceFarColorControls{{{4096,0,0}}},
                          l3::SourceFarColorControls{{{1,0,0}}},
                          l3::SourceFarColorControls{{{0xffffffffu,0,0}}}}) {
        require(!l3::source_dpcs_0780010(0x2c808080, far, 0, untouched), "unknown far domain guessed");
        require(untouched.rgb_code == before.rgb_code && untouched.mac == before.mac && untouched.ir == before.ir,
            "failed DPCS mutated output");
    }
    require(!l3::source_dpcs_0780010(0x2c808080, {}, 4097, untouched), "invalid IR0 admitted");
    require(untouched.rgb_code == before.rgb_code && untouched.mac == before.mac && untouched.ir == before.ir,
        "invalid IR0 mutated output");
}

void source_fixture(const char* path) {
    std::ifstream file(path); require(bool(file), "source color fixture unavailable");
    std::string line; size_t far_cases = 0, back_cases = 0, packet_cases = 0, dpcs_cases = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream stream(line); char kind = 0; stream >> kind;
        if (kind == 'F' || kind == 'B') {
            std::array<uint32_t,3> rgb{}; for (auto& value : rgb) value = number(stream);
            std::array<uint32_t,3> expected{}; for (auto& value : expected) value = number(stream);
            if (kind == 'F') { require(l3::source_far_color_8007b0f8(rgb).control21_23 == expected, "source far mismatch"); ++far_cases; }
            else { require(l3::source_back_color_8007b0d8(rgb).control13_15 == expected, "source back mismatch"); ++back_cases; }
        } else if (kind == 'P') {
            const auto prefix = number(stream), quad = number(stream), expected = number(stream);
            require(quad == 1, "source packet slice requires quad");
            require(l3::source_flat_textured_color_02ffffff(prefix, true) == expected, "source flat packet mismatch");
            ++packet_cases;
        } else if (kind == 'D') {
            const auto rgbc = number(stream);
            l3::SourceFarColorControls far;
            for (auto& value : far.control21_23) value = number(stream);
            const auto ir0 = number(stream); require(ir0 <= 4096, "fixture IR0 domain invalid");
            l3::SourceDpcsColor result;
            require(l3::source_dpcs_0780010(rgbc, far, uint16_t(ir0), result), "source DPCS held");
            require(result.rgb_code == number(stream), "source DPCS RGB mismatch");
            for (auto value : result.mac) require(value == int32_t(number(stream)), "source DPCS MAC mismatch");
            for (auto value : result.ir) require(value == int32_t(number(stream)), "source DPCS IR mismatch");
            ++dpcs_cases;
        } else throw std::runtime_error("unknown source color fixture row");
        std::string trailing; require(!(stream >> trailing), "unexpected source color fixture column");
    }
    require(far_cases == 128 && back_cases == 128 && packet_cases == 128 && dpcs_cases == 256,
        "source color fixture coverage changed");
}
}

int main(int argc, char** argv) {
    try {
        model_color_contract(); scalar_color_contract();
        if (argc == 2) source_fixture(argv[1]);
        else if (argc == 3 && std::string(argv[1]) == "--owner-model")
            (void)l3::load_l3model(l3::read_file(argv[2]));
        else require(argc == 1, "usage: test_source_render_color [fixture | --owner-model model]");
        std::cout << "source render color checks passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
