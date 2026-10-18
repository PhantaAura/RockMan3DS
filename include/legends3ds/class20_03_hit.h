#pragma once
#include "legends3ds/class20_03_behavior.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {
struct Class20_03HitRecordByte09 {
    int16_t actor_variant_32e = 0;
    uint8_t unsigned_byte09 = 0;
};
struct Class20_03HitInputs {
    std::optional<uint32_t> packet_33c;
    std::optional<Class20_03HitRecordByte09> selected_record;
    // Actual packet-context work34A and helper80031198 return. Neither is
    // inferred from a desktop projectile's direction or a target radius.
    std::optional<uint16_t> hit_word_34a;
    std::optional<uint32_t> angle_80031198;
    bool sound_consumer_ready = false;
};
enum class Class20_03HitStatus : uint8_t {
    SourceBackedPartial, NeedsPacket, NeedsHealth, NeedsCombatProfile,
    NeedsHitContext, NeedsAngle, NeedsSoundConsumer
};
struct Class20_03HitResult {
    Class20_03HitStatus status = Class20_03HitStatus::SourceBackedPartial;
    std::optional<uint16_t> sound_8001a050;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
};

// Exact scalar/control consumer801041B0. A low24-bit packet field and source
// masks are retained verbatim; no guessed projectile, damage, invulnerability
// or defeat rule supplies them. Nonzero packets are NOT cleared here: the
// proved outer callback clears them only after animation/effect/proxy owners.
// Missing branch prerequisites leave all source-used state unchanged.
Class20_03HitResult class20_03_consume_hit_801041b0(
    Class20_03CallbackState& state, const Class20_03HitInputs& inputs) noexcept;
} // namespace l3
