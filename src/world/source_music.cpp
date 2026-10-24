#include "legends3ds/source_music.h"
#include <algorithm>
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
constexpr size_t kHeader = 44, kRequest = 8, kSep = 12, kVab = 16, kSample = 16, kPitch = 384 + 72;
constexpr uint32_t kMaxFrames = 1u << 20;
constexpr uint16_t kSeqVolume_ad14a = 100;   // 0x80019DE0: 0x800AD14A = 100
constexpr uint8_t kFadeStep_98840 = 4;       // 0x80019DE0: 0x80098840 = 4
// VabHdr: +0x12 programs, +0x16 VAGs, +0x18 master volume; ProgAtr at +0x20
// (16 bytes x 128); VagAtr (tones) at +0x820 (0x20 bytes x 16 per program).
constexpr size_t kProgAttr = 0x20, kToneAttr = 0x820;
int16_t owner_id(int16_t access, uint8_t sequence) { return int16_t(access | sequence << 8); }
}

const SourceMusicRequest* SourceMusicBank::find(uint16_t id) const noexcept {
    for (const auto& r : requests) if (r.id == id) return &r;
    return nullptr;
}

SourceMusicBank load_l3music_bank(const uint8_t* b, size_t size) {
    require(b && size >= kHeader && !std::memcmp(b, "L3MU001\0", 8) && rd32(b + 8) == 1, "L3MU header");
    require((reinterpret_cast<uintptr_t>(b) & 1) == 0, "L3MU alignment");
    const uint32_t na = rd32(b + 12), nr = rd32(b + 16), ns = rd32(b + 20), nv = rd32(b + 24), nm = rd32(b + 28);
    require(na && na <= 64 && nr && nr <= 16 && ns && ns <= 4 && nv && nv <= 4 && nm && nm <= 256 &&
            rd32(b + 32) == 44100 && rd32(b + 36) == size, "L3MU counts");
    const size_t areas = (size_t(na) * 2 + 3) & ~size_t(3);
    const size_t table = kPitch + areas + nr * kRequest + ns * kSep + nv * kVab + nm * kSample;
    require(kHeader + table <= size && crc32(b + kHeader, table) == rd32(b + 40), "L3MU table CRC");
    SourceMusicBank bank;
    const uint8_t* p = b + kHeader;
    for (size_t i = 0; i < 192; ++i) bank.pitch_table[i] = rd16(p + i * 2);
    for (size_t i = 0; i < 4; ++i) bank.reverb_entry[i] = p[384 + i];
    bank.reverb_base = rd32(p + 388);
    for (size_t i = 0; i < 32; ++i) bank.reverb_regs[i] = rd16(p + 392 + i * 2);
    require(bank.reverb_entry[0] <= 9 && bank.reverb_base && bank.reverb_base <= 0xFFFE, "L3MU reverb");
    p += kPitch;
    for (uint32_t i = 0; i < na; ++i) bank.area_music.push_back(rd16(p + i * 2));
    p += areas;
    for (uint32_t i = 0; i < nr; ++i, p += kRequest) {
        SourceMusicRequest r{rd16(p), {p[2], p[3], p[4], p[5]}};
        require(r.row[0] >> 6 == 1 && (r.row[2] >> 4) < 4 && !bank.find(r.id), "L3MU request");
        bank.requests.push_back(r);
    }
    uint64_t expect = kHeader + table;
    for (uint32_t i = 0; i < ns; ++i, p += kSep) {
        const uint32_t bytes = rd32(p + 4), offset = rd32(p + 8);
        require(offset == expect && bytes >= 19 && offset + uint64_t(bytes) <= size, "L3MU SEP");
        SourceMusicSep s{p[0], p[1], b + offset, bytes};
        require(s.sequences >= 1 && s.sequences <= 4 && !std::memcmp(s.data, "pQES\0\0", 6), "L3MU SEP identity");
        bank.seps.push_back(s);
        expect += (bytes + 3u) & ~3u;
    }
    for (uint32_t i = 0; i < nv; ++i, p += kVab) {
        const uint32_t bytes = rd32(p + 4), offset = rd32(p + 8);
        require(offset == expect && bytes >= kToneAttr && offset + uint64_t(bytes) <= size, "L3MU VH");
        SourceMusicVab v{p[0], b + offset, bytes, rd16(p + 12), rd16(p + 14)};
        const uint16_t programs = rd16(v.vh + 0x12), vags = rd16(v.vh + 0x16);
        require(!std::memcmp(v.vh, "pBAV", 4) && programs <= 128 && vags == v.samples &&
                kToneAttr + size_t(programs) * 0x200 + 0x200 <= bytes &&
                size_t(v.first_sample) + v.samples <= nm, "L3MU VH identity");
        bank.vabs.push_back(v);
        expect += (bytes + 3u) & ~3u;
    }
    for (uint32_t i = 0; i < nm; ++i, p += kSample) {
        const uint32_t frames = rd32(p), loop = rd32(p + 4), offset = rd32(p + 8);
        require(frames && frames <= kMaxFrames && offset == expect && (loop == 0xFFFFFFFFu || loop < frames) &&
                offset + uint64_t(frames) * 2 <= size, "L3MU sample");
        require(crc32(b + offset, size_t(frames) * 2) == rd32(p + 12), "L3MU sample CRC");
        bank.samples.push_back({frames, loop, reinterpret_cast<const int16_t*>(b + offset)});
        expect += uint64_t(frames) * 2;
    }
    require(expect == size, "L3MU extent");
    for (const auto& r : bank.requests) {
        bool vab = false, sep = false;
        for (const auto& v : bank.vabs) vab |= v.tag == (r.row[0] & 0x3F);
        for (const auto& s : bank.seps) sep |= s.tag == (r.row[1] & 0x7F) && (r.row[3] & 0x1F) < s.sequences;
        require(vab && sep, "L3MU request binding");
    }
    for (uint16_t a : bank.area_music) require(a == 0xFFFF || bank.find(a), "L3MU area request");
    return bank;
}

