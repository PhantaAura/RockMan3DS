#include "legends3ds/map_initialization.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
uint32_t decoded_bytes(size_t count) {
    return static_cast<uint32_t>(sizeof(MapInitialization) + count * sizeof(MapInitializationEntry));
}
void counts(MapInitializationProfile profile, size_t idx, size_t descriptors,
            size_t records, size_t entries) {
    if (profile != MapInitializationProfile::Stage0Startup)
        throw std::runtime_error("L3MI unsupported startup profile");
    if (idx > kMapInitializationMaxIdentities || entries > idx ||
        descriptors > 65535u || records > 65535u)
        throw std::runtime_error("L3MI identity/count bound exceeded");
}
void entry_shape(const MapInitializationEntry& e, size_t idx, size_t descriptors,
                 size_t records, uint16_t prior, bool first) {
    if (e.idx_index >= idx || e.descriptor_index >= descriptors ||
        e.render_record_index >= records || (!first && e.idx_index <= prior))
        throw std::runtime_error("L3MI entry index/order invalid");
}
MapInitializationEntry entry_at(const std::vector<uint8_t>& b, size_t p) {
    return {rd_u16(&b[p]), rd_u16(&b[p + 2]), rd_u16(&b[p + 4]),
            rd_u16(&b[p + 6]), rd_u16(&b[p + 8])};
}
void metadata_shape(const MapInitialization& m) {
    counts(m.profile, m.idx_count, m.descriptor_count, m.render_record_count, m.entries.size());
    uint16_t prior = 0;
    bool first = true;
    for (const auto& e : m.entries) {
        entry_shape(e, m.idx_count, m.descriptor_count, m.render_record_count, prior, first);
        prior = e.idx_index;
        first = false;
    }
}
uint32_t render_offset(uint32_t pointer) {
    if (!pointer) return UINT32_MAX;
    if (pointer < kSt00StgBase) throw std::runtime_error("L3MI source render pointer below STG");
    return pointer - kSt00StgBase;
}
uint16_t source_descriptor(const St00Source& source, uint32_t pointer) {
    for (size_t i = 0; i < source.descriptors.size(); ++i)
        if (source.descriptors[i].source_ptr == pointer) return static_cast<uint16_t>(i);
    throw std::runtime_error("L3MI source descriptor identity missing");
}
uint16_t source_contact(const St00Source& source, uint32_t pointer) {
    if (!pointer) return 0xFFFFu;
    for (size_t i = 0; i < source.contact_lists.size(); ++i)
        if (source.contact_lists[i].source_ptr == pointer) return static_cast<uint16_t>(i);
    throw std::runtime_error("L3MI source contact identity missing");
}
void source_bindings(const St00Source& source, const std::vector<uint8_t>& stg,
                     const RuntimeMap& map, const St00RenderSource& render) {
    if (source.idx.size() != map.idx.size() || source.descriptors.size() != map.descriptors.size() ||
        source.descriptors.size() != render.descriptors.size() || source.contact_lists.size() > 65535u)
        throw std::runtime_error("L3MI source/cooked identity counts differ");
    for (size_t i = 0; i < source.idx.size(); ++i) {
        const auto& a = source.idx[i]; const auto& b = map.idx[i];
        const uint16_t descriptor = a.descriptor_ptr ? source_descriptor(source, a.descriptor_ptr) : 0xFFFFu;
        if (a.x != b.x || a.y != b.y || a.z != b.z || a.raw6 != b.raw6 || descriptor != b.descriptor_index)
            throw std::runtime_error("L3MI source/cooked IDX binding differs");
    }
    for (size_t i = 0; i < source.descriptors.size(); ++i) {
        const auto& a = source.descriptors[i]; const auto& b = map.descriptors[i];
        if (a.word0 != b.word0 || a.raw_words3_5 != b.raw_words3_5 ||
            source_contact(source, a.alt_contact_ptr) != b.alt_contact_index ||
            source_contact(source, a.default_contact_ptr) != b.default_contact_index)
            throw std::runtime_error("L3MI source/cooked descriptor binding differs");
        for (size_t side = 0; side < 6; ++side)
            if (render_offset(a.render_ptrs[side]) != b.render_offsets[side])
                throw std::runtime_error("L3MI source/cooked render offset differs");
    }
    // L3RG does not retain PS1 pointers. Rebuild its neutral identities from the
    // supplied owner STG and compare the complete cooked representation in memory.
    if (cook_l3render(parse_st00_render(source, stg)) != cook_l3render(render))
        throw std::runtime_error("L3MI source/cooked render identity differs");
}
}

