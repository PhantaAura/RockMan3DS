#include "legends3ds/reference_tables.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace { constexpr uint16_t kHeaderBytes=32; }
L3ReferenceHeader inspect_l3reference(const std::vector<uint8_t>&b,bool verify){
    if(b.size()<kHeaderBytes||b[0]!='L'||b[1]!='3'||b[2]!='R'||b[3]!='F')throw std::runtime_error("not L3RF");
    L3ReferenceHeader h;h.version=rd_u16(&b[4]);h.header_bytes=rd_u16(&b[6]);h.flags=rd_u32(&b[8]);h.trig_count=rd_u32(&b[12]);h.off_trig=rd_u32(&b[16]);h.file_bytes=rd_u32(&b[20]);h.payload_crc32=rd_u32(&b[24]);
    if((h.version!=1&&h.version!=kL3ReferenceVersion)||h.header_bytes!=kHeaderBytes||h.trig_count!=kReferenceTrigEntries)throw std::runtime_error("unsupported L3RF");
    const uint64_t trig_end=uint64_t(h.off_trig)+uint64_t(h.trig_count)*4u;
    if(h.version==1){
        if(h.flags!=1u||rd_u32(&b[28])!=0u||h.off_trig!=kHeaderBytes||h.file_bytes!=b.size()||trig_end!=b.size())throw std::runtime_error("L3RF layout mismatch");
    } else {
        h.off_quarter=rd_u32(&b[28]);
        // Pad the 2050-byte quarter table to a 4-byte file length.
        const uint64_t quarter_end=uint64_t(h.off_quarter)+kReferenceQuarterEntries*2u;
        if(h.flags!=3u||h.off_trig!=kHeaderBytes||h.off_quarter!=trig_end||h.file_bytes!=b.size()||
           quarter_end+2u!=b.size()||b[b.size()-1]!=0u||b[b.size()-2]!=0u)throw std::runtime_error("L3RF layout mismatch");
    }
    if (verify && crc32(b.data()+h.header_bytes,b.size()-h.header_bytes) != h.payload_crc32) {
        throw std::runtime_error("L3RF CRC mismatch");
    }
    return h;
}
ReferenceTables load_l3reference(const std::vector<uint8_t>&b,bool verify){
    const auto h=inspect_l3reference(b,verify);ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){const size_t p=h.off_trig+i*4u;r.trig[i].sin_q12=rd_s16(&b[p]);r.trig[i].cos_q12=rd_s16(&b[p+2]);}
    if(h.version>=2){for(size_t i=0;i<r.scalar_quarter.size();++i)r.scalar_quarter[i]=rd_s16(&b[h.off_quarter+i*2u]);r.scalar_quarter_known=true;}
    return r;
}
namespace {
// 0x8007A09C: the non-negative leaf shared by rsin.
int32_t rsin_leaf_8007a09c(const ReferenceTables& r,uint32_t a){
    const auto& q=r.scalar_quarter;
    if(a<2049){return a<1025?q[a]:q[2048-a];}
    return a<3073?-int32_t(q[a-2048]):-int32_t(q[4096-a]);
}
}
int32_t reference_rsin_8007a060(const ReferenceTables& r,int32_t a) noexcept {
    if(a>=0)return rsin_leaf_8007a09c(r,uint32_t(a)&4095u);
    return -rsin_leaf_8007a09c(r,(0u-uint32_t(a))&4095u);
}
int32_t reference_rcos_8007a12c(const ReferenceTables& r,int32_t a) noexcept {
    const uint32_t v=(a<0?0u-uint32_t(a):uint32_t(a))&4095u;
    const auto& q=r.scalar_quarter;
    if(v<2049){return v<1025?q[1024-v]:-int32_t(q[v-1024]);}
    return v<3073?-int32_t(q[3072-v]):q[v-3072];
}
ReferenceMotionDelta reference_player_motion_delta(const ReferenceTables&r,uint16_t yaw,int16_t forward,int16_t vertical,int16_t strafe){const auto&t=r.trig[yaw&0x0FFFu];ReferenceMotionDelta d;d.dx_fixed=int32_t(t.cos_q12)*strafe-int32_t(t.sin_q12)*forward;d.dy_fixed=int32_t(vertical)*4096;d.dz_fixed=-int32_t(t.sin_q12)*strafe-int32_t(t.cos_q12)*forward;return d;}
int16_t reference_fixed_to_short_80030150(int32_t v){const int32_t hi=v>>16;const int32_t round=(v>>15)&1;return static_cast<int16_t>(hi+round);}
} // namespace l3