uint16_t source_area_music_80019d18(const SourceMusicBank& bank, size_t area) noexcept {
    return area < bank.area_music.size() ? bank.area_music[area] : 0xFFFF;
}

void SourceMusicPlayer::bind(const SourceMusicBank* bank) noexcept {
    *this = SourceMusicPlayer{};
    bank_ = bank;
    if (!bank_) return;
    // 0x80019DE0: slot tables cleared (0x80098AFA = 0xFF).
    for (size_t v = 0; v < bank_->vabs.size() && v < program_volume_.size(); ++v)
        for (size_t p = 0; p < 128; ++p) program_volume_[v][p] = bank_->vabs[v].vh[kProgAttr + p * 16 + 1];
}

const SourceMusicVab* SourceMusicPlayer::vab(uint8_t id) const noexcept {
    return bank_ && id < bank_->vabs.size() ? &bank_->vabs[id] : nullptr;
}
const uint8_t* SourceMusicPlayer::program_attr(uint8_t id, int16_t program) const noexcept {
    const auto* v = vab(id);
    return v && program >= 0 && program < 128 ? v->vh + kProgAttr + size_t(program) * 16 : nullptr;
}
// SsVabOpenHead stores each present program's tone block in ProgAtr.reserved1
// (read by 0x800759E0 as +8). CANDIDATE: the block is the program's rank among
// programs with tones (the open is not decompiled).
int16_t SourceMusicPlayer::block(uint8_t id, int16_t program) const noexcept {
    const auto* v = vab(id);
    if (!v || program < 0 || program >= 128 || !v->vh[kProgAttr + size_t(program) * 16]) return -1;
    int16_t rank = 0;
    for (int16_t p = 0; p < program; ++p) rank = int16_t(rank + (v->vh[kProgAttr + size_t(p) * 16] != 0));
    return rank < int16_t(rd16(v->vh + 0x12)) ? rank : -1;
}
const uint8_t* SourceMusicPlayer::tone_attr(uint8_t id, int16_t blk, uint16_t tone) const noexcept {
    const auto* v = vab(id);
    if (!v || blk < 0 || tone >= 16) return nullptr;
    const size_t at = kToneAttr + (size_t(blk) * 16 + tone) * 0x20;
    return at + 0x20 <= v->vh_bytes ? v->vh + at : nullptr;
}
SourceMusicPlayer::Seq* SourceMusicPlayer::seq(int16_t owner) noexcept {
    const int a = owner & 0xFF, s = (owner >> 8) & 0xFF;
    return a < 4 && s < 4 && access_open_[size_t(a)] ? &seqs_[size_t(a)][size_t(s)] : nullptr;
}

// ---- game side -----------------------------------------------------------
size_t SourceMusicPlayer::consume_8001a110(const SourceAudioQueue& queue) noexcept {
    if (!bank_) return 0;
    volumes_8001aef0();
    for (size_t i = 0; i < slots_.size(); ++i) {                    // 0x8001A0A8
        auto& s = slots_[i];
        const Seq* q = s.type != 0xFF && s.access != -1 ? seq(owner_id(s.access, s.sequence)) : nullptr;
        s.playing = q ? q->playing_14 : 0;
    }
    size_t dispatched = 0;
    for (size_t i = 0; i < queue.count && i < queue.records.size(); ++i) {
        const auto& r = queue.records[i];
        if (!bank_->find(rd16(r.raw.data() + 2))) continue;
        handler_8001ac58(r);
        ++dispatched;
    }
    return dispatched;
}

// 0x8001AEF0: fade mode (0x80098788 >> 4) & 3 steps 0x800AD152 by 0x80098840;
// then SsSepSetVol(slot, 0x800AD14A * 0x800AD152 / 0x7F) for every slot.
void SourceMusicPlayer::volumes_8001aef0() noexcept {
    if (fade_mode_ & 1) {
        const int v = int(volume_ad152_) - kFadeStep_98840;
        if (v > 0) volume_ad152_ = uint8_t(v); else { volume_ad152_ = 0; fade_mode_ = 0; }
    } else if (fade_mode_ & 2) {
        const int v = int(volume_ad152_) + kFadeStep_98840;
        if (v < 0x7F) volume_ad152_ = uint8_t(v); else { volume_ad152_ = 0x7F; fade_mode_ = 0; }
    }
    for (const auto& s : slots_) {
        if (s.access == -1 || s.type == 0xFF) continue;
        const uint16_t v = uint16_t(int(kSeqVolume_ad14a) * volume_ad152_ / 0x7F);
        sep_set_vol_80070694(owner_id(s.access, s.type == 0 ? 0 : s.sequence), v, v);
    }
    // 0x800AD15A state: a new entry sets the type (SsUtSetReverbType, then
    // SpuSetReverb(1)); 0x1E frames later SsUtSetReverbDepth(depth) and
    // SsSetRVol(RVol) both write the reverb output volume.
    const auto& e = bank_->reverb_entry;
    if (reverb_state_ == 0 && reverb_pending_) {
        reverb_pending_ = false;
        reverb_type_ = e[0];
        reverb_out_ = 0;   // CANDIDATE: a mode change leaves the output at 0 until the depth call
        reverb_on_ = true;
        const uint32_t halfwords = (0x80000u - bank_->reverb_base * 8u) / 2u;
        reverb_buffer_.assign(e[0] ? halfwords : 0, 0);   // CANDIDATE: work area starts cleared
        reverb_address_ = 0;
        reverb_wait_ = 0x1E; reverb_state_ = 1;
    } else if (reverb_state_ == 1 && --reverb_wait_ == 0) {
        // SsUtSetReverbDepth(depth) is overwritten by SsSetRVol(RVol): same register.
        if (e[0]) reverb_out_ = int16_t(e[1] * 0x7FFF / 0x7F);
        reverb_state_ = 0;
    }
}

