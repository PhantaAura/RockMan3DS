
#include "mml_player_material.h"
bool mml_material_key_equal(MMLMaterialKey a,MMLMaterialKey b){return a.tpage==b.tpage&&a.clut==b.clut;}
MMLTPageAddr mml_decode_tpage(uint16_t t){
    MMLTPageAddr a; a.x_word=(uint16_t)((t&0x0Fu)*64u); a.y=(uint16_t)(((t>>4)&1u)*256u);
    uint8_t mode=(uint8_t)((t>>7)&3u); a.bpp=(mode==0)?4:(mode==1?8:16); return a;
}
MMLClutAddr mml_decode_clut(uint16_t c){MMLClutAddr a={(uint16_t)((c&0x3Fu)*16u),(uint16_t)((c>>6)&0x1FFu)};return a;}
