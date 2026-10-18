#pragma once

#include <array>

namespace l3 {

// PS1 quad packets retain strip order, with the diagonal between vertices 1 and 2.
// Reverse the second triangle's order so both triangles have matching winding.
inline constexpr std::array<unsigned, 6> kPs1QuadTriangleIndices{{0, 1, 2, 1, 3, 2}};

} // namespace l3
