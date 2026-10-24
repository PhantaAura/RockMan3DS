#include "legends3ds/source_sound.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
uint32_t rd32(const uint8_t* p) { return uint32_t(rd16(p)) | uint32_t(rd16(p + 2)) << 16; }
uint32_t crc32(const uint8_t* p, size_t n) {
    uint32_t c = ~0u;
    while (n--) { c ^= *p++; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
void require(bool ok, const char* what) { if (!ok) throw std::runtime_error(what); }
constexpr size_t kHeader = 36, kRequest = 8, kTone = 10, kSample = 16;
constexpr uint32_t kMaxRequests = 1024, kMaxTones = 4096, kMaxSamples = 1024, kMaxFrames = 1u << 20;
int16_t s16(uint32_t w) { return int16_t(uint16_t(w)); }
uint32_t isqrt(uint64_t v) { uint64_t r = uint64_t(std::sqrt(double(v))); while (r * r > v) --r; while ((r + 1) * (r + 1) <= v) ++r; return uint32_t(r); }
// C signed division/shift idiom the original uses: (x + (x<0 ? 0x7F : 0)) >> 7.
int32_t shr7(int32_t x) { return (x < 0 ? x + 0x7F : x) >> 7; }
}

const SourceSoundRequest* SourceSoundBank::find(uint16_t id) const noexcept {
    for (const auto& r : requests) if (r.id == id) return &r;
    return nullptr;
}

SourceSoundBank load_l3sound_bank(const uint8_t* b, size_t size) {
    require(b && size >= kHeader && !std::memcmp(b, "L3SB001\0", 8) && rd32(b + 8) == 1, "L3SB header");
    const uint32_t nr = rd32(b + 12), nt = rd32(b + 16), ns = rd32(b + 20);
    SourceSoundBank bank;
    bank.sample_rate = rd32(b + 24); bank.file_bytes = rd32(b + 28);
    require(nr && nr <= kMaxRequests && nt <= kMaxTones && ns <= kMaxSamples && bank.sample_rate == 44100, "L3SB counts");
    require(bank.file_bytes == size, "L3SB size");
    const size_t table = size_t(nr) * kRequest + size_t(nt) * kTone + size_t(ns) * kSample;
    require(kHeader + table <= size && crc32(b + kHeader, table) == rd32(b + 32), "L3SB table CRC");
    const uint8_t* p = b + kHeader;
    for (uint32_t i = 0; i < nr; ++i, p += kRequest) {
        SourceSoundRequest r{rd16(p), p[2], p[3], p[4], p[5], rd16(p + 6)};
        require(r.voice < kSourceSpuVoices && r.tones >= 1 && r.tones <= 4 && r.priority < 16 &&
                size_t(r.first_tone) + r.tones <= nt, "L3SB request");
        for (const auto& q : bank.requests) require(q.id != r.id, "L3SB duplicate request");
        bank.requests.push_back(r);
    }
    for (uint32_t i = 0; i < nt; ++i, p += kTone) {
        SourceSoundTone t{rd16(p), rd16(p + 2), rd16(p + 4), p[6], rd16(p + 8)};
        require(t.pitch <= 0x3FFF && t.volume < 0x80 && (t.sample == 0xFFFF || t.sample < ns), "L3SB tone");
        bank.tones.push_back(t);
    }
    uint64_t expect = kHeader + table;
    for (uint32_t i = 0; i < ns; ++i, p += kSample) {
        SourceSoundSample s{rd32(p), rd32(p + 4), rd32(p + 8), rd32(p + 12)};
        require(s.frames && s.frames <= kMaxFrames && s.offset == expect &&
                (s.loop_start == 0xFFFFFFFFu || s.loop_start < s.frames), "L3SB sample");
        require(crc32(b + s.offset, size_t(s.frames) * 2) == s.crc, "L3SB sample CRC");
        expect += uint64_t(s.frames) * 2;
        bank.samples.push_back(s);
    }
    require(expect == size, "L3SB PCM extent");
    return bank;
}

namespace {
// 0x8001A274: returns (left, right) for one tone. `distance` is 0x800C1CF0[v].
void volumes_8001a274(const uint8_t* record, uint8_t tone_volume, uint16_t distance,
                      const SourceSoundListener& l, const SourceSoundSettings& s,
                      uint16_t& left, uint16_t& right) {
    const uint16_t vol = uint16_t(tone_volume * 0x81);
    if (record[0] == 0) {
        const int32_t pan = int32_t(rd32(record + 4)), attenuation = int32_t(rd32(record + 8));
        if (pan < 1) { left = vol; right = uint16_t(shr7(int32_t(uint32_t(vol) * ((uint32_t(pan) + 0x80u) & 0xFFFFu)))); }
        else { left = uint16_t(shr7(int32_t(uint32_t(vol) * (0x80u - (uint32_t(pan) & 0xFFFFu))))); right = vol; }
        if (s.mono_c1b68) { if (right < left) right = left; else left = right; }
        left = uint16_t(shr7(int32_t(uint32_t(left) * uint32_t(0x80 - attenuation))));
        right = uint16_t(shr7(int32_t(uint32_t(right) * uint32_t(0x80 - attenuation))));
    } else {
        if (distance == 0) { left = right = vol; return; }   // the original returns before the settings
        const int32_t dx = int32_t(s16(rd32(record + 4))) - l.x, dz = int32_t(s16(rd32(record + 8))) - l.z;
        // 0x8001E83C(listener, source) in the heading convention: heading h
        // faces (-sin h, -cos h), so straight ahead gives heading - angle = 0.
        const int32_t angle = int32_t(std::lround(std::atan2(double(-dx), double(-dz)) * 2048.0 / 3.141592653589793)) & 0xFFF;
        const uint32_t u = uint32_t(l.heading - angle) & 0xFFFu;
        int32_t c = int32_t(std::lround(std::cos(double(u) * 3.141592653589793 / 2048.0) * 4096.0));
        c = (c << 7) >> 12; if (c < 0) c = -c;
        if (u < 0x800) { left = vol; right = uint16_t(shr7(int32_t(uint32_t(vol) * uint32_t(c)))); }
        else { left = uint16_t(shr7(int32_t(uint32_t(vol) * uint32_t(c)))); right = vol; }
        if (s.mono_c1b68) { if (right < left) right = left; else left = right; }
        left = uint16_t(int32_t(uint32_t(left) * (5000u - distance)) / 5000);
        right = uint16_t(int32_t(uint32_t(right) * (5000u - distance)) / 5000);
    }
    left = uint16_t(int32_t(uint32_t(left) * s.effect_volume_148) / 0x7F);
    right = uint16_t(int32_t(uint32_t(right) * s.effect_volume_148) / 0x7F);
    left = uint16_t(int32_t(uint32_t(left) * s.effect_volume_150) / 0x7F);
    right = uint16_t(int32_t(uint32_t(right) * s.effect_volume_150) / 0x7F);
}
}

SourceSoundConsumeResult consume_source_audio_8001a110(const SourceAudioQueue& queue,
    const SourceSoundBank& bank, const SourceSoundListener& l, const SourceSoundSettings& settings,
    const std::array<bool, kSourceSpuVoices>& busy_in, SourceSoundVoiceState& state,
    std::vector<SourceSoundKeyOn>& out) {
    SourceSoundConsumeResult result;
    auto busy = busy_in;
    for (size_t i = 0; i < queue.count && i < queue.records.size(); ++i) {
        const uint8_t* record = queue.records[i].raw.data();
        const uint16_t id = rd16(record + 2);
        const auto* request = bank.find(id);
        if (!request) { ++result.unknown; continue; }
        for (uint8_t k = 0; k < request->tones; ++k) {                     // 0x8001A968
            const uint8_t v = uint8_t(request->voice + k);
            if (v > 0x17) break;
            // 0x8001A834
            bool allowed = !busy[v] || state.priority_b48[v] <= request->priority;
            if (allowed && record[0] == 1) {
                const int32_t dx = int32_t(l.x) - s16(rd32(record + 4)), dz = int32_t(l.z) - s16(rd32(record + 8));
                const uint32_t d = isqrt(uint64_t(int64_t(dx) * dx + int64_t(dz) * dz));
                if (d < 5000 && (!busy[v] || state.priority_b48[v] < request->priority || d < 0xBB9))
                    state.distance_c1cf0[v] = uint16_t(d);
                else allowed = false;
            }
            if (!allowed) { ++result.rejected; continue; }
            const auto& tone = bank.tones[request->first_tone + k];        // 0x8001A744
            SourceSoundKeyOn key;
            key.voice = v; key.request = id; key.pitch = tone.pitch;
            key.adsr1 = tone.adsr1; key.adsr2 = tone.adsr2; key.sample = tone.sample;
            volumes_8001a274(record, tone.volume, state.distance_c1cf0[v], l, settings, key.volume_left, key.volume_right);
            state.priority_b48[v] = request->priority;
            busy[v] = true;
            out.push_back(key);
            ++result.key_ons;
        }
    }
    return result;
}

// SPU envelope: cycles = 1 << max(0, shift-11), step << max(0, 11-shift);
// exponential increase is 4x slower above 0x6000, exponential decrease
// scales the step by the level.
void SourceSpuEnvelope::key_on(uint16_t adsr1, uint16_t adsr2) noexcept {
    adsr1_ = adsr1; adsr2_ = adsr2; level_ = 0; counter_ = 0; phase_ = Phase::Attack;
}
void SourceSpuEnvelope::key_off() noexcept { if (phase_ != Phase::Off) { phase_ = Phase::Release; counter_ = 0; } }
void SourceSpuEnvelope::advance(uint32_t samples) noexcept { while (samples-- && phase_ != Phase::Off) tick(); }
void SourceSpuEnvelope::tick() noexcept {
    bool exponential = false, decrease = false;
    int shift = 0, step = 0;
    switch (phase_) {
    case Phase::Attack: exponential = adsr1_ >> 15; shift = (adsr1_ >> 10) & 0x1F; step = 7 - ((adsr1_ >> 8) & 3); break;
    case Phase::Decay: exponential = true; decrease = true; shift = ((adsr1_ >> 4) & 0xF) << 2; step = -8; break;
    case Phase::Sustain:
        exponential = adsr2_ >> 15; decrease = (adsr2_ >> 14) & 1; shift = (adsr2_ >> 8) & 0x1F;
        step = decrease ? -8 + ((adsr2_ >> 6) & 3) : 7 - ((adsr2_ >> 6) & 3); break;
    case Phase::Release: exponential = (adsr2_ >> 5) & 1; decrease = true; shift = adsr2_ & 0x1F; step = -8; break;
    case Phase::Off: return;
    }
    int32_t cycles = 1 << (shift > 11 ? shift - 11 : 0);
    int32_t delta = step << (shift < 11 ? 11 - shift : 0);
    if (exponential && !decrease && level_ > 0x6000) cycles *= 4;
    if (exponential && decrease) delta = (delta * level_) >> 15;
    if (++counter_ < cycles) return;
    counter_ = 0;
    int32_t next = int32_t(level_) + delta;
    if (next > 0x7FFF) next = 0x7FFF;
    if (next < 0) next = 0;
    level_ = int16_t(next);
    const int32_t sustain = (int32_t((adsr1_ & 0xF) + 1) * 0x800);
    if (phase_ == Phase::Attack && level_ >= 0x7FFF) phase_ = Phase::Decay;
    else if (phase_ == Phase::Decay && level_ <= sustain) phase_ = Phase::Sustain;
    else if (phase_ == Phase::Release && level_ == 0) phase_ = Phase::Off;
}
} // namespace l3
