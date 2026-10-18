#include "legends3ds/actor_detection_profile.h"
#include "legends3ds/l3map.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <stdexcept>

static bool rejects(const std::vector<uint8_t>& b) {
    try{(void)l3::load_l3actor_detection_profiles(b.data(),b.size());}
    catch(const std::runtime_error&){return true;}return false;
}
int main() {
    l3::ActorDetectionProfiles p{};p[0]=l3::ActorDetectionProfile{5,7,3};
    p[4]=l3::ActorDetectionProfile{11,9,17};p[255]=l3::ActorDetectionProfile{255,0,255};
    auto bytes=l3::encode_l3actor_detection_profiles(p);
    auto loaded=l3::load_l3actor_detection_profiles(bytes.data(),bytes.size());
    assert(loaded[0]->byte0==5 && loaded[4]->byte2==17 && loaded[255]->byte1==0);
    assert(!loaded[1] && !loaded[254]);assert(bytes.size()==44);
    for(size_t n=0;n<bytes.size();++n)assert(rejects(std::vector<uint8_t>(bytes.begin(),bytes.begin()+n)));
    for(const size_t off:{size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(28),size_t(33)}) {
        auto bad=bytes;bad[off]^=0x80;assert(rejects(bad));
    }
    auto bad=bytes;bad.push_back(0);assert(rejects(bad));
    bad=bytes;bad[36]=0;l3::patch_u32(bad,24,l3::crc32(bad.data()+32,bad.size()-32));assert(rejects(bad));
    bad=bytes;bad[36]=255;l3::patch_u32(bad,24,l3::crc32(bad.data()+32,bad.size()-32));assert(rejects(bad));
    for(size_t id=0;id<p.size();++id)p[id]=l3::ActorDetectionProfile{1,2,3};
    bytes=l3::encode_l3actor_detection_profiles(p);assert(bytes.size()==1056);
    loaded=l3::load_l3actor_detection_profiles(bytes.data(),bytes.size());for(const auto& row:loaded)assert(row);
    assert(rejects({}));bool rejected=false;
    try{(void)l3::encode_l3actor_detection_profiles({});}catch(const std::runtime_error&){rejected=true;}
    assert(rejected);
}
