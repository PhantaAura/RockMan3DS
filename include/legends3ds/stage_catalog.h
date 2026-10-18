#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace l3 {

constexpr uint16_t kL3StageCatalogVersion = 1;
constexpr uint32_t kL3StageCatalogFlagMap = 1u << 0;
constexpr uint32_t kL3StageCatalogFlagRender = 1u << 1;

struct StageCatalogEntry {
    // package_id is the source container stem (for example a base stage or one of
    // its retail variants).  It is metadata only; no source path or game bytes are retained.
    std::array<char,8> package_id{{0,0,0,0,0,0,0,0}};
    std::array<char,4> stage_code{{0,0,0,0}};
    uint32_t flags = 0;
    uint32_t map_bytes = 0;
    uint32_t render_bytes = 0;
    uint32_t map_crc32 = 0;
    uint32_t render_crc32 = 0;
    std::string package_string() const;
    std::string stage_string() const { return std::string(stage_code.data(), stage_code.size()); }
};

struct StageCatalog { std::vector<StageCatalogEntry> entries; };

StageCatalog load_l3stage_catalog(const std::vector<uint8_t>& bytes, bool verify_crc=true);
std::optional<size_t> find_stage_catalog_package(const StageCatalog& catalog, const std::string& package_id);
std::vector<size_t> find_stage_catalog_stage(const StageCatalog& catalog, const std::array<char,4>& stage_code);

} // namespace l3
