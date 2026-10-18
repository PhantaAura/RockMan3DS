#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace l3 {

constexpr uint16_t kL3ResourceCatalogVersion = 1;
enum class RuntimeResourceKind : uint8_t { Ebd = 1, Message = 2 };

struct ResourceCatalogEntry {
    std::array<char,8> package_id{{0,0,0,0,0,0,0,0}};
    RuntimeResourceKind kind = RuntimeResourceKind::Ebd;
    uint32_t source_load = 0;
    uint32_t runtime_bytes = 0;
    uint32_t runtime_crc32 = 0;
    std::string source_name;
    std::string runtime_path;
    std::string package_string() const;
};

struct ResourceCatalog { std::vector<ResourceCatalogEntry> entries; };

ResourceCatalog load_l3resource_catalog(const std::vector<uint8_t>& bytes,
                                        bool verify_crc = true);
std::vector<size_t> find_resource_catalog_package(const ResourceCatalog& catalog,
                                                   const std::string& package_id,
                                                   std::optional<RuntimeResourceKind> kind = std::nullopt);
std::optional<size_t> find_resource_catalog_name(const ResourceCatalog& catalog,
                                                 const std::string& package_id,
                                                 RuntimeResourceKind kind,
                                                 const std::string& source_name);

} // namespace l3
