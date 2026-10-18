#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace l3 {
constexpr uint32_t kXaFramesPerSector=2016;
constexpr uint32_t kXaSectorPcmBytes=kXaFramesPerSector*4;
constexpr uint32_t kXaMaxIndexEntries=4096; // admission policy, not hardware budget
struct XaStreamInfo {
    uint32_t id=0,start_lba=0,end_lba=0,sample_rate=0,filter_file=0,filter_channel=0;
    uint64_t total_frames=0;
};
struct XaSectorIndex {uint32_t lba=0,frames=0;uint64_t first_frame=0;uint32_t pcm_crc=0;};
// Private cooked PCM only. All payload CRCs are checked with bounded scratch at
// open; no whole stream is retained and reads recheck their individual CRC.
class XaStreamReader {
public:
    XaStreamReader()=default;
    ~XaStreamReader(){close();}
    XaStreamReader(const XaStreamReader&)=delete;
    XaStreamReader& operator=(const XaStreamReader&)=delete;
    bool open(const char* path);
    void close() noexcept;
    bool ready() const noexcept{return file_!=nullptr;}
    const char* error() const noexcept{return error_;}
    const XaStreamInfo& info() const noexcept{return info_;}
    const std::vector<XaSectorIndex>& index() const noexcept{return index_;}
    uint32_t resident_index_bytes() const noexcept{return uint32_t(index_.capacity()*sizeof(XaSectorIndex));}
    bool read_sector(size_t index,int16_t* stereo,size_t frame_capacity) noexcept;
    uint32_t sector_for_frame(uint64_t completed_frames) const noexcept;
private:
    bool reject(const char* error) noexcept;
    std::FILE* file_=nullptr;
    XaStreamInfo info_{};
    std::vector<XaSectorIndex> index_;
    uint64_t pcm_offset_=0;
    const char* error_="not opened";
};
} // namespace l3
