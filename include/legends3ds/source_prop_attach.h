#pragma once
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/reference_tables.h"
namespace l3 {
// Original80112D14 calls80031988 even though its record has an E0 identity.
// That helper computes family80-style mask addresses from the actual pointer.
// This typed external owner preserves the resulting alias; it is NOT an E0
// free operation. Missing words or wrong keys hold the entire attachment.
struct SourcePropForeignReleaseOwner {
    uint32_t prop_identity=0,pending_address=0,used_address=0;
    std::optional<uint32_t> pending_word,used_word;
    bool admitted=false;
    // For an admitted count owner, the exact word may remain unknown while
    // independently proved bits are known. Unknown bits have no fabricated
    // numeric value. Exact used_word (legacy authored receipts) takes priority.
    uint32_t used_bits_value=0,used_bits_known_mask=0;
};
enum class SourcePropForeignGlobalsStatus : uint8_t {
    Ready,NeedsFreshExecutableLoad,StaleSourceGeneration,NeedsRoster,
    UncheckedQueueProducerPresent,UnknownForeignAddress,NeedsKnownResult,StaleReceipt
};
class SourcePropForeignGlobals;
struct SourcePropForeignReleasePrepared {
    SourcePropForeignGlobalsStatus status=SourcePropForeignGlobalsStatus::NeedsFreshExecutableLoad;
    SourcePropForeignReleaseOwner owner;
    const SourcePropForeignGlobals* service=nullptr;
    uint64_t source_generation=0,revision=0;
    uint32_t pending_before=0;
};
// Original loaded EXE words988FC/9898C are zero. Capped enqueue/drain owners
// preserve counter range0..256; its low9bits stay unknown. Tutorial E0/subtype1
// has an unchecked writer, so its presence/unknown roster revokes the bound.
// This does not count native draw calls or pretend skipped source packets ran.
class SourcePropForeignGlobals {
public:
    bool admitted() const noexcept {return loaded_;}
    bool queue_bound_admitted() const noexcept {return queue_bound_;}
    uint64_t source_generation() const noexcept {return generation_;}
    std::optional<uint32_t> pending_8009898c() const noexcept {return loaded_?std::optional<uint32_t>{pending_}:std::nullopt;}
    static constexpr uint32_t counter_known_zero_mask=0xfffffe00u;
private:
    uint64_t generation_=0,revision_=0;
    uint32_t pending_=0;
    bool loaded_=false,queue_bound_=false;
    friend void initialize_source_prop_foreign_globals_from_executable_load(SourcePropForeignGlobals&,uint64_t,bool) noexcept;
    friend void invalidate_source_prop_foreign_globals(SourcePropForeignGlobals&) noexcept;
    friend SourcePropForeignReleasePrepared prepare_source_prop_foreign_release(SourcePropForeignGlobals&,
        uint32_t,uint64_t,const SourceEventPhase5Pool<32,80>&) noexcept;
    friend bool commit_source_prop_foreign_release(SourcePropForeignGlobals&,const SourcePropForeignReleasePrepared&,bool) noexcept;
};
// Caller passes the real fresh supported-executable-load lifecycle, not an area
// reset/warm restore. A false receipt keeps both original words unknown.
void initialize_source_prop_foreign_globals_from_executable_load(SourcePropForeignGlobals&,
    uint64_t source_generation,bool supported_fresh_executable_load) noexcept;
void invalidate_source_prop_foreign_globals(SourcePropForeignGlobals&) noexcept;
SourcePropForeignReleasePrepared prepare_source_prop_foreign_release(SourcePropForeignGlobals&,
    uint32_t prop_identity,uint64_t current_source_generation,
    const SourceEventPhase5Pool<32,80>& current_roster) noexcept;
// Call only after the entire attachment callback commits its release. The
// pending side effect is published under the same checked one-use revision.
bool commit_source_prop_foreign_release(SourcePropForeignGlobals&,
    const SourcePropForeignReleasePrepared&,bool complete_release_callback) noexcept;
struct SourcePropAttachInputs {
    const ReferenceTables* references=nullptr;
    const SourceEventWork* event_work=nullptr;
    const SourceEventPhase5Pool<4,1444>* actors=nullptr;
    SourcePropForeignReleaseOwner* release_owner=nullptr;
};
enum class SourcePropAttachStatus : uint8_t {
    SourceBackedPartial,Inactive,InvalidIdentity,UnsupportedProp,UnknownState,
    NeedsPropFields,NeedsEventOwner,NeedsActorOwner,NeedsActorFields,
    NeedsReferences,NeedsForeignReleaseOwner,ForeignReleaseMismatch
};
struct SourcePropAttachResult {
    SourcePropAttachStatus status=SourcePropAttachStatus::SourceBackedPartial;
    bool committed=false,released=false;
    uint32_t source_function=0;
    uint8_t invalidated_opaque_lanes=0;
    std::optional<uint16_t> missing_prop_offset,missing_actor_offset;
};
// Proved parameter0 states1/2/3 (80112D14/80112E80/80112F70), using the
// actual event+0x24 class40 pose bank. Integer GTE IR rounding/clipping is kept.
// State1's12 unowned temporary stack-copy lanes become UNKNOWN; raw bytes
// survive, rather than acquiring invented zero values. No heap allocation.
SourcePropAttachResult update_source_prop_attach_80112d14(
    SourceEventPhase5Bytes<80>& prop,uint32_t prop_source_identity,
    const SourcePropAttachInputs&) noexcept;
} // namespace l3
