#include "legends3ds/source_player_proxy.h"
#include <algorithm>
#include <limits>

namespace l3 {
namespace {
std::int32_t s16(std::uint32_t x) noexcept {
    x &= 65535U;
    return x<32768U ? static_cast<std::int32_t>(x) : static_cast<std::int32_t>(x)-65536;
}
std::int32_t s32(std::uint32_t x) noexcept {
    return x<0x80000000U ? static_cast<std::int32_t>(x) :
        static_cast<std::int32_t>(static_cast<std::int64_t>(x)-4294967296LL);
}
std::int64_t sar(std::int64_t x,unsigned shift) noexcept {
    const auto unit=std::int64_t{1}<<shift;
    return x>=0 ? x/unit : -((-x+unit-1)/unit);
}
bool mac_domain(std::int64_t x) noexcept {
    return x>=-(std::int64_t{1}<<43) && x<(std::int64_t{1}<<43);
}
std::int64_t clamp_flag(std::int64_t x,std::int64_t lo,std::int64_t hi,
                        std::uint32_t bit,std::uint32_t& flags) noexcept {
    if(x<lo){flags|=bit;return lo;}
    if(x>hi){flags|=bit;return hi;}
    return x;
}
void mac0_flags(std::int64_t x,std::uint32_t& flags) noexcept {
    if(x>std::numeric_limits<std::int32_t>::max())flags|=1U<<16;
    if(x<std::numeric_limits<std::int32_t>::min())flags|=1U<<15;
}
} // namespace

std::uint32_t source_gte_unr_divide(std::uint16_t h,std::uint16_t sz,
                                  bool& divide_overflow) noexcept {
    divide_overflow=static_cast<std::uint32_t>(h)>=static_cast<std::uint32_t>(sz)*2U;
    if(divide_overflow)return 0x1ffffU;
    unsigned shift=0;
    for(auto probe=static_cast<std::uint32_t>(sz);probe<0x8000U;probe<<=1U)++shift;
    const auto normalized=static_cast<std::uint32_t>(sz)<<shift;
    const auto index=(normalized-0x7fc0U)>>7U;
    // Generate the hardware lookup scalar, rather than copying a retail table.
    const auto quotient=((0x40000U/(index+0x100U))+1U)/2U;
    const auto table=quotient>0x101U ? quotient-0x101U : 0U;
    const auto u=table+0x101U;
    const auto first=(0x2000080U-normalized*u)>>8U;
    const auto reciprocal=(0x80U+first*u)>>8U;
    const auto numerator=static_cast<std::uint64_t>(h)<<shift;
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(0x1ffffU,
        (numerator*reciprocal+0x8000U)>>16U));
}

