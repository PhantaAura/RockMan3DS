#include "legends3ds/source_rng_service.h"
#include <algorithm>
#include <limits>

namespace l3 {
namespace {
constexpr uint8_t nibble(char c) { return static_cast<uint8_t>(c <= '9' ? c - '0' : c - 'a' + 10); }
constexpr SourceRngDigest digest(const char* s) {
    SourceRngDigest out{};
    for (size_t i = 0; i != out.size(); ++i) out[i] = static_cast<uint8_t>((nibble(s[i*2]) << 4) | nibble(s[i*2+1]));
    return out;
}
constexpr auto kBios = digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
constexpr auto kExe = digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
constexpr auto kOverlay = digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
constexpr uint32_t kRequiredWarnings = SourceRngUnmodeledConsumers | SourceRngSelectedReferenceNotRetail;
constexpr uint32_t kAllWarnings = 63;
uint32_t word(const uint8_t* p) noexcept {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
uint32_t checksum(const uint8_t* p, size_t n) noexcept {
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i != n; ++i) {
        crc ^= p[i];
        for (unsigned bit = 0; bit != 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
bool nonzero(const SourceRngDigest& d) noexcept {
    for (auto b : d) if (b != 0) return true;
    return false;
}
bool valid(const SourceRngProfile& p) noexcept {
    if (p.bios_sha256 != kBios || p.exe_sha256 != kExe || p.overlay_sha256 != kOverlay ||
        !nonzero(p.trace_sha256) || !nonzero(p.validated_prefix_sha256) ||
        (p.warnings & kRequiredWarnings) != kRequiredWarnings || (p.warnings & ~kAllWarnings) ||
        p.origin_rand_ordinal != 0 || p.origin_game_entry_count != 1 ||
        p.actual_rand_returns == 0 || p.actual_rand_returns > 32767 || p.actual_reseed_returns != 0 ||
        p.final_rand_ordinal != p.actual_rand_returns || p.observed_consumer_count == 0 ||
        p.observed_consumer_count > p.actual_rand_returns || p.trace_rows > 65536 ||
        p.validated_prefix_rows < 2u + p.actual_rand_returns * 2u ||
        p.validated_prefix_rows > p.trace_rows || p.excluded_rows != p.trace_rows - p.validated_prefix_rows ||
        p.captured_bytes == 0 || p.captured_bytes > 16u * 1024u * 1024u ||
        p.validated_prefix_bytes < p.validated_prefix_rows || p.validated_prefix_bytes > p.captured_bytes) return false;
    if (p.scope == SourceRngCaptureScope::WholeBoundedCapture)
        return p.warnings == kRequiredWarnings && p.excluded_rows == 0 &&
            p.validated_prefix_rows >= 3u + p.actual_rand_returns * 2u &&
            p.captured_bytes == p.validated_prefix_bytes && p.trace_sha256 == p.validated_prefix_sha256;
    if (p.scope != SourceRngCaptureScope::CompletedCapturePrefix || !(p.warnings & SourceRngCaptureIncomplete)) return false;
    if ((p.warnings & (SourceRngObserverFailed | SourceRngPendingCallExcluded)) && p.excluded_rows == 0) return false;
    if ((p.warnings & SourceRngPartialLineExcluded) && p.captured_bytes == p.validated_prefix_bytes) return false;
    const uint32_t excluded = uint32_t(bool(p.warnings & SourceRngObserverFailed)) + uint32_t(bool(p.warnings & SourceRngPendingCallExcluded));
    if (p.excluded_rows != excluded || (excluded != 0 && p.captured_bytes == p.validated_prefix_bytes)) return false;
    return true;
}
} // namespace

struct SourceRngAccess {
    static SourceRngStatus install(SourceRngService& s, const SourceRngProfile& p, SourceRngMode mode) noexcept {
        if (mode == SourceRngMode::Disabled) {
            s.admitted_ = false; s.last_count_ = 0; ++s.generation_; return SourceRngStatus::Disabled;
        }
        if (mode != SourceRngMode::CandidateOpenBios || !valid(p)) return SourceRngStatus::InvalidProfile;
        s.profile_ = p; s.seed_ = p.origin_seed32; s.ordinal_ = p.origin_rand_ordinal;
        s.last_count_ = 0; s.admitted_ = true; ++s.generation_; return SourceRngStatus::Ready;
    }
    static SourceRngStatus begin(const SourceRngService& s, SourceRngUpdateTransaction& t, const SourceRngUpdateContext& context) noexcept {
        if (!s.admitted_) return SourceRngStatus::NeedsObservedProfile;
        if (context.phase != SourceRngUpdatePhase::Class03Callbacks &&
            context.phase != SourceRngUpdatePhase::Effect2Callback &&
            context.phase != SourceRngUpdatePhase::SceneEventCallback &&
            context.phase != SourceRngUpdatePhase::PropLiftCallback &&
            context.phase != SourceRngUpdatePhase::Effect34Callback &&
            context.phase != SourceRngUpdatePhase::RewardPickupCallback &&
            context.phase != SourceRngUpdatePhase::E0ObjectCallback) return SourceRngStatus::InvalidContext;
        // Only the staged prefix is observable. Do not clear a120-call burst
        // buffer again when preparing a one-call actor transaction.
        t.count_=0;t.expected_calls_=0;t.used_slots_=0;
        t.owner_ = &s; t.generation_ = s.generation_; t.seed_ = s.seed_; t.ordinal_ = s.ordinal_;
        t.context_ = context; t.active_ = true; return SourceRngStatus::Ready;
    }
    static SourceRngStagedSample stage(SourceRngUpdateTransaction& t, const SourceRngClass03Context& actor) noexcept {
        SourceRngStagedSample out;
        if (!t.active_ || !t.owner_ || !t.owner_->admitted_ || t.generation_ != t.owner_->generation_) {
            out.status = SourceRngStatus::StaleTransaction; return out;
        }
        if (t.context_.phase != SourceRngUpdatePhase::Class03Callbacks ||
            (actor.caller_ra != kSourceClass03RngCallerRa && actor.caller_ra != kSourceClass1FRngCallerRa &&
             actor.caller_ra != kSourceClass19RngCallerRa) ||
            actor.slot >= 16 ||
            actor.work != 0x8009f108u + uint32_t(actor.slot) * 0x3c0u) {
            out.status = SourceRngStatus::InvalidContext; return out;
        }
        if (t.used_slots_ & (1u << actor.slot)) { out.status = SourceRngStatus::DuplicateCallback; return out; }
        if (t.count_ >= t.calls_.size()) { out.status = SourceRngStatus::UpdateFull; return out; }
        if (t.ordinal_ == std::numeric_limits<uint64_t>::max()) { out.status = SourceRngStatus::OrdinalExhausted; return out; }
        auto& row = t.calls_[t.count_];
        row=SourceRngCallRecord{};
        row.update = t.context_; row.actor = actor; row.seed_before32 = t.seed_;
        // Unsigned wrap and high15 return are independently checked against the
        // unchanged selected BIOS instructions; no actor-private state exists.
        t.seed_ = t.seed_ * 1103515245u + 12345u;
        row.seed_after32 = t.seed_; row.sample32a = static_cast<uint16_t>((t.seed_ >> 16) & 0x7fffu);
        row.native_global_ordinal = ++t.ordinal_;
        t.used_slots_ |= 1u << actor.slot; ++t.count_;
        out.status = SourceRngStatus::Ready; out.sample32a = row.sample32a; out.native_global_ordinal = row.native_global_ordinal;
        return out;
    }
    static SourceRngStagedSample effect2(SourceRngUpdateTransaction& t, const SourceRngEffect2Context& effect) noexcept {
        SourceRngStagedSample out;
        if (!t.active_ || !t.owner_ || !t.owner_->admitted_ || t.generation_ != t.owner_->generation_) {
            out.status = SourceRngStatus::StaleTransaction; return out;
        }
        if (t.context_.phase != SourceRngUpdatePhase::Effect2Callback || effect.slot >= 64 ||
            effect.work != 0x800a3b40u + uint32_t(effect.slot) * 72u ||
            effect.source_generation == 0 || effect.call_index >= 3 || effect.call_index != t.count_ ||
            (t.count_ && (t.calls_[0].actor.work != effect.work ||
                         t.calls_[0].source_generation != effect.source_generation))) {
            out.status = SourceRngStatus::InvalidContext; return out;
        }
        if (t.ordinal_ == std::numeric_limits<uint64_t>::max()) {
            out.status = SourceRngStatus::OrdinalExhausted; return out;
        }
        constexpr uint32_t ras[] = {0x80116f60u, 0x80116f98u, 0x80116fe4u};
        auto& row = t.calls_[t.count_];
        row=SourceRngCallRecord{};
        row.update = t.context_; row.actor.caller_ra = ras[effect.call_index];
        row.actor.work = effect.work; row.actor.slot = effect.slot;
        row.source_generation = effect.source_generation; row.seed_before32 = t.seed_;
        t.seed_ = t.seed_ * 1103515245u + 12345u;
        row.seed_after32 = t.seed_; row.sample32a = uint16_t((t.seed_ >> 16) & 0x7fffu);
        row.native_global_ordinal = ++t.ordinal_; ++t.count_;
        out.status = SourceRngStatus::Ready; out.sample32a = row.sample32a;
        out.native_global_ordinal = row.native_global_ordinal;
        return out;
    }
    static SourceRngStagedSample scene_event(SourceRngUpdateTransaction& t, const SourceRngSceneEventContext& event) noexcept {
        SourceRngStagedSample out;
        if (!t.active_ || !t.owner_ || !t.owner_->admitted_ || t.generation_ != t.owner_->generation_) {
            out.status = SourceRngStatus::StaleTransaction; return out;
        }
        if (t.context_.phase != SourceRngUpdatePhase::SceneEventCallback || !event.source_generation ||
            (event.call_count != 3 && event.call_count != 6) || event.call_index >= event.call_count ||
            event.call_index != t.count_ || (t.count_ &&
             (t.expected_calls_ != event.call_count || t.calls_[0].source_generation != event.source_generation))) {
            out.status = SourceRngStatus::InvalidContext; return out;
        }
        if (t.ordinal_ == std::numeric_limits<uint64_t>::max()) {
            out.status = SourceRngStatus::OrdinalExhausted; return out;
        }
        constexpr uint32_t ras[] = {0x80100dd4u,0x80100decu,0x80100e04u,0x80100e40u,0x80100e58u,0x80100e70u};
        auto& row=t.calls_[t.count_];row=SourceRngCallRecord{};row.update=t.context_;row.actor.caller_ra=ras[event.call_index];
        row.actor.work=0x800c4c48u;row.source_generation=event.source_generation;row.seed_before32=t.seed_;
        t.seed_=t.seed_*1103515245u+12345u;row.seed_after32=t.seed_;row.sample32a=uint16_t((t.seed_>>16)&0x7fffu);
        row.native_global_ordinal=++t.ordinal_;++t.count_;t.expected_calls_=event.call_count;
        out.status=SourceRngStatus::Ready;out.sample32a=row.sample32a;out.native_global_ordinal=row.native_global_ordinal;
        return out;
    }
    static SourceRngStagedSample prop_lift(SourceRngUpdateTransaction& t,const SourceRngPropLiftContext& prop) noexcept {
        SourceRngStagedSample out;
        if(!t.active_||!t.owner_||!t.owner_->admitted_||t.generation_!=t.owner_->generation_){out.status=SourceRngStatus::StaleTransaction;return out;}
        if(t.context_.phase!=SourceRngUpdatePhase::PropLiftCallback||prop.slot>=32||
           prop.work!=0x800bf118u+uint32_t(prop.slot)*80u||!prop.source_generation||
           prop.call_index>1||prop.call_index!=t.count_||(t.count_&&
           (t.calls_[0].actor.work!=prop.work||t.calls_[0].source_generation!=prop.source_generation))){out.status=SourceRngStatus::InvalidContext;return out;}
        if(t.ordinal_==std::numeric_limits<uint64_t>::max()){out.status=SourceRngStatus::OrdinalExhausted;return out;}
        constexpr uint32_t ras[]={0x80113238u,0x80113e54u};auto& row=t.calls_[t.count_];row=SourceRngCallRecord{};
        row.update=t.context_;row.actor.caller_ra=ras[prop.call_index];row.actor.work=prop.work;row.actor.slot=prop.slot;
        row.source_generation=prop.source_generation;row.seed_before32=t.seed_;t.seed_=t.seed_*1103515245u+12345u;
        row.seed_after32=t.seed_;row.sample32a=uint16_t((t.seed_>>16)&0x7fffu);row.native_global_ordinal=++t.ordinal_;++t.count_;
        out.status=SourceRngStatus::Ready;out.sample32a=row.sample32a;out.native_global_ordinal=row.native_global_ordinal;return out;
    }
    static SourceRngStagedSample effect34(SourceRngUpdateTransaction& t,const SourceRngEffect34Context& effect) noexcept {
        SourceRngStagedSample out;
        if(!t.active_||!t.owner_||!t.owner_->admitted_||t.generation_!=t.owner_->generation_){out.status=SourceRngStatus::StaleTransaction;return out;}
        const bool type3=effect.callback==0x801172fcu,type4=effect.callback==0x80117558u;
        const bool count_valid=type3?effect.call_count==2:(type4&&effect.call_count>=5&&effect.call_count<=120&&effect.call_count%5==0);
        if(t.context_.phase!=SourceRngUpdatePhase::Effect34Callback||!count_valid||effect.slot>=64||
           effect.work!=0x800a3b40u+uint32_t(effect.slot)*72u||!effect.source_generation||
           effect.call_index>=effect.call_count||effect.call_index!=t.count_||(t.count_&&
           (t.expected_calls_!=effect.call_count||t.calls_[0].actor.work!=effect.work||
            t.calls_[0].source_generation!=effect.source_generation||t.calls_[0].actor.caller_ra!=(type3?0x801173c4u:0x801177b4u)))){
            out.status=SourceRngStatus::InvalidContext;return out;
        }
        if(t.ordinal_==std::numeric_limits<uint64_t>::max()){out.status=SourceRngStatus::OrdinalExhausted;return out;}
        constexpr uint32_t ras3[]={0x801173c4u,0x80117408u};
        constexpr uint32_t ras4a[]={0x801177b4u,0x801177d0u,0x80117814u,0x801178a0u,0x801178b4u};
        constexpr uint32_t ras4b[]={0x801177e8u,0x80117804u,0x80117814u,0x801178a0u,0x801178b4u};
        const uint32_t ra=type3?ras3[effect.call_index]:(effect.call_index<60?ras4a[effect.call_index%5]:ras4b[effect.call_index%5]);
        auto& row=t.calls_[t.count_];row=SourceRngCallRecord{};row.update=t.context_;row.actor.caller_ra=ra;row.actor.work=effect.work;row.actor.slot=effect.slot;
        row.source_generation=effect.source_generation;row.seed_before32=t.seed_;t.seed_=t.seed_*1103515245u+12345u;
        row.seed_after32=t.seed_;row.sample32a=uint16_t((t.seed_>>16)&0x7fffu);row.native_global_ordinal=++t.ordinal_;++t.count_;t.expected_calls_=effect.call_count;
        out.status=SourceRngStatus::Ready;out.sample32a=row.sample32a;out.native_global_ordinal=row.native_global_ordinal;return out;
    }
    static SourceRngStagedSample reward(SourceRngUpdateTransaction& t,const SourceRngRewardContext& c) noexcept {
        SourceRngStagedSample out;
        if(!t.active_||!t.owner_||!t.owner_->admitted_||t.generation_!=t.owner_->generation_){out.status=SourceRngStatus::StaleTransaction;return out;}
        constexpr uint32_t ras[]={0x80046bacu,0x80046bf0u,0x80046c30u,0x80046dd8u,0x80046e18u,0x80046480u,0x800468ecu,
            0x80044b50u}; // + Buster impact 0x82 state 0
        bool known=false;for(auto ra:ras)known|=ra==c.caller_ra;
        const bool expansion_first=c.caller_ra==0x80046bacu;
        if(t.context_.phase!=SourceRngUpdatePhase::RewardPickupCallback||!known||!c.source_generation||
           (expansion_first?c.work!=0:(c.slot>=64||c.work!=0x800a3b40u+uint32_t(c.slot)*72u))){out.status=SourceRngStatus::InvalidContext;return out;}
        if(t.count_>=kSourceRngUpdateMaximumCalls){out.status=SourceRngStatus::UpdateFull;return out;}
        if(t.ordinal_==std::numeric_limits<uint64_t>::max()){out.status=SourceRngStatus::OrdinalExhausted;return out;}
        auto& row=t.calls_[t.count_];row=SourceRngCallRecord{};row.update=t.context_;row.actor.caller_ra=c.caller_ra;
        row.actor.work=c.work;row.actor.slot=c.slot;row.source_generation=c.source_generation;row.seed_before32=t.seed_;
        t.seed_=t.seed_*1103515245u+12345u;row.seed_after32=t.seed_;row.sample32a=uint16_t((t.seed_>>16)&0x7fffu);
        row.native_global_ordinal=++t.ordinal_;++t.count_;
        out.status=SourceRngStatus::Ready;out.sample32a=row.sample32a;out.native_global_ordinal=row.native_global_ordinal;return out;
    }
    static SourceRngStagedSample e0(SourceRngUpdateTransaction& t,const SourceRngE0Context& c) noexcept {
        SourceRngStagedSample out;
        if(!t.active_||!t.owner_||!t.owner_->admitted_||t.generation_!=t.owner_->generation_){out.status=SourceRngStatus::StaleTransaction;return out;}
        if(t.context_.phase!=SourceRngUpdatePhase::E0ObjectCallback||(c.caller_ra!=kSourceE005RngCallerRa&&c.caller_ra!=kSourceE008BlinkRngCallerRa&&
            std::find(kSourceE0BlockRngCallerRas.begin(),kSourceE0BlockRngCallerRas.end(),c.caller_ra)==kSourceE0BlockRngCallerRas.end())||
           c.slot>=32||c.work!=0x800bf118u+uint32_t(c.slot)*0x50u){out.status=SourceRngStatus::InvalidContext;return out;}
        if(t.count_>=kSourceRngUpdateMaximumCalls){out.status=SourceRngStatus::UpdateFull;return out;}
        if(t.ordinal_==std::numeric_limits<uint64_t>::max()){out.status=SourceRngStatus::OrdinalExhausted;return out;}
        auto& row=t.calls_[t.count_];row=SourceRngCallRecord{};row.update=t.context_;row.actor.caller_ra=c.caller_ra;
        row.actor.work=c.work;row.actor.slot=c.slot;row.seed_before32=t.seed_;
        t.seed_=t.seed_*1103515245u+12345u;row.seed_after32=t.seed_;row.sample32a=uint16_t((t.seed_>>16)&0x7fffu);
        row.native_global_ordinal=++t.ordinal_;++t.count_;
        out.status=SourceRngStatus::Ready;out.sample32a=row.sample32a;out.native_global_ordinal=row.native_global_ordinal;return out;
    }
    static SourceRngStatus commit(SourceRngService& s, SourceRngUpdateTransaction& t) noexcept {
        if (!s.admitted_ || !t.active_ || t.owner_ != &s || t.generation_ != s.generation_) return SourceRngStatus::StaleTransaction;
        if (t.context_.phase == SourceRngUpdatePhase::Effect2Callback && t.count_ != 3)
            return SourceRngStatus::InvalidContext;
        if (t.context_.phase == SourceRngUpdatePhase::SceneEventCallback &&
            (!t.expected_calls_ || t.count_ != t.expected_calls_)) return SourceRngStatus::InvalidContext;
        if (t.context_.phase == SourceRngUpdatePhase::PropLiftCallback && (t.count_ == 0 || t.count_ > 2))
            return SourceRngStatus::InvalidContext;
        if (t.context_.phase == SourceRngUpdatePhase::Effect34Callback &&
            (!t.expected_calls_ || t.count_ != t.expected_calls_)) return SourceRngStatus::InvalidContext;
        for(size_t i=0;i<t.count_;++i)s.last_calls_[i]=t.calls_[i];
        for(size_t i=t.count_;i<s.last_count_;++i)s.last_calls_[i]=SourceRngCallRecord{};
        s.seed_ = t.seed_; s.ordinal_ = t.ordinal_; s.last_count_ = t.count_;
        ++s.generation_; t.active_ = false; return SourceRngStatus::Ready;
    }
    static void rollback(SourceRngUpdateTransaction& t) noexcept { t.active_ = false; }
};

SourceRngStatus load_l3source_rng_profile(const uint8_t* b, size_t size, SourceRngProfile& result) noexcept {
    if (!b || size != kSourceRngProfileBytes || b[0] != 'L' || b[1] != '3' || b[2] != 'R' || b[3] != 'G' ||
        b[4] != 1 || b[5] != 0 || b[6] != 32 || b[7] != 0 || word(b+8) != 1 ||
        word(b+12) != size || word(b+16) != size-32 || word(b+24) != 0 || word(b+28) != 0 ||
        checksum(b+32, size-32) != word(b+20) || word(b+252) != 0) return SourceRngStatus::InvalidProfile;
    SourceRngProfile out;
    SourceRngDigest* digests[] = {&out.bios_sha256, &out.exe_sha256, &out.overlay_sha256, &out.trace_sha256, &out.validated_prefix_sha256};
    for (size_t i = 0; i != 5; ++i) std::copy_n(b+32+i*32, 32, digests[i]->begin());
    const uint8_t* p = b+192;
    out.scope = static_cast<SourceRngCaptureScope>(word(p)); out.warnings = word(p+4);
    out.origin_seed32 = word(p+8); out.origin_rand_ordinal = word(p+12); out.origin_vsync = word(p+16);
    out.origin_game_entry_count = word(p+20); out.actual_rand_returns = word(p+24); out.actual_reseed_returns = word(p+28);
    out.trace_rows = word(p+32); out.validated_prefix_rows = word(p+36); out.excluded_rows = word(p+40);
    out.captured_bytes = word(p+44); out.validated_prefix_bytes = word(p+48); out.final_rand_ordinal = word(p+52);
    out.observed_consumer_count = word(p+56);
    if (!valid(out)) return SourceRngStatus::InvalidProfile;
    result = out; return SourceRngStatus::Ready;
}
SourceRngStatus install_source_rng_service(SourceRngService& s, const SourceRngProfile& p, SourceRngMode mode) noexcept { return SourceRngAccess::install(s, p, mode); }
SourceRngStatus begin_source_rng_update(const SourceRngService& s, SourceRngUpdateTransaction& t, const SourceRngUpdateContext& c) noexcept { return SourceRngAccess::begin(s, t, c); }
SourceRngStagedSample stage_source_rng_class03(SourceRngUpdateTransaction& t, const SourceRngClass03Context& c) noexcept { return SourceRngAccess::stage(t, c); }
SourceRngStagedSample stage_source_rng_effect2(SourceRngUpdateTransaction& t, const SourceRngEffect2Context& c) noexcept { return SourceRngAccess::effect2(t, c); }
SourceRngStagedSample stage_source_rng_scene_event(SourceRngUpdateTransaction& t, const SourceRngSceneEventContext& c) noexcept { return SourceRngAccess::scene_event(t, c); }
SourceRngStagedSample stage_source_rng_prop_lift(SourceRngUpdateTransaction& t, const SourceRngPropLiftContext& c) noexcept { return SourceRngAccess::prop_lift(t, c); }
SourceRngStagedSample stage_source_rng_effect34(SourceRngUpdateTransaction& t, const SourceRngEffect34Context& c) noexcept { return SourceRngAccess::effect34(t, c); }
SourceRngStagedSample stage_source_rng_reward(SourceRngUpdateTransaction& t, const SourceRngRewardContext& c) noexcept { return SourceRngAccess::reward(t, c); }
SourceRngStagedSample stage_source_rng_e0(SourceRngUpdateTransaction& t, const SourceRngE0Context& c) noexcept { return SourceRngAccess::e0(t, c); }
SourceRngStatus commit_source_rng_update(SourceRngService& s, SourceRngUpdateTransaction& t) noexcept { return SourceRngAccess::commit(s, t); }
void rollback_source_rng_update(SourceRngUpdateTransaction& t) noexcept { SourceRngAccess::rollback(t); }
} // namespace l3