MapInitializationHeader inspect_map_initialization(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < kMapInitializationHeaderBytes || b.size() > kMapInitializationMaxFileBytes ||
        b[0] != 'L' || b[1] != '3' || b[2] != 'M' || b[3] != 'I' ||
        rd_u16(&b[4]) != kMapInitializationVersion || rd_u16(&b[6]) != kMapInitializationHeaderBytes)
        throw std::runtime_error("not supported L3MI");
    MapInitializationHeader h;
    h.profile = static_cast<MapInitializationProfile>(rd_u32(&b[8]));
    h.idx_count = rd_u32(&b[12]); h.descriptor_count = rd_u32(&b[16]);
    h.render_record_count = rd_u32(&b[20]); h.entry_count = rd_u32(&b[24]);
    h.map_crc32 = rd_u32(&b[28]); h.render_crc32 = rd_u32(&b[32]);
    counts(h.profile, h.idx_count, h.descriptor_count, h.render_record_count, h.entry_count);
    if (rd_u32(&b[36]) != kMapInitializationHeaderBytes || rd_u32(&b[40]) != b.size() ||
        uint64_t(kMapInitializationHeaderBytes) + uint64_t(h.entry_count) * kMapInitializationEntryBytes != b.size())
        throw std::runtime_error("L3MI section size/range invalid");
    if (verify_crc && crc32(b.data() + kMapInitializationHeaderBytes, b.size() - kMapInitializationHeaderBytes) != rd_u32(&b[44]))
        throw std::runtime_error("L3MI payload CRC mismatch");
    uint16_t prior = 0;
    for (uint32_t i = 0; i < h.entry_count; ++i) {
        const size_t p = kMapInitializationHeaderBytes + size_t(i) * kMapInitializationEntryBytes;
        const auto e = entry_at(b, p);
        entry_shape(e, h.idx_count, h.descriptor_count, h.render_record_count, prior, i == 0);
        if (rd_u16(&b[p + 10]) || rd_u32(&b[p + 12]))
            throw std::runtime_error("L3MI entry reserved fields nonzero");
        prior = e.idx_index;
    }
    h.decoded_bytes = decoded_bytes(h.entry_count);
    return h;
}

MapInitialization load_map_initialization(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_map_initialization(b, verify_crc);
    MapInitialization m;
    m.profile = h.profile; m.idx_count = h.idx_count; m.descriptor_count = h.descriptor_count;
    m.render_record_count = h.render_record_count; m.map_crc32 = h.map_crc32;
    m.render_crc32 = h.render_crc32; m.decoded_bytes = h.decoded_bytes;
    m.entries.reserve(h.entry_count);
    for (uint32_t i = 0; i < h.entry_count; ++i)
        m.entries.push_back(entry_at(b, kMapInitializationHeaderBytes + size_t(i) * kMapInitializationEntryBytes));
    return m;
}

std::vector<uint8_t> cook_map_initialization(const MapInitialization& m) {
    metadata_shape(m);
    std::vector<uint8_t> b{'L', '3', 'M', 'I'};
    b.reserve(kMapInitializationHeaderBytes + m.entries.size() * kMapInitializationEntryBytes);
    wr_u16(b, kMapInitializationVersion); wr_u16(b, kMapInitializationHeaderBytes);
    wr_u32(b, static_cast<uint32_t>(m.profile)); wr_u32(b, m.idx_count);
    wr_u32(b, m.descriptor_count); wr_u32(b, m.render_record_count);
    wr_u32(b, static_cast<uint32_t>(m.entries.size())); wr_u32(b, m.map_crc32); wr_u32(b, m.render_crc32);
    wr_u32(b, kMapInitializationHeaderBytes); wr_u32(b, 0); wr_u32(b, 0);
    for (const auto& e : m.entries) {
        wr_u16(b, e.idx_index); wr_u16(b, e.descriptor_index); wr_u16(b, e.render_record_index);
        wr_u16(b, e.expected_raw6); wr_u16(b, e.seed); wr_u16(b, 0); wr_u32(b, 0);
    }
    patch_u32(b, 40, static_cast<uint32_t>(b.size()));
    patch_u32(b, 44, crc32(b.data() + kMapInitializationHeaderBytes, b.size() - kMapInitializationHeaderBytes));
    return b;
}

