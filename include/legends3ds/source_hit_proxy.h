#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

// Neutral owner-local metadata; never compile the retail table into the program.
struct SourceHitProxyRules {
    std::array<std::uint8_t, 8> unsigned_80097d00{};
};
bool decode_source_hit_proxy_rules(const std::vector<std::uint8_t>& bytes,
                                   SourceHitProxyRules& out);

struct SourceHitProxyTarget {
    std::uint32_t source_address = 0;
    std::uint32_t packet_00 = 0;
    std::uint32_t flags_04 = 0;
    std::array<std::int16_t, 3> point_08{};
    std::uint16_t word_0e = 0;
};
struct SourceHitProxyOwnerPoint {
    std::array<std::uint32_t, 2> raw_bc_words{};
};
struct SourceHitProxyRecord {
    std::array<std::int16_t, 3> point{};
    std::uint16_t encoded_extent = 0;
    std::uint32_t encoded_identity = 0;
    std::uint32_t parallel_word = 0;
    SourceHitProxyTarget* target = nullptr;
};
struct SourceHitProxyBank {
    std::array<SourceHitProxyRecord, 64> records{};
    std::size_t count = 0;
};
struct SourceHitProxyRegistration {
    std::array<std::uint32_t, 2> pointed_words{};
    std::uint32_t a1 = 0;
    std::uint32_t a2 = 0;
    std::uint32_t a3 = 0;
    std::uint32_t stack10 = 0;
    std::uint32_t stack18 = 0;
    std::uint32_t stack1c = 0;
    std::uint32_t stack20 = 0;
    SourceHitProxyTarget* target = nullptr;
    std::optional<std::uint32_t> mask_80098798;
    std::array<SourceHitProxyOwnerPoint*, 24> owners_800be478{};
};
// Exact8003430C two-record producer. Its endpoint extents and stack words
// retain their source offset names; no modern capsule/ray rules are substituted.
struct SourceHitProxyCompositeRegistration {
    std::array<std::uint32_t,2> first_words{},second_words{};
    std::uint32_t first_extent=0,second_extent=0;
    std::uint32_t stack10=0,stack14=0,stack18=0,stack20=0,stack24=0,stack28=0;
    SourceHitProxyTarget* target=nullptr;
    std::optional<std::uint32_t> mask_80098798;
    std::array<SourceHitProxyOwnerPoint*,24> owners_800be478{};
    // Source8003430C authors three midpoint shorts on its stack. Fourth lane
    // is unowned and needed only when a masked lease receives that8-byte copy.
    std::optional<std::uint16_t> midpoint_padding_06;
};
// The original pair consumer can read an unowned second endpoint when it
// emits a sphere's absolute point using the OTHER shape's interpolation weight.
// Default behavior holds that uncommon branch; no zero scratch is invented.
struct SourceHitProxyCompositeScratch {
    std::size_t first_record=0,second_record=0;
    std::optional<std::array<std::int16_t,3>> first_unowned_second_point;
};
enum class SourceHitProxyStatus : std::uint8_t {
    SourceBackedPartial, Full, InvalidBank, NeedsMask, NeedsTarget,
    NeedsOwnerPoint, NeedsRules, NeedsCompositeShape, NeedsOwnerPointPadding,
    NeedsCompositePointScratch
};
struct SourceHitProxyResult {
    SourceHitProxyStatus status = SourceHitProxyStatus::InvalidBank;
    std::size_t admitted_pairs = 0;
    std::size_t overlapping_pairs = 0;
};

struct SourceHitProxyLeaseResult {
    SourceHitProxyStatus status = SourceHitProxyStatus::NeedsMask;
    std::int32_t index = -1;
};
// Exact8003545C scalar producer used by source80035724. The24 source owner
// tokens are pointer identities, not copied BC/C0 points. The current global
// mask must be admitted by its owner; no fresh startup value is assumed here.
SourceHitProxyLeaseResult source_hit_proxy_allocate_lease_8003545c(
    std::optional<std::uint32_t>& mask_80098798,
    std::array<std::uint32_t,24>& owners_800be478,
    std::uint32_t owner_source_address, SourceHitProxyTarget& target) noexcept;
// Exact800354E0 clears only the mask bit. The retained owner token and target
// flags are not cleared by this function. Original SRLV uses index&31.
SourceHitProxyStatus source_hit_proxy_release_lease_800354e0(
    std::optional<std::uint32_t>& mask_80098798, std::uint32_t index) noexcept;

// Source800341BC. An unavailable masked owner pointer holds the entire mutation.
// Read-only preflight of the same source registration; no bank/target/lease
// mutation. A synchronous owner may prepare all later consumers before commit.
SourceHitProxyResult source_hit_proxy_check_register_800341bc(
    const SourceHitProxyBank& bank,const SourceHitProxyRegistration& input) noexcept;
SourceHitProxyResult source_hit_proxy_register_800341bc(
    SourceHitProxyBank& bank, const SourceHitProxyRegistration& input) noexcept;
SourceHitProxyResult source_hit_proxy_check_register_composite_8003430c(
    const SourceHitProxyBank&,const SourceHitProxyCompositeRegistration&) noexcept;
SourceHitProxyResult source_hit_proxy_register_composite_8003430c(
    SourceHitProxyBank&,const SourceHitProxyCompositeRegistration&) noexcept;
// Source800345A8..80034668 +800346AC sampled sphere/two-record shapes.
// Original index advancement, signed interpolation, wrap arithmetic and
// relative/absolute point emission are preserved. Unowned point scratch holds.
//
// Does not implement the earlier GTE/player consumers80034EB8/800358F0.
SourceHitProxyResult source_hit_proxy_consume_pairs_8003450c(
    SourceHitProxyBank& bank, const std::optional<SourceHitProxyRules>& rules,
    const SourceHitProxyCompositeScratch* scratch=nullptr) noexcept;

} // namespace l3