// 0x8001AC58: request row = [1<<6 | VAB tag, loop<<7 | SEP tag, slot<<4 | priority, sequence].
void SourceMusicPlayer::handler_8001ac58(const SourceAudioQueueRecord& record) noexcept {
    const uint16_t id = rd16(record.raw.data() + 2);
    const auto* request = bank_->find(id);
    if (!request) return;
    const auto& row = request->row;
    int vab_index = -1, sep_index = -1;                                // 0x8001A1FC / 0x8001A238
    for (size_t i = 0; i < bank_->vabs.size(); ++i) if (bank_->vabs[i].tag == (row[0] & 0x3F)) { vab_index = int(i); break; }
    for (size_t i = 0; i < bank_->seps.size(); ++i) if (bank_->seps[i].tag == (row[1] & 0x7F)) { sep_index = int(i); break; }
    if (vab_index < 0 || sep_index < 0) return;
    auto& s = slots_[row[2] >> 4];
    if (rd32(record.raw.data() + 4) == 0 && s.playing && s.request == id) { volume_ad152_ = 0x7F; return; }
    if (s.priority > int8_t(row[2] & 0xF)) return;
    if (s.access != -1 && s.type != 0xFF) {
        sep_stop_8006ff90(owner_id(s.access, s.type == 0 ? 0 : s.sequence));
        access_open_[size_t(s.access)] = false;                         // SsSepClose 0x8006B488
    }
    s.access = sep_open_8006b664(size_t(sep_index), uint8_t(vab_index));
    s.type = 1; s.priority = int8_t(row[2] & 0xF);
    if (s.access == -1) return;
    s.sequence = row[3] & 0x1F;
    sep_play_8006e370(s.access, s.sequence, (row[1] & 0x80) == 0 ? 1 : 0);
    volume_ad152_ = 0x7F;
    s.request = id;
}

void SourceMusicPlayer::stop_all_8001b33c() noexcept {
    for (const auto& s : slots_)
        if (s.access != -1 && s.type != 0xFF) sep_stop_8006ff90(owner_id(s.access, s.type == 0 ? 0 : s.sequence));
}

// ---- libsnd sequence -----------------------------------------------------
uint8_t SourceMusicPlayer::next(Seq& s) noexcept {
    if (s.ptr >= s.end) { ++errors_; return 0xFF; }   // guard: 0xFF reads as meta, then end of data
    return *s.ptr++;
}

// 0x8006D7FC: variable-length delta, returned and accumulated times 10.
int32_t SourceMusicPlayer::read_delta_8006d7fc(Seq& s) noexcept {
    uint32_t v = next(s);
    if (!v) return 0;
    if (v & 0x80) {
        v &= 0x7F;
        uint8_t b;
        int guard = 0;
        do { b = next(s); v = v * 0x80 + (b & 0x7F); } while ((b & 0x80) && ++guard < 4);
    }
    const int32_t d = int32_t(v) * 10;
    s.total_88 += d;
    return d;
}

// 0x8006B664 + 0x8006D8AC per sequence.
int16_t SourceMusicPlayer::sep_open_8006b664(size_t sep_index, uint8_t vab_id) noexcept {
    int access = -1;
    for (int a = 0; a < 4; ++a) if (!access_open_[size_t(a)]) { access = a; break; }
    if (access < 0) return -1;
    const auto& sep = bank_->seps[sep_index];
    const uint8_t* p = sep.data;
    const uint8_t* end = sep.data + sep.bytes;
    for (uint8_t i = 0; i < sep.sequences; ++i) {
        auto& s = seqs_[size_t(access)][i];
        s = Seq{};
        s.vab_26 = vab_id;
        for (size_t c = 0; c < 16; ++c) { s.pan_27[c] = 0x40; s.program_37[c] = uint8_t(c); s.volume_60[c] = 0x7F; }
        const uint8_t* q = p + (i == 0 ? 8 : 2);
        if (q + 11 > end) return -1;
        s.resolution_50 = int16_t(q[0] << 8 | q[1]);
        const uint32_t tempo = uint32_t(q[2]) << 16 | uint32_t(q[3]) << 8 | q[4];
        const uint32_t size = uint32_t(q[7]) << 24 | uint32_t(q[8]) << 16 | uint32_t(q[9]) << 8 | q[10];
        if (!tempo || q + 11 + uint64_t(size) > end) return -1;
        s.bpm_8c = int32_t(60000000u / tempo + (tempo >> 1 < 60000000u % tempo ? 1 : 0));
        s.bpm_94 = s.bpm_8c;
        s.ptr = q + 11; s.end = q + 11 + size;
        const int32_t delta = read_delta_8006d7fc(s);
        s.loop_ptr = s.start = s.ptr;
        s.first_delta_84 = s.remain_90 = delta;
        const uint32_t u = uint32_t(s.resolution_50) * uint32_t(s.bpm_8c), rate = kSourceMusicTickRate * 60;
        if (u * 10 < rate) {
            if (!u) return -1;
            s.slow_52 = s.step_54 = int16_t(kSourceMusicTickRate * 600 / u);
        } else {
            s.slow_52 = -1;
            s.step_54 = int16_t(u * 10 / rate + (u * 10 % rate > kSourceMusicTickRate * 30 ? 1 : 0));
        }
        s.step_init_56 = s.step_54;
        p = q + 11 + size;
    }
    access_open_[size_t(access)] = true;
    access_sep_[size_t(access)] = uint8_t(sep_index);
    return int16_t(access);
}

void SourceMusicPlayer::sep_play_8006e370(int16_t access, uint8_t sequence, uint8_t l_count) noexcept {
    auto* s = seq(owner_id(access, sequence));
    if (!s || !s->start) return;
    s->ptr = s->loop_ptr = s->start;
    s->flags_98 &= ~0x204u;
    s->l_count_20 = l_count;
    s->flags_98 |= 1; s->play_count_21 = 0; s->playing_14 = 1;
    seq_vol_80074ce4(owner_id(access, sequence), s->vol_l_58, s->vol_r_5a);
}

