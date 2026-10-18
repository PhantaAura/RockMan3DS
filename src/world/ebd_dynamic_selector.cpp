#include "legends3ds/ebd_dynamic_selector.h"

namespace l3 {

EbdSelectorPlan ebd_selector_plan_80047354(uint8_t p3) {
    EbdSelectorPlan out;

    if ((p3 & 0x80u) != 0u) {
        out.source = EbdSelectorSource::FixedZeroFromBit80;
        out.selector_is_known = true;
        out.selector = 0;
        return out;
    }

    if ((p3 & 0x20u) != 0u) {
        out.source = EbdSelectorSource::FixedTwoWithClass80Helpers;
        out.selector_is_known = true;
        out.selector = 2;
        out.spawn_class80_helper_pair = true;
        out.helper_param3_a = 5;
        out.helper_param3_b = 6;
        return out;
    }

    const uint8_t low = static_cast<uint8_t>(p3 & 0x0fu);
    if (low == 4u || low == 5u || low == 6u) {
        out.source = EbdSelectorSource::FixedLowNibble;
        out.selector_is_known = true;
        out.selector = low;
        return out;
    }

    out.source = EbdSelectorSource::RuntimeProbeZeroOrOne;
    out.selector_is_known = false;
    return out;
}

uint8_t ebd_selector_from_probe_80047354(int32_t probe_value, int16_t work_3a) {
    const int32_t delta = probe_value - static_cast<int32_t>(work_3a);
    return delta < 30 ? 0u : 1u;
}

} // namespace l3
