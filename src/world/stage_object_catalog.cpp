#include "legends3ds/stage_object_catalog.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeaderBytes = 48;
constexpr size_t kTableBytes = 16;
constexpr size_t kListBytes = 16;
}

StageObjectCatalog load_l3stage_object_catalog(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < kHeaderBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'O' || b[3] != 'T')
        throw std::runtime_error("not L3OT");
    const uint16_t version = rd_u16(&b[4]), header = rd_u16(&b[6]);
    const uint32_t flags = rd_u32(&b[8]);
    const uint32_t table_count = rd_u32(&b[12]);
    const uint32_t list_count = rd_u32(&b[16]);
    const uint32_t object_count = rd_u32(&b[20]);
    const uint32_t source_base = rd_u32(&b[24]);
    const uint32_t off_tables = rd_u32(&b[28]);
    const uint32_t off_lists = rd_u32(&b[32]);
    const uint32_t off_objects = rd_u32(&b[36]);
    const uint32_t file_bytes = rd_u32(&b[40]);
    const uint32_t payload_crc = rd_u32(&b[44]);
    if (version != kL3StageObjectCatalogVersion || header != kHeaderBytes) throw std::runtime_error("unsupported L3OT");
    if (flags != 0u) throw std::runtime_error("L3OT reserved flags nonzero");
    if (off_tables != kHeaderBytes ||
        uint64_t(off_tables) + uint64_t(table_count) * kTableBytes != off_lists ||
        uint64_t(off_lists) + uint64_t(list_count) * kListBytes != off_objects ||
        uint64_t(off_objects) + uint64_t(object_count) * kStageObjectRecordBytes != file_bytes ||
        file_bytes != b.size()) throw std::runtime_error("L3OT size/range mismatch");
    if (verify_crc && crc32(b.data() + kHeaderBytes, b.size() - kHeaderBytes) != payload_crc)
        throw std::runtime_error("L3OT CRC mismatch");

    struct ListMeta { uint32_t source_off, object_first, object_count; };
    std::vector<ListMeta> lists; lists.reserve(list_count);
    uint32_t expect_obj = 0;
    for (uint32_t i = 0; i < list_count; ++i) {
        const size_t p = size_t(off_lists) + size_t(i) * kListBytes;
        ListMeta m{rd_u32(&b[p]), rd_u32(&b[p+4]), rd_u32(&b[p+8])};
        if (rd_u32(&b[p+12]) != 0u) throw std::runtime_error("L3OT list reserved field nonzero");
        if (m.object_first != expect_obj || uint64_t(m.object_first) + m.object_count > object_count)
            throw std::runtime_error("L3OT object ranges not contiguous");
        expect_obj += m.object_count; lists.push_back(m);
    }
    if (expect_obj != object_count) throw std::runtime_error("L3OT object coverage mismatch");

    std::vector<StageObjectRecord> objects; objects.reserve(object_count);
    for (uint32_t i = 0; i < object_count; ++i) {
        const size_t p = size_t(off_objects) + size_t(i) * kStageObjectRecordBytes;
        StageObjectRecord r; std::copy_n(&b[p], kStageObjectRecordBytes, r.raw.begin());
        r.id=b[p];r.object_class=b[p+1];
        for(size_t w=0;w<r.words_02_to_0a.size();++w)r.words_02_to_0a[w]=rd_u16(&b[p+2+w*2]);
        r.x=rd_s16(&b[p+0x0c]);r.y=rd_s16(&b[p+0x0e]);r.z=rd_s16(&b[p+0x10]);r.tail_12=rd_u16(&b[p+0x12]);
        objects.push_back(r);
    }

    StageObjectCatalog out; out.source_load_base=source_base; out.tables.reserve(table_count);
    uint32_t expect_list=0;
    for(uint32_t i=0;i<table_count;++i){
        const size_t p=size_t(off_tables)+size_t(i)*kTableBytes;
        StageObjectCatalogTable t; t.source_table_offset=rd_u32(&b[p]);
        const uint32_t first=rd_u32(&b[p+4]), count=rd_u32(&b[p+8]); t.evidence_flags=rd_u32(&b[p+12]);
        if(first!=expect_list||uint64_t(first)+count>list_count)throw std::runtime_error("L3OT list ranges not contiguous");
        if((t.evidence_flags&~kL3StageObjectTableStrongPointerRun)!=0u)throw std::runtime_error("L3OT unknown evidence flag");
        for(uint32_t j=0;j<count;++j){const auto&m=lists[first+j];StageObjectCatalogList l;l.source_list_offset=m.source_off;l.objects.records.insert(l.objects.records.end(),objects.begin()+m.object_first,objects.begin()+m.object_first+m.object_count);l.objects.bytes_consumed=size_t(m.object_count)*kStageObjectRecordBytes+1;t.lists.push_back(std::move(l));}
        expect_list+=count;out.tables.push_back(std::move(t));
    }
    if(expect_list!=list_count)throw std::runtime_error("L3OT list coverage mismatch");
    return out;
}

} // namespace l3