MapInitialization derive_map_initialization(const St00Source& source, const std::vector<uint8_t>& hed,
                                            const std::vector<uint8_t>& stg, const RuntimeMap& map,
                                            const St00RenderSource& render, uint32_t map_crc,
                                            uint32_t render_crc, MapInitializationProfile profile) {
    counts(profile, source.idx.size(), source.descriptors.size(), render.records.size(), 0);
    source_bindings(source, stg, map, render);
    if (hed.empty() || hed.size() % 8u) throw std::runtime_error("L3MI source HED row shape invalid");
    size_t searched = 0;
    for (; searched < 512; ++searched) {
        if ((searched + 1u) * 8u > hed.size())
            throw std::runtime_error("L3MI HED truncated before terminator/search cap");
        if (!rd_u32(&hed[searched * 8u + 4u])) break;
    }
    MapInitialization m;
    m.profile = profile; m.idx_count = static_cast<uint32_t>(source.idx.size());
    m.descriptor_count = static_cast<uint32_t>(source.descriptors.size());
    m.render_record_count = static_cast<uint32_t>(render.records.size());
    m.map_crc32 = map_crc; m.render_crc32 = render_crc;
    for (size_t i = 0; i < source.idx.size(); ++i) {
        const auto& idx = source.idx[i];
        if (!idx.descriptor_ptr) continue;
        bool admitted = false;
        for (size_t slot = 0; slot < searched; ++slot)
            if ((rd_u32(&hed[slot * 8u + 4u]) & 0x00FFFFFFu) == (idx.descriptor_ptr & 0x00FFFFFFu)) {
                admitted = true; break;
            }
        if (!admitted) continue;
        const uint16_t di = map.idx[i].descriptor_index;
        const uint32_t pointer = source.descriptors[di].render_ptrs[0];
        if (pointer < kSt00StgBase || uint64_t(pointer - kSt00StgBase) + 4u > stg.size())
            throw std::runtime_error("L3MI admitted primary header outside supplied STG");
        const uint16_t record = render.descriptors[di].record_indices[0];
        const uint16_t seed = rd_u16(&stg[size_t(pointer - kSt00StgBase) + 2u]);
        m.entries.push_back({static_cast<uint16_t>(i), di, record, static_cast<uint16_t>(idx.raw6), seed});
    }
    m.decoded_bytes = decoded_bytes(m.entries.size());
    (void)validate_map_initialization(map, render, m, map_crc, render_crc);
    return m;
}

MapInitializationDiagnostics validate_map_initialization(const RuntimeMap& map, const St00RenderSource& render,
                                                         const MapInitialization& m, uint32_t map_crc,
                                                         uint32_t render_crc) {
    metadata_shape(m);
    if (m.map_crc32 != map_crc || m.render_crc32 != render_crc ||
        m.idx_count != map.idx.size() || m.descriptor_count != map.descriptors.size() ||
        m.descriptor_count != render.descriptors.size() || m.render_record_count != render.records.size())
        throw std::runtime_error("L3MI cooked package binding mismatch");
    for (const auto& e : m.entries) {
        const auto& idx = map.idx[e.idx_index];
        if (idx.descriptor_index != e.descriptor_index || static_cast<uint16_t>(idx.raw6) != e.expected_raw6 ||
            render.descriptors[e.descriptor_index].record_indices[0] != e.render_record_index)
            throw std::runtime_error("L3MI expected IDX/render record mismatch");
        const auto& record = render.records[e.render_record_index];
        if ((uint16_t(record.byte2) | (uint16_t(record.byte3) << 8u)) != e.seed)
            throw std::runtime_error("L3MI render header seed mismatch");
    }
    return {static_cast<uint32_t>(m.entries.size()), decoded_bytes(m.entries.size())};
}

MapInitializationDiagnostics apply_map_initialization(RuntimeMap& map, const St00RenderSource& render,
                                                      const MapInitialization& m, uint32_t map_crc,
                                                      uint32_t render_crc) {
    const auto diagnostics = validate_map_initialization(map, render, m, map_crc, render_crc);
    for (const auto& e : m.entries) map.idx[e.idx_index].raw6 = static_cast<int16_t>(e.seed);
    return diagnostics;
}
}
