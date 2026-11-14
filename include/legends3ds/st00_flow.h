#pragma once
#include "legends3ds/stage_spawn.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3St00FlowVersion = 1;
constexpr size_t kSt00TutorialAreaCount = 5;
// Largest area count a stage flow may carry (ST00 has five; stage overlays
// with fewer areas fill only the first area_count rows).
constexpr size_t kStageFlowMaxAreas = 5;
constexpr size_t kSt00AreaSetupBlobBytes = 40;
constexpr size_t kSt00AreaSetupSeedBytes = 8;
constexpr size_t kSt00AuxRecordBytes = 24;

// Runtime records reached through the per-area pointer installed at scratch 0x1F800050.
// Common game code walks these in exact 0x18-byte steps and stops when the first signed
// halfword is -1. Field meanings are intentionally not guessed yet.
struct St00AuxRecord {
    std::array<uint8_t, kSt00AuxRecordBytes> raw{};
    std::array<int16_t, 12> halfwords{};
};

struct St00AuxList {
    std::vector<St00AuxRecord> records;
    size_t bytes_consumed = 0; // includes the full 24-byte sentinel slot
};

// One tutorial sub-area entry reconstructed from the USA ST00 program overlay.
// Unknown tables remain explicitly named as source/aux data instead of receiving guessed gameplay names.
struct St00FlowArea {
    StageObjectList objects;
    St00AuxList aux;
    std::array<uint8_t, kSt00AreaSetupBlobBytes> setup_blob{};
    std::array<uint8_t, kSt00AreaSetupSeedBytes> setup_seed{};
    uint32_t source_object_offset = 0;   // overlay-relative, never a runtime pointer
    uint32_t source_aux_offset = 0;      // overlay-relative pointer stored to scratch +0x50
    uint32_t source_resource_offset = 0; // overlay-relative pointer passed to common setup at 0x80022FD4
    uint32_t setup_selector = 0;         // value selected from 0x8011ADE4 before *8 indexing
};

struct St00FlowSource {
    uint32_t overlay_load_base = 0x80100000u;
    uint32_t area_count = kSt00TutorialAreaCount;   // rows of `areas` that are real
    std::array<St00FlowArea, kStageFlowMaxAreas> areas{};
};

// The fixed table addresses one stage overlay's area setup routine reads
// (ST00 0x8010038C, ST02 0x801000B0): each indexes its table by the signed
// current-area byte 0x800C1B61. Every value is read straight from the
// overlay's own instructions (docs/research_01516/STAGE_LOADING.md).
struct StageFlowAddresses {
    uint8_t stage = 0;
    uint32_t area_count = 0;
    uint32_t object_ptr_table = 0;     // -> 0x8002EA50 object list
    uint32_t aux_ptr_table = 0;        // -> scratch 0x1F800050
    uint32_t setup_blob_table = 0;     // 40 bytes per area
    uint32_t selector_table = 0;       // *8 into the seed table (0x800371A0)
    uint32_t seed_table = 0;
    uint32_t resource_ptr_table = 0;   // -> 0x80022FD4
};
// The known stage overlays (USA). nullptr for a stage whose routine is not traced.
const StageFlowAddresses* stage_flow_addresses_us(uint8_t stage) noexcept;

St00FlowSource parse_stage_flow_us(const std::vector<uint8_t>& overlay_payload,
                                   const StageFlowAddresses& addresses,
                                   uint32_t load_base = 0x80100000u);

struct L3St00FlowHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t area_count = 0;
    uint32_t object_count = 0;
    uint32_t aux_count = 0;
    uint32_t off_areas = 0;
    uint32_t off_objects = 0;
    uint32_t off_setup_blobs = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

// Parses the five-area tutorial flow tables from the USA R3_ST00.BIN type-0 payload.
// The table addresses are direct executable/overlay evidence; every absolute PS1 pointer is
// validated and converted into an overlay-relative offset before it leaves this layer.
St00AuxList parse_st00_aux_list(const uint8_t* data, size_t bytes);
St00AuxList parse_st00_aux_list(const std::vector<uint8_t>& data, size_t offset = 0);

St00FlowSource parse_st00_flow_us(const std::vector<uint8_t>& overlay_payload,
                                  uint32_t load_base = 0x80100000u);

std::vector<uint8_t> cook_l3st00_flow(const St00FlowSource& flow);
L3St00FlowHeader inspect_l3st00_flow(const std::vector<uint8_t>& bytes,
                                     bool verify_crc = true);
St00FlowSource load_l3st00_flow(const std::vector<uint8_t>& bytes,
                                  bool verify_crc = true);

} // namespace l3
