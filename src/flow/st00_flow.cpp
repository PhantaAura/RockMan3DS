#include "legends3ds/st00_flow.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
constexpr uint32_t kExpectedLoadBase = 0x80100000u;
constexpr uint32_t kObjectPtrTableAddr = 0x8011AB3Cu;
constexpr uint32_t kAuxPtrTableAddr = 0x8011AC40u;
constexpr uint32_t kAreaSetupBlobAddr = 0x8011AC54u;
constexpr uint32_t kSetupSelectorTableAddr = 0x8011ADE4u;
constexpr uint32_t kSetupSeedTableAddr = 0x8011AD1Cu;
constexpr uint32_t kResourcePtrTableAddr = 0x8011A6F0u;
constexpr size_t kL3FlowHeaderBytes = 40;
constexpr size_t kL3FlowAreaBytes = 48;

size_t addr_to_offset(uint32_t addr, uint32_t base, size_t bytes, size_t need = 1) {
    if (addr < base) throw std::runtime_error("ST00 flow pointer is below overlay load base");
    const uint64_t off = uint64_t(addr) - uint64_t(base);
    if (off + need > bytes) throw std::runtime_error("ST00 flow pointer is outside overlay payload");
    return static_cast<size_t>(off);
}

uint32_t pointer_to_offset(uint32_t ptr, uint32_t base, size_t bytes, size_t need = 1) {
    const size_t off = addr_to_offset(ptr, base, bytes, need);
    if (off > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("ST00 flow relative pointer overflows u32");
    return static_cast<uint32_t>(off);
}

uint32_t read_table_u32(const std::vector<uint8_t>& d, uint32_t table_addr,
                        size_t index, uint32_t base) {
    const size_t off = addr_to_offset(table_addr, base, d.size(), (index + 1u) * 4u);
    return rd_u32(d.data() + off + index * 4u);
}
}

St00AuxList parse_st00_aux_list(const uint8_t* data, size_t bytes) {
    if (!data || bytes < kSt00AuxRecordBytes) throw std::runtime_error("empty/truncated ST00 aux list");
    St00AuxList out;
    size_t p = 0;
    while (bytes - p >= kSt00AuxRecordBytes) {
        const int16_t first = static_cast<int16_t>(rd_u16(data + p));
        if (first == -1) {
            out.bytes_consumed = p + kSt00AuxRecordBytes;
            return out;
        }
        St00AuxRecord r;
        std::copy_n(data + p, kSt00AuxRecordBytes, r.raw.begin());
        for (size_t i = 0; i < r.halfwords.size(); ++i) {
            r.halfwords[i] = static_cast<int16_t>(rd_u16(data + p + i * 2u));
        }
        out.records.push_back(r);
        p += kSt00AuxRecordBytes;
    }
    throw std::runtime_error("ST00 aux list has no full 24-byte -1 sentinel slot");
}

St00AuxList parse_st00_aux_list(const std::vector<uint8_t>& data, size_t offset) {
    if (offset >= data.size()) throw std::runtime_error("ST00 aux list offset out of range");
    return parse_st00_aux_list(data.data() + offset, data.size() - offset);
}

