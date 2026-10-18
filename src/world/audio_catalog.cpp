#include "legends3ds/audio_catalog.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace { constexpr size_t kHeaderBytes=40, kEntryBytes=56; }
RuntimeAudioCatalog load_l3audio_catalog(const std::vector<uint8_t>& b,bool verify_crc){
    if(b.size()<kHeaderBytes||b[0]!='L'||b[1]!='3'||b[2]!='A'||b[3]!='C')throw std::runtime_error("not L3AC");
    const uint16_t v=rd_u16(&b[4]),h=rd_u16(&b[6]); const uint32_t flags=rd_u32(&b[8]),count=rd_u32(&b[12]),eb=rd_u32(&b[16]),oe=rd_u32(&b[20]),os=rd_u32(&b[24]),sb=rd_u32(&b[28]),fb=rd_u32(&b[32]),pc=rd_u32(&b[36]);
    if(v!=kL3AudioCatalogVersion||h!=kHeaderBytes||eb!=kEntryBytes)throw std::runtime_error("unsupported L3AC");
    if((flags&~1u)!=0u)throw std::runtime_error("L3AC reserved flags nonzero");
    if(oe!=kHeaderBytes||uint64_t(oe)+uint64_t(count)*kEntryBytes!=os||uint64_t(os)+sb!=fb||fb!=b.size())throw std::runtime_error("L3AC size/range mismatch");
    if(verify_crc&&crc32(b.data()+kHeaderBytes,b.size()-kHeaderBytes)!=pc)throw std::runtime_error("L3AC CRC mismatch");
    RuntimeAudioCatalog out; out.routing_proven=(flags&1u)!=0u; out.entries.reserve(count);
    for(uint32_t i=0;i<count;++i){
        const size_t p=size_t(oe)+size_t(i)*kEntryBytes; RuntimeAudioSourceEntry e;
        const uint8_t kind=b[p],aligned=b[p+1]; if(b[p+2]||b[p+3]||aligned>1)throw std::runtime_error("L3AC invalid entry flags");
        if(kind!=uint8_t(RuntimeAudioSourceKind::Xa)&&kind!=uint8_t(RuntimeAudioSourceKind::Str))throw std::runtime_error("L3AC invalid source kind");
        e.kind=static_cast<RuntimeAudioSourceKind>(kind); e.aligned_2048=aligned!=0; e.source_bytes=rd_u32(&b[p+4]);e.blocks_2048=rd_u32(&b[p+8]);
        std::copy_n(&b[p+12],32,e.sha256.begin()); const uint32_t no=rd_u32(&b[p+44]),nl=rd_u32(&b[p+48]); if(rd_u32(&b[p+52])!=0u||nl==0||uint64_t(no)+nl>sb)throw std::runtime_error("L3AC invalid name metadata");
        e.source_name.assign(reinterpret_cast<const char*>(&b[os+no]),nl); out.entries.push_back(std::move(e));
    }
    return out;
}
}
