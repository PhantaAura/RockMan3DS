#pragma once
#include "legends3ds/source_event_command.h"
#include <vector>

namespace l3 {
// Exact owner-local lookup scalar and cold LOAD fields. Admission is one-time
// fresh diagnostic initialization, never a scene reset or CD receipt.
constexpr size_t kSourceEventCommandPackageBytes=112;
struct SourceEventCommandPackage {
    SourceEventCommandProfile profile;
    SourceEventCommandContext cold_context;
};
SourceEventCommandPackage load_source_event_command_package(const std::vector<uint8_t>& bytes);
} // namespace l3
