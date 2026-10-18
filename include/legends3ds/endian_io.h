#pragma once
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace l3 {
inline uint16_t rd_u16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
inline int16_t rd_s16(const uint8_t* p) { return static_cast<int16_t>(rd_u16(p)); }
inline uint32_t rd_u32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void wr_u16(std::vector<uint8_t>& o, uint16_t v) {
    o.push_back(uint8_t(v)); o.push_back(uint8_t(v >> 8));
}
inline void wr_s16(std::vector<uint8_t>& o, int16_t v) { wr_u16(o, static_cast<uint16_t>(v)); }
inline void wr_u32(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(uint8_t(v)); o.push_back(uint8_t(v >> 8)); o.push_back(uint8_t(v >> 16)); o.push_back(uint8_t(v >> 24));
}
inline void patch_u32(std::vector<uint8_t>& o, size_t at, uint32_t v) {
    if (at + 4 > o.size()) throw std::runtime_error("patch_u32 out of range");
    o[at] = uint8_t(v); o[at+1] = uint8_t(v >> 8); o[at+2] = uint8_t(v >> 16); o[at+3] = uint8_t(v >> 24);
}
}
