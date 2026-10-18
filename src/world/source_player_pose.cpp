#include "legends3ds/source_player_pose.h"
#include <algorithm>
#include <cstddef>

namespace l3 {
namespace {
using Matrix = std::array<std::int16_t,9>;

std::int16_t short_bits(std::uint32_t value) noexcept {
    value &= 0xffffu;
    return static_cast<std::int16_t>(value < 0x8000u ? static_cast<std::int32_t>(value)
        : static_cast<std::int32_t>(value) - 0x10000);
}
std::int32_t signed_bits(std::uint32_t value) noexcept {
    return value < 0x80000000u ? static_cast<std::int32_t>(value)
        : static_cast<std::int32_t>(static_cast<std::int64_t>(value)-0x100000000LL);
}
std::int32_t asr32(std::uint32_t value,unsigned count) noexcept {
    count&=31u;if(!count)return signed_bits(value);
    return signed_bits((value>>count)|((value&0x80000000u)?(0xffffffffu<<(32u-count)):0u));
}
std::int32_t asr12(std::int64_t value) noexcept {
    return static_cast<std::int32_t>(value>=0?value/4096:-((-value+4095)/4096));
}
std::uint32_t product(std::int32_t a,std::int32_t b) noexcept {
    return static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b);
}
struct Trig { std::int32_t s=0,c=0; };
Trig angle(const ReferenceTables& reference,std::uint16_t bits) noexcept {
    const auto value=short_bits(bits);
    const auto magnitude=static_cast<std::uint32_t>(value<0?-static_cast<std::int32_t>(value):value)&4095u;
    const auto& entry=reference.trig[magnitude];
    return {value<0?-static_cast<std::int32_t>(entry.sin_q12):entry.sin_q12,entry.cos_q12};
}
Matrix rotate_xyz(const ReferenceTables& reference,std::uint16_t ax,
                  std::uint16_t ay,std::uint16_t az) noexcept {
    const auto x=angle(reference,ax),y=angle(reference,ay),z=angle(reference,az);Matrix m{};
    m[2]=short_bits(static_cast<std::uint32_t>(y.s));
    m[5]=short_bits(static_cast<std::uint32_t>(asr32(0u-product(y.c,x.s),12)));
    m[8]=short_bits(static_cast<std::uint32_t>(asr32(product(y.c,x.c),12)));
    m[0]=short_bits(static_cast<std::uint32_t>(asr32(product(z.c,y.c),12)));
    m[1]=short_bits(static_cast<std::uint32_t>(asr32(0u-product(z.s,y.c),12)));
    auto temporary=asr32(product(z.c,-y.s),12);
    m[3]=short_bits(static_cast<std::uint32_t>(asr32(product(z.s,x.c),12))-
                    static_cast<std::uint32_t>(asr32(product(temporary,x.s),12)));
    m[6]=short_bits(static_cast<std::uint32_t>(asr32(product(z.s,x.s),12))+
                    static_cast<std::uint32_t>(asr32(product(temporary,x.c),12)));
    temporary=asr32(product(z.s,-y.s),12);
    m[4]=short_bits(static_cast<std::uint32_t>(asr32(product(z.c,x.c),12))+
                    static_cast<std::uint32_t>(asr32(product(temporary,x.s),12)));
    m[7]=short_bits(static_cast<std::uint32_t>(asr32(product(z.c,x.s),12))-
                    static_cast<std::uint32_t>(asr32(product(temporary,x.c),12)));
    return m;
}
Matrix multiply(const Matrix& a,const Matrix& b) noexcept {
    Matrix out{};
    for(std::size_t row=0;row<3;++row)for(std::size_t col=0;col<3;++col){
        std::int64_t sum=0;for(std::size_t k=0;k<3;++k)sum+=std::int64_t(a[row*3+k])*b[k*3+col];
        out[row*3+col]=static_cast<std::int16_t>(std::clamp(
            asr12(sum),std::int32_t{-32768},std::int32_t{32767}));
    }
    return out;
}
std::array<std::int32_t,3> apply_matrix(const Matrix& matrix,
                                        const std::array<std::int16_t,3>& vector) noexcept {
    std::array<std::int32_t,3> out{};
    for(std::size_t row=0;row<3;++row){std::int64_t sum=0;
        for(std::size_t col=0;col<3;++col)sum+=std::int64_t(matrix[row*3+col])*vector[col];
        out[row]=asr12(sum);
    }
    return out;
}
SourcePlayerIntegerPose hold(SourcePlayerPoseStatus status) noexcept {
    SourcePlayerIntegerPose out;out.status=status;return out;
}
constexpr std::array<std::int8_t,kPbdMainPosePartCount> kSourceParents{{
    -1,0,0,2,3,0,5,6,0,8,9,10,0,12,13
}};
} // namespace

