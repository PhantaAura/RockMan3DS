#include "legends3ds/actor_pose_runtime.h"

namespace l3 {
namespace {
int16_t short_bits(uint32_t value) noexcept {
    const auto low = value & 65535U;
    return static_cast<int16_t>(low < 32768U ? static_cast<int32_t>(low) :
                               static_cast<int32_t>(low) - 65536);
}
int32_t signed_bits(uint32_t value) noexcept {
    return value < 0x80000000U ? static_cast<int32_t>(value) :
        static_cast<int32_t>(static_cast<int64_t>(value) - 4294967296LL);
}
int32_t asr32(uint32_t value, unsigned count) noexcept {
    count &= 31U;
    if (!count) return signed_bits(value);
    const uint32_t shifted = value >> count;
    return signed_bits(value & 0x80000000U ? shifted | (0xffffffffU << (32U-count)) : shifted);
}
int32_t asr12(int64_t value) noexcept {
    return static_cast<int32_t>(value >= 0 ? value / 4096 : -((-value + 4095) / 4096));
}
uint32_t product(int32_t a, int32_t b) noexcept {
    return static_cast<uint32_t>(a) * static_cast<uint32_t>(b);
}
struct Trig { int32_t s, c; };
Trig angle(const ReferenceTables& reference, uint16_t bits) noexcept {
    const int32_t value = short_bits(bits);
    const auto& entry = reference.trig[static_cast<uint32_t>(value < 0 ? -value : value) & 4095U];
    return {value < 0 ? -static_cast<int32_t>(entry.sin_q12) : entry.sin_q12, entry.cos_q12};
}
std::array<int16_t,9> heading_matrix(const ReferenceTables& reference, uint16_t heading) noexcept {
    // The original routine consumes X=Z=0 but still reads trig[0]. Retaining
    // its CPU product order also handles authored/noncanonical tables exactly.
    const auto x = angle(reference, 0), y = angle(reference, heading), z = angle(reference, 0);
    std::array<int16_t,9> matrix{};
    matrix[2] = short_bits(static_cast<uint32_t>(y.s));
    matrix[5] = short_bits(static_cast<uint32_t>(asr32(0U-product(y.c,x.s),12)));
    matrix[8] = short_bits(static_cast<uint32_t>(asr32(product(y.c,x.c),12)));
    matrix[0] = short_bits(static_cast<uint32_t>(asr32(product(z.c,y.c),12)));
    matrix[1] = short_bits(static_cast<uint32_t>(asr32(0U-product(z.s,y.c),12)));
    auto intermediate = asr32(product(z.c,-y.s),12);
    matrix[3] = short_bits(static_cast<uint32_t>(asr32(product(z.s,x.c),12)) -
                           static_cast<uint32_t>(asr32(product(intermediate,x.s),12)));
    matrix[6] = short_bits(static_cast<uint32_t>(asr32(product(z.s,x.s),12)) +
                           static_cast<uint32_t>(asr32(product(intermediate,x.c),12)));
    intermediate = asr32(product(z.s,-y.s),12);
    matrix[4] = short_bits(static_cast<uint32_t>(asr32(product(z.c,x.c),12)) +
                           static_cast<uint32_t>(asr32(product(intermediate,x.s),12)));
    matrix[7] = short_bits(static_cast<uint32_t>(asr32(product(z.c,x.s),12)) -
                           static_cast<uint32_t>(asr32(product(intermediate,x.c),12)));
    return matrix;
}
ActorPoseCollisionResult hold(ActorPoseCollisionStatus status) noexcept {
    ActorPoseCollisionResult result; result.status = status; return result;
}
} // namespace

ActorPoseCollisionResult produce_actor_pose_collision_point_8003346c(
    const ActorPoseCollisionInputs& inputs) noexcept {
    using Status = ActorPoseCollisionStatus;
    if (!inputs.reference) return hold(Status::NeedsReference);
    if (!inputs.scratch_shift_1f800074) return hold(Status::NeedsScratchShift);
    if ((inputs.flags_06 & 0x40U) && !inputs.root_scale_388) return hold(Status::NeedsRootScale);
    if (!inputs.preserved_186) return hold(Status::NeedsPreservedHalfword);
    const unsigned shift = *inputs.scratch_shift_1f800074 & 31U;
    std::array<int32_t,3> scales{};
    std::array<int16_t,3> root{};
    for (size_t i = 0; i < 3; ++i) {
        const auto scale = static_cast<uint32_t>(static_cast<int32_t>(inputs.scale_158[i]));
        // The ordinary branch narrows the shifted scale via a stack halfword.
        scales[i] = (inputs.flags_06 & 0x40U)
            ? asr32((scale * (*inputs.root_scale_388)[i]) << shift,5)
            : short_bits(scale << shift);
        root[i] = short_bits(static_cast<uint32_t>(static_cast<int32_t>(inputs.root_translation[i])) << 3U);
    }
    ActorPoseCollisionResult result;
    result.status = Status::SourceBackedPartial;
    result.scaled_heading_160 = heading_matrix(*inputs.reference, inputs.heading_56);
    for (size_t i = 0; i < 9; ++i)
        result.scaled_heading_160[i] = short_bits(static_cast<uint32_t>(asr32(
            product(result.scaled_heading_160[i],scales[i%3]),12)));
    std::array<int16_t,4> point{};
    for (size_t i = 0; i < 3; ++i) {
        int64_t raw = 0;
        for (size_t j = 0; j < 3; ++j)
            raw += static_cast<int64_t>(result.scaled_heading_160[i*3+j]) * root[j];
        // Three signed16 products fit the GTE44-bit accumulator and its
        // shifted signed32 MAC output. IR saturation is not the stored output.
        const auto translated = static_cast<uint32_t>(asr12(raw)) +
            (static_cast<uint32_t>(static_cast<int32_t>(inputs.raw_14[i])) << shift);
        point[i] = short_bits(translated);
    }
    point[3] = *inputs.preserved_186;
    result.point_180 = point;
    return result;
}
} // namespace l3
