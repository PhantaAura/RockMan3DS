#pragma once
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/reference_tables.h"

namespace l3 {
// Original action4 phase/timer decision, excluding its consumer calls.
struct Class20_03Action4Plan {
    uint8_t phase_after=0,request_after=0;
    int16_t timer_after=0;
    bool entry_sound=false,spawn_child=false,install_byte_c=false;
    std::optional<int16_t> forward;
};
Class20_03Action4Plan class20_03_plan_action4(const Class20_03CallbackState&) noexcept;

struct Class20_03PreparedAction4 {
    std::array<int16_t,4> raw_14{};
    std::array<int32_t,4> fixed_1c{};
    int32_t source_floor_word=0;
    int16_t source_forward=0;
    uint16_t heading=0;
};
struct Class20_03Action4Inputs {
    std::optional<Class20_03PreparedAction4> prepared_motion;
    std::optional<uint8_t> combat_record_byte_c;
    // An independently committed source child producer, including the full
    // allocator-exhaustion result zero. Absence never means exhaustion.
    std::optional<uint32_t> produced_child_374;
    bool sound_consumer_ready=false;
};
enum class Class20_03Action4Status : uint8_t {
    SourceBackedPartial,UnknownAction,NeedsMotion,StaleMotion,
    NeedsCombatProfile,NeedsChildConsumer,NeedsSoundConsumer
};
struct Class20_03Action4Result {
    Class20_03Action4Status status=Class20_03Action4Status::SourceBackedPartial;
    std::optional<uint16_t> sound_8001a050;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
    bool child_80104078=false;
};
Class20_03Action4Result class20_03_step_action4(
    Class20_03CallbackState&,const Class20_03Action4Inputs&) noexcept;

// Scalar representation of the original class60 pool. Source identities are
// tokens, never native pointers. Unknown bytes survive allocator reuse.
constexpr uint32_t class60_source_pool_base=0x8009cf88u;
constexpr size_t class60_pool_capacity=32,class60_slot_bytes=0x10c;
struct Class60SourcePool {
    uint32_t allocated_bitmap=0,pending_release_bitmap=0;
    std::array<std::array<uint8_t,class60_slot_bytes>,class60_pool_capacity> slots{};
};
std::optional<size_t> class60_source_slot(uint32_t source_identity) noexcept;
uint32_t class60_allocate_80031518(Class60SourcePool&) noexcept;
//800318D8 marks pending release and clears byte0; it does not immediately make
// the slot allocatable. The batch cleanup consumes that mask separately.
bool class60_mark_release_800318d8(Class60SourcePool&,uint32_t identity) noexcept;
void class60_apply_release_mask_80031b3c(Class60SourcePool&,uint32_t mask) noexcept;
struct Class20_03ChildResult {
    Class20_03Action4Status status=Class20_03Action4Status::SourceBackedPartial;
    uint32_t source_identity=0;
    std::optional<uint16_t> sound_8001a050;
    std::optional<std::array<uint32_t,2>> sound_pointed_words;
};
//80104078's scalar producer only. It never runs the child callback or claims
// collision/damage/rendering. Readiness is intentionally absent from this API.
Class20_03ChildResult class20_03_produce_child_80104078(
    const Class20_03CallbackState&,Class60SourcePool&,
    const ReferenceTables&,bool sound_consumer_ready) noexcept;
} // namespace l3
