#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {
// Owner-local ST00 tables at8011BEE0/BEEC/BEF8. No table values are built in.
struct SourceReactionProfile {
    std::array<std::array<uint8_t,12>,3> timer_pairs{};
};
SourceReactionProfile load_source_reaction_profile(const uint8_t*,size_t);
std::vector<uint8_t> encode_source_reaction_profile(const SourceReactionProfile&);
} // namespace l3