St00FlowSource parse_st00_flow_us(const std::vector<uint8_t>& d, uint32_t load_base) {
    if (load_base != kExpectedLoadBase) {
        throw std::runtime_error("USA ST00 flow parser currently requires overlay load base 0x80100000");
    }
    // Touch/validate the complete fixed tables before following any pointer from them.
    (void)addr_to_offset(kObjectPtrTableAddr, load_base, d.size(), kSt00TutorialAreaCount * 4u);
    (void)addr_to_offset(kAuxPtrTableAddr, load_base, d.size(), kSt00TutorialAreaCount * 4u);
    (void)addr_to_offset(kResourcePtrTableAddr, load_base, d.size(), kSt00TutorialAreaCount * 4u);
    (void)addr_to_offset(kAreaSetupBlobAddr, load_base, d.size(), kSt00TutorialAreaCount * kSt00AreaSetupBlobBytes);
    (void)addr_to_offset(kSetupSelectorTableAddr, load_base, d.size(), kSt00TutorialAreaCount * 4u);

    St00FlowSource out;
    out.overlay_load_base = load_base;
    for (size_t area = 0; area < kSt00TutorialAreaCount; ++area) {
        auto& a = out.areas[area];
        const uint32_t object_ptr = read_table_u32(d, kObjectPtrTableAddr, area, load_base);
        const uint32_t aux_ptr = read_table_u32(d, kAuxPtrTableAddr, area, load_base);
        const uint32_t resource_ptr = read_table_u32(d, kResourcePtrTableAddr, area, load_base);
        a.source_object_offset = pointer_to_offset(object_ptr, load_base, d.size());
        a.source_aux_offset = pointer_to_offset(aux_ptr, load_base, d.size());
        a.source_resource_offset = pointer_to_offset(resource_ptr, load_base, d.size());
        a.objects = parse_stage_object_list(d, a.source_object_offset);
        a.aux = parse_st00_aux_list(d, a.source_aux_offset);

        const size_t blob_off = addr_to_offset(kAreaSetupBlobAddr, load_base, d.size(),
                                               (area + 1u) * kSt00AreaSetupBlobBytes) +
                                area * kSt00AreaSetupBlobBytes;
        // addr_to_offset above returns the table beginning; use a separately computed fixed offset
        // to keep all bounds validation explicit and avoid pointer arithmetic on PS1 addresses.
        const size_t blob_base = addr_to_offset(kAreaSetupBlobAddr, load_base, d.size(),
                                                kSt00TutorialAreaCount * kSt00AreaSetupBlobBytes);
        std::copy_n(d.data() + blob_base + area * kSt00AreaSetupBlobBytes,
                    kSt00AreaSetupBlobBytes, a.setup_blob.begin());
        (void)blob_off; // validated by the same fixed-table check; retained to make the bound proof obvious.

        a.setup_selector = read_table_u32(d, kSetupSelectorTableAddr, area, load_base);
        if (a.setup_selector > (std::numeric_limits<uint32_t>::max() / kSt00AreaSetupSeedBytes)) {
            throw std::runtime_error("ST00 setup selector overflows");
        }
        const uint64_t seed_addr64 = uint64_t(kSetupSeedTableAddr) +
                                     uint64_t(a.setup_selector) * kSt00AreaSetupSeedBytes;
        if (seed_addr64 > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("ST00 setup seed address overflows");
        const size_t seed_off = addr_to_offset(static_cast<uint32_t>(seed_addr64), load_base, d.size(),
                                               kSt00AreaSetupSeedBytes);
        std::copy_n(d.data() + seed_off, kSt00AreaSetupSeedBytes, a.setup_seed.begin());
    }
    return out;
}

std::vector<uint8_t> cook_l3st00_flow(const St00FlowSource& flow) {
    uint64_t total_objects = 0;
    uint64_t total_aux = 0;
    for (const auto& a : flow.areas) {
        total_objects += a.objects.records.size();
        total_aux += a.aux.records.size();
    }
    if (total_objects > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("too many ST00 flow objects");
    if (total_aux > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("too many ST00 flow aux records");

    std::vector<uint8_t> o;
    o.reserve(kL3FlowHeaderBytes + kSt00TutorialAreaCount * kL3FlowAreaBytes +
              size_t(total_objects) * kStageObjectRecordBytes +
              size_t(total_aux) * kSt00AuxRecordBytes +
              kSt00TutorialAreaCount * kSt00AreaSetupBlobBytes);
    o.insert(o.end(), {'L','3','S','F'});
    wr_u16(o, kL3St00FlowVersion);
    wr_u16(o, static_cast<uint16_t>(kL3FlowHeaderBytes));
    // bit0: five-area selector proven; bit1: 20-byte object-list dispatch proven;
    // bit2: common setup copies 40 bytes per area; bit3: source pointers were converted to offsets;
    // bit4: 24-byte per-area aux stream with signed-halfword -1 sentinel is preserved.
    wr_u32(o, 0x1Fu);
    wr_u32(o, static_cast<uint32_t>(kSt00TutorialAreaCount));
    wr_u32(o, static_cast<uint32_t>(total_objects));
    const size_t p_areas = o.size(); wr_u32(o, 0);
    const size_t p_objects = o.size(); wr_u32(o, 0);
    const size_t p_blobs = o.size(); wr_u32(o, 0);
    const size_t p_file = o.size(); wr_u32(o, 0);
    const size_t p_crc = o.size(); wr_u32(o, 0);
    if (o.size() != kL3FlowHeaderBytes) throw std::runtime_error("L3SF header size bug");

    const uint32_t off_areas = static_cast<uint32_t>(o.size());
    uint32_t object_first = 0;
    uint32_t aux_first = 0;
    for (size_t area = 0; area < flow.areas.size(); ++area) {
        const auto& a = flow.areas[area];
        wr_u32(o, static_cast<uint32_t>(area));
        wr_u32(o, object_first);
        wr_u32(o, static_cast<uint32_t>(a.objects.records.size()));
        wr_u32(o, a.source_object_offset);
        wr_u32(o, a.source_aux_offset);
        wr_u32(o, a.source_resource_offset);
        wr_u32(o, a.setup_selector);
        o.insert(o.end(), a.setup_seed.begin(), a.setup_seed.end());
        wr_u32(o, aux_first);
        wr_u32(o, static_cast<uint32_t>(a.aux.records.size()));
        wr_u32(o, 0u);
        if (o.size() != size_t(off_areas) + (area + 1u) * kL3FlowAreaBytes) {
            throw std::runtime_error("L3SF area record size bug");
        }
        object_first += static_cast<uint32_t>(a.objects.records.size());
        aux_first += static_cast<uint32_t>(a.aux.records.size());
    }

    const uint32_t off_objects = static_cast<uint32_t>(o.size());
    for (const auto& a : flow.areas) {
        for (const auto& r : a.objects.records) o.insert(o.end(), r.raw.begin(), r.raw.end());
    }
    for (const auto& a : flow.areas) {
        for (const auto& r : a.aux.records) o.insert(o.end(), r.raw.begin(), r.raw.end());
    }
    const uint32_t off_blobs = static_cast<uint32_t>(o.size());
    for (const auto& a : flow.areas) o.insert(o.end(), a.setup_blob.begin(), a.setup_blob.end());

    patch_u32(o, p_areas, off_areas);
    patch_u32(o, p_objects, off_objects);
    patch_u32(o, p_blobs, off_blobs);
    patch_u32(o, p_file, static_cast<uint32_t>(o.size()));
    patch_u32(o, p_crc, crc32(o.data() + kL3FlowHeaderBytes, o.size() - kL3FlowHeaderBytes));
    return o;
}

L3St00FlowHeader inspect_l3st00_flow(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < kL3FlowHeaderBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'S' || b[3] != 'F') {
        throw std::runtime_error("not L3SF");
    }
    L3St00FlowHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]); h.flags = rd_u32(&b[8]);
    h.area_count = rd_u32(&b[12]); h.object_count = rd_u32(&b[16]);
    h.off_areas = rd_u32(&b[20]); h.off_objects = rd_u32(&b[24]); h.off_setup_blobs = rd_u32(&b[28]);
    h.file_bytes = rd_u32(&b[32]); h.payload_crc32 = rd_u32(&b[36]);
    if (h.version != kL3St00FlowVersion || h.header_bytes != kL3FlowHeaderBytes) throw std::runtime_error("unsupported L3SF");
    if (h.area_count != kSt00TutorialAreaCount) throw std::runtime_error("L3SF area count mismatch");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3SF file size mismatch");
    if (h.off_areas < kL3FlowHeaderBytes ||
        uint64_t(h.off_areas) + uint64_t(h.area_count) * kL3FlowAreaBytes > b.size()) {
        throw std::runtime_error("L3SF area table range invalid");
    }
    if (h.off_objects < h.off_areas ||
        uint64_t(h.off_objects) + uint64_t(h.object_count) * kStageObjectRecordBytes > b.size()) {
        throw std::runtime_error("L3SF object table range invalid");
    }
    uint64_t aux_total = 0;
    uint64_t expected_aux_first = 0;
    for (uint32_t area = 0; area < h.area_count; ++area) {
        const size_t aoff = size_t(h.off_areas) + size_t(area) * kL3FlowAreaBytes;
        const uint32_t aux_first = rd_u32(&b[aoff + 36]);
        const uint32_t aux_count = rd_u32(&b[aoff + 40]);
        if (aux_first != expected_aux_first) throw std::runtime_error("L3SF aux area ranges are not contiguous");
        expected_aux_first += aux_count;
        aux_total += aux_count;
        if (aux_total > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("L3SF aux count overflows u32");
    }
    h.aux_count = static_cast<uint32_t>(aux_total);
    const uint64_t aux_begin = uint64_t(h.off_objects) + uint64_t(h.object_count) * kStageObjectRecordBytes;
    const uint64_t aux_end = aux_begin + aux_total * kSt00AuxRecordBytes;
    if (aux_end != h.off_setup_blobs) throw std::runtime_error("L3SF aux table range invalid");
    if (uint64_t(h.off_setup_blobs) + uint64_t(h.area_count) * kSt00AreaSetupBlobBytes > b.size()) {
        throw std::runtime_error("L3SF setup blob range invalid");
    }
    if (verify_crc && crc32(b.data() + kL3FlowHeaderBytes, b.size() - kL3FlowHeaderBytes) != h.payload_crc32) {
        throw std::runtime_error("L3SF CRC mismatch");
    }
    return h;
}


St00FlowSource load_l3st00_flow(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3st00_flow(b, verify_crc);
    St00FlowSource out;
    // L3SF v1 is the pointer-free representation of the fixed USA ST00 overlay.
    // Source addresses in each area row are offsets relative to this proven load base.
    out.overlay_load_base = kExpectedLoadBase;

    const uint64_t aux_begin64 = uint64_t(h.off_objects) +
                                 uint64_t(h.object_count) * kStageObjectRecordBytes;
    if (aux_begin64 > b.size()) throw std::runtime_error("L3SF aux table begins outside file");
    const size_t aux_begin = static_cast<size_t>(aux_begin64);

    uint32_t expected_object_first = 0;
    uint32_t expected_aux_first = 0;
    for (uint32_t area = 0; area < h.area_count; ++area) {
        const size_t aoff = size_t(h.off_areas) + size_t(area) * kL3FlowAreaBytes;
        if (rd_u32(&b[aoff + 0]) != area) throw std::runtime_error("L3SF area id/order mismatch");
        const uint32_t object_first = rd_u32(&b[aoff + 4]);
        const uint32_t object_count = rd_u32(&b[aoff + 8]);
        const uint32_t source_object_offset = rd_u32(&b[aoff + 12]);
        const uint32_t source_aux_offset = rd_u32(&b[aoff + 16]);
        const uint32_t source_resource_offset = rd_u32(&b[aoff + 20]);
        const uint32_t setup_selector = rd_u32(&b[aoff + 24]);
        const uint32_t aux_first = rd_u32(&b[aoff + 36]);
        const uint32_t aux_count = rd_u32(&b[aoff + 40]);
        if (rd_u32(&b[aoff + 44]) != 0u) throw std::runtime_error("L3SF area reserved field is nonzero");
        if (object_first != expected_object_first) throw std::runtime_error("L3SF object area ranges are not contiguous");
        if (aux_first != expected_aux_first) throw std::runtime_error("L3SF aux area ranges are not contiguous");
        if (uint64_t(object_first) + object_count > h.object_count) throw std::runtime_error("L3SF area object range invalid");
        if (uint64_t(aux_first) + aux_count > h.aux_count) throw std::runtime_error("L3SF area aux range invalid");

        auto& a = out.areas[area];
        a.source_object_offset = source_object_offset;
        a.source_aux_offset = source_aux_offset;
        a.source_resource_offset = source_resource_offset;
        a.setup_selector = setup_selector;
        std::copy_n(&b[aoff + 28], kSt00AreaSetupSeedBytes, a.setup_seed.begin());

        a.objects.records.reserve(object_count);
        for (uint32_t i = 0; i < object_count; ++i) {
            const size_t p = size_t(h.off_objects) + size_t(object_first + i) * kStageObjectRecordBytes;
            StageObjectRecord r;
            std::copy_n(&b[p], kStageObjectRecordBytes, r.raw.begin());
            r.id = b[p + 0];
            r.object_class = b[p + 1];
            for (size_t w = 0; w < r.words_02_to_0a.size(); ++w) {
                r.words_02_to_0a[w] = rd_u16(&b[p + 2 + w * 2]);
            }
            r.x = static_cast<int16_t>(rd_u16(&b[p + 0x0C]));
            r.y = static_cast<int16_t>(rd_u16(&b[p + 0x0E]));
            r.z = static_cast<int16_t>(rd_u16(&b[p + 0x10]));
            r.tail_12 = rd_u16(&b[p + 0x12]);
            a.objects.records.push_back(r);
        }
        // Cooked L3SF omits the source 0xFF terminator; report exactly the retained rows.
        a.objects.bytes_consumed = size_t(object_count) * kStageObjectRecordBytes;

        a.aux.records.reserve(aux_count);
        for (uint32_t i = 0; i < aux_count; ++i) {
            const size_t p = aux_begin + size_t(aux_first + i) * kSt00AuxRecordBytes;
            St00AuxRecord r;
            std::copy_n(&b[p], kSt00AuxRecordBytes, r.raw.begin());
            for (size_t w = 0; w < r.halfwords.size(); ++w) {
                r.halfwords[w] = static_cast<int16_t>(rd_u16(&b[p + w * 2]));
            }
            a.aux.records.push_back(r);
        }
        // Cooked L3SF likewise omits the source sentinel slot.
        a.aux.bytes_consumed = size_t(aux_count) * kSt00AuxRecordBytes;

        const size_t boff = size_t(h.off_setup_blobs) + size_t(area) * kSt00AreaSetupBlobBytes;
        std::copy_n(&b[boff], kSt00AreaSetupBlobBytes, a.setup_blob.begin());

        expected_object_first += object_count;
        expected_aux_first += aux_count;
    }
    if (expected_object_first != h.object_count || expected_aux_first != h.aux_count) {
        throw std::runtime_error("L3SF area coverage does not match header totals");
    }
    return out;
}

} // namespace l3
