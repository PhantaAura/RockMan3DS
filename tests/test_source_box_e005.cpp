// ST00 E0/05 breakable box (0x80113EE0): hits, break, flag 0x620, reward record.
#include "legends3ds/source_box_e005.h"
#include <cassert>
#include <iostream>

int main(){
    using namespace l3;
    auto b=source_e005_init_80113f2c({{5120,-385,12544}},0x02,0x620);
    assert(b.hp_44==0xA0&&b.flags_2c==0x0E&&b.flag_46==0x620);
    uint32_t packet=0;
    auto s=source_e005_step(b,packet,uint16_t(7));
    assert(s.register_proxy&&s.proxy_point[1]==-385-0x80&&s.proxy_mode==0x8000&&s.proxy_word_18==0x04080000&&s.sound==0);
    packet=0x00000018u;s=source_e005_step(b,packet,uint16_t(7));     // no damage bits: 0xA3
    assert(s.sound==0xA3&&b.hp_44==0xA0&&packet==0);
    for(int i=0;i<6;++i){packet=0x08000018u;s=source_e005_step(b,packet,uint16_t(7));assert(s.sound==0xA0);}
    assert(b.hp_44==16);
    packet=0x08000018u;s=source_e005_step(b,packet,uint16_t(6));      // 16 - 24 <= 0
    assert(s.sound==0xA2&&b.action_09==1&&!(b.flags_2c&2u));
    int frames=1;while(!s.released&&frames<10){s=source_e005_step(b,packet,uint16_t(6));++frames;}
    assert(s.released&&frames==4&&s.set_flag==uint16_t(0x620)&&s.reward_record==uint8_t(0));  // 6 & 1
    assert(!source_e005_step(b,packet,uint16_t(1)).register_proxy);
    // Parameter 1: reward record 2 and the flag; parameter 0: reward only, held without a sample.
    auto c=source_e005_init_80113f2c({{0,0,0}},0x01,0x700);c.action_09=1;
    do{s=source_e005_step(c,packet,std::nullopt);}while(!s.released);
    assert(s.reward_record==uint8_t(2)&&s.set_flag==uint16_t(0x700));
    auto d=source_e005_init_80113f2c({{0,0,0}},0x00,0x700);d.action_09=1;
    do{s=source_e005_step(d,packet,std::nullopt);}while(!s.released);
    assert(!s.set_flag&&!s.reward_record&&s.reward_needs_random);
    std::cout<<"PASS: E0/05 0xA3/0xA0/0xA2, 4-frame break, flag 0x620, reward record by parameter\n";
}
