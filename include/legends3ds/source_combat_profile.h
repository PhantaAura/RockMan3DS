#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {
// Exact source selection identity, not a gameplay/difficulty label. The current
// proved contract admits class20/subtype03, raw selector1/2, record index0C=0
// and context0F=0 only. Unknown/restored session selector must not be defaulted.
struct SourceCombatProfileIdentity {
    uint8_t source_class = 0;
    uint8_t subtype = 0;
    uint8_t raw_stats_selector = 0;
    // Legacy source-table tag: selector2 ->1; selector1 ->0. This is NOT
    // actor.variant_32e, which indexes separate16B records in source actions.
    uint8_t source_variant = 0;
    uint8_t record_index_0c = 0;
    uint8_t context_high_0f = 0;
};
struct SourceCombatProfile {
    SourceCombatProfileIdentity identity;
    uint8_t unsigned_byte08 = 0;
    uint8_t unsigned_byte0a = 0;
    uint8_t unsigned_byte0f = 0;
    // Original record byte0C, not a guessed gameplay field. Missing in v1 and
    // explicitly absent v2 rows stays unknown; zero is a legitimate value.
    std::optional<uint8_t> unsigned_byte0c = std::nullopt;
    // Original record byte09 consumed by801041B0. Its broader meaning is not
    // inferred. Absent from v1/v2, and never defaulted in those packages.
    std::optional<uint8_t> unsigned_byte09 = std::nullopt;
    // Original selected statistics record word+00. Defeat wrapper80047228
    // forwards this value unchanged to reward producer80046B34. It is an
    // opaque source descriptor, not a native currency amount.
    std::optional<uint32_t> reward_word00 = std::nullopt;
    // Original record byte0B: action 8 with parameter 0x40 (the airborne
    // entrance, e.g. after the ST00 subtype-0x1F pop-out) stores it in work330.
    // v5 only; never defaulted for older packages.
    std::optional<uint8_t> unsigned_byte0b = std::nullopt;
    int16_t initial_hp() const noexcept {
        return static_cast<int16_t>(static_cast<uint16_t>(unsigned_byte08) * 16U);
    }
    int16_t action3_work330() const noexcept { return static_cast<int16_t>(unsigned_byte0a); }
    uint8_t detection_profile() const noexcept { return unsigned_byte0f; }
};
constexpr size_t kSourceCombatProfileCapacity = 16;
constexpr size_t kSourceCombatProfileHeaderBytes = 32;
constexpr size_t kSourceCombatProfileLegacyEntryBytes = 12;
constexpr size_t kSourceCombatProfileEntryBytes = 16;
constexpr size_t kSourceCombatProfileV5EntryBytes = 20;
struct SourceCombatProfiles {
    std::array<SourceCombatProfile, kSourceCombatProfileCapacity> entries{};
    uint32_t count = 0;
    uint16_t format_version = 5; // Parsed v1/v2/v3 retain their exact version.
};

// L3CB:32B canonical header and ordered sparse12B records, payloadCRC32.
// v1 row9..11 are zero. v2 row9=byte0C, row10=presence0/1, row11=zero;
// absent byte0C requires row9=zero. No implicit v1→v2 conversion on encode.
// v3 row11=byte09 and row10 uses bit0=byte0C present, bit1=byte09 present.
// v4 adds word00 at row12 and row10 bit2. Unknown bits reject; absent values
// require zero in their payload lanes. v5 rows are 20B: byte0B at row16,
// row10 bit3, rows17..19 zero.
// No heap ownership or fallback identity. Parsing occurs at package load.
SourceCombatProfiles load_l3source_combat_profiles(const uint8_t* bytes, size_t size);
std::vector<uint8_t> encode_l3source_combat_profiles(const SourceCombatProfiles& profiles);
const SourceCombatProfile* find_source_combat_profile(
    const SourceCombatProfiles& profiles, uint8_t raw_stats_selector,
    uint8_t source_class, uint8_t subtype, uint8_t record_index_0c,
    uint8_t context_high_0f) noexcept;
} // namespace l3
