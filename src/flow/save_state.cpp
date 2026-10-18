#include "legends3ds/save_state.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <fstream>
#include <stdexcept>

namespace l3 {
namespace {
constexpr uint16_t kHeaderBytes = 24;
constexpr uint32_t kPayloadBytes = 4 + 2 + 2 + 4*3 + 2 + 2 + 4 + kL3StoryFlagBytes + 14;

void put_qol(std::vector<uint8_t>& out, const L3QolSettings& q) {
    out.push_back(q.version);
    out.push_back(q.control_preset);
    out.push_back(q.camera_behavior);
    out.push_back(q.lock_on_mode);
    out.push_back(q.text_speed);
    out.push_back(q.bottom_screen_mode);
    out.push_back(q.dialogue_history);
    out.push_back(q.objective_panel);
    out.push_back(q.quick_restart_confirm);
    out.push_back(q.touch_shortcuts);
    out.push_back(q.invert_camera_x);
    out.push_back(q.invert_camera_y);
    wr_u16(out, q.camera_sensitivity_percent);
}

L3QolSettings get_qol(const uint8_t* p) {
    L3QolSettings q{};
    q.version=p[0]; q.control_preset=p[1]; q.camera_behavior=p[2]; q.lock_on_mode=p[3];
    q.text_speed=p[4]; q.bottom_screen_mode=p[5]; q.dialogue_history=p[6]; q.objective_panel=p[7];
    q.quick_restart_confirm=p[8]; q.touch_shortcuts=p[9]; q.invert_camera_x=p[10]; q.invert_camera_y=p[11];
    q.camera_sensitivity_percent=rd_u16(p+12);
    l3_qol_settings_sanitize(&q);
    return q;
}
}

std::vector<uint8_t> cook_l3save(const RuntimeSaveState& state) {
    RuntimeSaveState s=state;
    l3_qol_settings_sanitize(&s.qol);
    std::vector<uint8_t> o;
    o.reserve(kHeaderBytes+kPayloadBytes);
    o.insert(o.end(), {'L','3','S','V'});
    wr_u16(o,kL3SaveVersion); wr_u16(o,kHeaderBytes); wr_u32(o,0x1u); wr_u32(o,kPayloadBytes);
    const size_t crc_pos=o.size(); wr_u32(o,0); wr_u32(o,0);
    if(o.size()!=kHeaderBytes) throw std::runtime_error("L3SV header size bug");
    wr_u32(o,s.checkpoint_id); wr_u16(o,s.area_id); wr_u16(o,s.player_yaw);
    wr_u32(o,static_cast<uint32_t>(s.player_x)); wr_u32(o,static_cast<uint32_t>(s.player_y)); wr_u32(o,static_cast<uint32_t>(s.player_z));
    wr_u16(o,s.hp); wr_u16(o,s.hp_max); wr_u32(o,s.play_frames);
    o.insert(o.end(),s.story_flags.begin(),s.story_flags.end());
    put_qol(o,s.qol);
    if(o.size()!=kHeaderBytes+kPayloadBytes) throw std::runtime_error("L3SV payload size bug");
    const uint32_t c=crc32(o.data()+kHeaderBytes,kPayloadBytes);
    patch_u32(o,crc_pos,c);
    return o;
}

L3SaveHeader inspect_l3save(const std::vector<uint8_t>& b,bool verify_crc){
    if(b.size()<kHeaderBytes||b[0]!='L'||b[1]!='3'||b[2]!='S'||b[3]!='V') throw std::runtime_error("not L3SV");
    L3SaveHeader h; h.version=rd_u16(&b[4]); h.header_bytes=rd_u16(&b[6]); h.flags=rd_u32(&b[8]); h.payload_bytes=rd_u32(&b[12]); h.payload_crc32=rd_u32(&b[16]);
    if(h.version!=kL3SaveVersion||h.header_bytes!=kHeaderBytes) throw std::runtime_error("unsupported L3SV");
    if(rd_u32(&b[20])!=0u) throw std::runtime_error("L3SV reserved field nonzero");
    if(h.payload_bytes!=kPayloadBytes||b.size()!=size_t(h.header_bytes)+h.payload_bytes) throw std::runtime_error("L3SV size mismatch");
    if(verify_crc&&crc32(b.data()+h.header_bytes,h.payload_bytes)!=h.payload_crc32) throw std::runtime_error("L3SV CRC mismatch");
    return h;
}

RuntimeSaveState load_l3save(const std::vector<uint8_t>& b,bool verify_crc){
    const auto h=inspect_l3save(b,verify_crc); size_t p=h.header_bytes; RuntimeSaveState s;
    s.checkpoint_id=rd_u32(&b[p]); p+=4; s.area_id=rd_u16(&b[p]); p+=2; s.player_yaw=rd_u16(&b[p]); p+=2;
    s.player_x=static_cast<int32_t>(rd_u32(&b[p])); p+=4; s.player_y=static_cast<int32_t>(rd_u32(&b[p])); p+=4; s.player_z=static_cast<int32_t>(rd_u32(&b[p])); p+=4;
    s.hp=rd_u16(&b[p]); p+=2; s.hp_max=rd_u16(&b[p]); p+=2; s.play_frames=rd_u32(&b[p]); p+=4;
    for(auto& v:s.story_flags) v=b[p++];
    s.qol=get_qol(&b[p]); p+=14;
    if(p!=b.size()) throw std::runtime_error("L3SV decode cursor mismatch");
    return s;
}

void runtime_story_flag_set(RuntimeSaveState& s,uint16_t bit,bool value){
    if(bit>=kL3StoryFlagBytes*8u) throw std::out_of_range("story flag out of range");
    const uint8_t mask=uint8_t(1u<<(bit&7u)); auto& v=s.story_flags[bit>>3u];
    if(value)v=uint8_t(v|mask); else v=uint8_t(v&uint8_t(~mask));
}
bool runtime_story_flag_get(const RuntimeSaveState& s,uint16_t bit){
    if(bit>=kL3StoryFlagBytes*8u) return false;
    return (s.story_flags[bit>>3u]&(1u<<(bit&7u)))!=0;
}

bool write_l3save_atomic(const std::filesystem::path& path,const RuntimeSaveState& state,bool preserve_backup){
    try{
        const auto bytes=cook_l3save(state); const auto tmp=path.string()+".tmp"; const auto bak=path.string()+".bak";
        {
            std::ofstream f(tmp,std::ios::binary|std::ios::trunc); if(!f)return false;
            f.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())); f.flush(); if(!f)return false;
        }
        if(preserve_backup&&std::filesystem::exists(path)){
            std::error_code ec; std::filesystem::remove(bak,ec); ec.clear(); std::filesystem::rename(path,bak,ec);
            if(ec){ std::filesystem::remove(tmp); return false; }
        }else if(std::filesystem::exists(path)){
            std::error_code ec; std::filesystem::remove(path,ec); if(ec){std::filesystem::remove(tmp);return false;}
        }
        std::error_code ec; std::filesystem::rename(tmp,path,ec); if(ec)return false;
        return true;
    }catch(...){return false;}
}

} // namespace l3
