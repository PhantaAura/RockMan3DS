// 0x80025900 face depth (otz/near cull), CLUT ramp step and the L3CR scale
// sidecar. Authored palettes and ramp bytes only.
#include "legends3ds/render/terrain_clut_ramp.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace l3;
int main(){
    // otz: bit31 clear -> (max + (sum >> 2)) >> 1; bit27 -> max - 16.
    auto d=terrain_face_depth_80025900({{1000,1200,1400,1600}},0x00808080u);
    assert(!d.near_culled&&d.otz==(1600+(5200>>2))>>1);
    assert(terrain_face_depth_80025900({{1000,1200,1400,1600}},0x08808080u).otz==1584);
    // bit31: max, or SZ0 with bit27; never near-culled.
    assert(terrain_face_depth_80025900({{100,150,120,90}},0x80808080u).otz==150);
    assert(terrain_face_depth_80025900({{100,150,120,90}},0x88808080u).otz==100);
    // Near cull: bit31 clear and max SZ < 0xC1.
    assert(terrain_face_depth_80025900({{100,150,192,90}},0x00808080u).near_culled);
    assert(!terrain_face_depth_80025900({{100,150,193,90}},0x00808080u).near_culled);
    // Ramp: area bytes +0..31 indexed by otz >> (9 + LOD), clamped to 7.
    std::array<uint8_t,40> area{};
    for(size_t i=10;i<32;++i)area[i]=uint8_t(i<20?(i-10)/2+1:9);
    assert(terrain_clut_ramp_step(5119,0x00808080u,area,0)==0);
    assert(terrain_clut_ramp_step(5120,0x00808080u,area,0)==1);
    assert(terrain_clut_ramp_step(7*512,0x00808080u,area,0)==0);
    assert(terrain_clut_ramp_step(12*512,0x00808080u,area,0)==2);
    assert(terrain_clut_ramp_step(12*512,0x00808080u,area,1)==0);       // LOD 1 halves the index
    assert(terrain_clut_ramp_step(25*512,0x00808080u,area,0)==7);       // 9 clamps to 7
    assert(terrain_clut_ramp_step(12*512,0x10808080u,area,0)==0);       // bit 28 skips the ramp write
    // Fit: an exact half-brightness variant gives 2048 on every channel; a
    // missing step keeps unity.
    std::vector<uint16_t> base{0x0000,0x7fff,0x3def,0x56b5};
    std::array<std::vector<uint16_t>,8> variants{};
    std::vector<uint16_t> half;
    for(auto p:base){uint16_t q=0;for(int c=0;c<3;++c)q|=uint16_t((((p>>(5*c))&31)/2)<<(5*c));half.push_back(q);}
    variants[1]=half;
    const auto e=fit_terrain_clut_ramp(0x7e40001cu,base,variants);
    assert(e.scale_q12[0][0]==4096&&e.scale_q12[2][1]==4096);
    for(int c=0;c<3;++c)assert(e.scale_q12[1][c]>1900&&e.scale_q12[1][c]<2100);
    // L3CR round trip and rejection.
    const auto bytes=cook_l3clut_ramp({e});
    const auto back=load_l3clut_ramp(bytes.data(),bytes.size());
    assert(back.size()==1&&back[0].material_word==0x7e40001cu&&back[0].scale_q12==e.scale_q12);
    assert(find_terrain_clut_ramp(back,0x7e40001cu)&&!find_terrain_clut_ramp(back,1));
    auto bad=bytes;bad.back()^=1;
    bool threw=false;try{(void)load_l3clut_ramp(bad.data(),bad.size());}catch(const std::exception&){threw=true;}
    assert(threw);
    threw=false;try{(void)load_l3clut_ramp(bytes.data(),bytes.size()-1);}catch(const std::exception&){threw=true;}
    assert(threw);
    auto over=e;over.scale_q12[3][0]=5000;threw=false;
    try{(void)cook_l3clut_ramp({over});}catch(const std::exception&){threw=true;}
    assert(threw);
    std::cout<<"PASS: 0x80025900 otz/near cull, ramp step by area bytes/LOD/bit28, L3CR fit and round trip\n";
}
