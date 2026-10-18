#include "legends3ds/st00_spawn.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeader = 40, kArea = 8, kRegion = 16, kRecord = 24;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void put16(uint8_t* p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
void put32(uint8_t* p, uint32_t v) { for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); }
size_t offset(uint32_t address, uint32_t base, size_t bytes, size_t need) {
    require(address >= base && uint64_t(address) - base + need <= bytes,
            "ST00 spawn pointer outside overlay");
    return address - base;
}
StageObjectRecord object(const uint8_t* p) {
    StageObjectRecord r;
    std::copy_n(p, kStageObjectRecordBytes, r.raw.begin());
    r.id = p[0]; r.object_class = p[1];
    for (size_t i = 0; i < 5; ++i) r.words_02_to_0a[i] = rd_u16(p + 2 + 2 * i);
    r.x = rd_s16(p + 12); r.y = rd_s16(p + 14); r.z = rd_s16(p + 16);
    r.tail_12 = rd_u16(p + 18);
    return r;
}
int32_t tile(int16_t value) {
    const int32_t v = value;
    return v >= 0 ? v / 512 : -((-v + 511) / 512);
}
}
St00SpawnCatalog parse_st00_spawn_us(const std::vector<uint8_t>& overlay, uint32_t base) {
    require(base == 0x80100000u, "ST00 spawn requires USA overlay base");
    const uint32_t table = rd_u32(overlay.data() + offset(0x8011C898u, base, overlay.size(), 4));
    require(table == 0x8011A46Cu, "ST00 spawn unsupported table profile");
    const size_t table_offset = offset(table, base, overlay.size(), kSt00SpawnAreaCount * 4);
    St00SpawnCatalog catalog;
    uint32_t region_total = 0, record_total = 0;
    for (size_t area = 0; area < kSt00SpawnAreaCount; ++area) {
        const uint32_t pointer = rd_u32(overlay.data() + table_offset + area * 4);
        if (!pointer) continue;
        size_t cursor = offset(pointer, base, overlay.size(), 12);
        while (true) {
            require(cursor <= overlay.size() && overlay.size() - cursor >= 12,
                    "ST00 spawn truncated descriptor sentinel");
            const auto* p = overlay.data() + cursor;
            if (rd_s16(p) == -1) break;
            require(++region_total <= kSt00SpawnMaxRegions, "ST00 spawn region limit");
            St00SpawnRegion r;
            r.id = rd_s16(p); r.qualifier = rd_s16(p + 2);
            require(r.id >= 0 && r.id <= 127 && r.qualifier >= -128 && r.qualifier <= 127,
                    "ST00 spawn unrepresentable work selector");
            r.min_x = static_cast<int8_t>(p[4]); r.min_z = static_cast<int8_t>(p[5]);
            r.width = p[6]; r.depth = p[7];
            size_t record_cursor = offset(rd_u32(p + 8), base, overlay.size(), 1);
            while (true) {
                require(record_cursor < overlay.size(), "ST00 spawn missing object sentinel");
                if (overlay[record_cursor] & 0x80u) break;
                require(overlay.size() - record_cursor >= kRecord, "ST00 spawn truncated conditional object");
                require(++record_total <= kSt00SpawnMaxRecords, "ST00 spawn object limit");
                const auto* q = overlay.data() + record_cursor;
                const uint32_t predicate = rd_u32(q + 20);
                require(predicate < 2048, "ST00 spawn predicate outside story flag bank");
                r.records.push_back({object(q), predicate});
                record_cursor += kRecord;
            }
            catalog.areas[area].regions.push_back(std::move(r));
            cursor += 12;
        }
    }
    return catalog;
}
std::vector<uint8_t> cook_l3st00_spawn(const St00SpawnCatalog& catalog) {
    uint64_t regions = 0, records = 0;
    for (const auto& a : catalog.areas) for (const auto& r : a.regions) {
        ++regions; records += r.records.size();
    }
    require(regions <= kSt00SpawnMaxRegions && records <= kSt00SpawnMaxRecords, "L3SP catalog limit");
    const size_t off_areas = kHeader, off_regions = off_areas + kSt00SpawnAreaCount * kArea;
    const size_t off_records = off_regions + regions * kRegion, size = off_records + records * kRecord;
    std::vector<uint8_t> bytes(size);
    std::memcpy(bytes.data(), "L3SP", 4);
    put16(bytes.data() + 4, 1); put16(bytes.data() + 6, kHeader);
    const uint32_t fields[] = {kSt00SpawnAreaCount, static_cast<uint32_t>(regions),
        static_cast<uint32_t>(records), static_cast<uint32_t>(off_areas),
        static_cast<uint32_t>(off_regions), static_cast<uint32_t>(off_records), static_cast<uint32_t>(size)};
    for (size_t i = 0; i < 7; ++i) put32(bytes.data() + 8 + 4 * i, fields[i]);
    uint32_t ri = 0, oi = 0;
    for (size_t area = 0; area < catalog.areas.size(); ++area) {
        const auto& a = catalog.areas[area]; auto* ap = bytes.data() + off_areas + area * kArea;
        put32(ap, ri); put32(ap + 4, static_cast<uint32_t>(a.regions.size()));
        for (const auto& r : a.regions) {
            auto* rp = bytes.data() + off_regions + ri++ * kRegion;
            put16(rp, static_cast<uint16_t>(r.id)); put16(rp + 2, static_cast<uint16_t>(r.qualifier));
            rp[4] = static_cast<uint8_t>(r.min_x); rp[5] = static_cast<uint8_t>(r.min_z);
            rp[6] = r.width; rp[7] = r.depth;
            put32(rp + 8, oi); put32(rp + 12, static_cast<uint32_t>(r.records.size()));
            for (const auto& o : r.records) {
                auto* op = bytes.data() + off_records + oi++ * kRecord;
                std::copy(o.object.raw.begin(), o.object.raw.end(), op); put32(op + 20, o.predicate);
            }
        }
    }
    put32(bytes.data() + 36, crc32(bytes.data() + kHeader, bytes.size() - kHeader));
    (void)inspect_l3st00_spawn(bytes, true);
    return bytes;
}
L3St00SpawnHeader inspect_l3st00_spawn(const std::vector<uint8_t>& bytes, bool verify_crc) {
    require(bytes.size() >= kHeader && std::memcmp(bytes.data(), "L3SP", 4) == 0,
            "L3SP missing header");
    require(rd_u16(bytes.data() + 4) == 1 && rd_u16(bytes.data() + 6) == kHeader,
            "L3SP unsupported version/header");
    L3St00SpawnHeader h{rd_u32(bytes.data()+8), rd_u32(bytes.data()+12), rd_u32(bytes.data()+16),
        rd_u32(bytes.data()+20), rd_u32(bytes.data()+24), rd_u32(bytes.data()+28),
        rd_u32(bytes.data()+32), rd_u32(bytes.data()+36)};
    require(h.area_count == kSt00SpawnAreaCount && h.region_count <= kSt00SpawnMaxRegions &&
            h.record_count <= kSt00SpawnMaxRecords && h.off_areas == kHeader &&
            uint64_t(h.off_areas) + h.area_count * kArea == h.off_regions &&
            uint64_t(h.off_regions) + h.region_count * kRegion == h.off_records &&
            uint64_t(h.off_records) + h.record_count * kRecord == h.file_bytes && h.file_bytes == bytes.size(),
            "L3SP noncanonical layout/count");
    if (verify_crc) require(crc32(bytes.data() + kHeader, bytes.size() - kHeader) == h.payload_crc32,
                            "L3SP CRC mismatch");
    uint64_t cursor = 0;
    for (uint32_t a = 0; a < h.area_count; ++a) {
        const auto* p = bytes.data() + h.off_areas + a * kArea;
        require(rd_u32(p) == cursor, "L3SP overlapping/gapped area ranges");
        cursor += rd_u32(p + 4); require(cursor <= h.region_count, "L3SP area overflow");
    }
    require(cursor == h.region_count, "L3SP incomplete area coverage");
    cursor = 0;
    for (uint32_t r = 0; r < h.region_count; ++r) {
        const auto* p = bytes.data() + h.off_regions + r * kRegion;
        require(rd_s16(p) >= 0 && rd_s16(p) <= 127 && rd_s16(p + 2) >= -128 && rd_s16(p + 2) <= 127,
                "L3SP work selector domain");
        require(rd_u32(p + 8) == cursor, "L3SP overlapping/gapped object ranges");
        cursor += rd_u32(p + 12); require(cursor <= h.record_count, "L3SP object overflow");
    }
    require(cursor == h.record_count, "L3SP incomplete object coverage");
    for (uint32_t o = 0; o < h.record_count; ++o) {
        const auto* p = bytes.data() + h.off_records + o * kRecord;
        require(!(p[0] & 0x80u) && rd_u32(p + 20) < 2048, "L3SP invalid conditional object/predicate");
    }
    return h;
}
St00SpawnCatalog load_l3st00_spawn(const std::vector<uint8_t>& bytes, bool verify_crc) {
    const auto h = inspect_l3st00_spawn(bytes, verify_crc);
    St00SpawnCatalog out;
    for (size_t area = 0; area < out.areas.size(); ++area) {
        const auto* ap = bytes.data() + h.off_areas + area * kArea;
        const uint32_t first = rd_u32(ap), count = rd_u32(ap + 4);
        auto& regions = out.areas[area].regions; regions.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            const auto* p = bytes.data() + h.off_regions + (first + i) * kRegion;
            St00SpawnRegion r; r.id = rd_s16(p); r.qualifier = rd_s16(p + 2);
            r.min_x = static_cast<int8_t>(p[4]); r.min_z = static_cast<int8_t>(p[5]);
            r.width = p[6]; r.depth = p[7];
            const uint32_t ofirst = rd_u32(p + 8), ocount = rd_u32(p + 12);
            r.records.reserve(ocount);
            for (uint32_t j = 0; j < ocount; ++j) {
                const auto* q = bytes.data() + h.off_records + (ofirst + j) * kRecord;
                r.records.push_back({object(q), rd_u32(q + 20)});
            }
            regions.push_back(std::move(r));
        }
    }
    return out;
}
bool st00_spawn_region_contains(const St00SpawnRegion& r, int16_t x, int16_t z) {
    return static_cast<uint32_t>(tile(x) - r.min_x) < r.width &&
           static_cast<uint32_t>(tile(z) - r.min_z) < r.depth;
}
std::optional<size_t> st00_spawn_selection(const St00SpawnArea& a, int8_t current, int16_t x, int16_t z) {
    std::optional<size_t> selected;
    for (size_t i = 0; i < a.regions.size(); ++i)
        if (a.regions[i].qualifier != current && st00_spawn_region_contains(a.regions[i], x, z)) selected = i;
    return selected;
}
} // namespace l3
