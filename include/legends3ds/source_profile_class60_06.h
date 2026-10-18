#pragma once
#include "legends3ds/class60_06.h"
#include <cstddef>
#include <vector>
namespace l3 {
// Bounded neutral owner metadata. Original full unsigned LHU values survive;
// only the child primitive stores the low UV bytes. No asset/pointer fallback.
Class60_06Profile load_source_profile_class60_06(const uint8_t*,size_t);
std::vector<uint8_t> encode_source_profile_class60_06(const Class60_06Profile&);
}
