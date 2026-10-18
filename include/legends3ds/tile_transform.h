#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LegendsTileXZ { int32_t x; int32_t z; } LegendsTileXZ;
LegendsTileXZ legends_tile_transform_xz(int16_t origin_x, int16_t origin_z,
    int8_t local_x, int8_t local_z, uint8_t effective_shift, uint16_t packed_tile_flags);
#ifdef __cplusplus
}
#endif