void SourceMusicPlayer::sep_stop_8006ff90(int16_t owner) noexcept {
    auto* s = seq(owner);
    if (!s) return;
    s->flags_98 = (s->flags_98 & ~0x40Bu) | 4;
    keys_off_80075314(owner);
    damper_b5168_ = 0;
    s->playing_14 = s->loop_active_15 = s->channel_17 = s->rpn_18 = s->rpn_19 = 0;
    s->nrpn_1a = s->nrpn_1b = s->loop_marker_1c = s->loop_count_1d = s->rpn_count_1e = s->nrpn_count_1f = 0;
    s->play_count_21 = 0; s->status_16 = 0; s->total_88 = 0;
    s->remain_90 = s->first_delta_84; s->bpm_94 = s->bpm_8c; s->step_54 = s->step_init_56;
    s->ptr = s->loop_ptr = s->start;
    for (size_t c = 0; c < 16; ++c) { s->program_37[c] = uint8_t(c); s->pan_27[c] = 0x40; s->volume_60[c] = 0x7F; }
}

void SourceMusicPlayer::sep_set_vol_80070694(int16_t owner, uint16_t l, uint16_t r) noexcept {
    auto* s = seq(owner);
    if (!s) return;
    if (s->flags_98 == 1) seq_vol_80074ce4(owner, l, r);
    else { s->vol_l_58 = l; s->vol_r_5a = r; }
}

void SourceMusicPlayer::tick_60hz() noexcept {
    flush_800725e0();
    if (!bank_) return;
    for (int a = 0; a < 4; ++a) {                                       // SsSeqCalledTbyT
        if (!access_open_[size_t(a)]) continue;
        for (uint8_t q = 0; q < 4; ++q) {
            const int16_t owner = owner_id(int16_t(a), q);
            auto& s = seqs_[size_t(a)][q];
            if (s.flags_98 & 1) snd_play_8006f744(owner);
            if (s.flags_98 & 4) { sep_stop_8006ff90(owner); s.flags_98 = 0; }
        }
    }
}

// 0x8006F744 (_SsSndPlay): remaining delta and step are both in tenths of a tick.
void SourceMusicPlayer::snd_play_8006f744(int16_t owner) noexcept {
    auto& s = *seq(owner);
    const int32_t step = s.step_54;
    int32_t rem = s.remain_90, d = rem - step;
    if (d < 1) {
        if (step < rem) return;
        int guard = 0;
        do {
            do { get_seq_data_8006fa90(owner); } while (s.remain_90 == 0 && ++guard < 4096);
            rem += s.remain_90;
            d = rem - step;
        } while (rem < step && ++guard < 4096);
        if (guard >= 4096) { ++errors_; s.flags_98 = (s.flags_98 & ~1u) | 4; }
    } else {
        if (s.slow_52 > 0) { --s.slow_52; return; }
        if (s.slow_52 != 0) { s.remain_90 = d; return; }
        s.slow_52 = int16_t(step);
        d = s.remain_90 - 1;
    }
    s.remain_90 = d;
}

// 0x8006FA90 (_SsGetSeqData): one event with running status; the note and
// controller handlers read the following delta into +0x90.
void SourceMusicPlayer::get_seq_data_8006fa90(int16_t owner) noexcept {
    auto& s = *seq(owner);
    uint8_t b = next(s);
    uint8_t status = s.status_16;
    if (b & 0x80) {
        s.channel_17 = b & 0xF;
        status = (b & 0xF0) == 0xF0 ? 0xFF : (b & 0xF0);
        if (status != 0x90 && status != 0xB0 && status != 0xC0 && status != 0xE0 && status != 0xFF) return;
        s.status_16 = status;
        if (status == 0xE0) { next(s); b = 0; } else b = next(s);
    } else if (status != 0x90 && status != 0xB0 && status != 0xC0 && status != 0xE0 && status != 0xFF) {
        return;
    }
    const uint8_t ch = s.channel_17;
    switch (status) {
    case 0x90: {                                                        // 0x8006D6B8
        const uint8_t velocity = next(s);
        s.remain_90 = read_delta_8006d7fc(s);
        if (!velocity) key_off_80073684(owner, s.vab_26, s.program_37[ch], b);
        else if (!((s.mute_80 >> ch) & 1)) key_on_80073100(owner, s.vab_26, s.program_37[ch], b, velocity, s.pan_27[ch]);
        return;
    }
    case 0xB0: control_change_8006d2b0(owner, b); return;
    case 0xC0: s.program_37[ch] = b; s.remain_90 = read_delta_8006d7fc(s); return;  // 0x8006D78C
    case 0xE0: {                                                        // 0x8006D200
        const uint8_t bend = next(s);
        pitch_bend_80074a64(owner, s.vab_26, s.program_37[ch], bend);
        s.remain_90 = read_delta_8006d7fc(s);
        return;
    }
    default:
        if (b == 0x2F) { end_of_track_8006f84c(owner); return; }
        if (s.ptr + 3 > s.end) { ++errors_; end_of_track_8006f84c(owner); return; }
        tempo_8006d4e8(s, uint32_t(s.ptr[0]) << 16 | uint32_t(s.ptr[1]) << 8 | s.ptr[2]);
        s.ptr += 3;
        s.remain_90 = read_delta_8006d7fc(s);
        return;
    }
}

// 0x8006D4E8: tempo meta (any FF xx other than 2F reads three tempo bytes).
void SourceMusicPlayer::tempo_8006d4e8(Seq& s, uint32_t tempo) noexcept {
    if (!tempo) { ++errors_; return; }
    s.bpm_94 = int32_t(60000000u / tempo);
    const uint32_t u = uint32_t(s.resolution_50) * uint32_t(s.bpm_94), rate = kSourceMusicTickRate * 60;
    if (u * 10 < rate) {
        if (!u) { ++errors_; return; }
        s.slow_52 = s.step_54 = int16_t(kSourceMusicTickRate * 600 / u);
    } else {
        s.slow_52 = -1;
        s.step_54 = int16_t(u * 10 / rate + (u * 10 % rate > kSourceMusicTickRate * 30 ? 1 : 0));
    }
}

