#pragma once
#include <cstdint>

namespace l3 {

// Evidence-only reconstruction of the selector policy used by the original USA
// executable initializer at 0x80047354. This helper deliberately does NOT assign
// that initializer to every E0/subtype-08 object; the owning behavior-dispatch
// mapping must be proved separately before runtime integration.
enum class EbdSelectorSource : uint8_t {
    FixedZeroFromBit80,
    FixedTwoWithClass80Helpers,
    FixedLowNibble,
    RuntimeProbeZeroOrOne,
};

struct EbdSelectorPlan {
    EbdSelectorSource source = EbdSelectorSource::RuntimeProbeZeroOrOne;
    bool selector_is_known = false;
    uint8_t selector = 0;
    bool spawn_class80_helper_pair = false;
    uint8_t helper_param3_a = 0;
    uint8_t helper_param3_b = 0;
};

// Reconstruct only the branch that chooses the optional EBD selector.
// Branch precedence is exact: bit 0x80, then bit 0x20, then low-nibble 4/5/6,
// then the runtime-probe comparison path.
EbdSelectorPlan ebd_selector_plan_80047354(uint8_t work_param3);

// Completes the final runtime-probe path when the caller has translated
// func_8004920C and work+0x3A. The original signed comparison is:
//     (probe_value - work_3a) < 30 ? selector 0 : selector 1
uint8_t ebd_selector_from_probe_80047354(int32_t probe_value, int16_t work_3a);

} // namespace l3
