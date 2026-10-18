#include "legends3ds/source_combat_profile.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
std::array<uint8_t, 6> key(const SourceCombatProfileIdentity& id) noexcept {
    return {{id.source_class, id.subtype, id.raw_stats_selector, id.source_variant,
             id.record_index_0c, id.context_high_0f}};
}
bool admitted(const SourceCombatProfileIdentity& id) noexcept {
    return id.source_class == 0x20 && id.subtype == 3 &&
           (id.raw_stats_selector == 1 || id.raw_stats_selector == 2) &&
           id.source_variant == static_cast<uint8_t>(id.raw_stats_selector == 2) &&
           id.record_index_0c == 0 && id.context_high_0f == 0;
}
void validate(const SourceCombatProfiles& profiles) {
    if (profiles.format_version < 1 || profiles.format_version > 4)
        throw std::runtime_error("L3CB unsupported format version");
    if (profiles.count == 0 || profiles.count > profiles.entries.size())
        throw std::runtime_error("L3CB invalid bounded record count");
    for (size_t i = 0; i < profiles.count; ++i) {
        if (!admitted(profiles.entries[i].identity))
            throw std::runtime_error("L3CB unsupported source selection identity");
        if (profiles.format_version == 1 && profiles.entries[i].unsigned_byte0c)
            throw std::runtime_error("L3CB v1 cannot discard known byte0C");
        if (profiles.format_version < 3 && profiles.entries[i].unsigned_byte09)
            throw std::runtime_error("L3CB old version cannot discard known byte09");
        if (profiles.format_version < 4 && profiles.entries[i].reward_word00)
            throw std::runtime_error("L3CB old version cannot discard known reward word00");
        if (i != 0 && !(key(profiles.entries[i - 1].identity) < key(profiles.entries[i].identity)))
            throw std::runtime_error("L3CB duplicate/unordered source identities");
    }
}
} // namespace

SourceCombatProfiles load_l3source_combat_profiles(const uint8_t* bytes, size_t size) {
    if (bytes == nullptr || size < kSourceCombatProfileHeaderBytes ||
        size > kSourceCombatProfileHeaderBytes + kSourceCombatProfileCapacity * kSourceCombatProfileEntryBytes ||
        bytes[0] != 'L' || bytes[1] != '3' || bytes[2] != 'C' || bytes[3] != 'B')
        throw std::runtime_error("L3CB invalid magic/size");
    const uint32_t count = rd_u32(bytes + 12);
    const uint16_t version = rd_u16(bytes + 4);
    const uint32_t entry_bytes = rd_u32(bytes + 16);
    const uint32_t expected_entry_bytes = version == 4 ? kSourceCombatProfileEntryBytes : kSourceCombatProfileLegacyEntryBytes;
    if (version < 1 || version > 4 || rd_u16(bytes + 6) != kSourceCombatProfileHeaderBytes ||
        rd_u32(bytes + 8) != 1 || count == 0 || count > kSourceCombatProfileCapacity ||
        entry_bytes != expected_entry_bytes || rd_u32(bytes + 20) != size ||
        size != kSourceCombatProfileHeaderBytes + size_t(count) * entry_bytes ||
        rd_u32(bytes + 28) != 0 ||
        crc32(bytes + kSourceCombatProfileHeaderBytes, size - kSourceCombatProfileHeaderBytes) != rd_u32(bytes + 24))
        throw std::runtime_error("L3CB invalid canonical header/CRC");
    SourceCombatProfiles out;
    out.count = count;
    out.format_version = version;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* row = bytes + kSourceCombatProfileHeaderBytes + i * entry_bytes;
        if ((version < 3 && row[11] != 0) ||
            (version == 1 && (row[9] != 0 || row[10] != 0)) ||
            (version == 2 && (row[10] > 1 || (row[10] == 0 && row[9] != 0))) ||
            (version == 3 && ((row[10] & 0xfcu) ||
                (!(row[10] & 1u) && row[9] != 0) || (!(row[10] & 2u) && row[11] != 0))) ||
            (version == 4 && ((row[10] & 0xf8u) ||
                (!(row[10] & 1u) && row[9] != 0) || (!(row[10] & 2u) && row[11] != 0) ||
                (!(row[10] & 4u) && rd_u32(row + 12) != 0))))
            throw std::runtime_error("L3CB reserved entry fields nonzero");
        out.entries[i].identity = {row[0], row[1], row[2], row[3], row[4], row[5]};
        out.entries[i].unsigned_byte08 = row[6];
        out.entries[i].unsigned_byte0a = row[7];
        out.entries[i].unsigned_byte0f = row[8];
        if (version >= 2 && (row[10] & 1u)) out.entries[i].unsigned_byte0c = row[9];
        if (version == 3 && (row[10] & 2u)) out.entries[i].unsigned_byte09 = row[11];
        if (version == 4 && (row[10] & 2u)) out.entries[i].unsigned_byte09 = row[11];
        if (version == 4 && (row[10] & 4u)) out.entries[i].reward_word00 = rd_u32(row + 12);
    }
    validate(out);
    return out;
}

std::vector<uint8_t> encode_l3source_combat_profiles(const SourceCombatProfiles& profiles) {
    validate(profiles);
    std::vector<uint8_t> payload;
    payload.reserve(size_t(profiles.count) * kSourceCombatProfileEntryBytes);
    for (size_t i = 0; i < profiles.count; ++i) {
        const auto& entry = profiles.entries[i];
        const auto id = key(entry.identity);
        payload.insert(payload.end(), id.begin(), id.end());
        payload.insert(payload.end(), {entry.unsigned_byte08, entry.unsigned_byte0a,
                                      entry.unsigned_byte0f, entry.unsigned_byte0c.value_or(0),
                                      static_cast<uint8_t>(entry.unsigned_byte0c.has_value() |
                                          (entry.unsigned_byte09.has_value() << 1u) |
                                          (entry.reward_word00.has_value() << 2u)),
                                      entry.unsigned_byte09.value_or(0)});
        if (profiles.format_version == 4) wr_u32(payload, entry.reward_word00.value_or(0));
    }
    std::vector<uint8_t> out{'L', '3', 'C', 'B'};
    wr_u16(out, profiles.format_version); wr_u16(out, kSourceCombatProfileHeaderBytes);
    const uint32_t entry_bytes = profiles.format_version == 4 ? kSourceCombatProfileEntryBytes : kSourceCombatProfileLegacyEntryBytes;
    wr_u32(out, 1); wr_u32(out, profiles.count); wr_u32(out, entry_bytes);
    wr_u32(out, static_cast<uint32_t>(kSourceCombatProfileHeaderBytes + payload.size()));
    wr_u32(out, crc32(payload.data(), payload.size())); wr_u32(out, 0);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

const SourceCombatProfile* find_source_combat_profile(
    const SourceCombatProfiles& profiles, uint8_t raw_stats_selector,
    uint8_t source_class, uint8_t subtype, uint8_t record_index_0c,
    uint8_t context_high_0f) noexcept {
    const SourceCombatProfileIdentity wanted{source_class, subtype, raw_stats_selector,
        static_cast<uint8_t>(raw_stats_selector == 2), record_index_0c, context_high_0f};
    if (!admitted(wanted) || profiles.count == 0 || profiles.count > profiles.entries.size()) return nullptr;
    for (size_t i = 0; i < profiles.count; ++i)
        if (key(profiles.entries[i].identity) == key(wanted)) return &profiles.entries[i];
    return nullptr;
}
} // namespace l3
