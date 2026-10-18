#include "legends3ds/tile_transform.h"
#include <limits.h>
/* Convert modulo-32-bit arithmetic without implementation-defined signed casts. */
static int32_t signed_bits(uint32_t v) {
    return v <= INT32_MAX ? (int32_t)v : (int32_t)((int64_t)v - 4294967296LL);
}
LegendsTileXZ legends_tile_transform_xz(int16_t origin_x, int16_t origin_z, int8_t local_x, int8_t local_z,
                                         uint8_t effective_shift, uint16_t packed_tile_flags) {
    uint32_t x = (uint32_t)(int32_t)local_x << (effective_shift & 31u);
    uint32_t z = (uint32_t)(int32_t)local_z << (effective_shift & 31u);
    uint32_t rx, rz;
    switch (packed_tile_flags & 3u) {
        case 0u: rx=x;  rz=z;  break;
        case 1u: rx=z;  rz=-x; break;
        case 2u: rx=-x; rz=-z; break;
        default: rx=-z; rz=x;  break;
    }
    LegendsTileXZ out = {signed_bits((uint32_t)(int32_t)origin_x + rx), signed_bits((uint32_t)(int32_t)origin_z + rz)};
    return out;
}
