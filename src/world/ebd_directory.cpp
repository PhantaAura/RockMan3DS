#include "legends3ds/ebd_directory.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
uint32_t pointer_to_offset_or_null(uint32_t ptr, uint32_t base, size_t bytes) {
    if (ptr == 0) return std::numeric_limits<uint32_t>::max();
    if (ptr < base) throw std::runtime_error("EBD directory pointer below load base");
    const uint64_t off = uint64_t(ptr) - uint64_t(base);
    if (off >= bytes || off > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("EBD directory pointer outside payload");
    }
    return static_cast<uint32_t>(off);
}
}

uint8_t normalize_ebd_object_class(uint8_t c) {
    uint8_t out;
    if (c < 0x20u) out = static_cast<uint8_t>(c & 0x10u);
    else out = static_cast<uint8_t>(c & 0xE0u);
    if (out == 0xA0u) out = 0x80u;
    return out;
}

EbdDirectory parse_ebd_directory(const uint8_t* data, size_t bytes, uint32_t load_base) {
    if (!data || bytes < 4) throw std::runtime_error("empty/truncated EBD payload");
    if (bytes > std::numeric_limits<uint32_t>::max() ||
        uint64_t(load_base) + bytes > (uint64_t(1) << 32u))
        throw std::runtime_error("EBD source address range overflow");
    const uint32_t count = rd_u32(data);
    const uint64_t directory_end = 4ull + uint64_t(count) * kEbdDirectoryEntryBytes;
    if (directory_end > bytes) throw std::runtime_error("EBD directory exceeds payload");

    EbdDirectory out;
    out.load_base = load_base;
    out.entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const size_t p = 4u + size_t(i) * kEbdDirectoryEntryBytes;
        EbdDirectoryEntry e;
        e.object_class = data[p + 0];
        e.subtype = data[p + 1];
        e.variant = data[p + 2];
        e.key_flags = data[p + 3];
        e.resource_offset[0] = pointer_to_offset_or_null(rd_u32(data + p + 4), load_base, bytes);
        e.resource_offset[1] = pointer_to_offset_or_null(rd_u32(data + p + 8), load_base, bytes);
        e.resource_offset[2] = pointer_to_offset_or_null(rd_u32(data + p + 12), load_base, bytes);
        out.entries.push_back(e);
    }
    out.source_payload.assign(data, data + bytes);
    return out;
}

EbdResourceSpan ebd_resource_span(const EbdDirectory& dir, size_t entry_index,
                                  size_t resource_slot, size_t requested_bytes) {
    if (entry_index >= dir.entries.size() || resource_slot >= 3)
        throw std::runtime_error("EBD resource row/slot out of range");
    EbdResourceSpan out;
    out.source_offset = dir.entries[entry_index].resource_offset[resource_slot];
    if (out.source_offset == std::numeric_limits<uint32_t>::max()) {
        out.status = EbdResourceSpanStatus::NULL_POINTER;
        return out;
    }
    if (dir.source_payload.empty()) return out;
    const size_t off = out.source_offset;
    if (off >= dir.source_payload.size() || requested_bytes > dir.source_payload.size() - off)
        throw std::runtime_error("EBD resource window outside source payload");
    out.status = EbdResourceSpanStatus::AVAILABLE;
    out.data = dir.source_payload.data() + off;
    out.size = requested_bytes;
    return out;
}

EbdDirectory parse_ebd_directory(const std::vector<uint8_t>& data, uint32_t load_base) {
    return parse_ebd_directory(data.data(), data.size(), load_base);
}

std::vector<size_t> find_ebd_entries(const EbdDirectory& dir, uint8_t c, uint8_t subtype) {
    const uint8_t normalized = normalize_ebd_object_class(c);
    std::vector<size_t> out;
    for (size_t i = 0; i < dir.entries.size(); ++i) {
        const auto& e = dir.entries[i];
        if (e.object_class == normalized && e.subtype == subtype) out.push_back(i);
    }
    return out;
}

std::vector<size_t> find_ebd_entries(const EbdDirectory& dir, uint8_t c,
                                     uint8_t subtype, uint8_t variant) {
    const uint8_t normalized = normalize_ebd_object_class(c);
    std::vector<size_t> out;
    for (size_t i = 0; i < dir.entries.size(); ++i) {
        const auto& e = dir.entries[i];
        if (e.object_class == normalized && e.subtype == subtype && e.variant == variant) out.push_back(i);
    }
    return out;
}

std::vector<size_t> stage_object_ebd_candidates(const StageObjectRecord& object,
                                                const EbdDirectory& dir) {
    return find_ebd_entries(dir, object.object_class, object.raw[2]);
}

std::vector<size_t> stage_object_ebd_candidates_record_variant(const StageObjectRecord& object,
                                                               const EbdDirectory& dir) {
    return find_ebd_entries(dir, object.object_class, object.raw[2], object.raw[3]);
}

} // namespace l3

