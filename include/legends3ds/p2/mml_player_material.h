
#ifndef MML_PLAYER_MATERIAL_H
#define MML_PLAYER_MATERIAL_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint16_t tpage, clut; } MMLMaterialKey;
typedef struct { uint16_t x_word,y; uint8_t bpp; } MMLTPageAddr;
typedef struct { uint16_t x_word,y; } MMLClutAddr;
bool mml_material_key_equal(MMLMaterialKey a,MMLMaterialKey b);
MMLTPageAddr mml_decode_tpage(uint16_t tpage);
MMLClutAddr mml_decode_clut(uint16_t clut);
#endif
