#include "legends3ds/source_camera.h"

namespace l3 {
namespace {
std::int32_t s32(std::uint32_t value) noexcept {
    return value<0x80000000U ? static_cast<std::int32_t>(value) :
        static_cast<std::int32_t>(static_cast<std::int64_t>(value)-4294967296LL);
}
std::int16_t s16(std::uint32_t value) noexcept {
    value&=65535U;
    return static_cast<std::int16_t>(value<32768U ? static_cast<std::int32_t>(value) :
                                    static_cast<std::int32_t>(value)-65536);
}
std::int32_t asr32(std::uint32_t value,unsigned count) noexcept {
    const auto shifted=value>>count;
    return s32(value&0x80000000U ? shifted|(0xffffffffU<<(32U-count)) : shifted);
}
std::int64_t asr12(std::int64_t value) noexcept {
    return value>=0 ? value/4096 : -((-value+4095)/4096);
}
std::int32_t mul12(std::int32_t a,std::int32_t b) noexcept {
    return asr32(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b),12);
}
std::uint32_t signed_short_control(std::uint32_t value) noexcept {
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(s16(value)));
}
SourceCameraDirectResult hold(SourceCameraStatus status) noexcept {
    SourceCameraDirectResult result;result.status=status;return result;
}
} // namespace

std::array<std::int16_t,9> source_rot_matrix_yx_transposed_8007b838(const ReferenceTables& reference,
    std::uint32_t pitch,std::uint32_t yaw) noexcept {
    const auto x=reference.trig[(0U-pitch)&4095U];
    const auto y=reference.trig[yaw&4095U];
    const auto z=reference.trig[0];
    const std::int32_t sx=x.sin_q12,cx=x.cos_q12,sy=y.sin_q12,cy=y.cos_q12;
    const std::int32_t sz=z.sin_q12,cz=z.cos_q12;
    const auto a=mul12(sy,sx),b=mul12(cy,sx);
    const std::array<std::uint32_t,9> full{{
        static_cast<std::uint32_t>(mul12(cy,cz))+static_cast<std::uint32_t>(mul12(a,sz)),
        0U-static_cast<std::uint32_t>(mul12(cy,sz))+static_cast<std::uint32_t>(mul12(a,cz)),
        static_cast<std::uint32_t>(mul12(sy,cx)),
        static_cast<std::uint32_t>(mul12(sz,cx)),
        static_cast<std::uint32_t>(mul12(cz,cx)),
        0U-static_cast<std::uint32_t>(sx),
        0U-static_cast<std::uint32_t>(mul12(sy,cz))+static_cast<std::uint32_t>(mul12(b,sz)),
        static_cast<std::uint32_t>(mul12(sy,sz))+static_cast<std::uint32_t>(mul12(b,cz)),
        static_cast<std::uint32_t>(mul12(cy,cx))}};
    std::array<std::int16_t,9> result{};
    for(std::size_t row=0;row<3;++row)
        for(std::size_t col=0;col<3;++col)result[row*3+col]=s16(full[col*3+row]);
    return result;
}
std::array<std::uint32_t,3> source_apply_matrix_lv_8007a9b8(const std::array<std::int16_t,9>& matrix,
    const std::array<std::uint32_t,3>& vector) noexcept {
    std::array<std::int16_t,3> high{},low{};
    for(std::size_t i=0;i<3;++i){
        // The helper splits signed magnitude; INT_MIN retains its original wrap.
        const auto value=vector[i];
        const bool negative=s32(value)<0;
        const auto magnitude=negative ? 0U-value : value;
        const auto h=static_cast<std::uint32_t>(asr32(magnitude,15));
        high[i]=s16(negative ? 0U-h : h);
        low[i]=s16(negative ? 0U-(magnitude&32767U) : magnitude&32767U);
    }
    std::array<std::uint32_t,3> result{};
    for(std::size_t row=0;row<3;++row){
        std::int64_t h=0,l=0;
        for(std::size_t col=0;col<3;++col){
            h+=static_cast<std::int64_t>(matrix[row*3+col])*high[col];
            l+=static_cast<std::int64_t>(matrix[row*3+col])*low[col];
        }
        // MVMVA(sf0) returns low32 MAC, then CPU shifts that word by3.
        // MVMVA(sf1) returns floor(sum/4096); three s16 products fit44bits.
        result[row]=(static_cast<std::uint32_t>(h)<<3U)+static_cast<std::uint32_t>(asr12(l));
    }
    return result;
}
std::array<std::uint32_t,8> source_pack_camera_words(const std::array<std::int16_t,9>& matrix,
    std::uint16_t retained_pad_1f8000f2,const std::array<std::uint32_t,3>& translation) noexcept {
    std::array<std::uint32_t,8> words{};
    for(std::size_t i=0;i<4;++i)words[i]=static_cast<std::uint16_t>(matrix[2*i])|
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(matrix[2*i+1]))<<16U);
    words[4]=static_cast<std::uint16_t>(matrix[8])|(static_cast<std::uint32_t>(retained_pad_1f8000f2)<<16U);
    for(std::size_t i=0;i<3;++i)words[5+i]=translation[i];
    return words;
}