SourceGteProjectionResult source_gte_rtps_0180001(SourceGteRtpsState& state) noexcept {
    if(!state.matrix_translation_bound)return {};
    if(!state.projection_controls_bound)return {SourcePlayerProxyStatus::NeedsProjectionControls};
    std::array<std::int32_t,9> matrix{};
    for(std::size_t i=0;i<9;++i)matrix[i]=s16(state.control[i/2]>>(16U*(i&1U)));
    const std::array<std::int32_t,3> vertex{{s16(state.data[0]),s16(state.data[0]>>16U),s16(state.data[1])}};
    std::array<std::int64_t,3> transformed{};
    for(std::size_t row=0;row<3;++row){
        auto raw=static_cast<std::int64_t>(s32(state.control[5+row]))*4096;
        if(!mac_domain(raw))return {SourcePlayerProxyStatus::NeedsBoundedArithmetic};
        for(std::size_t col=0;col<3;++col){
            raw+=static_cast<std::int64_t>(matrix[row*3+col])*vertex[col];
            if(!mac_domain(raw))return {SourcePlayerProxyStatus::NeedsBoundedArithmetic};
        }
        transformed[row]=sar(raw,12);
    }
    auto next=state;
    std::uint32_t flags=0;
    std::array<std::int64_t,3> ir{};
    for(std::size_t i=0;i<3;++i){
        ir[i]=clamp_flag(transformed[i],-32768,32767,1U<<(24U-i),flags);
        next.data[9+i]=static_cast<std::uint32_t>(ir[i]);
        next.data[25+i]=static_cast<std::uint32_t>(transformed[i]);
    }
    const auto depth=static_cast<std::uint16_t>(clamp_flag(transformed[2],0,65535,1U<<18,flags));
    for(std::size_t i=16;i<19;++i)next.data[i]=state.data[i+1];
    next.data[19]=depth;
    bool overflow=false;
    const auto reciprocal=source_gte_unr_divide(static_cast<std::uint16_t>(state.control[26]),depth,overflow);
    if(overflow)flags|=1U<<17;
    std::array<std::int64_t,2> screen{};
    for(std::size_t i=0;i<2;++i){
        const auto raw=static_cast<std::int64_t>(reciprocal)*ir[i]+s32(state.control[24+i]);
        mac0_flags(raw,flags);
        screen[i]=clamp_flag(sar(raw,16),-1024,1023,1U<<(14U-i),flags);
    }
    const auto packed=static_cast<std::uint32_t>(static_cast<std::uint16_t>(screen[0]))|
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(screen[1]))<<16U);
    next.data[12]=state.data[13];next.data[13]=state.data[14];next.data[14]=packed;
    // SXYP's architectural read mirrors14; raw software backing15 is retained.
    const auto raw0=static_cast<std::int64_t>(reciprocal)*s16(state.control[27])+s32(state.control[28]);
    mac0_flags(raw0,flags);next.data[24]=static_cast<std::uint32_t>(raw0);
    next.data[8]=static_cast<std::uint32_t>(clamp_flag(sar(raw0,12),0,4096,1U<<12,flags));
    if(flags&0x7f87e000U)flags|=0x80000000U;
    next.control[31]=flags;state=next;
    return {SourcePlayerProxyStatus::SourceBackedPartial,packed,depth,reciprocal,flags};
}

std::optional<std::uint32_t> source_gte_read_data(
    const SourceGteRtpsState& state,std::size_t index) noexcept {
    if(index>=32 || index==28 || index==29)return std::nullopt;
    if(index==15)return state.data[14];
    if(index==1 || index==3 || index==5 || (index>=8 && index<=11))
        return static_cast<std::uint32_t>(s16(state.data[index]));
    if(index==7 || (index>=16 && index<=19))return state.data[index]&65535U;
    return state.data[index];
}

SourcePlayerLeaseResult source_player_consume_leases_80035508(
    SourceGteRtpsState& state,const SourcePlayerLeaseInputs& input) noexcept {
    SourcePlayerLeaseResult result;
    if(!input.scratch_camera_1f800008)return result;
    if(!input.mask_80098798){result.status=SourcePlayerProxyStatus::NeedsMask;return result;}
    auto next=state;
    for(std::size_t i=0;i<8;++i)next.control[i]=(*input.scratch_camera_1f800008)[i];
    next.control[4]=static_cast<std::uint32_t>(s16(next.control[4]));
    next.matrix_translation_bound=true;
    const auto mask=*input.mask_80098798&0xffffff00U;
    for(std::size_t i=0;i<24;++i){
        const auto bit=0x80000000U>>i;
        if(!(mask&bit))continue;
        const auto owner=input.owners_800be478[i];
        if(!owner)return {SourcePlayerProxyStatus::NeedsOwnerPoint};
        // Repeated source pointers see the earlier clear, not the old lease point.
        bool cleared=false;
        for(std::size_t prior=0;prior<i;++prior)
            if((mask&(0x80000000U>>prior)) && input.owners_800be478[prior]==owner)cleared=true;
        next.data[0]=cleared ? 0U : owner->raw_bc_words[0];
        next.data[1]=static_cast<std::uint32_t>(s16(cleared ? 0U : owner->raw_bc_words[1]));
        const auto projection=source_gte_rtps_0180001(next);
        if(projection.status!=SourcePlayerProxyStatus::SourceBackedPartial)return {projection.status};
        result.screen_xy[i]=projection.screen_xy;result.depth[i]=projection.depth;
        result.consumed_mask|=bit;++result.projected;
    }
    state=next;
    for(std::size_t i=0;i<24;++i)if(mask&(0x80000000U>>i))input.owners_800be478[i]->raw_bc_words={{0,0}};
    result.status=SourcePlayerProxyStatus::SourceBackedPartial;
    return result;
}

} // namespace l3
