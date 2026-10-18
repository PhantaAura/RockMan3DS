#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace l3 {

// A reference capture, not a generator. Installation may be MID_SESSION and
// the installed OpenBIOS profile is not a proved Sony retail BIOS profile.
using RngTraceDigest = std::array<uint8_t,32>;
constexpr size_t kRetailRngTapeMaximumSamples = 65536;

struct RetailRngActorContext {
    uint32_t work = 0;
    uint8_t slot = 0;
    uint8_t state08 = 0, action09 = 0, phase0a = 0;
    uint8_t stats_index0c = 0, context_high0f = 0;
    std::array<int16_t,3> raw_position14{};
    uint16_t heading56 = 0;
};

struct RetailRngObservedSample {
    uint32_t global_ordinal = 0, reference_vsync = 0;
    uint32_t caller_ra = 0;
    uint32_t seed_before32 = 0, seed_after32 = 0, actual_v0 = 0;
    std::optional<RetailRngActorContext> class03;
};

// The loader must obtain these facts from full strict trace validation, never
// from a failed-prefix proof or a caller's hope that captures are accurate.
// Only the currently hash-guarded installed BIOS/EXE/overlay profile is allowed.
struct RetailRngTraceProof {
    RngTraceDigest trace_sha256{};
    bool supported_source_identities = false;
    bool whole_trace_validated = false;
    bool continuous_since_installation = false;
    bool pending_return_at_end = true;
    uint32_t actual_rand_returns = 0;
    uint32_t actual_reseed_returns = 0;
    uint32_t initial_seed32 = 0;
};

enum class RetailRngTapeStatus : uint8_t {
    Ready,
    NeedsValidatedTrace,
    InvalidSample,
    NeedsChronologyBinding,
    Exhausted,
    NextConsumerMismatch,
    ActorContextMismatch,
    StaleTicket
};

// Independently measured input/update chronology must establish this boundary.
// Merely copying native frame counters, finding a similar position or checking
// BIOS recurrence does NOT make either match true. No default binding exists.
struct RetailRngReplayBoundary {
    RngTraceDigest trace_sha256{};
    uint32_t reference_vsync = 0, global_ordinal = 0;
    bool input_timeline_matches = false;
    bool callback_schedule_matches = false;
};

class RetailRngTape {
public:
    size_t size() const noexcept { return count_; }
    bool admitted() const noexcept { return samples_ != nullptr; }
private:
    const RetailRngObservedSample* samples_ = nullptr;
    size_t count_ = 0;
    RngTraceDigest digest_{};
    uint64_t generation_ = 0;
    friend RetailRngTapeStatus bind_retail_rng_tape(RetailRngTape&,
        const RetailRngObservedSample*,size_t,const RetailRngTraceProof&) noexcept;
    friend struct RetailRngTapeAccess;
};

class RetailRngTapeCursor {
public:
    size_t position() const noexcept { return next_sample_; }
private:
    size_t next_sample_ = 0;
    const RetailRngObservedSample* origin_ = nullptr;
    RngTraceDigest digest_{};
    uint64_t generation_ = 0;
    friend struct RetailRngTapeAccess;
};
struct RetailRngSampleTicket {
    size_t sample_index = 0;
    uint32_t global_ordinal = 0;
    uint16_t sample32a = 0;
private:
    const RetailRngObservedSample* origin_ = nullptr;
    RngTraceDigest digest_{};
    uint64_t generation_ = 0;
    friend struct RetailRngTapeAccess;
};

struct RetailRngSamplePeek {
    RetailRngTapeStatus status = RetailRngTapeStatus::NeedsValidatedTrace;
    std::optional<RetailRngSampleTicket> ticket;
};

// The immutable sample owner must outlive the view and every ticket. This call
// performs bounded structural/profile validation once, outside the hot loop.
// All observed global random calls remain in order, including other consumers.
// Reseeding, omitted consumers and gaps are unsupported, rather than filled.
RetailRngTapeStatus bind_retail_rng_tape(RetailRngTape& tape,
    const RetailRngObservedSample* samples,size_t count,
    const RetailRngTraceProof& proof) noexcept;

// Start once at the validated installation boundary. Replacing/unloading a
// tape invalidates every cursor/ticket tied to its prior immutable view.
RetailRngTapeStatus begin_retail_rng_tape(const RetailRngTape& tape,
    RetailRngTapeCursor& cursor) noexcept;

// No mutation/allocation. Class03 requires its exact original pool identity and
// captured pre-state. Other consumers require their own exact caller identity;
// they cannot be silently skipped to reach the desired enemy sample.
RetailRngSamplePeek peek_retail_rng_sample(const RetailRngTape& tape,
    const RetailRngTapeCursor& cursor,
    const std::optional<RetailRngReplayBoundary>& boundary,
    uint32_t caller_ra,const std::optional<RetailRngActorContext>& actor) noexcept;

// Call only AFTER the complete consuming transaction commits. Failed motion,
// pose/audio, event or lifecycle consumers leave this cursor unchanged. This
// API never wraps, repeats a scalar, rewinds or seeds a private actor generator.
RetailRngTapeStatus commit_retail_rng_sample(const RetailRngTape& tape,
    RetailRngTapeCursor& cursor,const RetailRngSampleTicket& ticket) noexcept;

} // namespace l3
