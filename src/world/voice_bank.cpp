#include "legends3ds/voice_bank.h"
#include <cstring>

namespace l3 {
namespace {
uint32_t rd32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
uint32_t crc32_update(uint32_t crc, const uint8_t* p, size_t n) {
    crc = ~crc;
    while (n--) { crc ^= *p++; for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u))); }
    return ~crc;
}
constexpr size_t kHeaderBytes = 32, kEntryBytes = 16;
}

bool VoiceBankReader::reject(const char* error) noexcept { close(); error_ = error; return false; }
void VoiceBankReader::close() noexcept {
    if (file_) std::fclose(file_);
    file_ = nullptr; entries_.clear(); sample_rate_ = 0; error_ = "closed";
}
bool VoiceBankReader::open(const char* path) {
    close();
    file_ = std::fopen(path, "rb");
    if (!file_) return reject("voice bank missing");
    uint8_t header[kHeaderBytes];
    if (std::fread(header, 1, kHeaderBytes, file_) != kHeaderBytes) return reject("short voice bank header");
    if (std::memcmp(header, "L3VB001\0", 8) || rd32(header + 8) != 1 || rd32(header + 20) != kEntryBytes)
        return reject("unsupported voice bank header");
    const uint32_t count = rd32(header + 12), rate = rd32(header + 16), bytes = rd32(header + 24), table_crc = rd32(header + 28);
    if (!count || count > kVoiceBankMaxEntries || rate == 0 || rate > 48000) return reject("voice bank count/rate policy");
    if (std::fseek(file_, 0, SEEK_END) || std::ftell(file_) != long(bytes) || std::fseek(file_, long(kHeaderBytes), SEEK_SET))
        return reject("voice bank size mismatch");
    std::vector<uint8_t> table(size_t(count) * kEntryBytes);
    if (std::fread(table.data(), 1, table.size(), file_) != table.size()) return reject("short voice bank table");
    if (crc32_update(0, table.data(), table.size()) != table_crc) return reject("voice bank table CRC");
    entries_.resize(count);
    uint64_t expect = kHeaderBytes + uint64_t(count) * kEntryBytes;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* p = table.data() + i * kEntryBytes;
        auto& e = entries_[i];
        e = {rd32(p), rd32(p + 4), rd32(p + 8), rd32(p + 12)};
        if (!e.frames || e.frames > kVoiceBankMaxClipFrames || e.offset != expect) return reject("voice bank entry bounds");
        for (uint32_t j = 0; j < i; ++j) if (entries_[j].request == e.request) return reject("duplicate voice request");
        expect += uint64_t(e.frames) * 2;
    }
    if (expect != bytes) return reject("voice bank PCM extent");
    sample_rate_ = rate; error_ = "ready";
    return true;
}
const VoiceBankEntry* VoiceBankReader::find(uint32_t request) const noexcept {
    for (const auto& e : entries_) if (e.request == request) return &e;
    return nullptr;
}
bool VoiceBankReader::read_clip(const VoiceBankEntry& e, int16_t* samples, size_t capacity) noexcept {
    if (!file_ || !samples || capacity < e.frames) { error_ = "voice clip capacity"; return false; }
    if (std::fseek(file_, long(e.offset), SEEK_SET) ||
        std::fread(samples, 2, e.frames, file_) != e.frames) { error_ = "short voice clip"; return false; }
    // PCM is little-endian on disk; both supported hosts are little-endian.
    if (crc32_update(0, reinterpret_cast<const uint8_t*>(samples), size_t(e.frames) * 2) != e.crc) {
        error_ = "voice clip CRC"; return false;
    }
    error_ = "ready";
    return true;
}
} // namespace l3