void source_projection_init_geom_8007a4f0(SourceProjectionControls& owner) noexcept {
    owner.h=signed_short_control(1000U);owner.dqa=signed_short_control(0U-4194U);
    owner.dqb=0x01400000U;owner.ofx=0U;owner.ofy=0U;
}
void source_projection_offset_8007b118(SourceProjectionControls& owner,
    std::uint32_t x,std::uint32_t y) noexcept {owner.ofx=x<<16U;owner.ofy=y<<16U;}
void source_projection_h_8007b138(SourceProjectionControls& owner,
    std::uint32_t value) noexcept {owner.h=signed_short_control(value);}
void source_projection_dqa_8007b0c0(SourceProjectionControls& owner,
    std::uint32_t value) noexcept {owner.dqa=signed_short_control(value);}
void source_projection_dqb_8007b0cc(SourceProjectionControls& owner,
    std::uint32_t value) noexcept {owner.dqb=value;}
bool source_projection_fog_8007a1cc(SourceProjectionControls& owner,
    std::uint32_t distance,std::uint32_t denominator) noexcept {
    const auto numerator=s32(0U-(((distance<<2U)+distance)<<6U));
    const auto divisor=s32(denominator);
    if(!divisor || (numerator==(-2147483647-1) && divisor==-1))return false;
    const auto quotient=static_cast<std::int32_t>(static_cast<std::int64_t>(numerator)/divisor);
    source_projection_dqa_8007b0c0(owner,static_cast<std::uint32_t>(quotient));
    source_projection_dqb_8007b0cc(owner,0x01400000U);
    return true;
}
bool bind_source_projection_controls(SourceGteRtpsState& state,
    const SourceProjectionControls& owner) noexcept {
    if(!owner.ofx || !owner.ofy || !owner.h || !owner.dqa || !owner.dqb)return false;
    state.control[24]=*owner.ofx;state.control[25]=*owner.ofy;state.control[26]=*owner.h;
    state.control[27]=*owner.dqa;state.control[28]=*owner.dqb;
    state.projection_controls_bound=true;return true;
}
SourceCameraDirectResult produce_source_camera_800384f0(
    const SourceCameraDirectInputs& inputs) noexcept {
    using Status=SourceCameraStatus;
    if(!inputs.reference)return hold(Status::NeedsReference);
    if(!inputs.eye)return hold(Status::NeedsEye);
    if(!inputs.yaw || !inputs.pitch)return hold(Status::NeedsAngles);
    if(!inputs.retained_matrix_padding_1f8000f2)return hold(Status::NeedsRetainedMatrixPadding);
    const auto matrix=source_rot_matrix_yx_transposed_8007b838(*inputs.reference,
        static_cast<std::uint32_t>(*inputs.pitch),static_cast<std::uint32_t>(*inputs.yaw));
    // 0x800384F0 negates the eye with low32 wrap before ApplyMatrixLV.
    std::array<std::uint32_t,3> negated{};
    for(std::size_t i=0;i<3;++i)negated[i]=0U-static_cast<std::uint32_t>((*inputs.eye)[i]);
    const auto words=source_pack_camera_words(matrix,*inputs.retained_matrix_padding_1f8000f2,
        source_apply_matrix_lv_8007a9b8(matrix,negated));
    std::array<std::int16_t,3> short_eye{};
    std::array<std::int32_t,3> mirror_eye{};
    for(std::size_t i=0;i<3;++i){short_eye[i]=s16(static_cast<std::uint32_t>((*inputs.eye)[i]));
        mirror_eye[i]=short_eye[i];}
    SourceCameraDirectResult result;
    result.status=Status::SourceBackedPartial;result.scratch_camera_1f800008=words;
    result.scratch_eye_1f800028=short_eye;result.eye_800a39f0=mirror_eye;
    result.rotation_800a3a10={{s16((0U-static_cast<std::uint32_t>(*inputs.pitch))&4095U),
                             s16(static_cast<std::uint32_t>(*inputs.yaw)&4095U),0}};
    return result;
}

} // namespace l3