// 0x8006F84C: end of track. l_count 0 repeats forever; otherwise the
// sequence stops after l_count plays and its voices are keyed off.
void SourceMusicPlayer::end_of_track_8006f84c(int16_t owner) noexcept {
    auto& s = *seq(owner);
    ++s.play_count_21;
    if (s.l_count_20 == 0 || s.play_count_21 < s.l_count_20) {
        s.total_88 = 0; s.loop_marker_1c = 0; s.remain_90 = 0;
        s.ptr = s.start;
        if (s.l_count_20) s.loop_ptr = s.ptr;
        ++loops_;
        return;
    }
    s.flags_98 = (s.flags_98 & ~0xBu) | 0x204;
    s.playing_14 = 0;
    s.loop_ptr = s.start;
    keys_off_80075314(owner);
    s.remain_90 = s.step_54;
}

// 0x8006D2B0 and the controller handlers 0x8006B9E0..0x8006C574.
void SourceMusicPlayer::control_change_8006d2b0(int16_t owner, uint8_t control) noexcept {
    auto& s = *seq(owner);
    const uint8_t value = next(s);
    const uint8_t ch = s.channel_17;
    switch (control) {
    case 0: s.vab_26 = value; break;                                    // bank (VAB id)
    case 6:                                                             // 0x8006BA58 data entry
        if (s.loop_marker_1c == 1 && s.loop_active_15 == 0) {
            s.loop_count_1d = value; s.loop_marker_1c = 0; s.loop_active_15 = 1;
        } else if (s.rpn_count_1e == 2) {
            // RPN data (0x800721E4 VagAtr writes) is not ported: no tutorial use.
            s.rpn_count_1e = 0;
        } else if (s.nrpn_count_1f == 2) {
            // NRPN data (vibrato/portamento table 0x800A328C) is not ported.
            s.nrpn_count_1f = 0;
        }
        break;
    case 7:                                                             // 0x8006BE80 volume
        set_volume_80075464(owner, s.vab_26, s.program_37[ch], value, s.pan_27[ch]);
        s.volume_60[ch] = value;
        break;
    case 10:                                                            // 0x8006BF50 pan
        set_volume_80075464(owner, s.vab_26, s.program_37[ch], uint16_t(s.volume_60[ch]), value);
        s.pan_27[ch] = value;
        break;
    case 11: {                                                          // 0x8006C018 expression
        const int16_t program = s.program_37[ch];
        if (s.vab_26 < program_volume_.size() && program < 128) program_volume_[s.vab_26][size_t(program)] = value;
        set_volume_80075464(owner, s.vab_26, program, uint16_t(s.volume_60[ch]), s.pan_27[ch]);
        break;
    }
    case 0x40: damper_b5168_ = value < 0x40 ? 0 : 2; break;             // 0x8006C100
    case 0x5B: break;                                                   // 0x8006C1A8 reverb depth: no reverb
    case 0x62:                                                          // 0x8006C230 NRPN LSB
        if (s.loop_marker_1c == 1 && s.loop_active_15 == 0) {
            s.loop_count_1d = value; s.loop_marker_1c = 0; s.loop_active_15 = 1;
        } else if (s.nrpn_1b != 0x1E && s.nrpn_1b != 0x14) {
            s.nrpn_1a = value; s.loop_marker_1c = 0; ++s.nrpn_count_1f;
        }
        break;
    case 0x63:                                                          // 0x8006C360 NRPN MSB
        if (value == 0x14) {                                            // loop start
            s.nrpn_1b = 0x14; s.loop_marker_1c = 1;
            s.remain_90 = read_delta_8006d7fc(s);
            s.loop_ptr = s.ptr;
            return;
        }
        if (value == 0x1E) {                                            // loop end
            s.nrpn_1b = 0x1E;
            if (s.loop_count_1d == 0) { s.loop_active_15 = 0; break; }
            if (s.loop_count_1d > 0x7E) {                               // 127: forever
                read_delta_8006d7fc(s);
                s.remain_90 = 0; s.ptr = s.loop_ptr; ++loops_;
                return;
            }
            --s.loop_count_1d;
            s.remain_90 = read_delta_8006d7fc(s);
            if (s.loop_count_1d) { s.ptr = s.loop_ptr; ++loops_; }
            else s.loop_active_15 = 0;
            return;
        }
        s.nrpn_1b = value; ++s.nrpn_count_1f;
        break;
    case 100: s.rpn_18 = value; ++s.rpn_count_1e; break;                // 0x8006C494
    case 0x65: s.rpn_19 = value; ++s.rpn_count_1e; break;               // 0x8006C504
    case 0x79:                                                          // 0x8006C574 reset all
        damper_b5168_ = 0;
        s.program_37[ch] = ch; s.rpn_18 = s.rpn_19 = 0; s.volume_60[ch] = 0x7F; s.pan_27[ch] = 0x40;
        break;
    default: break;
    }
    s.remain_90 = read_delta_8006d7fc(s);
}

// ---- libsnd voice manager -------------------------------------------------
// The key-on volume law of 0x80074398 / 0x80074CE4 / 0x80075464: velocity x
// channel volume, VAB, program and tone volumes, sequence L/R, three pans
// (tone, program, channel), then squared.
void SourceMusicPlayer::voice_volume(Voice& v, uint32_t velocity, uint16_t seq_l, uint16_t seq_r, uint8_t channel_pan) noexcept {
    const auto* vb = vab(uint8_t(v.vab_f8));
    const uint8_t* prog = program_attr(uint8_t(v.vab_f8), v.program_f4);
    const uint8_t* tone = tone_attr(uint8_t(v.vab_f8), v.block_f2, v.tone_f6);
    if (!vb || !prog || !tone) return;
    const uint32_t prog_vol = program_volume_[size_t(v.vab_f8)][size_t(v.program_f4)];
    uint32_t base = ((velocity * vb->vh[0x18] * 0x3FFFu) / 0x3F01u) * prog_vol * tone[2] / 0x3F01u;
    uint32_t l = base * seq_l / 0x7F, r = base * seq_r / 0x7F;
    for (uint32_t pan : {uint32_t(tone[3]), uint32_t(prog[4]), uint32_t(channel_pan)}) {
        if (pan < 0x40) r = r * pan / 0x3F; else l = l * (0x7F - pan) / 0x3F;
    }
    v.reg_vol_l = uint16_t(l * l / 0x3FFF);
    v.reg_vol_r = uint16_t(r * r / 0x3FFF);
    v.dirty_3940 |= 3;
}

