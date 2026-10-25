#include "legends3ds/render/psx_gpu.h"
#include <cstdlib>
#include <stdexcept>

namespace l3::psxgpu {

uint32_t PrimitiveBuffer::allocate(uint8_t payload_words, uint8_t extra_words) {
    const size_t at = words_.size();
    if (at + 1u + payload_words + extra_words > 0xFFFFFFu) throw std::length_error("primitive buffer full");
    words_.resize(at + 1u + payload_words + extra_words, 0);
    words_[at] = uint32_t(payload_words) << 24;
    return uint32_t(at + 1u);
}

void OrderingTable::add_prim(PrimitiveBuffer& buffer, size_t bucket, uint32_t packet) {
    if (heads_.empty()) return;
    if (bucket >= heads_.size()) bucket = heads_.size() - 1u;
    auto& t = buffer.tag(packet);
    t = (t & 0xFF000000u) | heads_[bucket];
    heads_[bucket] = packet;
}

void OrderingTable::insert_after(PrimitiveBuffer& buffer, uint32_t prev, uint32_t packet) {
    auto& pt = buffer.tag(prev);
    auto& t = buffer.tag(packet);
    t = (t & 0xFF000000u) | (pt & 0xFFFFFFu);
    pt = (pt & 0xFF000000u) | packet;
}

std::optional<Gp0Polygon> decode_gp0_polygon(const uint32_t* w, size_t count) noexcept {
    if (!w || count == 0) return std::nullopt;
    const uint8_t code = uint8_t(w[0] >> 24);
    if ((code & 0xE0u) != 0x20u || count < gp0_polygon_words(code)) return std::nullopt;
    Gp0Polygon p;
    p.code = code;
    p.vertices = (code & 0x08u) ? 4 : 3;
    p.gouraud = code & 0x10u; p.textured = code & 0x04u;
    p.semi_transparent = code & 0x02u; p.raw_texture = code & 0x01u;
    // Flat: [code|rgb] then xy[, uv] per vertex. Gouraud: vertex 0's colour
    // is the code word, each later vertex starts with its own colour word.
    size_t i = 1;
    for (uint8_t k = 0; k < p.vertices; ++k) {
        p.rgb[k] = (p.gouraud && k > 0) ? (w[i++] & 0xFFFFFFu) : (w[0] & 0xFFFFFFu);
        const uint32_t xy = w[i++];
        p.x[k] = int16_t(uint16_t(xy)); p.y[k] = int16_t(uint16_t(xy >> 16));
        if (p.textured) {
            const uint32_t uv = w[i++];
            p.u[k] = uint8_t(uv); p.v[k] = uint8_t(uv >> 8);
            if (k == 0) p.clut = uint16_t(uv >> 16);
            if (k == 1) p.tpage = uint16_t(uv >> 16);
        }
    }
    return p;
}

void write_poly_ft4(uint32_t* w, const PolyFt4& p) noexcept {
    w[0] = p.code_rgb;
    for (size_t k = 0; k < 4; ++k) {
        w[1 + 2 * k] = uint32_t(uint16_t(p.x[k])) | (uint32_t(uint16_t(p.y[k])) << 16);
        uint32_t uv = uint32_t(p.u[k]) | (uint32_t(p.v[k]) << 8);
        if (k == 0) uv |= uint32_t(p.clut) << 16;
        if (k == 1) uv |= uint32_t(p.tpage) << 16;
        w[2 + 2 * k] = uv;
    }
}

void write_poly_ft3(uint32_t* w, const PolyFt4& p) noexcept {
    w[0] = p.code_rgb;
    for (size_t k = 0; k < 3; ++k) {
        w[1 + 2 * k] = uint32_t(uint16_t(p.x[k])) | (uint32_t(uint16_t(p.y[k])) << 16);
        uint32_t uv = uint32_t(p.u[k]) | (uint32_t(p.v[k]) << 8);
        if (k == 0) uv |= uint32_t(p.clut) << 16;
        if (k == 1) uv |= uint32_t(p.tpage) << 16;
        w[2 + 2 * k] = uv;
    }
}

PolyFt4 terrain_poly_ft4_80026158(const TerrainPacketInput& in) noexcept {
    PolyFt4 p;
    p.code_rgb = in.black ? 0x2C010101u
        : (((in.prefix & 0x02FFFFFFu) | 0x2C000000u) & 0xFF000000u) | (in.rgb & 0xFFFFFFu);
    p.code_rgb |= in.see_through_code;
    for (size_t k = 0; k < 4; ++k) {
        p.x[k] = in.sx[k]; p.y[k] = in.sy[k];
        p.u[k] = in.uv[2 * k]; p.v[k] = in.uv[2 * k + 1];
    }
    p.clut = uint16_t((in.material_word >> 16) + in.ramp);
    p.tpage = uint16_t(in.material_word);
    return p;
}

NegativePacket terrain_negative_packet_80021b14(uint32_t prefix, uint32_t rgb, uint32_t material_word,
    uint8_t ramp, const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx,
    const std::array<int16_t,4>& sy) noexcept {
    NegativePacket out;
    out.triangle = (prefix & 0x20000000u) != 0;
    out.poly.code_rgb = (((prefix & 0x03FFFFFFu) | (out.triangle ? 0x24000000u : 0x2C000000u)) & 0xFF000000u) |
                        (rgb & 0xFFFFFFu);
    for (size_t k = 0; k < 4; ++k) {
        out.poly.x[k] = sx[k]; out.poly.y[k] = sy[k];
        out.poly.u[k] = uv[2 * k]; out.poly.v[k] = uv[2 * k + 1];
    }
    out.poly.clut = uint16_t((material_word >> 16) + ramp);
    out.poly.tpage = uint16_t(material_word);
    return out;
}

uint8_t terrain_negative_ramp_8002196c(int32_t cell_depth, const std::array<uint8_t,40>& area_record) noexcept {
    const int32_t d = cell_depth < 0 ? cell_depth + 0x1FF : cell_depth;
    return area_record[size_t((d >> 9) & 0x1F)];
}

PolyFt4 player_packet_80027be0(uint32_t code_word, bool quad, uint32_t material_word, uint8_t ramp,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept {
    PolyFt4 p;
    p.code_rgb = quad ? (code_word | 0x08000000u) : code_word;
    for (size_t k = 0; k < 4; ++k) {
        p.x[k] = sx[k]; p.y[k] = sy[k];
        p.u[k] = uv[2 * k]; p.v[k] = uv[2 * k + 1];
    }
    p.clut = uint16_t((uint16_t(material_word >> 16) & 0xFFF8u) | ramp);
    p.tpage = uint16_t(material_word);
    return p;
}

uint8_t actor_ramp_80027f78(int32_t sz_quarter, const std::array<uint8_t,40>& area_record) noexcept {
    const int32_t d = sz_quarter < 0 ? sz_quarter + 0x7F : sz_quarter;
    return area_record[size_t((d >> 7) & 0x1F)];
}

namespace {
PolyFt4 model_packet(uint32_t code, uint16_t clut, uint16_t tpage,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept {
    PolyFt4 p;
    p.code_rgb = code;
    for (size_t k = 0; k < 4; ++k) {
        p.x[k] = sx[k]; p.y[k] = sy[k];
        p.u[k] = uv[2 * k]; p.v[k] = uv[2 * k + 1];
    }
    p.clut = clut; p.tpage = tpage;
    return p;
}
}

PolyFt4 mesh_packet_8002a6e8(uint32_t code_rgb, bool quad, uint32_t material_word, uint16_t ramp_bits,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept {
    return model_packet((code_rgb & 0x02FFFFFFu) | (quad ? 0x2C000000u : 0x24000000u),
        uint16_t(uint16_t(material_word >> 16) | ramp_bits), uint16_t(material_word), uv, sx, sy);
}

uint16_t e0_ramp_8002a3ec(uint8_t flags, int32_t sz_quarter, const std::array<uint8_t,40>& area_record) noexcept {
    if (flags & 0x08u) return 0;
    const int32_t d = sz_quarter < 0 ? sz_quarter + 0x7F : sz_quarter;
    const uint8_t r = area_record[size_t((d >> 7) & 0x1F)];
    return (flags & 0x80u) ? uint16_t(((r >> 2) << 6) + (r & 3u)) : r;
}

PolyFt4 class20_packet_800289a0(uint32_t colour_code, bool quad, uint32_t material_word, uint8_t ramp,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept {
    return model_packet(colour_code | (quad ? 0x2C000000u : 0x24000000u),
        uint16_t(uint16_t(material_word >> 16) | ramp), uint16_t(uint16_t(material_word) | 0x20u), uv, sx, sy);
}

bool terrain_needs_subdivision_8002602c(int64_t mac0, const std::array<int16_t,4>& sx,
    const std::array<int16_t,4>& sy) noexcept {
    const int64_t area = mac0 < 0 ? -mac0 : mac0;
    if (area < 513) return false;
    if (area >= 2049) return true;
    const auto flat = [](int32_t a0, int32_t a1, int32_t a2, int32_t a3) {
        return uint32_t(((a0 + a3 - a1 - a2) >> 1) + 3) < 7u;
    };
    return !(flat(sx[0], sx[1], sx[2], sx[3]) && flat(sy[0], sy[1], sy[2], sy[3]));
}

} // namespace l3::psxgpu
