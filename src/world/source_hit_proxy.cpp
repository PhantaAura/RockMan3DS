#include "legends3ds/source_hit_proxy.h"
#include "legends3ds/l3map.h"

namespace l3 {
namespace {
std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t i) {
    return static_cast<std::uint16_t>(b[i] | (static_cast<unsigned>(b[i + 1]) << 8U));
}
std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t i) {
    return static_cast<std::uint32_t>(u16(b, i)) |
           (static_cast<std::uint32_t>(u16(b, i + 2)) << 16U);
}
std::int16_t s16(std::uint32_t value) noexcept {
    const auto low = value & 0xffffU;
    return static_cast<std::int16_t>(low < 0x8000U ? static_cast<std::int32_t>(low) :
                                    static_cast<std::int32_t>(low) - 65536);
}
std::int32_t s32(std::uint32_t value) noexcept {
    return value < 0x80000000U ? static_cast<std::int32_t>(value) :
           static_cast<std::int32_t>(static_cast<std::int64_t>(value) - 4294967296LL);
}
bool target_matches(const SourceHitProxyRecord& r) noexcept {
    return r.target && (r.encoded_identity & 0x007fffffU) ==
                       (r.target->source_address & 0x007fffffU);
}
bool overlap(const SourceHitProxyRecord& a, const SourceHitProxyRecord& b) noexcept {
    const std::uint32_t radius = ((a.encoded_extent & 0x1ffU) +
                                 (b.encoded_extent & 0x1ffU)) << 2U;
    std::uint32_t distance2 = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto difference = static_cast<std::int32_t>(a.point[i]) - b.point[i];
        if (static_cast<std::uint32_t>(difference) + radius > radius * 2U) return false;
        const auto bits = static_cast<std::uint32_t>(difference);
        distance2 += bits * bits;
    }
    return s32(distance2) <= s32(radius * radius);
}
void emit(const SourceHitProxyRecord& own, const SourceHitProxyRecord& other) noexcept {
    auto& target = *own.target;
    auto packet = target.packet_00 | (other.parallel_word & 0xff000000U);
    const auto other_mode = static_cast<unsigned>(other.encoded_extent >> 13U);
    const auto selected = (own.parallel_word & 0x00ff0000U) << (other_mode + 8U);
    if (!(selected & 0x80000000U)) packet += other.parallel_word & 0xffffU;
    target.packet_00 = packet;
    if (own.encoded_identity & 0x00800000U) {
        target.point_08 = other.point;
        target.word_0e = 1;
    }
}
} // namespace

bool decode_source_hit_proxy_rules(const std::vector<std::uint8_t>& bytes,
                                   SourceHitProxyRules& out) {
    if (bytes.size() != 32 || bytes[0] != 'L' || bytes[1] != '3' ||
        bytes[2] != 'P' || bytes[3] != 'R' || u16(bytes, 4) != 1 ||
        u16(bytes, 6) != 24 || u32(bytes, 8) != 32 || u32(bytes, 12) != 8 ||
        u32(bytes, 16) != crc32(bytes.data() + 24, 8) || u32(bytes, 20) != 0) return false;
    SourceHitProxyRules next;
    for (std::size_t i = 0; i < 8; ++i) next.unsigned_80097d00[i] = bytes[24 + i];
    out = next;
    return true;
}

SourceHitProxyLeaseResult source_hit_proxy_allocate_lease_8003545c(
    std::optional<std::uint32_t>& mask,
    std::array<std::uint32_t,24>& owners, std::uint32_t owner,
    SourceHitProxyTarget& target) noexcept {
    if (!mask) return {};
    // Original unsigned fast full test includes all low-byte mask values.
    if (*mask >= 0xffffff00U) return {SourceHitProxyStatus::Full,-1};
    for (std::size_t i=0; i<owners.size(); ++i) {
        const auto bit=0x80000000U>>i;
        if (*mask&bit) continue;
        *mask|=bit;
        target.flags_04|=bit;
        owners[i]=owner;
        return {SourceHitProxyStatus::SourceBackedPartial,static_cast<std::int32_t>(i)};
    }
    return {SourceHitProxyStatus::Full,-1};
}
SourceHitProxyStatus source_hit_proxy_release_lease_800354e0(
    std::optional<std::uint32_t>& mask,std::uint32_t index) noexcept {
    if (!mask) return SourceHitProxyStatus::NeedsMask;
    *mask&=~(0x80000000U>>(index&31U));
    return SourceHitProxyStatus::SourceBackedPartial;
}

