#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {
// Bounded prefix of original pad owners800C0C00/800C0D68. Default bytes are
// unknown. The source BIOS packet producer and native control adapter are
// separate from the scalar80012298 consumer below.
constexpr size_t kSourceMainLoopPadBytes=72;
struct SourceMainLoopPadWork {
    std::array<uint8_t,kSourceMainLoopPadBytes> raw{},known{};
};
using SourceMainLoopPads=std::array<SourceMainLoopPadWork,2>;
// Exact cold executable80011F64/78→800177CC zero-clear subset. This operation
// is explicit; warm scene/save resets must preserve the retained pad state.
void initialize_source_main_loop_pads_from_executable_boot(SourceMainLoopPads&) noexcept;
// Packet layout:0=status(FF unavailable),1=type,2..3=active-low buttons,
//4..7=extra raw word. A native adapter must label its own input translation.
void bind_source_main_loop_pad_packet(SourceMainLoopPadWork&,
    const std::array<uint8_t,8>& packet) noexcept;
enum class SourceMainLoopPadStatus : uint8_t { Consumed,NeedsField };
struct SourceMainLoopPadResult {
    SourceMainLoopPadStatus status=SourceMainLoopPadStatus::NeedsField;
    bool committed=false;
    uint8_t missing_port=0;
    std::optional<uint8_t> missing_offset;
};
// Two-port exact decode/repeat stores. All required owner fields are
// preflighted by local copies, so missing port1 cannot partially advance port0.
SourceMainLoopPadResult consume_source_main_loop_pads_80012298(SourceMainLoopPads&) noexcept;
std::optional<uint16_t> source_main_loop_pad_pressed_800c0c2a(
    const SourceMainLoopPadWork&) noexcept;

enum class SourceMainLoopPhaseStatus : uint8_t {
    ReadyForUpdate,NotScheduled,NeedsMainPhase,NeedsSubphase,
    NeedsPhaseConsumer,NeedsBranchOwners
};
struct SourceMainLoopPhaseResult {
    SourceMainLoopPhaseStatus status=SourceMainLoopPhaseStatus::NeedsMainPhase;
    uint32_t source_function=0x800155d4;
    uint32_t update_pass=0,e0_sweep=0;
};
// Signed byte800C1B10 selects the cooperative phase callback; selected phase3
// uses signed byte800C1B11. Only unconditional calls to80015DB4 are admitted.
// Subphase2 also admits the exact active-event fast path: signed byte800C356C
// is0 and event-active byte800C4C48 is nonzero. Other conditional paths hold.
// This gate proves
// dispatch eligibility only, not the preceding actor-family consumers.
SourceMainLoopPhaseResult evaluate_source_main_loop_phase_800155d4(
    std::optional<int8_t> phase_800c1b10,
    std::optional<int8_t> subphase_800c1b11,
    std::optional<int8_t> byte_800c356c={},
    std::optional<uint8_t> active_800c4c48={}) noexcept;

struct SourceMainLoopScratchOwners {
    std::optional<uint8_t> byte_1f800000,byte_1f800001;
    std::optional<uint16_t> scratch_half_1f800002;
};
// Original80012004..8001201C writes only bytes0/1; halfword2 is preserved,
// including UNKNOWN. BSS/BIOS/native frame count do not supply it.
void initialize_source_main_loop_scratch_prefix_80012004(SourceMainLoopScratchOwners&) noexcept;
enum class SourceMainLoopGateStatus : uint8_t {
    Ready,WaitingForVblank,NeedsCadenceByte,NeedsVblankWord
};
SourceMainLoopGateStatus evaluate_source_main_loop_gate_80011ce8(
    std::optional<uint8_t> byte_1f800001,
    std::optional<uint32_t> word_80098888) noexcept;
// Original installed callback800123FC increments the shared vblank word.
// Caller must establish the source callback boundary; no host frame is used.
bool consume_source_main_loop_vblank_800123fc(std::optional<uint32_t>&) noexcept;
} // namespace l3