SourcePlayerIntegerPose produce_source_player_integer_pose_800325d4(
    const SourcePlayerPoseInputs& input) noexcept {
    using Status=SourcePlayerPoseStatus;
    if(!input.skeleton)return hold(Status::NeedsSkeleton);
    if(!input.primary_pose)return hold(Status::NeedsPose);
    if(!input.reference)return hold(Status::NeedsReference);
    if(!input.scratch_shift_1f800074)return hold(Status::NeedsScratchShift);
    if(*input.scratch_shift_1f800074>2)return hold(Status::UnsupportedScratchShift);
    if(!input.limb_pose_selector_106)return hold(Status::NeedsLimbPoseSelector);
    if(*input.limb_pose_selector_106!=0)return hold(Status::NeedsAlternateLimbPose);
    if(!input.player_position_14)return hold(Status::NeedsPlayerPosition);
    if(input.skeleton->parent!=kSourceParents)return hold(Status::InvalidHierarchy);

    const unsigned shift=*input.scratch_shift_1f800074;
    SourcePlayerIntegerPose out;out.scratch_shift=static_cast<std::uint8_t>(shift);
    const auto root_heading=static_cast<std::uint16_t>(input.heading_56-input.heading_offset_116);
    const Matrix actor_rotation=rotate_xyz(*input.reference,0,root_heading,0);
    std::array<std::int32_t,3> actor_translation{};
    std::array<std::int16_t,3> shifted_player{};
    for(std::size_t axis=0;axis<3;++axis){
        const auto bits=static_cast<std::uint32_t>(static_cast<std::int32_t>((*input.player_position_14)[axis]))<<shift;
        shifted_player[axis]=short_bits(bits);actor_translation[axis]=shifted_player[axis];
    }

    for(std::size_t part=0;part<kPbdMainPosePartCount;++part){
        const auto parent=input.skeleton->parent[part];
        const Matrix& parent_rotation=parent<0?actor_rotation:out.parts[static_cast<std::size_t>(parent)].rotation_q12;
        const auto& parent_translation=parent<0?actor_translation:out.parts[static_cast<std::size_t>(parent)].accumulated_translation;
        const auto& source_translation=part==0?input.primary_pose->root_translation:
            input.skeleton->base_translation[part];
        std::array<std::int16_t,3> local{};
        for(std::size_t axis=0;axis<3;++axis)
            local[axis]=short_bits(static_cast<std::uint32_t>(static_cast<std::int32_t>(source_translation[axis]))<<shift);
        const auto delta=apply_matrix(parent_rotation,local);
        auto& result=out.parts[part];
        for(std::size_t axis=0;axis<3;++axis){
            const auto accumulated=static_cast<std::uint32_t>(parent_translation[axis])+
                static_cast<std::uint32_t>(delta[axis]);
            result.accumulated_translation[axis]=signed_bits(accumulated);
            const auto numerator=accumulated+
                static_cast<std::uint32_t>(static_cast<std::int32_t>(shifted_player[axis])*7)+4u;
            result.position_3a8[axis]=short_bits(static_cast<std::uint32_t>(asr32(numerator,3)));
        }
        const auto& rotation=input.primary_pose->part_rotation[part];
        result.rotation_q12=multiply(parent_rotation,
            rotate_xyz(*input.reference,rotation[0],rotation[1],rotation[2]));
    }
    out.part_count=static_cast<std::uint8_t>(kPbdMainPosePartCount);
    out.status=Status::SourceBackedPartial;return out;
}

SourcePlayerMuzzleReceipt source_player_muzzles_800331a0(
    const SourcePlayerIntegerPose& pose) noexcept {
    SourcePlayerMuzzleReceipt out;out.status=pose.status;
    if(pose.status!=SourcePlayerPoseStatus::SourceBackedPartial||
       pose.part_count!=kPbdMainPosePartCount)return out;
    for(std::size_t axis=0;axis<3;++axis){
        out.muzzle_3c8_part4[axis]=static_cast<std::uint16_t>(pose.parts[4].position_3a8[axis]);
        out.muzzle_3e0_part7[axis]=static_cast<std::uint16_t>(pose.parts[7].position_3a8[axis]);
    }
    return out;
}

bool bind_source_player_muzzles_80035724(const SourcePlayerMuzzleReceipt& receipt,
                                         SourceBusterPlayer& player) noexcept {
    if(receipt.status!=SourcePlayerPoseStatus::SourceBackedPartial)return false;
    player.lanes[0].muzzle_3c8_or_3e0=receipt.muzzle_3c8_part4;
    player.lanes[1].muzzle_3c8_or_3e0=receipt.muzzle_3e0_part7;
    return true;
}

} // namespace l3
