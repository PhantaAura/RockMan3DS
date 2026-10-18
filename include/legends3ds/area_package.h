#pragma once
#include "legends3ds/l3map.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/render/negative_materials.h"
#include "legends3ds/tutorial_runtime.h"
#include "legends3ds/st00_spawn.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <vector>

namespace l3 {
constexpr uint32_t kAreaNegativeMaterialsSection = 0x20;
constexpr uint32_t kAreaRuntimeBindingsSection = 0x21;
constexpr uint32_t kAreaSpawnsSection = 0x22;
constexpr uint32_t kAreaMapInitializationSection = 0x23;
// L3RB v1 names its decoder explicitly. Only the existing five-area L3SF v1
// source-backed flow and complete L3EB v2 payload backend is presently supported.
enum class AreaRuntimeBindingBackend : uint32_t { St00FlowV1 = 1 };
// Admission limits are policy chosen by the caller, not a measured hardware budget.
struct AreaPackageLimits {
    uint64_t max_file_bytes = 16u * 1024u * 1024u;
    uint64_t max_section_bytes = 8u * 1024u * 1024u;
    uint64_t max_decoded_bytes = 16u * 1024u * 1024u;
    uint64_t max_load_peak_bytes = 32u * 1024u * 1024u;
    uint32_t max_sections = 64;
    uint32_t max_map_areas = 256;
};
struct AreaPackageSection {
    uint32_t kind = 0, offset = 0, size = 0, crc32 = 0;
    bool decoded = false;
};
struct AreaPackageDiagnostics {
    uint64_t file_bytes = 0;
    uint64_t map_estimated_bytes = 0;
    uint64_t render_estimated_bytes = 0;
    uint64_t negative_material_estimated_bytes = 0;
    uint64_t runtime_binding_estimated_bytes = 0;
    uint64_t runtime_source_payload_bytes = 0;
    uint64_t spawn_estimated_bytes = 0;
    uint64_t map_initialization_temporary_bytes = 0;
    uint32_t map_initialization_writes = 0;
    uint32_t map_initialization_profile = 0; // zero means absent/UNKNOWN.
    uint64_t decoded_estimated_bytes = 0;
    uint64_t load_peak_estimated_bytes = 0;
    uint32_t source_profile_crc = 0;
    uint32_t skipped_sections = 0;
    uint32_t unknown_sections = 0;
};
// Owns decoded CPU data. Optional runtime bindings preserve source records and
// actor resources for the existing backend, without supplying new event behavior,
// texture state, audio/camera semantics or retail parity.
struct LoadedAreaPackage {
    LoadedAreaPackage() = default;
    LoadedAreaPackage(const LoadedAreaPackage&) = delete;
    LoadedAreaPackage& operator=(const LoadedAreaPackage&) = delete;
    LoadedAreaPackage(LoadedAreaPackage&&) noexcept = default;
    LoadedAreaPackage& operator=(LoadedAreaPackage&&) noexcept = default;
    RuntimeMap map;
    St00RenderSource render;
    NegativeMaterialTables negative_materials;
    // Sorted by local map area_index. May be sparse; absent areas have no runtime
    // binding. The source-flow area selector is separate from this local identity.
    std::vector<TutorialRuntimeArea> runtime_areas;
    // Optional five-area ST00 source catalog. Its current backend requires
    // five local map areas and identity local/source runtime selectors.
    std::optional<St00SpawnCatalog> spawns;
    std::vector<AreaPackageSection> directory;
    AreaPackageDiagnostics diagnostics;
    void unload() noexcept;
};
// Throws on malformed data, absent required sections or admission failure. The
// returned value owns its data; caller can replace its active area only on success.
// Optional section32 contains bounded area-local negative material tables.
// Optional section33 contains L3RB v1 runtime bindings; malformed bindings fail
// the complete load rather than admitting a partially bound area.
// Optional section34 contains a bounded L3SP v1 conditional spawn catalog.
// Optional section35 contains CRC-bound L3MI v1 startup initialization. The
// bounded metadata is released after applying it to the shared decoded map.
// Unknown sections have their CRC checked but their payload is not retained.
LoadedAreaPackage load_area_package(const std::string& path, const AreaPackageLimits& limits);
// Counts the positive-cell Mid/Primary primitive instances used by the current
// native terrain frontend. Reused cells/streams count on every expansion. This
// allocates no geometry and assumes no triangle topology or vertex format.
// Valid empty areas return zero; the caller chooses whether it can render them.
uint64_t count_area_render_primitives(const RuntimeMap& map, const St00RenderSource& render,
                                     size_t area_index, uint64_t max_primitives);
// Native area switches use this ordering while retaining outgoing CPU session
// and actors. Prepare may release/rebuild GPU resources to avoid double GPU
// residency; restore must clean partial resources and rebuild the outgoing view.
// A failed prepare/commit rethrows its original exception after restoration.
// Restoration failure propagates instead: callers must clean up and stop.
template <typename Prepare, typename Commit, typename Restore>
void transition_area_resources(Prepare&& prepare, Commit&& commit, Restore&& restore) {
    try {
        prepare();
        commit();
    } catch (...) {
        const auto failure = std::current_exception();
        restore();
        std::rethrow_exception(failure);
    }
}
// Bounded native sidecar admission, before decoding or releasing the active
// area's resources. Requires the existing cooker's canonical payload layout;
// shared/overlapping payload ranges would amplify decoded/texture residency.
// This estimate covers the decoded CPU cache only, not GPU or total-device RAM.
uint64_t preflight_area_material_cache(const std::vector<uint8_t>& bytes,
                                       uint64_t max_decoded_bytes);
} // namespace l3
