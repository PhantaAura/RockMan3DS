#include "legends3ds/class20_03_proxy.h"

namespace l3 {
namespace {
Class20_03ProxyResult hold(Class20_03ProxyStatus status) noexcept {
    Class20_03ProxyResult result; result.status = status; return result;
}
std::array<uint32_t,2> words(const std::array<int16_t,4>& point) noexcept {
    return {{uint32_t(uint16_t(point[0])) | (uint32_t(uint16_t(point[1])) << 16U),
             uint32_t(uint16_t(point[2])) | (uint32_t(uint16_t(point[3])) << 16U)}};
}
} // namespace
Class20_03ProxyResult class20_03_check_proxy_80102398(
    const Class20_03CallbackState& state,const SourceHitProxyBank& bank,
    const Class20_03ProxyInputs& inputs) noexcept {
    using Status = Class20_03ProxyStatus;
    Class20_03ProxyResult result;
    if (!(state.work_00 & 1U) || !(state.behavior.flags_328 & 4U)) {
        result.packet_cleared = true;
        return result;
    }
    if (!inputs.pose_point_180) return hold(Status::NeedsPosePoint);
    if (bank.count > bank.records.size()) return hold(Status::InvalidBank);
    if (bank.count < bank.records.size() && (!inputs.target_33c ||
        inputs.target_33c->packet_00 != state.packet_33c)) return hold(Status::NeedsTarget);
    SourceHitProxyRegistration registration;
    registration.pointed_words = words(*inputs.pose_point_180);
    registration.a1 = 80; registration.a2 = 0; registration.a3 = 0x8000;
    registration.stack10 = inputs.source_work_01;
    registration.stack18 = uint32_t(inputs.producer_330) | 0x04880000U;
    registration.stack1c = 0x00800000U; registration.stack20 = 0;
    registration.target = inputs.target_33c;
    registration.mask_80098798 = inputs.mask_80098798;
    registration.owners_800be478 = inputs.owners_800be478;
    const auto registered = source_hit_proxy_check_register_800341bc(bank,registration);
    switch (registered.status) {
        case SourceHitProxyStatus::NeedsMask: return hold(Status::NeedsMask);
        case SourceHitProxyStatus::NeedsTarget: return hold(Status::NeedsTarget);
        case SourceHitProxyStatus::NeedsOwnerPoint: return hold(Status::NeedsOwnerPoint);
        case SourceHitProxyStatus::Full: result.bank_full = true; break;
        case SourceHitProxyStatus::SourceBackedPartial: result.registered = true; break;
        default: return hold(Status::InvalidBank);
    }
    result.packet_cleared = true;
    return result;
}
Class20_03ProxyResult class20_03_finalize_proxy_80102398(
    Class20_03CallbackState& state,SourceHitProxyBank& bank,
    const Class20_03ProxyInputs& inputs) noexcept {
    const auto checked=class20_03_check_proxy_80102398(state,bank,inputs);
    if(checked.status!=Class20_03ProxyStatus::SourceBackedPartial)return checked;
    if((state.work_00&1U)&&(state.behavior.flags_328&4U)){
        SourceHitProxyRegistration registration;
        registration.pointed_words=words(*inputs.pose_point_180);
        registration.a1=80;registration.a2=0;registration.a3=0x8000;
        registration.stack10=inputs.source_work_01;
        registration.stack18=uint32_t(inputs.producer_330)|0x04880000U;
        registration.stack1c=0x00800000U;registration.stack20=0;
        registration.target=inputs.target_33c;registration.mask_80098798=inputs.mask_80098798;
        registration.owners_800be478=inputs.owners_800be478;
        // Both outcomes admitted by the preflight (registered/full) have the
        // original packet-clear continuation. No concurrent owner runs here.
        (void)source_hit_proxy_register_800341bc(bank,registration);
        state.proxy_position_180=*inputs.pose_point_180;
        state.previous_proxy_position_334=*inputs.pose_point_180;
    }
    state.packet_33c=0;
    if(inputs.target_33c)inputs.target_33c->packet_00=0;
    return checked;
}
} // namespace l3
