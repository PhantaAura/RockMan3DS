#include "legends3ds/render/terrain_clut_ramp.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeader = 20, kEntry = 4 + 8 * 3 * 2;
constexpr uint32_t kMaxEntries = 4096;
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = ~0u;
    while (n--) { c ^= *p++; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
void put16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); }
void put32(std::vector<uint8_t>& b, uint32_t v) { put16(b, uint16_t(v)); put16(b, uint16_t(v >> 16)); }
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
uint32_t rd32(const uint8_t* p) { return uint32_t(rd16(p)) | uint32_t(rd16(p + 2)) << 16; }
}

TerrainFaceDepth terrain_face_depth_80025900(const std::array<uint16_t,4>& sz, uint32_t prefix) noexcept {
    const int32_t mx = *std::max_element(sz.begin(), sz.end());
    const int32_t sum = int32_t(sz[0]) + sz[1] + sz[2] + sz[3];
    TerrainFaceDepth d;
    if (prefix & 0x80000000u) {
        d.otz = (prefix & 0x08000000u) ? int32_t(sz[0]) : mx;
        return d;
    }
    if (mx < 0xC1) { d.near_culled = true; return d; }
    d.otz = (prefix & 0x08000000u) ? mx - 16 : (mx + (sum >> 2)) >> 1;
    return d;
}

uint8_t terrain_clut_ramp_step(int32_t otz, uint32_t prefix,
    const std::array<uint8_t,40>& area, uint8_t lod) noexcept {
    if (prefix & 0x10000000u) return 0;   // CANDIDATE: the original keeps the previous face's value
    const uint8_t step = area[size_t((otz >> ((9u + lod) & 31u)) & 31)];
    return step > 7 ? 7 : step;
}

TerrainClutRampEntry fit_terrain_clut_ramp(uint32_t word, const std::vector<uint16_t>& base,
    const std::array<std::vector<uint16_t>,8>& variants) {
    TerrainClutRampEntry e;
    e.material_word = word;
    for (size_t k = 0; k < 8; ++k) {
        for (size_t c = 0; c < 3; ++c) {
            double num = 0, den = 0;
            const auto& v = variants[k];
            if (k > 0 && v.size() == base.size()) {
                for (size_t i = 0; i < base.size(); ++i) {
                    const double b = (base[i] >> (5 * c)) & 31, q = (v[i] >> (5 * c)) & 31;
                    num += b * q; den += b * b;
                }
            }
            const double s = (k == 0 || den == 0) ? 1.0 : std::clamp(num / den, 0.0, 1.0);
            e.scale_q12[k][c] = uint16_t(std::lround(s * 4096.0));
        }
    }
    return e;
}

std::vector<uint8_t> cook_l3clut_ramp(const std::vector<TerrainClutRampEntry>& entries) {
    if (entries.size() > kMaxEntries) throw std::runtime_error("L3CR entry count");
    std::vector<uint8_t> payload;
    for (const auto& e : entries) {
        put32(payload, e.material_word);
        for (const auto& s : e.scale_q12) for (const auto v : s) {
            if (v > 4096) throw std::runtime_error("L3CR scale above unity");
            put16(payload, v);
        }
    }
    std::vector<uint8_t> out = {'L','3','C','R','0','0','1',0};
    put32(out, 1); put32(out, uint32_t(entries.size())); put32(out, crc32(payload.data(), payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<TerrainClutRampEntry> load_l3clut_ramp(const uint8_t* b, size_t size) {
    if (!b || size < kHeader || std::memcmp(b, "L3CR001\0", 8) || rd32(b + 8) != 1)
        throw std::runtime_error("L3CR header");
    const uint32_t n = rd32(b + 12);
    if (n > kMaxEntries || size != kHeader + size_t(n) * kEntry) throw std::runtime_error("L3CR size");
    if (crc32(b + kHeader, size - kHeader) != rd32(b + 16)) throw std::runtime_error("L3CR CRC");
    std::vector<TerrainClutRampEntry> out(n);
    const uint8_t* p = b + kHeader;
    for (auto& e : out) {
        e.material_word = rd32(p); p += 4;
        for (auto& s : e.scale_q12) for (auto& v : s) {
            v = rd16(p); p += 2;
            if (v > 4096) throw std::runtime_error("L3CR scale above unity");
        }
    }
    for (size_t i = 1; i < out.size(); ++i)
        for (size_t j = 0; j < i; ++j)
            if (out[j].material_word == out[i].material_word) throw std::runtime_error("L3CR duplicate word");
    return out;
}

const TerrainClutRampEntry* find_terrain_clut_ramp(const std::vector<TerrainClutRampEntry>& entries,
    uint32_t word) noexcept {
    for (const auto& e : entries) if (e.material_word == word) return &e;
    return nullptr;
}
} // namespace l3
