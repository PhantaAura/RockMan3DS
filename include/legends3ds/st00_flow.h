#pragma once
#include "legends3ds/stage_spawn.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3St00FlowVersion = 1;
constexpr size_t kSt00TutorialAreaCount = 5;
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
    std::array<St00FlowArea, kSt00TutorialAreaCount> areas{};
};

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
