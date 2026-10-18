#include "legends3ds/stage_spawn.h"
#include "legends3ds/endian_io.h"
#include <stdexcept>

namespace l3 {
StageObjectList parse_stage_object_list(const uint8_t* data, size_t bytes) {
    if (!data || bytes == 0) throw std::runtime_error("empty stage object list");
    StageObjectList out;
    size_t p=0;
    while (p < bytes) {
        if (data[p] == 0xFFu) { out.bytes_consumed = p + 1u; return out; }
        if (bytes - p < kStageObjectRecordBytes) throw std::runtime_error("truncated stage object record");
        StageObjectRecord r;
        for (size_t i=0;i<kStageObjectRecordBytes;++i) r.raw[i]=data[p+i];
        r.id=data[p+0]; r.object_class=data[p+1];
        for(size_t i=0;i<5;++i) r.words_02_to_0a[i]=rd_u16(data+p+2+i*2);
        r.x=static_cast<int16_t>(rd_u16(data+p+0x0C));
        r.y=static_cast<int16_t>(rd_u16(data+p+0x0E));
        r.z=static_cast<int16_t>(rd_u16(data+p+0x10));
        r.tail_12=rd_u16(data+p+0x12);
        out.records.push_back(r);
        p += kStageObjectRecordBytes;
    }
    throw std::runtime_error("stage object list has no 0xFF terminator");
}
StageObjectList parse_stage_object_list(const std::vector<uint8_t>& data, size_t offset) {
    if (offset >= data.size()) throw std::runtime_error("stage object list offset out of range");
    return parse_stage_object_list(data.data()+offset, data.size()-offset);
}
} // namespace l3
