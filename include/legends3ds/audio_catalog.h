#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace l3 {
constexpr uint16_t kL3AudioCatalogVersion = 1;
enum class RuntimeAudioSourceKind : uint8_t { Xa = 1, Str = 2 };
struct RuntimeAudioSourceEntry {
    RuntimeAudioSourceKind kind = RuntimeAudioSourceKind::Xa;
    bool aligned_2048 = false;
    uint32_t source_bytes = 0;
    uint32_t blocks_2048 = 0xFFFFFFFFu;
    std::array<uint8_t,32> sha256{};
    std::string source_name;
};
struct RuntimeAudioCatalog {
    bool routing_proven = false;
    std::vector<RuntimeAudioSourceEntry> entries;
};
RuntimeAudioCatalog load_l3audio_catalog(const std::vector<uint8_t>& bytes,
                                         bool verify_crc = true);
}
