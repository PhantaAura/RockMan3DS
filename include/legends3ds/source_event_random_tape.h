#pragma once
#include "legends3ds/source_event_random_effect.h"

namespace l3 {
constexpr size_t kSourceEventRandomTapeHeaderBytes=256,kSourceEventRandomTapeRecordBytes=144;
constexpr size_t kSourceEventRandomTapeMaximumRecords=64;
using SourceEventRandomTapeDigest=std::array<uint8_t,32>;
enum class SourceEventRandomTapeMode : uint32_t { Disabled=0,RecordedOpenBiosScene=1 };
struct SourceEventRandomTapeConstructor {
    uint16_t scratch_half=0;
    uint8_t scratch0=0,cadence=0,main_phase=0,subphase=0,event_id=0,event_phase=0;
};
struct SourceEventRandomTapeRecord {
    uint32_t event_frame=0,source_actor=0;
    uint8_t actor_state08=0,count=0;
    uint16_t scratch_half=0;
    std::array<uint16_t,3> position{};
    std::array<uint32_t,2> pool80_before{};
    uint64_t global_ordinal_before=0,global_ordinal_after=0;
    std::array<SourceEventRandomEffectReceipt,6> calls{};
};
struct SourceEventRandomTapeProfile {
    SourceEventRandomTapeMode scope=SourceEventRandomTapeMode::RecordedOpenBiosScene;
    uint32_t record_count=0;
    SourceEventRandomTapeDigest exe_sha256{},overlay_sha256{},ebd_sha256{},bios_sha256{};
    SourceEventRandomTapeDigest semantic_trace_sha256{},global_trace_sha256{};
    SourceEventRandomTapeConstructor constructor;
    std::array<SourceEventRandomTapeRecord,kSourceEventRandomTapeMaximumRecords> records{};
};
enum class SourceEventRandomTapeStatus : uint8_t {
    Ready,Disabled,NeedsProfile,InvalidProfile,NeedsOwner,StaleOwner,OwnerMismatch,
    NeedsEventWork,NeedsActorFields,ContextMismatch,TapeExhausted,AwaitingCommit,InvalidCommit
};
bool is_supported_source_event_random_tape_profile(const SourceEventRandomTapeProfile&) noexcept;
SourceEventRandomTapeStatus load_l3source_event_random_tape(const uint8_t*,size_t,
    SourceEventRandomTapeProfile&) noexcept;
class SourceEventRandomTapeService;
struct SourceEventRandomTapePrepared {
    SourceEventRandomTapeStatus status=SourceEventRandomTapeStatus::NeedsProfile;
    SourceEventRandomEffectReceipts receipts;
    SourceEventRandomEffectReceiptOwner receipt_owner;
    const SourceEventRandomTapeService* service=nullptr;
    uint32_t cursor=0;
    uint64_t generation=0;
};
// Caller supplies a checked active P5 residency receipt. Its bool must come
// from matching the P5 witness/profile and current EBD; it is not a fallback.
struct SourceEventRandomTapeContext {
    const SourceEventWork* work=nullptr;
    const SourceEventPhase5Context* scene=nullptr;
    uint32_t callback=0;
    std::optional<uint16_t> scratch_half;
    bool resource_owner_admitted=false;
    const void* resource_owner=nullptr;
    const void* current_resource_owner=nullptr;
    uint64_t resource_generation=0,current_resource_generation=0;
    SourceEventRandomTapeDigest exe_sha256{},overlay_sha256{},ebd_sha256{},bios_sha256{};
};
class SourceEventRandomTapeService {
public:
    bool admitted() const noexcept{return admitted_;}
    uint32_t cursor() const noexcept{return cursor_;}
    uint64_t generation() const noexcept{return generation_;}
    const SourceEventRandomTapeProfile& profile() const noexcept{return profile_;}
private:
    SourceEventRandomTapeProfile profile_;
    uint32_t cursor_=0;
    uint64_t generation_=0;
    bool admitted_=false;
    friend struct SourceEventRandomTapeAccess;
};
// Optional RECORDED_OPENBIOS_SCENE replay. No seed, recurrence, host frame or
// generated random result exists in this service. All cursor updates are bounded.
SourceEventRandomTapeStatus install_source_event_random_tape(SourceEventRandomTapeService&,
    const SourceEventRandomTapeProfile&,SourceEventRandomTapeMode) noexcept;
void invalidate_source_event_random_tape(SourceEventRandomTapeService&) noexcept;
SourceEventRandomTapePrepared prepare_source_event_random_tape(const SourceEventRandomTapeService&,
    const SourceEventRandomTapeContext&) noexcept;
// A held callback leaves the cursor/generation unchanged. A successful original
// callback advances exactly one record, including an original zero-call frame.
// The caller must pass completed D6C prefix execution, not a downstream receipt.
SourceEventRandomTapeStatus commit_source_event_random_tape(SourceEventRandomTapeService&,
    const SourceEventRandomTapePrepared&,bool callback_completed,uint8_t consumed_count) noexcept;
} // namespace l3