// 0x800738F8: a free voice (state 0 and envelope 0), else the lowest-priority
// voice at or below the new priority, quietest envelope, then oldest.
uint8_t SourceMusicPlayer::allocate_800738f8(uint16_t priority) noexcept {
    uint8_t chosen = 99, candidate = 99;
    uint16_t best_env = 0xFFFF, threshold = priority;
    uint32_t best_age = 0;
    int matches = 0;
    for (uint8_t v = 0; v < kSourceMusicVoices; ++v) {
        const auto& x = voices_[v];
        if (x.state_fd == 0 && x.env_e6 == 0) { chosen = v; break; }
        if (x.priority_fa < threshold) {
            best_env = x.env_e6; matches = 1; best_age = x.age_e2; threshold = x.priority_fa; candidate = v;
        } else if (x.priority_fa == threshold) {
            ++matches;
            if (x.env_e6 < best_env) { best_env = x.env_e6; best_age = x.age_e2; candidate = v; }
            else if (x.env_e6 == best_env && x.age_e2 > best_age) { best_age = x.age_e2; candidate = v; }
        }
    }
    if (chosen == 99) chosen = matches ? candidate : uint8_t(kSourceMusicVoices);
    if (chosen < kSourceMusicVoices) {
        for (auto& x : voices_) ++x.age_e2;
        voices_[chosen].age_e2 = 0;
        voices_[chosen].priority_fa = priority;
    }
    return chosen;
}

// 0x80073C48: pitch of `note` with a 1/128-semitone fine offset and the
// tone's centre/fine tune, from the 12x16 table 0x80090C80.
uint16_t SourceMusicPlayer::pitch_80073c48(int32_t note, uint32_t fine, const uint8_t* tone) const noexcept {
    uint32_t f = ((fine & 0xFFFF) + tone[5]) >> 3;
    const bool carry = f > 15;
    if (carry) f -= 16;
    const int32_t n = int16_t(note + (carry ? 1 : 0) + 60 - tone[4]);
    const int32_t octave = n / 12 - 5, index = (n % 12) * 16 + int32_t(f);
    if (index < 0 || index >= 192) return 0;
    uint32_t p = bank_->pitch_table[size_t(index)];
    if (octave > 0) p <<= octave; else if (octave < 0) p >>= -octave;
    return uint16_t(p);
}

void SourceMusicPlayer::key_on_80073100(int16_t owner, uint8_t vab_id, int16_t program, uint8_t note, uint8_t velocity, uint8_t pan) noexcept {
    const auto* vb = vab(vab_id);
    const uint8_t* prog = program_attr(vab_id, program);
    const int16_t blk = block(vab_id, program);
    if (!vb || !prog || blk < 0) return;
    auto* s = seq(owner);
    const uint8_t ch = s ? s->channel_17 : 0;
    const uint32_t scaled = s ? uint32_t(velocity) * uint32_t(s->volume_60[ch]) / 0x7F : velocity;
    for (uint8_t t = 0; t < prog[0] && t < 16; ++t) {                     // 0x800753B0
        const uint8_t* tone = tone_attr(vab_id, blk, t);
        if (!tone || note < tone[6] || note > tone[7]) continue;
        const uint16_t vag = rd16(tone + 0x16);
        const uint8_t v = allocate_800738f8(tone[0]);
        if (v >= kSourceMusicVoices) continue;
        auto& x = voices_[v];
        x.state_fd = 1; x.age_e2 = 0; x.owner_f0 = owner; x.vab_f8 = vab_id; x.block_f2 = blk;
        x.program_f4 = program; x.velocity_e8 = velocity; x.channel_ec = ch; x.pan_ea = pan;
        x.tone_f6 = t; x.note_ee = note; x.priority_fa = tone[0]; x.vag_e0 = vag;
        // 0x800723AC: envelope marked busy, VAG address, ADSR (+ damper on ADSR2).
        x.env_e6 = 0x7FFF;
        for (auto& ring : env_zero_c5640_) ring &= ~(1u << v);
        x.reg_sample = vag && vag <= vb->samples ? uint16_t(vb->first_sample + vag - 1) : 0xFFFF;
        x.reg_adsr1 = rd16(tone + 0x10);
        x.reg_adsr2 = uint16_t(rd16(tone + 0x12) + damper_b5168_);
        x.dirty_3940 |= 0x38;
        // 0x80073B84: key-on pitch from the note, centre and fine tune.
        int32_t n = int16_t(note + 60 - tone[4]);
        const int32_t idx = (n % 12) * 16 + std::min<int32_t>(tone[5] >> 3, 15), octave = n / 12 - 5;
        uint32_t p = idx >= 0 && idx < 192 ? bank_->pitch_table[size_t(idx)] : 0;
        if (octave > 0) p <<= octave; else if (octave < 0) p >>= -octave;
        x.reg_pitch = x.pitch_e4 = uint16_t(p);
        x.dirty_3940 |= 4;
        // 0x80074398: volume with the scaled velocity, then key on.
        voice_volume(x, scaled, s ? s->vol_l_58 : 0x7F, s ? s->vol_r_5a : 0x7F, pan);
        x.reg_reverb = (tone[1] & 4) != 0;
        x.key_on = true; x.key_off = false;
        ++key_ons_;
    }
}

void SourceMusicPlayer::voice_key_off_800742c8(size_t v) noexcept {
    auto& x = voices_[v];
    x.state_fd = 0; x.pitch_e4 = 0; x.vag_e0 = 0;
    x.key_off = true; x.key_on = false;
}