SourceHitProxyResult source_hit_proxy_check_register_800341bc(
    const SourceHitProxyBank& bank, const SourceHitProxyRegistration& input) noexcept {
    if (bank.count > bank.records.size()) return {SourceHitProxyStatus::InvalidBank, 0, 0};
    if (bank.count == bank.records.size()) return {SourceHitProxyStatus::Full, 0, 0};
    if (!input.target) return {SourceHitProxyStatus::NeedsTarget, 0, 0};
    if (!input.mask_80098798) return {SourceHitProxyStatus::NeedsMask, 0, 0};
    auto masked = input.target->flags_04 & *input.mask_80098798;
    if (masked & 0x80U) masked &= 0xffffff00U;
    for (std::size_t i = 0; i < 24; ++i) {
        if ((masked & (0x80000000U >> i)) && !input.owners_800be478[i])
            return {SourceHitProxyStatus::NeedsOwnerPoint, 0, 0};
    }
    return {SourceHitProxyStatus::SourceBackedPartial, 0, 0};
}

SourceHitProxyResult source_hit_proxy_register_800341bc(
    SourceHitProxyBank& bank, const SourceHitProxyRegistration& input) noexcept {
    const auto checked=source_hit_proxy_check_register_800341bc(bank,input);
    if(checked.status!=SourceHitProxyStatus::SourceBackedPartial)return checked;
    auto masked=input.target->flags_04 & *input.mask_80098798;
    if(masked & 0x80U)masked &= 0xffffff00U;
    SourceHitProxyRecord row;
    row.point = {s16(input.pointed_words[0]), s16(input.pointed_words[0] >> 16U),
                 s16(input.pointed_words[1])};
    row.encoded_extent = static_cast<std::uint16_t>((input.a1 >> 2U) | input.a2 | input.a3);
    row.encoded_identity = (input.stack10 << 24U) |
                          (input.target->source_address & 0x007fffffU) | input.stack1c;
    row.parallel_word = input.stack18;
    row.target = input.target;
    bank.records[bank.count++] = row;
    input.target->flags_04 = masked | input.stack20;
    for (std::size_t i = 0; i < 24; ++i) {
        if (masked & (0x80000000U >> i))
            input.owners_800be478[i]->raw_bc_words = input.pointed_words;
    }
    return {SourceHitProxyStatus::SourceBackedPartial, 0, 0};
}

