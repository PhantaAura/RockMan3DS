#pragma once
#include "legends3ds/stage_spawn.h"
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3StageObjectCatalogVersion = 1;
constexpr uint32_t kL3StageObjectTableStrongPointerRun = 1u << 0;

struct StageObjectCatalogList {
    uint32_t source_list_offset = 0;
    StageObjectList objects;
};

struct StageObjectCatalogTable {
    uint32_t source_table_offset = 0;
    uint32_t evidence_flags = 0;
    std::vector<StageObjectCatalogList> lists;
};

struct StageObjectCatalog {
    uint32_t source_load_base = 0;
    std::vector<StageObjectCatalogTable> tables;
};

StageObjectCatalog load_l3stage_object_catalog(const std::vector<uint8_t>& bytes,
                                               bool verify_crc = true);

} // namespace l3
