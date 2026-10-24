#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>

namespace l3 {
// L3VB1: the message voice clips cooked by tools/cook_xa_voice.py from the
// owner XA (mono 4-bit, 18900 Hz). Header: magic "L3VB001\0", version 1,
// count, sample rate, entry bytes 16, file bytes, CRC32 of the entry table.
// Entries: 0x8001D58C request id, mono frames, PCM byte offset, PCM CRC32.
// Clips are read on demand; nothing but the table stays resident.
struct VoiceBankEntry { uint32_t request = 0, frames = 0, offset = 0, crc = 0; };
constexpr uint32_t kVoiceBankMaxEntries = 512;
constexpr uint32_t kVoiceBankMaxClipFrames = 256u * 4032u;
class VoiceBankReader {
public:
    VoiceBankReader() = default;
    ~VoiceBankReader() { close(); }
    VoiceBankReader(const VoiceBankReader&) = delete;
    VoiceBankReader& operator=(const VoiceBankReader&) = delete;
    bool open(const char* path);
    void close() noexcept;
    bool ready() const noexcept { return file_ != nullptr; }
    const char* error() const noexcept { return error_; }
    uint32_t sample_rate() const noexcept { return sample_rate_; }
    const std::vector<VoiceBankEntry>& entries() const noexcept { return entries_; }
    const VoiceBankEntry* find(uint32_t request) const noexcept;
    // Reads one clip into caller storage of at least entry.frames samples and
    // checks its CRC. Returns false (storage contents unspecified) on failure.
    bool read_clip(const VoiceBankEntry& entry, int16_t* samples, size_t capacity) noexcept;
private:
    bool reject(const char* error) noexcept;
    std::FILE* file_ = nullptr;
    std::vector<VoiceBankEntry> entries_;
    uint32_t sample_rate_ = 0;
    const char* error_ = "not opened";
};
} // namespace l3
