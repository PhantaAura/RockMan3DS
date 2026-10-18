#include "legends3ds/retail_rng_tape.h"

namespace l3 {
namespace {
bool nonzero(const RngTraceDigest& digest) noexcept {
    for(const auto byte:digest) if(byte!=0)return true;
    return false;
}
bool valid_actor(const RetailRngActorContext& actor) noexcept {
    return actor.slot<16 && actor.work==0x8009f108u+uint32_t(actor.slot)*0x3c0u;
}
bool same_actor(const RetailRngActorContext& a,const RetailRngActorContext& b) noexcept {
    return a.work==b.work && a.slot==b.slot && a.state08==b.state08 &&
        a.action09==b.action09 && a.phase0a==b.phase0a &&
        a.stats_index0c==b.stats_index0c && a.context_high0f==b.context_high0f &&
        a.raw_position14==b.raw_position14 && a.heading56==b.heading56;
}
}

struct RetailRngTapeAccess {
    static const RetailRngObservedSample* samples(const RetailRngTape& t) noexcept { return t.samples_; }
    static const RngTraceDigest& digest(const RetailRngTape& t) noexcept { return t.digest_; }
    static void begin(const RetailRngTape& t,RetailRngTapeCursor& c) noexcept {
        c=RetailRngTapeCursor{};c.origin_=t.samples_;c.digest_=t.digest_;c.generation_=t.generation_;
    }
    static bool same_cursor(const RetailRngTape& t,const RetailRngTapeCursor& c) noexcept {
        return c.origin_==t.samples_ && c.digest_==t.digest_ && c.generation_==t.generation_;
    }
    static void advance(RetailRngTapeCursor& c) noexcept { ++c.next_sample_; }
    static RetailRngSampleTicket ticket(const RetailRngTape& t,size_t index) noexcept {
        RetailRngSampleTicket out;
        out.sample_index=index;out.global_ordinal=t.samples_[index].global_ordinal;
        out.sample32a=static_cast<uint16_t>(t.samples_[index].actual_v0);
        out.origin_=t.samples_;out.digest_=t.digest_;out.generation_=t.generation_;return out;
    }
    static bool same_ticket(const RetailRngTape& t,const RetailRngSampleTicket& ticket) noexcept {
        return ticket.origin_==t.samples_ && ticket.digest_==t.digest_ && ticket.generation_==t.generation_;
    }
};

RetailRngTapeStatus bind_retail_rng_tape(RetailRngTape& tape,
    const RetailRngObservedSample* samples,size_t count,
    const RetailRngTraceProof& proof) noexcept {
    // A failed replacement must revoke any earlier view, not leave old samples
    // accidentally usable under a newly requested encounter/source identity.
    const uint64_t next_generation=tape.generation_+1;
    tape=RetailRngTape{};tape.generation_=next_generation;
    if(next_generation==0 || !samples || !count || count>kRetailRngTapeMaximumSamples ||
       !nonzero(proof.trace_sha256) || !proof.supported_source_identities ||
       !proof.whole_trace_validated || !proof.continuous_since_installation ||
       proof.pending_return_at_end || proof.actual_rand_returns!=count || proof.actual_reseed_returns!=0)
        return RetailRngTapeStatus::NeedsValidatedTrace;
    uint32_t previous_seed=proof.initial_seed32,previous_vsync=0;
    for(size_t index=0;index<count;++index) {
        const auto& sample=samples[index];
        const uint32_t expected_seed=sample.seed_before32*1103515245u+12345u;
        if(sample.global_ordinal!=index+1 || sample.reference_vsync<previous_vsync ||
           sample.seed_before32!=previous_seed || sample.seed_after32!=expected_seed ||
           sample.actual_v0!=((expected_seed>>16)&0x7fffu) ||
           sample.caller_ra==0 || (sample.caller_ra&3u)!=0 ||
           (sample.class03 && (!valid_actor(*sample.class03)||sample.caller_ra!=0x801022f8u)) ||
           (!sample.class03 && sample.caller_ra==0x801022f8u))
            return RetailRngTapeStatus::InvalidSample;
        previous_seed=sample.seed_after32;previous_vsync=sample.reference_vsync;
    }
    tape.samples_=samples;tape.count_=count;tape.digest_=proof.trace_sha256;
    return RetailRngTapeStatus::Ready;
}

RetailRngTapeStatus begin_retail_rng_tape(const RetailRngTape& tape,
    RetailRngTapeCursor& cursor) noexcept {
    RetailRngTapeAccess::begin(tape,cursor);
    return tape.admitted()?RetailRngTapeStatus::Ready:RetailRngTapeStatus::NeedsValidatedTrace;
}

RetailRngSamplePeek peek_retail_rng_sample(const RetailRngTape& tape,
    const RetailRngTapeCursor& cursor,
    const std::optional<RetailRngReplayBoundary>& boundary,
    uint32_t caller_ra,const std::optional<RetailRngActorContext>& actor) noexcept {
    RetailRngSamplePeek out;
    if(!tape.admitted() || !RetailRngTapeAccess::same_cursor(tape,cursor))return out;
    if(!boundary || !boundary->input_timeline_matches || !boundary->callback_schedule_matches ||
       boundary->trace_sha256!=RetailRngTapeAccess::digest(tape)) {
        out.status=RetailRngTapeStatus::NeedsChronologyBinding;return out;
    }
    if(cursor.position()>=tape.size()) {out.status=RetailRngTapeStatus::Exhausted;return out;}
    const auto& sample=RetailRngTapeAccess::samples(tape)[cursor.position()];
    if(sample.global_ordinal!=boundary->global_ordinal ||
       sample.reference_vsync!=boundary->reference_vsync || sample.caller_ra!=caller_ra) {
        out.status=RetailRngTapeStatus::NextConsumerMismatch;return out;
    }
    if(sample.class03.has_value()!=actor.has_value() ||
       (sample.class03 && (!valid_actor(*actor)||!same_actor(*sample.class03,*actor)))) {
        out.status=RetailRngTapeStatus::ActorContextMismatch;return out;
    }
    out.status=RetailRngTapeStatus::Ready;
    out.ticket=RetailRngTapeAccess::ticket(tape,cursor.position());return out;
}

RetailRngTapeStatus commit_retail_rng_sample(const RetailRngTape& tape,
    RetailRngTapeCursor& cursor,const RetailRngSampleTicket& ticket) noexcept {
    if(!tape.admitted() || !RetailRngTapeAccess::same_cursor(tape,cursor))return RetailRngTapeStatus::NeedsValidatedTrace;
    if(cursor.position()>=tape.size())return RetailRngTapeStatus::Exhausted;
    const auto& sample=RetailRngTapeAccess::samples(tape)[cursor.position()];
    if(!RetailRngTapeAccess::same_ticket(tape,ticket) || ticket.sample_index!=cursor.position() ||
       ticket.global_ordinal!=sample.global_ordinal || ticket.sample32a!=sample.actual_v0)
        return RetailRngTapeStatus::StaleTicket;
    RetailRngTapeAccess::advance(cursor);return RetailRngTapeStatus::Ready;
}
} // namespace l3