SourceHitProxyResult source_hit_proxy_check_register_composite_8003430c(
    const SourceHitProxyBank& bank,const SourceHitProxyCompositeRegistration& input) noexcept {
    if(bank.count>bank.records.size())return {SourceHitProxyStatus::InvalidBank,0,0};
    if(bank.count>=63)return {SourceHitProxyStatus::Full,0,0};
    if(!input.target)return {SourceHitProxyStatus::NeedsTarget,0,0};
    if(!input.mask_80098798)return {SourceHitProxyStatus::NeedsMask,0,0};
    const uint32_t masked=input.target->flags_04&*input.mask_80098798;
    if(masked&0xffffff00u){
        if(!input.midpoint_padding_06)return {SourceHitProxyStatus::NeedsOwnerPointPadding,0,0};
        for(size_t i=0;i<24;++i)if((masked&(0x80000000u>>i))&&!input.owners_800be478[i])return {SourceHitProxyStatus::NeedsOwnerPoint,0,0};
    }
    return {SourceHitProxyStatus::SourceBackedPartial,0,0};
}
SourceHitProxyResult source_hit_proxy_register_composite_8003430c(
    SourceHitProxyBank& bank,const SourceHitProxyCompositeRegistration& input) noexcept {
    const auto checked=source_hit_proxy_check_register_composite_8003430c(bank,input);
    if(checked.status!=SourceHitProxyStatus::SourceBackedPartial)return checked;
    const uint32_t masked=input.target->flags_04&*input.mask_80098798;
    SourceHitProxyRecord first,second;
    first.point={{s16(input.first_words[0]),s16(input.first_words[0]>>16),s16(input.first_words[1])}};
    second.point={{s16(input.second_words[0]),s16(input.second_words[0]>>16),s16(input.second_words[1])}};
    first.encoded_extent=uint16_t((input.first_extent>>2)|input.stack10|input.stack14);
    second.encoded_extent=uint16_t((input.second_extent>>2)|input.stack10|input.stack14);
    first.encoded_identity=second.encoded_identity=(input.stack18<<24)|(input.target->source_address&0x007fffffu)|input.stack24;
    first.parallel_word=second.parallel_word=input.stack20;first.target=second.target=input.target;
    std::array<uint32_t,2> midpoint{};
    if(masked&0xffffff00u){
        std::array<int16_t,3> point{};for(size_t i=0;i<3;++i){const int32_t sum=int32_t(first.point[i])+second.point[i];point[i]=int16_t(sum>=0?sum/2:-((-sum+1)/2));}
        midpoint={{uint32_t(uint16_t(point[0]))|(uint32_t(uint16_t(point[1]))<<16),uint32_t(uint16_t(point[2]))|(uint32_t(*input.midpoint_padding_06)<<16)}};
    }
    bank.records[bank.count++]=first;bank.records[bank.count++]=second;
    input.target->flags_04=masked|input.stack28;
    for(size_t i=0;i<24;++i)if(masked&(0x80000000u>>i))input.owners_800be478[i]->raw_bc_words=midpoint;
    return {SourceHitProxyStatus::SourceBackedPartial,0,0};
}
namespace {
unsigned shape(const SourceHitProxyRecord& r) noexcept {return (r.encoded_extent>>9)&3u;}
unsigned mode(const SourceHitProxyRecord& r) noexcept {return r.encoded_extent>>13;}
uint32_t radius(const SourceHitProxyRecord& r) noexcept {return uint32_t(r.encoded_extent&0x1ffu)<<2;}
bool paired(const SourceHitProxyBank& bank,size_t at,size_t count) noexcept {
    if(at+1>=count)return false;
    const auto& first=bank.records[at];const auto& second=bank.records[at+1];
    return first.encoded_identity==second.encoded_identity&&first.target==second.target&&first.parallel_word==second.parallel_word&&
        (first.encoded_extent&0xfe00u)==(second.encoded_extent&0xfe00u);
}
struct SampledPair {
    SourceHitProxyStatus status=SourceHitProxyStatus::SourceBackedPartial;
    size_t first=0,second=0;unsigned first_shape=0,second_shape=0,second_weight=0;
    bool contact=false;
};
SampledPair sampled_pair(const SourceHitProxyBank& bank,size_t a,size_t b,size_t count,
    const SourceHitProxyCompositeScratch* scratch) noexcept {
    SampledPair out;out.first=a;out.second=b;const auto& first=bank.records[a];const auto& second=bank.records[b];
    const auto ka=shape(first),kb=shape(second);out.first_shape=ka;out.second_shape=kb;
    if((ka&&!paired(bank,a,count))||(kb&&!paired(bank,b,count))){out.status=SourceHitProxyStatus::NeedsCompositeShape;return out;}
    // The original caller indices advance even when the first-point AABB fails.
    out.first+=ka?1:0;out.second+=kb?1:0;
    const uint32_t broad_radius=radius(first)+radius(second);
    for(size_t i=0;i<3;++i){const int32_t d=int32_t(first.point[i])-second.point[i];if(uint32_t(d)+broad_radius>broad_radius*2u)return out;}
    if(!ka&&!kb){out.contact=overlap(first,second);return out;}
    const auto& last_a=bank.records[out.first];const auto& last_b=bank.records[out.second];
    const unsigned step_a=ka?(8u>>ka):9u,step_b=kb?(8u>>kb):9u;
    for(unsigned wa=0;wa<9;wa+=step_a)for(unsigned wb=0;wb<9;wb+=step_b){
        uint32_t distance2=0;
        for(size_t i=0;i<3;++i){const int32_t d=(int32_t(first.point[i])*int32_t(8-wa)+int32_t(last_a.point[i])*int32_t(wa)-
            int32_t(second.point[i])*int32_t(8-wb)-int32_t(last_b.point[i])*int32_t(wb))/8;
            distance2+=uint32_t(d)*uint32_t(d);}
        const uint32_t r=(radius(first)*(8-wa)+radius(last_a)*wa+radius(second)*(8-wb)+radius(last_b)*wb)/8;
        if(s32(distance2)>s32(r*r))continue;
        out.contact=true;out.second_weight=wb;
        // Original80034D8C uses the second shape's weight for a sphere's
        // emitted absolute point. Its unowned second stack endpoint matters
        // only on this source branch; the normal Buster target has no abs bit.
        if(!ka&&wb&&(last_b.encoded_identity&0x00800000u)&&
            (!scratch||scratch->first_record!=a||scratch->second_record!=b||!scratch->first_unowned_second_point)){
            out.status=SourceHitProxyStatus::NeedsCompositePointScratch;
        }
        return out;
    }
    return out;
}
void emit_sampled(const SourceHitProxyBank& bank,size_t original_a,size_t original_b,
    const SampledPair& pair,const SourceHitProxyCompositeScratch* scratch) noexcept {
    const auto& a=bank.records[pair.first];const auto& b=bank.records[pair.second];
    emit(a,b);emit(b,a); // Packet arithmetic/ordered alias accumulation unchanged.
    if(a.encoded_identity&0x00800000u){
        if(pair.second_shape){for(size_t i=0;i<3;++i)a.target->point_08[i]=s16(uint32_t(int32_t(bank.records[original_b].point[i])-b.point[i]));a.target->word_0e=0;}
        else {a.target->point_08=bank.records[original_b].point;a.target->word_0e=1;}
    }
    if(b.encoded_identity&0x00800000u){
        if(pair.first_shape){for(size_t i=0;i<3;++i)b.target->point_08[i]=s16(uint32_t(int32_t(bank.records[original_a].point[i])-a.point[i]));b.target->word_0e=0;}
        else {
            const auto& first=bank.records[original_a].point;
            for(size_t i=0;i<3;++i){const int32_t last=pair.second_weight?(*scratch->first_unowned_second_point)[i]:0;
                b.target->point_08[i]=s16(uint32_t((int32_t(first[i])*int32_t(8-pair.second_weight)+last*int32_t(pair.second_weight))/8));}
            b.target->word_0e=1;
        }
    }
}
}
SourceHitProxyResult source_hit_proxy_consume_pairs_8003450c(
    SourceHitProxyBank& bank,const std::optional<SourceHitProxyRules>& rules,
    const SourceHitProxyCompositeScratch* scratch) noexcept {
    if(bank.count>bank.records.size())return {SourceHitProxyStatus::InvalidBank,0,0};
    if(!rules)return {SourceHitProxyStatus::NeedsRules,0,0};
    const size_t count=bank.count;
    for(size_t i=0;i<count;++i){
        const auto& r=bank.records[i];if(!target_matches(r))return {SourceHitProxyStatus::NeedsTarget,0,0};
        for(size_t j=0;j<i;++j)if(((r.encoded_identity^bank.records[j].encoded_identity)&0x007fffffu)==0&&r.target!=bank.records[j].target)return {SourceHitProxyStatus::NeedsTarget,0,0};
    }
    // Validate producer grouping, including a trailing malformed single row.
    for(size_t i=0;i<count;++i)if(shape(bank.records[i])){if(!paired(bank,i,count))return {SourceHitProxyStatus::NeedsCompositeShape,0,0};++i;}
    // Atomic preflight follows the original mutable raw indices. It catches
    // later ownership/scratch holds before resetting count or publishing hits.
    for(size_t i=0;i+1<count;++i)for(size_t j=i+1;j<count;++j){
        const auto& a=bank.records[i];const auto& b=bank.records[j];
        if(!((uint32_t(rules->unsigned_80097d00[mode(a)])<<mode(b))&0x80u)||!((a.encoded_identity^b.encoded_identity)&0xff000000u))continue;
        const auto pair=sampled_pair(bank,i,j,count,scratch);if(pair.status!=SourceHitProxyStatus::SourceBackedPartial)return {pair.status,0,0};i=pair.first;j=pair.second;
    }
    SourceHitProxyResult result{SourceHitProxyStatus::SourceBackedPartial,0,0};bank.count=0;
    for(size_t i=0;i+1<count;++i)for(size_t j=i+1;j<count;++j){
        const auto& a=bank.records[i];const auto& b=bank.records[j];
        if(!((uint32_t(rules->unsigned_80097d00[mode(a)])<<mode(b))&0x80u)||!((a.encoded_identity^b.encoded_identity)&0xff000000u))continue;
        ++result.admitted_pairs;const auto original_a=i,original_b=j;
        const auto pair=sampled_pair(bank,i,j,count,scratch);i=pair.first;j=pair.second;
        if(!pair.contact)continue;
        ++result.overlapping_pairs;
        emit_sampled(bank,original_a,original_b,pair,scratch);
    }
    return result;
}
} // namespace l3