namespace l3 {
namespace {
constexpr size_t kL3EbdV1HeaderBytes = 32;
constexpr size_t kL3EbdV2HeaderBytes = 40;
constexpr size_t kL3EbdEntryBytes = 16;
}

std::vector<uint8_t> cook_l3ebd_directory(const EbdDirectory& dir) {
    if (dir.entries.size() > std::numeric_limits<uint32_t>::max() ||
        dir.source_payload.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("L3EB count/payload exceeds u32 format range");
    const bool payload = !dir.source_payload.empty();
    const size_t header_bytes = payload ? kL3EbdV2HeaderBytes : kL3EbdV1HeaderBytes;
    const uint64_t entry_end = header_bytes + uint64_t(dir.entries.size()) * kL3EbdEntryBytes;
    const uint64_t total = entry_end + dir.source_payload.size();
    if (total > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("L3EB size exceeds u32 format range");
    std::vector<uint8_t> out;
    out.reserve(static_cast<size_t>(total));
    out.push_back('L'); out.push_back('3'); out.push_back('E'); out.push_back('B');
    wr_u16(out, payload ? kL3EbdDirectoryVersion : 1u);
    wr_u16(out, static_cast<uint16_t>(header_bytes));
    // bit0 offsets; bit1 null=UINT32_MAX; v2 bit2 complete source bytes.
    wr_u32(out, payload ? 0x7u : 0x3u);
    wr_u32(out, static_cast<uint32_t>(dir.entries.size()));
    wr_u32(out, dir.load_base);
    wr_u32(out, static_cast<uint32_t>(header_bytes));
    wr_u32(out, static_cast<uint32_t>(total));
    wr_u32(out, 0);
    if (payload) {
        wr_u32(out, static_cast<uint32_t>(entry_end));
        wr_u32(out, static_cast<uint32_t>(dir.source_payload.size()));
    }
    for (const auto& e : dir.entries) {
        out.push_back(e.object_class); out.push_back(e.subtype);
        out.push_back(e.variant); out.push_back(e.key_flags);
        for (uint32_t off : e.resource_offset) wr_u32(out, off);
    }
    out.insert(out.end(), dir.source_payload.begin(), dir.source_payload.end());
    patch_u32(out, 28, crc32(out.data() + header_bytes, out.size() - header_bytes));
    // Reject caller-created v2 rows that disagree with the retained source.
    (void)inspect_l3ebd_directory(out, false);
    return out;
}

L3EbdDirectoryHeader inspect_l3ebd_directory(const std::vector<uint8_t>& b,
                                             bool verify_crc) {
    if (b.size() < kL3EbdV1HeaderBytes || b.size() > std::numeric_limits<uint32_t>::max() ||
        b[0] != 'L' || b[1] != '3' || b[2] != 'E' || b[3] != 'B')
        throw std::runtime_error("not L3EB");
    L3EbdDirectoryHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]);
    h.flags = rd_u32(&b[8]); h.entry_count = rd_u32(&b[12]);
    h.source_load_base = rd_u32(&b[16]); h.off_entries = rd_u32(&b[20]);
    h.file_bytes = rd_u32(&b[24]); h.payload_crc32 = rd_u32(&b[28]);
    const bool v2 = h.version == kL3EbdDirectoryVersion;
    const size_t required_header = v2 ? kL3EbdV2HeaderBytes : kL3EbdV1HeaderBytes;
    if ((!v2 && h.version != 1) || h.header_bytes != required_header || b.size() < required_header)
        throw std::runtime_error("unsupported/truncated L3EB header");
    if (h.flags != (v2 ? 0x7u : 0x3u)) throw std::runtime_error("unsupported L3EB flags");
    if (h.off_entries != required_header) throw std::runtime_error("L3EB entry offset invalid");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3EB file size mismatch");
    const uint64_t end = uint64_t(h.off_entries) + uint64_t(h.entry_count) * kL3EbdEntryBytes;
    if (v2) {
        h.off_source_payload = rd_u32(&b[32]); h.source_payload_bytes = rd_u32(&b[36]);
        if (end != h.off_source_payload || end > b.size() || h.source_payload_bytes < 4 ||
            end + h.source_payload_bytes != b.size() ||
            uint64_t(h.source_load_base) + h.source_payload_bytes > (uint64_t(1) << 32u))
            throw std::runtime_error("L3EB source payload range invalid");
        const auto* source = b.data() + h.off_source_payload;
        if (rd_u32(source) != h.entry_count ||
            4ull + uint64_t(h.entry_count) * kEbdDirectoryEntryBytes > h.source_payload_bytes)
            throw std::runtime_error("L3EB source directory count/range mismatch");
        for (uint32_t i = 0; i < h.entry_count; ++i) {
            const auto* row = b.data() + h.off_entries + size_t(i) * kL3EbdEntryBytes;
            const auto* raw = source + 4 + size_t(i) * kEbdDirectoryEntryBytes;
            for (size_t k = 0; k < 4; ++k)
                if (row[k] != raw[k]) throw std::runtime_error("L3EB source directory key mismatch");
            for (size_t slot = 0; slot < 3; ++slot) {
                const uint32_t normalized = pointer_to_offset_or_null(rd_u32(raw + 4 + slot * 4),
                    h.source_load_base, h.source_payload_bytes);
                if (rd_u32(row + 4 + slot * 4) != normalized)
                    throw std::runtime_error("L3EB source resource offset mismatch");
            }
        }
    } else if (end != b.size()) throw std::runtime_error("L3EB entry table size mismatch");
    if (verify_crc && crc32(b.data() + required_header, b.size() - required_header) != h.payload_crc32)
        throw std::runtime_error("L3EB CRC mismatch");
    return h;
}

EbdDirectory load_l3ebd_directory(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3ebd_directory(b, verify_crc);
    EbdDirectory dir;
    dir.load_base = h.source_load_base;
    dir.entries.reserve(h.entry_count);
    for (uint32_t i = 0; i < h.entry_count; ++i) {
        const size_t p = size_t(h.off_entries) + size_t(i) * kL3EbdEntryBytes;
        EbdDirectoryEntry e;
        e.object_class = b[p]; e.subtype = b[p + 1];
        e.variant = b[p + 2]; e.key_flags = b[p + 3];
        for (size_t slot = 0; slot < 3; ++slot) e.resource_offset[slot] = rd_u32(&b[p + 4 + slot * 4]);
        dir.entries.push_back(e);
    }
    if (h.source_payload_bytes)
        dir.source_payload.assign(b.begin() + h.off_source_payload, b.end());
    return dir;
}
} // namespace l3