void SourceMusicPlayer::key_off_80073684(int16_t owner, uint8_t vab_id, int16_t program, uint8_t note) noexcept {
    for (size_t v = 0; v < kSourceMusicVoices; ++v) {
        const auto& x = voices_[v];
        if (x.note_ee == note && x.program_f4 == program && x.owner_f0 == owner && x.vab_f8 == vab_id) voice_key_off_800742c8(v);
    }
}

void SourceMusicPlayer::keys_off_80075314(int16_t owner) noexcept {
    for (size_t v = 0; v < kSourceMusicVoices; ++v) if (voices_[v].owner_f0 == owner) voice_key_off_800742c8(v);
}

void SourceMusicPlayer::seq_vol_80074ce4(int16_t owner, uint16_t l, uint16_t r) noexcept {
    auto* s = seq(owner);
    if (!s) return;
    s->vol_l_58 = l > 0x7E ? 0x7F : l;
    s->vol_r_5a = r > 0x7E ? 0x7F : r;
    for (auto& x : voices_) {
        if (x.owner_f0 != owner) continue;
        const uint32_t velocity = uint32_t(x.velocity_e8) * uint32_t(s->volume_60[x.channel_ec & 15]) / 0x7F;
        voice_volume(x, velocity, s->vol_l_58, s->vol_r_5a, x.pan_ea);
    }
}

void SourceMusicPlayer::set_volume_80075464(int16_t owner, uint8_t vab_id, int16_t program, uint16_t volume, uint16_t pan) noexcept {
    auto* s = seq(owner);
    if (!s) return;
    if (!volume) volume = 1;
    if (!pan) pan = 1;
    for (auto& x : voices_) {
        if (x.owner_f0 != owner || x.program_f4 != program || x.vab_f8 != vab_id) continue;
        voice_volume(x, uint32_t(x.velocity_e8) * volume / 0x7F, s->vol_l_58, s->vol_r_5a, uint8_t(pan));
    }
}

// 0x80074A64 / 0x80074864: bend 64 = centre; the tone's pbmax (+0x0D) or
// pbmin (+0x0C) gives the range in semitones.
void SourceMusicPlayer::pitch_bend_80074a64(int16_t owner, uint8_t vab_id, int16_t program, uint8_t bend) noexcept {
    for (auto& x : voices_) {
        if (x.owner_f0 != owner || x.vab_f8 != vab_id || x.program_f4 != program) continue;
        const uint8_t* tone = tone_attr(vab_id, x.block_f2, x.tone_f6);
        if (!tone) continue;
        int32_t note = x.note_ee, fine = 0;
        const int32_t b = int32_t(bend) - 0x40;
        if (b > 0) { const int32_t s = b * tone[0x0D]; note += s / 0x3F; fine = (s % 0x3F) * 2; }
        else if (b < 0) {
            const int32_t s = b * tone[0x0C], t = (s < 0 ? s + 0x3F : s) >> 6;
            note = note + t - 1; fine = (s - t * 0x40) * 2 + 0x7F;
        }
        x.reg_pitch = pitch_80073c48(note, uint32_t(fine), tone);
        x.dirty_3940 |= 4;
    }
}

// 0x800725E0 (top of SsSeqCalledTbyT): sample every voice's envelope, retire
// voices whose envelope stayed 0 (ring 0x800C5640), then write the pending
// registers, key-offs and key-ons.
void SourceMusicPlayer::flush_800725e0() noexcept {
    env_zero_index_ = uint8_t((env_zero_index_ + 1) & 15);
    env_zero_c5640_[env_zero_index_] = 0;
    for (size_t v = 0; v < kSourceMusicVoices; ++v) {
        auto& x = voices_[v];
        x.env_e6 = uint16_t(x.sounding ? x.envelope.level() : 0);
        if (!x.env_e6) env_zero_c5640_[env_zero_index_] |= 1u << v;
    }
    uint32_t idle = ~0u;
    for (size_t i = 0; i < 15; ++i) idle &= env_zero_c5640_[i];
    for (size_t v = 0; v < kSourceMusicVoices; ++v) if (idle & (1u << v)) voices_[v].state_fd = 0;
    for (auto& x : voices_) {
        if (x.dirty_3940 & 3) { x.vol_l = x.reg_vol_l; x.vol_r = x.reg_vol_r; }
        if (x.dirty_3940 & 4) x.pitch = x.reg_pitch;
        if (x.dirty_3940 & 8) x.sample = x.reg_sample;
        if (x.dirty_3940 & 0x30) { x.adsr1 = x.reg_adsr1; x.adsr2 = x.reg_adsr2; }
        x.dirty_3940 = 0;
    }
    for (auto& x : voices_) if (x.key_off) { x.envelope.key_off(); x.key_off = false; }
    for (auto& x : voices_) x.reverb = x.reg_reverb;           // 0x80072D08(8, 0x8009AE44)
    for (auto& x : voices_) {
        if (!x.key_on) continue;
        x.key_on = false;
        x.position = 0; x.phase = 0;
        x.envelope.key_on(x.adsr1, x.adsr2);
        x.sounding = bank_ && x.sample < bank_->samples.size();
        if (!x.sounding) x.envelope.mute();
    }
}

size_t SourceMusicPlayer::sounding_voices() const noexcept {
    size_t n = 0;
    for (const auto& x : voices_) n += x.sounding;
    return n;
}

