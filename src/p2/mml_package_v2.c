#include "mml_package_v2.h"
/* Disk data is little endian and may be unaligned. Never cast it to a struct. */
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t crc(const uint8_t *p, size_t n) {
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (unsigned b = 0; b < 8; ++b) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return ~c;
}
bool mml_package_v2_validate(const uint8_t *d, size_t n) {
    if (!d || n < MML_PKG_HEADER_BYTES || n > UINT32_MAX) return false;
    const uint16_t count = u16(d + 6);
    if (u32(d) != MML_PKG_MAGIC || u16(d + 4) != MML_PKG_VERSION ||
        u32(d + 8) != n || count > MML_PKG_MAX_SECTIONS) return false;
    const size_t table_end = MML_PKG_HEADER_BYTES + (size_t)count * MML_PKG_SECTION_BYTES;
    if (table_end > n || crc(d + MML_PKG_HEADER_BYTES, table_end - MML_PKG_HEADER_BYTES) != u32(d + 16)) return false;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *s = d + MML_PKG_HEADER_BYTES + i * MML_PKG_SECTION_BYTES;
        const uint32_t off = u32(s + 4), size = u32(s + 8);
        if (off < table_end || off > n || size > n - off || crc(d + off, size) != u32(s + 12)) return false;
        for (unsigned j = 0; j < i; ++j) {
            const uint8_t *t = d + MML_PKG_HEADER_BYTES + j * MML_PKG_SECTION_BYTES;
            const uint32_t other = u32(t + 4), bytes = u32(t + 8);
            if (u32(s) == u32(t) || (size && bytes && off < (uint64_t)other + bytes && other < (uint64_t)off + size)) return false;
        }
    }
    return true;
}
bool mml_package_v2_section(const uint8_t *d, size_t n, uint32_t kind, const uint8_t **payload, size_t *size) {
    if (!payload || !size) return false;
    *payload = NULL; *size = 0;
    if (!mml_package_v2_validate(d, n)) return false;
    for (unsigned i = 0; i < u16(d + 6); ++i) {
        const uint8_t *s = d + MML_PKG_HEADER_BYTES + i * MML_PKG_SECTION_BYTES;
        if (u32(s) == kind) { *payload = d + u32(s + 4); *size = u32(s + 8); return true; }
    }
    return false;
}
