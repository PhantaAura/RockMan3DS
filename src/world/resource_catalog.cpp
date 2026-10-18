#include "legends3ds/resource_catalog.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeaderBytes = 40;
constexpr size_t kEntryBytes = 40;

std::string read_catalog_string(const std::vector<uint8_t>& b, uint32_t strings_off,
                                uint32_t strings_bytes, uint32_t off, uint32_t len) {
    if (uint64_t(off) + len > strings_bytes) throw std::runtime_error("L3RC string range invalid");
    return std::string(reinterpret_cast<const char*>(b.data() + strings_off + off), len);
}
}

std::string ResourceCatalogEntry::package_string() const {
    size_t n = 0;
    while (n < package_id.size() && package_id[n] != 0) ++n;
    return std::string(package_id.data(), n);
}

ResourceCatalog load_l3resource_catalog(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < kHeaderBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'R' || b[3] != 'C')
        throw std::runtime_error("not L3RC");
    const uint16_t version = rd_u16(&b[4]);
    const uint16_t header = rd_u16(&b[6]);
    const uint32_t flags = rd_u32(&b[8]);
    const uint32_t count = rd_u32(&b[12]);
    const uint32_t entry_bytes = rd_u32(&b[16]);
    const uint32_t off_entries = rd_u32(&b[20]);
    const uint32_t off_strings = rd_u32(&b[24]);
    const uint32_t string_bytes = rd_u32(&b[28]);
    const uint32_t file_bytes = rd_u32(&b[32]);
    const uint32_t payload_crc = rd_u32(&b[36]);
    if (version != kL3ResourceCatalogVersion || header != kHeaderBytes || entry_bytes != kEntryBytes)
        throw std::runtime_error("unsupported L3RC");
    if (flags != 0u) throw std::runtime_error("L3RC reserved flags nonzero");
    if (off_entries != kHeaderBytes ||
        uint64_t(off_entries) + uint64_t(count) * kEntryBytes != off_strings ||
        uint64_t(off_strings) + string_bytes != file_bytes || file_bytes != b.size())
        throw std::runtime_error("L3RC size/range mismatch");
    if (verify_crc && crc32(b.data() + kHeaderBytes, b.size() - kHeaderBytes) != payload_crc)
        throw std::runtime_error("L3RC CRC mismatch");

    ResourceCatalog out;
    out.entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const size_t p = size_t(off_entries) + size_t(i) * kEntryBytes;
        ResourceCatalogEntry e;
        std::copy_n(reinterpret_cast<const char*>(&b[p]), 8, e.package_id.begin());
        if (e.package_id[0] == '\0') throw std::runtime_error("L3RC empty package id");
        const uint8_t kind = b[p + 8];
        if (b[p + 9] || b[p + 10] || b[p + 11]) throw std::runtime_error("L3RC entry reserved bytes nonzero");
        if (kind != uint8_t(RuntimeResourceKind::Ebd) && kind != uint8_t(RuntimeResourceKind::Message))
            throw std::runtime_error("L3RC unknown resource kind");
        e.kind = static_cast<RuntimeResourceKind>(kind);
        e.source_load = rd_u32(&b[p + 12]);
        e.runtime_bytes = rd_u32(&b[p + 16]);
        e.runtime_crc32 = rd_u32(&b[p + 20]);
        const uint32_t name_off = rd_u32(&b[p + 24]);
        const uint32_t name_len = rd_u32(&b[p + 28]);
        const uint32_t path_off = rd_u32(&b[p + 32]);
        const uint32_t path_len = rd_u32(&b[p + 36]);
        if (e.runtime_bytes == 0u || name_len == 0u || path_len == 0u)
            throw std::runtime_error("L3RC empty resource metadata");
        e.source_name = read_catalog_string(b, off_strings, string_bytes, name_off, name_len);
        e.runtime_path = read_catalog_string(b, off_strings, string_bytes, path_off, path_len);
        const auto duplicate = std::find_if(out.entries.begin(), out.entries.end(), [&](const auto& x) {
            return x.package_string() == e.package_string() && x.kind == e.kind && x.source_name == e.source_name;
        });
        if (duplicate != out.entries.end()) throw std::runtime_error("L3RC duplicate resource key");
        out.entries.push_back(std::move(e));
    }
    return out;
}

std::vector<size_t> find_resource_catalog_package(const ResourceCatalog& c,
                                                   const std::string& package_id,
                                                   std::optional<RuntimeResourceKind> kind) {
    std::vector<size_t> out;
    for (size_t i = 0; i < c.entries.size(); ++i) {
        if (c.entries[i].package_string() == package_id && (!kind || c.entries[i].kind == *kind)) out.push_back(i);
    }
    return out;
}

std::optional<size_t> find_resource_catalog_name(const ResourceCatalog& c,
                                                 const std::string& package_id,
                                                 RuntimeResourceKind kind,
                                                 const std::string& source_name) {
    for (size_t i = 0; i < c.entries.size(); ++i)
        if (c.entries[i].package_string() == package_id && c.entries[i].kind == kind && c.entries[i].source_name == source_name) return i;
    return std::nullopt;
}

} // namespace l3