// Software SPU voice: pitch 0x1000 = 44100 Hz (12-bit phase), linear
// interpolation (CANDIDATE: the SPU interpolates with a 4-tap Gaussian),
// ADSR per sample, volume 0x3FFF = full scale.
void SourceMusicPlayer::render(int16_t* out, size_t frames) noexcept {
    const bool reverb = reverb_on_ && !reverb_buffer_.empty();
    while (frames) {
        const size_t n = std::min<size_t>(frames, kSourceMusicFramesPerTick);
        mix_.fill(0);
        if (reverb) reverb_in_.fill(0);
        if (bank_) for (auto& x : voices_) if (x.sounding) render_voice(x, n);
        if (reverb) {
            // CANDIDATE: the SPU decimates/interpolates around its 22050 Hz
            // reverb with a 39-tap FIR; this averages pairs and holds the output.
            for (size_t i = 0; i + 1 < n; i += 2) {
                int32_t l = 0, r = 0;
                reverb_step((reverb_in_[i * 2] + reverb_in_[i * 2 + 2]) >> 1,
                            (reverb_in_[i * 2 + 1] + reverb_in_[i * 2 + 3]) >> 1, l, r);
                mix_[i * 2] += l; mix_[i * 2 + 2] += l;
                mix_[i * 2 + 1] += r; mix_[i * 2 + 3] += r;
            }
        }
        for (size_t i = 0; i < n * 2; ++i) {
            const int32_t v = mix_[i];
            out[i] = int16_t(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
        out += n * 2; frames -= n;
    }
}

void SourceMusicPlayer::render_voice(Voice& x, size_t frames) noexcept {
    const auto& smp = bank_->samples[x.sample];
    const uint32_t step = x.pitch > 0x3FFF ? 0x4000u : x.pitch;
    const bool looped = smp.loop_start != 0xFFFFFFFFu;
    const bool wet = x.reverb && reverb_on_;
    for (size_t i = 0; i < frames; ++i) {
        const int32_t a = smp.pcm[x.position];
        const uint32_t nxt = x.position + 1;
        const int32_t b = nxt < smp.frames ? smp.pcm[nxt] : looped ? smp.pcm[smp.loop_start] : 0;
        const int32_t s = a + (((b - a) * int32_t(x.phase)) >> 12);
        x.envelope.advance(1);
        const int32_t e = (s * x.envelope.level()) >> 15;
        const int32_t l = (e * int32_t(x.vol_l)) >> 14, r = (e * int32_t(x.vol_r)) >> 14;
        mix_[i * 2] += l; mix_[i * 2 + 1] += r;
        if (wet) { reverb_in_[i * 2] += l; reverb_in_[i * 2 + 1] += r; }
        x.phase += step;
        x.position += x.phase >> 12;
        x.phase &= 0xFFF;
        if (x.position >= smp.frames) {
            if (!looped) { x.sounding = false; x.envelope.mute(); return; }
            x.position = smp.loop_start + (x.position - smp.frames) % (smp.frames - smp.loop_start);
        }
        if (x.envelope.finished()) { x.sounding = false; return; }
    }
}

// SPU reverb at 22050 Hz (registers 0x1F801DC0.., addresses in 8-byte units
// relative to the moving buffer address, "-2" = one halfword earlier).
void SourceMusicPlayer::reverb_step(int32_t in_l, int32_t in_r, int32_t& out_l, int32_t& out_r) noexcept {
    const auto& g = bank_->reverb_regs;
    const int32_t size = int32_t(reverb_buffer_.size());
    auto at = [&](int32_t units, int32_t back) -> int16_t& {
        int32_t i = int32_t(reverb_address_) + units * 4 - back;
        i %= size; if (i < 0) i += size;
        return reverb_buffer_[size_t(i)];
    };
    auto sat = [](int32_t v) { return int16_t(v > 32767 ? 32767 : v < -32768 ? -32768 : v); };
    auto mul = [](int32_t a, uint16_t v) { return (a * int32_t(int16_t(v))) >> 15; };
    enum { dAPF1, dAPF2, vIIR, vCOMB1, vCOMB2, vCOMB3, vCOMB4, vWALL, vAPF1, vAPF2, mLSAME, mRSAME,
           mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2, dLSAME, dRSAME, mLDIFF, mRDIFF, mLCOMB3, mRCOMB3,
           mLCOMB4, mRCOMB4, dLDIFF, dRDIFF, mLAPF1, mRAPF1, mLAPF2, mRAPF2, vLIN, vRIN };
    const int32_t lin = mul(sat(in_l), g[vLIN]), rin = mul(sat(in_r), g[vRIN]);
    auto wall = [&](int32_t input, uint16_t m, uint16_t d) {
        const int32_t prev = at(g[m], 1);
        at(g[m], 0) = sat(mul(input + mul(at(g[d], 0), g[vWALL]) - prev, g[vIIR]) + prev);
    };
    wall(lin, mLSAME, dLSAME); wall(rin, mRSAME, dRSAME);
    wall(lin, mLDIFF, dRDIFF); wall(rin, mRDIFF, dLDIFF);
    int32_t l = mul(at(g[mLCOMB1], 0), g[vCOMB1]) + mul(at(g[mLCOMB2], 0), g[vCOMB2]) +
                mul(at(g[mLCOMB3], 0), g[vCOMB3]) + mul(at(g[mLCOMB4], 0), g[vCOMB4]);
    int32_t r = mul(at(g[mRCOMB1], 0), g[vCOMB1]) + mul(at(g[mRCOMB2], 0), g[vCOMB2]) +
                mul(at(g[mRCOMB3], 0), g[vCOMB3]) + mul(at(g[mRCOMB4], 0), g[vCOMB4]);
    auto apf = [&](int32_t v, uint16_t m, uint16_t d, uint16_t vol) {
        const int32_t delayed = at(int32_t(g[m]) - int32_t(g[d]), 0);
        const int32_t t = v - mul(delayed, g[vol]);
        at(g[m], 0) = sat(t);
        return mul(t, g[vol]) + delayed;
    };
    l = apf(l, mLAPF1, dAPF1, vAPF1); r = apf(r, mRAPF1, dAPF1, vAPF1);
    l = apf(l, mLAPF2, dAPF2, vAPF2); r = apf(r, mRAPF2, dAPF2, vAPF2);
    out_l = (sat(l) * int32_t(reverb_out_)) >> 15;
    out_r = (sat(r) * int32_t(reverb_out_)) >> 15;
    reverb_address_ = (reverb_address_ + 1) % uint32_t(size);
}
} // namespace l3
