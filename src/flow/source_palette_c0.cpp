#include "legends3ds/source_palette_c0.h"

namespace l3 {
namespace {
uint16_t rd16(const uint8_t* p) noexcept { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) noexcept { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
uint32_t crc32(const uint8_t* p, size_t n) noexcept {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) { c ^= p[i]; for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    return ~c;
}
bool header(const std::vector<uint8_t>& b, const char* magic, size_t payload) noexcept {
    return b.size() == 16 + payload && b[0] == uint8_t(magic[0]) && b[1] == uint8_t(magic[1]) && b[2] == uint8_t(magic[2]) &&
           b[3] == uint8_t(magic[3]) && rd16(&b[4]) == 1 && rd16(&b[6]) == 16 && rd32(&b[8]) == b.size() &&
           rd32(&b[12]) == crc32(b.data() + 16, payload);
}
constexpr size_t kTimelineBytes = 8 + 4 * kSourcePaletteTimelineCapacity;
constexpr size_t kC0Payload = 32 + 10 * kTimelineBytes + 16 + 36;
constexpr size_t kBankWords = 32 * kSourcePaletteRowWords;
constexpr size_t kBankPayload = 4 + 2 * kBankWords + kBankWords;

using Work = SourceEventPhase5Bytes<36>;
bool known(const Work& w, size_t at, size_t n) noexcept { for (size_t i = 0; i < n; ++i) if (!w.known[at + i]) return false; return true; }
uint32_t get32(const Work& w, size_t at) noexcept { return rd32(&w.raw[at]); }
void put(Work& w, size_t at, size_t n, uint32_t v) noexcept {
    for (size_t i = 0; i < n; ++i) { w.raw[at + i] = uint8_t(v >> (8 * i)); w.known[at + i] = 1; }
}
std::optional<size_t> colour_index(uint32_t token) noexcept {
    if (token < kSourcePaletteBuffer800ad168 || (token - kSourcePaletteBuffer800ad168) & 1u) return std::nullopt;
    const size_t i = (token - kSourcePaletteBuffer800ad168) / 2;
    if (i >= kSourcePaletteRows * kSourcePaletteRowWords) return std::nullopt;
    return i;
}
uint32_t colour_token(const std::array<uint8_t,4>& e) noexcept {
    return kSourcePaletteBuffer800ad168 + 2u * (uint32_t(e[0]) | (uint32_t(e[1]) << 8));
}

struct Machine {
    Work& w; SourcePaletteBuffer& buf; const SourcePaletteTimeline* const* tables; size_t table_count;
    const std::array<uint8_t,4>* entry(uint32_t cursor) const noexcept {
        for (size_t t = 0; t < table_count; ++t) {
            const auto* tl = tables[t];
            if (cursor >= tl->base_token && (cursor - tl->base_token) % 4 == 0 && (cursor - tl->base_token) / 4 < tl->count)
                return &tl->entries[(cursor - tl->base_token) / 4];
        }
        return nullptr;
    }
    // 0x8011974C / 0x80119B34. Returns -1 (end), 0 (next entry), or 1 (hold).
    int advance() noexcept {
        if (!known(w, 0x0C, 2) || !known(w, 0x18, 4) || !known(w, 0x1C, 8)) return 1;
        uint32_t cursor = get32(w, 0x20);
        const auto* e = entry(cursor);
        if (!e) return 1;
        put(w, 0x14, 4, get32(w, 0x18));
        const int8_t control = int8_t((*e)[3]);
        if (control >= 0) cursor += 4;
        else if (control == -1) { if (int8_t(w.raw[0x0C]) == int8_t(w.raw[0x0D])) return -1; }
        else cursor = get32(w, 0x1C) + 4u * uint32_t(control & 127);
        e = entry(cursor);
        if (!e) return 1;
        put(w, 0x20, 4, cursor);
        put(w, 0x0D, 1, 0); put(w, 0x0C, 1, (*e)[2]); put(w, 0x18, 4, colour_token(*e));
        return 0;
    }
    // 0x80119580 / 0x80119940 per-colour blend, or the step-0 copy (mode 0).
    bool blend(uint16_t destination, uint16_t count, bool mode) noexcept {
        if (!known(w, 0x0C, 2) || !known(w, 0x14, 8)) return false;
        const int32_t duration = int8_t(w.raw[0x0C]), step = int8_t(w.raw[0x0D]);
        const auto src = colour_index(get32(w, 0x14)), dst = colour_index(get32(w, 0x18));
        if (!src || !dst) return false;
        const size_t total = buf.words.size();
        if (!mode) {
            if (step != 0) return true;
            const size_t n = size_t(count / 16) * 16;
            if (*src + n > total || size_t(destination) + n > total) return false;
            for (size_t i = 0; i < n; ++i) if (!buf.known[*src + i]) return false;
            for (size_t i = 0; i < n; ++i) {
                buf.words[destination + i] = buf.words[*src + i]; buf.known[destination + i] = 1;
                buf.changed_rows |= uint64_t(1) << ((destination + i) / kSourcePaletteRowWords);
            }
            return true;
        }
        if (!count) return true;
        if (duration == 0) return false;   // the original divides by zero here
        if (*src + count > total || *dst + count > total || size_t(destination) + count > total) return false;
        for (size_t i = 0; i < count; ++i) if (!buf.known[*src + i] || !buf.known[*dst + i]) return false;
        for (size_t i = 0; i < count; ++i) {
            const uint16_t a = buf.words[*src + i], b = buf.words[*dst + i];
            const auto lerp = [&](int shift) {
                const int32_t from = (a >> shift) & 31, to = (b >> shift) & 31;
                return uint16_t(from + ((to - from) * step) / duration);
            };
            const uint16_t out = uint16_t(lerp(0) | (lerp(5) << 5) | 0x8000u | (lerp(10) << 10));
            buf.words[destination + i] = out; buf.known[destination + i] = 1;
            buf.changed_rows |= uint64_t(1) << ((destination + i) / kSourcePaletteRowWords);
        }
        return true;
    }
};
}

bool load_source_palette_c0_profile(const std::vector<uint8_t>& b, SourcePaletteC0Profile& out) {
    out = {};
    if (!header(b, "L3C0", kC0Payload)) return false;
    const uint8_t* p = b.data() + 16 + 32;
    const auto timeline = [&](SourcePaletteTimeline& t) {
        t.base_token = rd32(p); t.count = p[4];
        for (size_t i = 0; i < kSourcePaletteTimelineCapacity; ++i)
            for (size_t k = 0; k < 4; ++k) t.entries[i][k] = p[8 + 4 * i + k];
        p += kTimelineBytes;
        return t.count <= kSourcePaletteTimelineCapacity;
    };
    for (auto& t : out.sub3) if (!timeline(t)) return false;
    for (auto& t : out.sub4) if (!timeline(t)) return false;
    for (auto& d : out.sub3_destination) { d = {{rd16(p), rd16(p + 2)}}; p += 4; }
    for (auto& r : out.sub4_record) { r = {{rd16(p), rd16(p + 2), rd16(p + 4)}}; p += 6; }
    out.loaded = true;
    return true;
}

bool load_source_palette_bank(const std::vector<uint8_t>& b, SourcePaletteBuffer& out) {
    out = {};
    if (!header(b, "L3PB", kBankPayload)) return false;
    const uint8_t* p = b.data() + 16;
    const uint32_t dirty = rd32(p);
    for (size_t i = 0; i < kBankWords; ++i) {
        out.words[i] = rd16(p + 4 + 2 * i);
        out.known[i] = p[4 + 2 * kBankWords + i] ? 1 : 0;
    }
    for (size_t row = 0; row < 32; ++row) {
        if (!(dirty & (uint32_t(1) << row))) continue;
        for (size_t x = 0; x < kSourcePaletteRowWords; ++x) {
            const size_t from = row * kSourcePaletteRowWords + x, to = (32 + row) * kSourcePaletteRowWords + x;
            out.words[to] = out.words[from]; out.known[to] = out.known[from];
        }
    }
    return true;
}

std::optional<size_t> spawn_source_c0_record_8002ed6c(SourceEventPhase5Pool<16,36>& pool,
    const std::array<uint8_t,20>& r) noexcept {
    if (!pool.used[0]) return std::nullopt;
    for (size_t i = 0; i < 16; ++i) {
        const uint32_t bit = 0x80000000u >> i;
        if (*pool.used[0] & bit) continue;
        auto& w = pool.slots[i];
        if (!w.known[1] || w.raw[1] != uint8_t(0xC0 | i)) return std::nullopt;
        pool.used[0] = *pool.used[0] | bit;
        put(w, 0x10, 4, 0); put(w, 2, 2, 0); put(w, 4, 4, 0); put(w, 0x0C, 4, 0); put(w, 8, 4, 0); put(w, 6, 1, 3);
        put(w, 0, 1, r[0]); put(w, 2, 2, rd16(&r[2])); put(w, 4, 2, rd16(&r[4])); put(w, 0x0C, 4, rd32(&r[8]));
        put(w, 0x14, 4, rd32(&r[12])); put(w, 0x18, 4, rd32(&r[16]));
        return i;
    }
    return std::nullopt;
}

SourcePaletteC0Pass update_source_palette_c0_80017478(SourceEventPhase5Pool<16,36>& pool,
    SourcePaletteBuffer& buffer, const SourcePaletteC0Profile& profile) noexcept {
    SourcePaletteC0Pass out;
    if (!profile.loaded) return out;
    const SourcePaletteTimeline* t3[4] = {&profile.sub3[0], &profile.sub3[1], &profile.sub3[2], &profile.sub3[3]};
    const SourcePaletteTimeline* t4[6] = {&profile.sub4[0], &profile.sub4[1], &profile.sub4[2], &profile.sub4[3], &profile.sub4[4], &profile.sub4[5]};
    for (auto& w : pool.slots) {
        if (!known(w, 0, 1) || !known(w, 6, 1) || !((w.raw[0] & w.raw[6]) & 1)) continue;
        if (!known(w, 2, 2) || !known(w, 8, 1) || (w.raw[2] != 3 && w.raw[2] != 4)) { ++out.held; continue; }
        const bool four = w.raw[2] == 4;
        const uint8_t variant = w.raw[3];
        if (variant >= (four ? 6 : 4)) { ++out.held; continue; }
        const auto before = w;
        Machine m{w, buffer, four ? t4 : t3, four ? size_t(6) : size_t(4)};
        bool released = false, ok = true;
        const auto step_and_advance = [&]() {
            if (!known(w, 0x0C, 2)) { ok = false; return; }
            put(w, 0x0D, 1, uint8_t(w.raw[0x0D] + 1));
            if (int8_t(w.raw[0x0C]) < int8_t(w.raw[0x0D])) {
                const int r = m.advance();
                if (r == 1) ok = false; else if (r) released = true;
            }
        };
        const auto do_blend = [&]() {
            if (four) { const auto& rec = profile.sub4_record[variant]; ok = ok && m.blend(rec[0], rec[1], rec[2] != 0); }
            else { const auto& d = profile.sub3_destination[variant]; ok = ok && m.blend(d[0], 16, d[1] != 0); }
        };
        const uint8_t state = w.raw[8];
        if (state == 0) {
            const auto& tl = four ? profile.sub4[variant] : profile.sub3[variant];
            if (!tl.count) { ++out.held; continue; }
            put(w, 0x20, 4, tl.base_token); put(w, 0x1C, 4, tl.base_token);
            put(w, 0x0D, 1, 0); put(w, 0x0C, 1, tl.entries[0][2]); put(w, 0x18, 4, colour_token(tl.entries[0]));
            const int r = m.advance();
            if (r == 1) ok = false; else if (r) released = true;
            put(w, 8, 1, 1);
        } else if (state == 1 && !four) {
            const uint32_t old_cursor = known(w, 0x20, 4) ? get32(w, 0x20) : 0;
            const auto* old_entry = m.entry(old_cursor);
            step_and_advance();
            if (ok && variant == 1 && old_entry && (*old_entry)[0] == 0x70) {
                put(w, 8, 1, 2); put(w, 0x20, 4, profile.sub3[1].base_token); put(w, 0x1C, 4, profile.sub3[1].base_token);
            }
            do_blend();
        } else if ((state == 2 && !four) || (state == 1 && four)) {
            step_and_advance();
            do_blend();
        } else { ++out.held; continue; }
        if (!ok) { w = before; ++out.held; continue; }
        if (released) { put(w, 0, 1, 0); ++out.released; }
        ++out.updated;
    }
    return out;
}
} // namespace l3
