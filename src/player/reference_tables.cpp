#include "legends3ds/reference_tables.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace { constexpr uint16_t kHeaderBytes=32; }
L3ReferenceHeader inspect_l3reference(const std::vector<uint8_t>&b,bool verify){
    if(b.size()<kHeaderBytes||b[0]!='L'||b[1]!='3'||b[2]!='R'||b[3]!='F')throw std::runtime_error("not L3RF");
    L3ReferenceHeader h;h.version=rd_u16(&b[4]);h.header_bytes=rd_u16(&b[6]);h.flags=rd_u32(&b[8]);h.trig_count=rd_u32(&b[12]);h.off_trig=rd_u32(&b[16]);h.file_bytes=rd_u32(&b[20]);h.payload_crc32=rd_u32(&b[24]);
    if(h.version!=kL3ReferenceVersion||h.header_bytes!=kHeaderBytes||h.trig_count!=kReferenceTrigEntries)throw std::runtime_error("unsupported L3RF");
    if(rd_u32(&b[28])!=0u||h.off_trig!=kHeaderBytes||h.file_bytes!=b.size()||uint64_t(h.off_trig)+uint64_t(h.trig_count)*4u!=b.size())throw std::runtime_error("L3RF layout mismatch");
    if (verify && crc32(b.data()+h.header_bytes,b.size()-h.header_bytes) != h.payload_crc32) {
        throw std::runtime_error("L3RF CRC mismatch");
    }
    return h;
}
ReferenceTables load_l3reference(const std::vector<uint8_t>&b,bool verify){const auto h=inspect_l3reference(b,verify);ReferenceTables r;for(size_t i=0;i<r.trig.size();++i){const size_t p=h.off_trig+i*4u;r.trig[i].sin_q12=rd_s16(&b[p]);r.trig[i].cos_q12=rd_s16(&b[p+2]);}return r;}
ReferenceMotionDelta reference_player_motion_delta(const ReferenceTables&r,uint16_t yaw,int16_t forward,int16_t vertical,int16_t strafe){const auto&t=r.trig[yaw&0x0FFFu];ReferenceMotionDelta d;d.dx_fixed=int32_t(t.cos_q12)*strafe-int32_t(t.sin_q12)*forward;d.dy_fixed=int32_t(vertical)*4096;d.dz_fixed=-int32_t(t.sin_q12)*strafe-int32_t(t.cos_q12)*forward;return d;}
int16_t reference_fixed_to_short_80030150(int32_t v){const int32_t hi=v>>16;const int32_t round=(v>>15)&1;return static_cast<int16_t>(hi+round);}
} // namespace l3
