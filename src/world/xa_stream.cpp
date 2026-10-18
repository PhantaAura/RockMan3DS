#include "legends3ds/xa_stream.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace l3 {
namespace {
constexpr auto crc_table() {
    std::array<uint32_t,256> out{};
    for(uint32_t i=0;i<256;++i){uint32_t v=i;for(unsigned b=0;b<8;++b)v=(v>>1)^((v&1)?0xedb88320u:0);out[i]=v;}
    return out;
}
constexpr auto table=crc_table();
uint32_t crc_update(uint32_t v,const uint8_t* p,size_t count) noexcept {
    for(size_t i=0;i<count;++i)v=(v>>8)^table[(v^p[i])&255];
    return v;
}
uint32_t crc(const uint8_t* p,size_t count) noexcept{return crc_update(~0u,p,count)^~0u;}
uint64_t u64(const uint8_t* p) noexcept{return uint64_t(rd_u32(p))|(uint64_t(rd_u32(p+4))<<32);}
bool seek(std::FILE* f,uint64_t position) noexcept {
    return position<=uint64_t(std::numeric_limits<long>::max())&&std::fseek(f,long(position),SEEK_SET)==0;
}
constexpr std::array<uint8_t,32> exe{{0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
}
bool XaStreamReader::reject(const char* error) noexcept {close();error_=error;return false;}
void XaStreamReader::close() noexcept {
    if(file_)std::fclose(file_);
    file_=nullptr;info_={};pcm_offset_=0;std::vector<XaSectorIndex>().swap(index_);error_="not opened";
}
bool XaStreamReader::open(const char* path) {
    close();file_=std::fopen(path,"rb");if(!file_)return reject("XA package missing");
    // No deferred stdio buffer allocation in streaming reads.
    std::setvbuf(file_,nullptr,_IONBF,0);
    std::array<uint8_t,256> header{};
    if(std::fread(header.data(),1,header.size(),file_)!=header.size())return reject("XA header truncated");
    const auto* h=header.data();const auto header_crc=rd_u32(h+96);
    std::fill(header.begin()+96,header.begin()+100,0);
    if(std::memcmp(h,"L3XA001\0",8)||rd_u32(h+8)!=256||rd_u32(h+12)!=1||rd_u32(h+16)!=1||rd_u32(h+124)!=0)
        return reject("XA header/profile rejected");
    if(crc(h,256)!=header_crc)return reject("XA header CRC rejected");
    if(!std::equal(exe.begin(),exe.end(),h+224))return reject("XA executable identity rejected");
    const auto entries=rd_u32(h+48);const auto start=rd_u32(h+24),end=rd_u32(h+28);
    if(!entries||entries>kXaMaxIndexEntries||rd_u32(h+20)>65535||start>=end||rd_u32(h+32)!=37800||
       rd_u32(h+36)!=2||rd_u32(h+40)!=16||rd_u32(h+44)!=1||rd_u32(h+52)!=24||
       rd_u32(h+112)>255||rd_u32(h+116)>=8||rd_u32(h+120)!=8||
       uint64_t(entries)*8!=uint64_t(end)-start)return reject("XA unsupported coding/index domain");
    const uint64_t frames=uint64_t(entries)*kXaFramesPerSector;
    const auto pcm=uint64_t(256)+uint64_t(entries)*24;
    const auto bytes=frames*4,file_bytes=pcm+bytes;
    if(u64(h+56)!=frames||u64(h+64)!=256||u64(h+72)!=pcm||u64(h+80)!=bytes||u64(h+88)!=file_bytes)
        return reject("XA noncanonical offsets/counts");
    if(std::fseek(file_,0,SEEK_END)!=0)return reject("XA size unavailable");
    const auto physical_size=std::ftell(file_);
    if(physical_size<0||uint64_t(physical_size)!=file_bytes||!seek(file_,256))return reject("XA size/trailing bytes rejected");
    index_.reserve(entries);uint32_t index_crc=~0u;std::array<uint8_t,24> raw{};
    for(uint32_t i=0;i<entries;++i){
        if(std::fread(raw.data(),1,raw.size(),file_)!=raw.size())return reject("XA index truncated");
        index_crc=crc_update(index_crc,raw.data(),raw.size());
        XaSectorIndex x{rd_u32(raw.data()),rd_u32(raw.data()+4),u64(raw.data()+8),rd_u32(raw.data()+16)};
        if(x.lba<start||x.lba>=end||x.frames!=kXaFramesPerSector||x.first_frame!=uint64_t(i)*kXaFramesPerSector||
           rd_u32(raw.data()+20)!=0||(!i&&uint64_t(start)+rd_u32(h+116)!=x.lba)||
           (i&&uint64_t(index_.back().lba)+8!=x.lba))return reject("XA index ordering/span rejected");
        index_.push_back(x);
    }
    if((index_crc^~0u)!=rd_u32(h+100))return reject("XA index CRC rejected");
    std::array<uint8_t,16384> scratch{};uint32_t pcm_crc=~0u;uint64_t remaining=bytes;
    while(remaining){
        const auto count=size_t(std::min<uint64_t>(remaining,scratch.size()));
        if(std::fread(scratch.data(),1,count,file_)!=count)return reject("XA PCM truncated");
        pcm_crc=crc_update(pcm_crc,scratch.data(),count);remaining-=count;
    }
    if((pcm_crc^~0u)!=rd_u32(h+104))return reject("XA PCM CRC rejected");
    info_={rd_u32(h+20),start,end,37800,rd_u32(h+112),rd_u32(h+116),frames};
    pcm_offset_=pcm;error_="ready";return true;
}
bool XaStreamReader::read_sector(size_t index,int16_t* stereo,size_t capacity) noexcept {
    if(!file_||!stereo||index>=index_.size()||capacity<kXaFramesPerSector){error_="XA read domain rejected";return false;}
    if(!seek(file_,pcm_offset_+index_[index].first_frame*4)||
       std::fread(stereo,1,kXaSectorPcmBytes,file_)!=kXaSectorPcmBytes){error_="XA PCM read failed";return false;}
    if(crc(reinterpret_cast<const uint8_t*>(stereo),kXaSectorPcmBytes)!=index_[index].pcm_crc){error_="XA sector CRC rejected";return false;}
    // Packages use little endian on every host; Old3DS is little endian.
    const auto* bytes=reinterpret_cast<const uint8_t*>(stereo);
    for(size_t i=0;i<kXaFramesPerSector*2;++i){const auto x=rd_u16(bytes+i*2);stereo[i]=x<=32767?int16_t(x):int16_t(int32_t(x)-65536);}
    error_="ready";return true;
}
uint32_t XaStreamReader::sector_for_frame(uint64_t frame) const noexcept {
    if(!file_||index_.empty())return 0;
    if(frame>=info_.total_frames)return info_.end_lba;
    return index_[size_t(frame/kXaFramesPerSector)].lba;
}
} // namespace l3
