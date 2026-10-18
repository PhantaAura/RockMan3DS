#pragma once
#include "legends3ds/stage_spawn.h"
#include "legends3ds/ebd_directory.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>

namespace l3 {

enum class StageObjectFamily : uint8_t {
    Class00,
    Class20,
    Class40,
    Class60,
    Class80,
    ClassC0,
    ClassE0,
    Unknown,
};

struct StageRuntimeObject {
    size_t source_index = 0;
    uint8_t id = 0;
    uint8_t source_class = 0;
    uint8_t normalized_ebd_class = 0;
    uint8_t subtype = 0;      // source byte +0x02, compared by EBD lookup
    uint8_t source_param3 = 0; // source byte +0x03; preserved without claiming variant semantics
    StageObjectFamily family = StageObjectFamily::Unknown;
    std::array<int16_t, 3> source_position{};
    std::array<int32_t, 3> fixed_position_16_16{};
    std::array<uint8_t, kStageObjectRecordBytes> raw{};
};

StageObjectFamily classify_stage_object_family(uint8_t object_class);
StageRuntimeObject instantiate_stage_runtime_object(const StageObjectRecord& source,
                                                    size_t source_index);
std::vector<StageRuntimeObject> instantiate_stage_runtime_objects(const StageObjectList& list);

struct StageObjectPoolSpec {
    StageObjectFamily family = StageObjectFamily::Unknown;
    uint8_t source_class = 0;
    size_t capacity = 0;
    size_t original_stride_bytes = 0;
    size_t banks = 1;
};

// Exact pool geometry used by the original common stage-object allocator
// (USA ROCK_NEO.EXE 0x80031418..0x800317A8). Class00 is special-cased
// through player work rather than these generic pools.
std::optional<StageObjectPoolSpec> stage_object_pool_spec(StageObjectFamily family);
std::optional<StageObjectPoolSpec> stage_object_pool_spec_for_class(uint8_t source_class);

struct StageRuntimeAllocation {
    size_t source_index = 0;
    StageObjectFamily family = StageObjectFamily::Unknown;
    std::optional<size_t> pool_slot; // null for Class00 / unsupported classes
};

// On initial load the original allocators take the first free slot in each pool.
// This mirrors that deterministic first-fit order for a freshly-cleared native area.
// Throws if a source list exceeds a proven original pool capacity.
std::vector<StageRuntimeAllocation> assign_fresh_stage_pool_slots(
    const std::vector<StageRuntimeObject>& objects);

enum class StageWorkSeedLayout : uint8_t {
    None,
    Common20_40_60,
    CompactC0,
    Compact80_E0,
};

struct StageRuntimeWork {
    size_t source_index = 0;
    StageObjectFamily family = StageObjectFamily::Unknown;
    size_t pool_slot = 0;
    StageWorkSeedLayout layout = StageWorkSeedLayout::None;
    std::vector<uint8_t> bytes; // original work-stride mirror, pointer-free
};

// Seeds a freshly allocated generic work slot with the exact source-field copies
// observed in the common stage parser at 0x8002EA50. This deliberately mirrors
// offsets used by the PS1 runtime so translated EBD behavior can be brought up
// incrementally without inventing semantics for every field. Class00 is special
// player-work setup and returns no generic work slot.
std::optional<StageRuntimeWork> seed_fresh_stage_runtime_work(
    const StageRuntimeObject& object, const StageRuntimeAllocation& allocation);

enum class StageEbdBindingState : uint8_t {
    NoLocalCandidate,
    UniqueClassSubtype,
    AmbiguousClassSubtype,
    AmbiguousNarrowedBySourceByte3,
};

struct StageEbdBinding {
    StageEbdBindingState state = StageEbdBindingState::NoLocalCandidate;
    std::vector<size_t> class_subtype_candidates;
    std::vector<size_t> source_byte3_candidates;
};

struct StageRuntimeInstance {
    StageRuntimeObject object;
    StageRuntimeAllocation allocation;
    std::optional<StageRuntimeWork> work;
    StageEbdBinding ebd;
};

struct StageRuntimeArea {
    std::vector<StageRuntimeInstance> instances;
};

// Builds a pointer-free native area from source object records plus the area's EBD
// directory. A source-byte+3 singleton is explicitly only a narrowed candidate:
// some original initializers pass work+3 as the EBD variant, while others derive
// a dynamic selector. The six known ST00 dynamic cases therefore remain ambiguous.
StageRuntimeArea build_fresh_stage_runtime_area(const StageObjectList& objects,
                                                const EbdDirectory& ebd);

} // namespace l3
