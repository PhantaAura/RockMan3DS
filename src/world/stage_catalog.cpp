#include "legends3ds/stage_catalog.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
constexpr size_t kHeaderBytes=32;
constexpr size_t kEntryBytes=32;
}

std::string StageCatalogEntry::package_string() const {
    size_t n=0;while(n<package_id.size()&&package_id[n]!=0)++n;
    return std::string(package_id.data(),n);
}

StageCatalog load_l3stage_catalog(const std::vector<uint8_t>& b,bool verify_crc){
    if(b.size()<kHeaderBytes||b[0]!='L'||b[1]!='3'||b[2]!='S'||b[3]!='C')throw std::runtime_error("not L3SC");
    const uint16_t version=rd_u16(&b[4]),header=rd_u16(&b[6]);
    const uint32_t flags=rd_u32(&b[8]),count=rd_u32(&b[12]),entry_bytes=rd_u32(&b[16]),off_entries=rd_u32(&b[20]),file_bytes=rd_u32(&b[24]),payload_crc=rd_u32(&b[28]);
    if(version!=kL3StageCatalogVersion||header!=kHeaderBytes||entry_bytes!=kEntryBytes)throw std::runtime_error("unsupported L3SC");
    if(flags!=0u)throw std::runtime_error("L3SC reserved flags nonzero");
    if(file_bytes!=b.size()||off_entries!=kHeaderBytes||uint64_t(off_entries)+uint64_t(count)*kEntryBytes!=b.size())throw std::runtime_error("L3SC size/range mismatch");
    if(verify_crc&&crc32(b.data()+kHeaderBytes,b.size()-kHeaderBytes)!=payload_crc)throw std::runtime_error("L3SC CRC mismatch");
    StageCatalog out;out.entries.reserve(count);
    for(uint32_t i=0;i<count;++i){
        const size_t p=size_t(off_entries)+size_t(i)*kEntryBytes;StageCatalogEntry e;
        std::copy_n(reinterpret_cast<const char*>(&b[p]),8,e.package_id.begin());
        std::copy_n(reinterpret_cast<const char*>(&b[p+8]),4,e.stage_code.begin());
        if(e.package_id[0]=='\0'||e.stage_code[0]!='S'||e.stage_code[1]!='T')throw std::runtime_error("L3SC invalid package/stage id");
        e.flags=rd_u32(&b[p+12]);e.map_bytes=rd_u32(&b[p+16]);e.render_bytes=rd_u32(&b[p+20]);e.map_crc32=rd_u32(&b[p+24]);e.render_crc32=rd_u32(&b[p+28]);
        if((e.flags&~(kL3StageCatalogFlagMap|kL3StageCatalogFlagRender))!=0u)throw std::runtime_error("L3SC unknown entry flags");
        if(!(e.flags&kL3StageCatalogFlagMap)||e.map_bytes==0)throw std::runtime_error("L3SC stage lacks required map package");
        if((e.flags&kL3StageCatalogFlagRender)==0u && (e.render_bytes!=0u||e.render_crc32!=0u))throw std::runtime_error("L3SC render metadata without render flag");
        if((e.flags&kL3StageCatalogFlagRender)!=0u && e.render_bytes==0u)throw std::runtime_error("L3SC empty render package");
        if(std::any_of(out.entries.begin(),out.entries.end(),[&](const auto&x){return x.package_string()==e.package_string();}))throw std::runtime_error("L3SC duplicate package id");
        out.entries.push_back(e);
    }
    return out;
}

std::optional<size_t> find_stage_catalog_package(const StageCatalog& c,const std::string& package_id){
    for(size_t i=0;i<c.entries.size();++i){if(c.entries[i].package_string()==package_id)return i;}
    return std::nullopt;
}
std::vector<size_t> find_stage_catalog_stage(const StageCatalog& c,const std::array<char,4>& code){
    std::vector<size_t> out;for(size_t i=0;i<c.entries.size();++i)if(c.entries[i].stage_code==code)out.push_back(i);return out;
}
} // namespace l3
