#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {
using SourceLoopSnapshotDigest=std::array<uint8_t,32>;
constexpr size_t kSourceLoopSnapshotBytes=176;
constexpr uint32_t kSourceLoopSnapshotConstructorPc=0x80100efc;
enum class SourceLoopSnapshotScope : uint32_t { RecordedOpenBiosConstructor=1 };
// An optional owner-local recorded boundary. It is not a retail BIOS default,
// recovered native boot state, or a value derived from host frames/buttons.
struct SourceLoopSnapshot {
    SourceLoopSnapshotScope scope=SourceLoopSnapshotScope::RecordedOpenBiosConstructor;
    uint32_t pc=0,loops=0;
    uint16_t scratch_half=0;
    uint8_t cadence=0,event_id=0,event_phase=0,main_phase=0,subphase=0,scratch0=0;
    SourceLoopSnapshotDigest exe_sha256{},overlay_sha256{},ebd_sha256{},bios_sha256{};
};
enum class SourceLoopSnapshotStatus : uint8_t {
    Ready,InvalidSnapshot,Disabled,NeedsSnapshot,NeedsConstructorReceipt,
    NeedsBoundary,WrongBoundary,NeedsOwner,StaleOwner,OwnerMismatch,AlreadyAdmitted
};
using SourceLoopSnapshotBytes=std::array<uint8_t,kSourceLoopSnapshotBytes>;
bool is_supported_source_loop_snapshot(const SourceLoopSnapshot&) noexcept;
// Canonical L3LS version1. Header/scalars/all four identities/CRC/reserved
// bytes are strict. Failed decode/encode never changes the caller's output.
SourceLoopSnapshotStatus load_l3source_loop_snapshot(const uint8_t*,size_t,
    SourceLoopSnapshot&) noexcept;
SourceLoopSnapshotStatus encode_l3source_loop_snapshot(const SourceLoopSnapshot&,
    SourceLoopSnapshotBytes&) noexcept;

struct SourceLoopSnapshotBoundary {
    bool recorded_replay_selected=false,constructor_consumed=false,already_admitted=false;
    // Source pre-constructor event fields plus the successful constructor
    // receipt. Main/subphase are explicitly recorded replay owners; they must
    // not be described as recovered live native global state.
    std::optional<uint32_t> source_pc;
    std::optional<uint8_t> event_active,event_id,event_phase,main_phase,subphase,cadence,scratch0;
    SourceLoopSnapshotDigest exe_sha256{},overlay_sha256{},ebd_sha256{},bios_sha256{};
    // Caller supplies the admitted resident owner and current lifecycle token.
    // Accepted area/save/profile changes must invalidate their generation.
    const void* owner=nullptr;
    const void* current_owner=nullptr;
    uint64_t generation=0,current_generation=0;
};
// Pure admission predicate: no stores, counter initialization, frame sampling,
// lifecycle mutation, or hardware claim. Root owns the one-shot publication.
SourceLoopSnapshotStatus evaluate_source_loop_snapshot_admission(
    const SourceLoopSnapshot*,const SourceLoopSnapshotBoundary&) noexcept;
} // namespace l3
