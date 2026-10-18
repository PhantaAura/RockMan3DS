#pragma once
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/source_hit_proxy.h"
#include <array>
#include <cstdint>
#include <optional>

namespace l3 {
struct Class20_03ProxyInputs {
    // Receipt from the original fixed-point pose producer. Never a render center.
    std::optional<std::array<int16_t, 4>> pose_point_180;
    uint8_t source_work_01 = 0;
    uint16_t producer_330 = 0;
    SourceHitProxyTarget* target_33c = nullptr;
    std::optional<uint32_t> mask_80098798;
    std::array<SourceHitProxyOwnerPoint*, 24> owners_800be478{};
};
enum class Class20_03ProxyStatus : uint8_t {
    SourceBackedPartial, NeedsPosePoint, NeedsTarget,
    NeedsMask, NeedsOwnerPoint, InvalidBank, NeedsBodyTransaction
};
struct Class20_03ProxyResult {
    Class20_03ProxyStatus status = Class20_03ProxyStatus::SourceBackedPartial;
    bool registered = false;
    bool bank_full = false;
    bool packet_cleared = false;
};
// Read-only shared preflight for an atomic body/animation/tail transaction.
Class20_03ProxyResult class20_03_check_proxy_80102398(
    const Class20_03CallbackState&,const SourceHitProxyBank&,
    const Class20_03ProxyInputs&) noexcept;
// Exact outer tail80102370..801023F4. Gate work00&1 and work328&4;
// copy full pointed8B180→334; register extent80/mode8000/producer04880000;
// clear packet33C after the registration call, including the source full-bank
// return. Missing prerequisites hold all tail mutations. Other callback
// consumers must have committed before the caller invokes this tail.
Class20_03ProxyResult class20_03_finalize_proxy_80102398(
    Class20_03CallbackState& state, SourceHitProxyBank& bank,
    const Class20_03ProxyInputs& inputs) noexcept;
} // namespace l3
