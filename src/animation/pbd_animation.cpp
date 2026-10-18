#include "legends3ds/pbd_animation.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace l3 {
namespace {
bool local_ptr(uint32_t p, uint32_t base, size_t n, size_t need = 1) {
    return p >= base && uint64_t(p) + need <= uint64_t(base) + n;
}

uint16_t infer_action_slots(const std::vector<uint8_t>& s, uint32_t base) {
    uint32_t first = std::numeric_limits<uint32_t>::max();
    const size_t scan = std::min<size_t>(s.size(), 0x400);
    for (size_t off = 0; off + 4 <= scan; off += 4) {
        const uint32_t p = rd_u32(s.data() + off);
        if (local_ptr(p, base, s.size())) first = std::min(first, p - base);
    }
    if (first == std::numeric_limits<uint32_t>::max() || (first & 3u) != 0 || first == 0 || first > 0x400u) {
        throw std::runtime_error("cannot infer S action pointer table");
    }
    const uint32_t slots = first / 4u;
    if (slots == 0 || slots > 256 || slots * 4u > s.size()) throw std::runtime_error("invalid S action slot count");
    return static_cast<uint16_t>(slots);
}

std::vector<uint32_t> read_prefix_ptrs(const std::vector<uint8_t>& b, uint16_t slots) {
    if (size_t(slots) * 4 > b.size()) throw std::runtime_error("pointer prefix outside file");
    std::vector<uint32_t> p(slots);
    for (uint32_t i = 0; i < slots; ++i) p[i] = rd_u32(b.data() + size_t(i) * 4);
    return p;
}

size_t next_local_offset(const std::vector<uint32_t>& ptrs, uint32_t p, uint32_t base, size_t file_bytes) {
    size_t next = file_bytes;
    for (uint32_t q : ptrs) if (q > p && local_ptr(q, base, file_bytes)) next = std::min(next, size_t(q - base));
    return next;
}
}

std::array<uint16_t, kPbdPackedAngleChannels> unpack_pbd_12bit_frame(const uint8_t* src) {
    if (!src) throw std::runtime_error("null packed frame");
    std::array<uint16_t, kPbdPackedAngleChannels> out{};
    // Little-endian 12-bit bitstream. Every two channels occupy three bytes:
    // A = b0 | ((b1 & 0x0F) << 8), B = (b1 >> 4) | (b2 << 4).
    for (size_t i = 0; i < kPbdPackedAngleChannels / 2; ++i) {
        const uint8_t b0 = src[i * 3 + 0], b1 = src[i * 3 + 1], b2 = src[i * 3 + 2];
        out[i * 2 + 0] = static_cast<uint16_t>(uint16_t(b0) | (uint16_t(b1 & 0x0F) << 8));
        out[i * 2 + 1] = static_cast<uint16_t>(uint16_t(b1 >> 4) | (uint16_t(b2) << 4));
    }
    return out;
}

std::array<uint8_t, kPbdPackedFrameBytes> pack_pbd_12bit_frame(const std::array<uint16_t, kPbdPackedAngleChannels>& c) {
    std::array<uint8_t, kPbdPackedFrameBytes> out{};
    for (size_t i = 0; i < kPbdPackedAngleChannels / 2; ++i) {
        const uint16_t a = static_cast<uint16_t>(c[i * 2] & 0x0FFFu);
        const uint16_t b = static_cast<uint16_t>(c[i * 2 + 1] & 0x0FFFu);
        out[i * 3 + 0] = static_cast<uint8_t>(a & 0xFFu);
        out[i * 3 + 1] = static_cast<uint8_t>((a >> 8) | ((b & 0x0Fu) << 4));
        out[i * 3 + 2] = static_cast<uint8_t>(b >> 4);
    }
    return out;
}

int16_t sign_extend_pbd_12bit(uint16_t value) {
    value &= 0x0FFFu;
    return static_cast<int16_t>((value & 0x0800u) ? (int32_t(value) - 0x1000) : int32_t(value));
}

PbdMainPoseBank decode_pbd_main_pose_bank(const std::array<uint16_t, kPbdPackedAngleChannels>& c) {
    PbdMainPoseBank out;
    out.root_translation = {sign_extend_pbd_12bit(c[0]), sign_extend_pbd_12bit(c[1]), sign_extend_pbd_12bit(c[2])};

    // Exact scratch-vector mapping reconstructed from the player pose path.
    out.part_rotation[0]  = {c[3],  c[4],  c[5]};
    out.part_rotation[1]  = {c[6],  c[7],  c[8]};
    out.part_rotation[2]  = {c[9],  c[10], c[11]};
    out.part_rotation[3]  = {c[12], 0,     0};
    out.part_rotation[4]  = {0,     c[13], c[14]};
    out.part_rotation[5]  = {c[15], c[16], c[17]};
    out.part_rotation[6]  = {c[18], 0,     0};
    out.part_rotation[7]  = {0,     c[19], c[20]};
    out.part_rotation[8]  = {c[21], c[22], c[23]};
    out.part_rotation[9]  = {c[24], c[25], c[26]};
    out.part_rotation[10] = {c[27], 0,     0};
    out.part_rotation[11] = {c[28], c[29], c[30]};
    out.part_rotation[12] = {c[31], c[32], c[33]};
    out.part_rotation[13] = {c[34], 0,     0};
    out.part_rotation[14] = {c[35], c[36], c[37]};
    out.trailing_channels = {c[38], c[39]};
    return out;
}

bool pbd_main_pose_trailing_channels_zero(const std::array<uint16_t, kPbdPackedAngleChannels>& c) {
    return (c[38] & 0x0FFFu) == 0 && (c[39] & 0x0FFFu) == 0;
}

const PbdAnimationAction* find_pbd_animation_action(const PbdAnimation& animation, uint16_t action_index) {
    for (const auto& action : animation.actions) {
        if (action.action_index == action_index) return &action;
    }
    return nullptr;
}

const PbdAnimationAction* pbd_main_body_pose_action_for_request(const PbdAnimation& animation,
                                                                uint8_t request) {
    // PL00M00 starts with the non-frame-table skeleton anchor at M slot 0.
    // ROCK_NEO 0x800329D4 therefore indexes its pose pointer family from
    // m_load_base + 4 + request*4, i.e. cooked M action request+1.
    if (request == 0xFFu || uint16_t(request) + 1u >= animation.action_slots) return nullptr;
    const auto* s_action = find_pbd_animation_action(animation, request);
    const auto* m_action = find_pbd_animation_action(animation, uint16_t(request) + 1u);
    if (!s_action || !s_action->has_s_records || s_action->s_records.empty()) return nullptr;
    if (!m_action || !m_action->m_is_frame_table || m_action->angle_frames.empty()) return nullptr;
    return m_action;
}

namespace {
uint8_t s_record_pose(const PbdSRecord& r) { return static_cast<uint8_t>(r.word0 & 0x00FFu); }
uint8_t s_record_ticks(const PbdSRecord& r) { return static_cast<uint8_t>((r.word0 >> 8) & 0x00FFu); }
int8_t s_record_control(const PbdSRecord& r) { return static_cast<int8_t>((r.word1 >> 8) & 0x00FFu); }

bool load_main_body_record(const PbdAnimation& animation, PbdMainBodyTrack& track, uint8_t index) {
    const auto* s_action = find_pbd_animation_action(animation, track.request);
    const auto* m_action = pbd_main_body_pose_action_for_request(animation, track.request);
    if (!s_action || !m_action || index >= s_action->s_records.size()) return false;
    const auto& r = s_action->s_records[index];
    const uint8_t pose = s_record_pose(r);
    if (pose >= m_action->angle_frames.size()) return false;
    track.record_index = index;
    track.ticks_remaining = s_record_ticks(r);
    track.pose_frame = pose;
    track.valid = true;
    return true;
}
}

PbdMainBodyTrack pbd_main_body_track_begin(const PbdAnimation& animation,
                                           uint8_t request,
                                           uint8_t substate) {
    PbdMainBodyTrack out;
    out.request = request;
    if (!load_main_body_record(animation, out, substate)) out.valid = false;
    return out;
}

bool pbd_main_body_track_tick(const PbdAnimation& animation, PbdMainBodyTrack& track) {
    if (!track.valid) return false;
    const auto* s_action = find_pbd_animation_action(animation, track.request);
    if (!s_action || track.record_index >= s_action->s_records.size()) {
        track.valid = false;
        return false;
    }

    // 0x8003234C decrements byte +A5 first and only changes records when the
    // 8-bit result reaches zero.  A zero seed therefore wraps to 0xFF exactly
    // like the retail lbu/addiu/sb sequence.
    track.ticks_remaining = static_cast<uint8_t>(track.ticks_remaining - 1u);
    if (track.ticks_remaining != 0u) return true;

    const auto& current = s_action->s_records[track.record_index];
    const int8_t control = s_record_control(current);
    if (control < 0) {
        if (control == -1) {
            // Retail reloads byte 1 of the same S record without changing +98.
            track.ticks_remaining = s_record_ticks(current);
            return true;
        }
        const uint8_t target = static_cast<uint8_t>(control) & 0x7Fu;
        if (!load_main_body_record(animation, track, target)) {
            track.valid = false;
            return false;
        }
        return true;
    }

    const size_t next = size_t(track.record_index) + 1u;
    if (next >= s_action->s_records.size() || next > 0xFFu ||
        !load_main_body_record(animation, track, static_cast<uint8_t>(next))) {
        track.valid = false;
        return false;
    }
    return true;
}

const std::array<uint16_t, kPbdPackedAngleChannels>* pbd_main_body_track_frame(
    const PbdAnimation& animation, const PbdMainBodyTrack& track) {
    if (!track.valid) return nullptr;
    const auto* m_action = pbd_main_body_pose_action_for_request(animation, track.request);
    if (!m_action || track.pose_frame >= m_action->angle_frames.size()) return nullptr;
    return &m_action->angle_frames[track.pose_frame];
}


PbdMainSkeleton parse_pbd_main_skeleton(const std::vector<uint8_t>& m, uint32_t mbase) {
    if (m.size() < 4) throw std::runtime_error("main M PBD too small for skeleton anchor pointer");
    const uint32_t p = rd_u32(m.data());
    constexpr size_t kEntryBytes = 8;
    constexpr size_t kTableBytes = kPbdMainPosePartCount * kEntryBytes;
    if (!local_ptr(p, mbase, m.size(), kTableBytes)) throw std::runtime_error("main M action-0 skeleton anchor table out of range");
    PbdMainSkeleton out;
    out.source_special_offset = p - mbase;
    const size_t off = out.source_special_offset;
    for (size_t part = 0; part < kPbdMainPosePartCount; ++part) {
        const size_t q = off + part * kEntryBytes;
        out.base_translation[part] = {static_cast<int16_t>(rd_u16(m.data()+q+0)),
                                      static_cast<int16_t>(rd_u16(m.data()+q+2)),
                                      static_cast<int16_t>(rd_u16(m.data()+q+4))};
        out.source_reserved[part] = static_cast<int16_t>(rd_u16(m.data()+q+6));
    }
    return out;
}

std::vector<uint8_t> cook_l3skeleton(const PbdMainSkeleton& s) {
    std::vector<uint8_t> o; o.reserve(48u + kPbdMainPosePartCount * 12u);
    o.insert(o.end(), {'L','3','S','K'}); wr_u16(o, kL3SkeletonVersion); wr_u16(o, 48);
    wr_u32(o, 3u); // bit0: parent tree proven from matrix push/pop path; bit1: part-0 source vector retained though current pose path uses animated root translation instead.
    wr_u32(o, kPbdMainPosePartCount); wr_u32(o, s.source_special_offset);
    const size_t po=o.size(); wr_u32(o,0); const size_t pf=o.size(); wr_u32(o,0); const size_t pc=o.size(); wr_u32(o,0);
    for(int i=0;i<4;++i) wr_u32(o,0);
    if(o.size()!=48) throw std::runtime_error("L3SK header size bug");
    const uint32_t off=uint32_t(o.size());
    for(size_t part=0;part<kPbdMainPosePartCount;++part){
        o.push_back(static_cast<uint8_t>(part)); o.push_back(static_cast<uint8_t>(s.parent[part]));
        uint16_t flags = part==0 ? 1u : 0u; wr_u16(o,flags);
        wr_u16(o,static_cast<uint16_t>(s.base_translation[part][0]));
        wr_u16(o,static_cast<uint16_t>(s.base_translation[part][1]));
        wr_u16(o,static_cast<uint16_t>(s.base_translation[part][2]));
        wr_u16(o,static_cast<uint16_t>(s.source_reserved[part]));
    }
    patch_u32(o,po,off); patch_u32(o,pf,uint32_t(o.size())); patch_u32(o,pc,crc32(o.data()+48,o.size()-48));
    return o;
}

L3SkeletonHeader inspect_l3skeleton(const std::vector<uint8_t>& b,bool verify){
    if(b.size()<48||b[0]!='L'||b[1]!='3'||b[2]!='S'||b[3]!='K')throw std::runtime_error("not L3SK");
    L3SkeletonHeader h;h.version=rd_u16(&b[4]);h.header_bytes=rd_u16(&b[6]);h.flags=rd_u32(&b[8]);h.part_count=rd_u32(&b[12]);h.source_special_offset=rd_u32(&b[16]);h.off_parts=rd_u32(&b[20]);h.file_bytes=rd_u32(&b[24]);h.payload_crc32=rd_u32(&b[28]);
    if(h.version!=kL3SkeletonVersion||h.header_bytes!=48||h.part_count!=kPbdMainPosePartCount)throw std::runtime_error("unsupported L3SK");
    if(h.file_bytes!=b.size()||h.off_parts<48||uint64_t(h.off_parts)+uint64_t(h.part_count)*12u>h.file_bytes)throw std::runtime_error("L3SK size/range mismatch");
    if(verify&&crc32(b.data()+48,b.size()-48)!=h.payload_crc32)throw std::runtime_error("L3SK CRC mismatch");
    return h;
}

PbdAnimation parse_player_pbd_animation(const std::vector<uint8_t>& s, uint32_t sbase,
                                        const std::vector<uint8_t>& m, uint32_t mbase) {
    if (s.empty() || m.empty()) throw std::runtime_error("empty S/M PBD animation file");
    PbdAnimation out;
    out.s_load_base = sbase; out.m_load_base = mbase; out.action_slots = infer_action_slots(s, sbase);
    const auto sp = read_prefix_ptrs(s, out.action_slots);
    const auto mp = read_prefix_ptrs(m, out.action_slots);

    for (uint16_t i = 0; i < out.action_slots; ++i) {
        const bool hs = local_ptr(sp[i], sbase, s.size());
        const bool hm = local_ptr(mp[i], mbase, m.size());
        if (!hs && !hm) continue;
        PbdAnimationAction a;
        a.action_index = i; a.has_s_records = hs; a.has_m_pointer = hm;
        if (hs) {
            const size_t so = sp[i] - sbase;
            const size_t end = next_local_offset(sp, sp[i], sbase, s.size());
            if (end < so || ((end - so) % 8) != 0) throw std::runtime_error("S action region is not 8-byte-record aligned");
            a.source_s_offset = static_cast<uint32_t>(so);
            a.s_records.reserve((end - so) / 8);
            for (size_t p = so; p < end; p += 8) a.s_records.push_back({rd_u16(s.data()+p), rd_u16(s.data()+p+2), rd_u32(s.data()+p+4)});
        }
        if (hm) {
            const size_t mo = mp[i] - mbase;
            a.source_m_offset = static_cast<uint32_t>(mo);
            if (mo + 4 > m.size()) throw std::runtime_error("M action pointer out of range");
            const uint32_t first_frame = rd_u32(m.data() + mo);
            bool frame_table = false;
            size_t frame_count = 0;
            if (local_ptr(first_frame, mbase, m.size(), kPbdPackedFrameBytes) && first_frame >= mp[i] && ((first_frame - mp[i]) % 4u) == 0) {
                frame_count = (first_frame - mp[i]) / 4u;
                if (frame_count >= 1 && frame_count <= 512 && mo + frame_count * 4 <= m.size()) {
                    frame_table = true;
                    for (size_t f = 0; f < frame_count; ++f) {
                        const uint32_t fp = rd_u32(m.data() + mo + f * 4);
                        if (!local_ptr(fp, mbase, m.size(), kPbdPackedFrameBytes)) { frame_table = false; break; }
                    }
                }
            }
            if (frame_table) {
                a.m_is_frame_table = true;
                a.angle_frames.reserve(frame_count);
                for (size_t f = 0; f < frame_count; ++f) {
                    const uint32_t fp = rd_u32(m.data() + mo + f * 4);
                    a.angle_frames.push_back(unpack_pbd_12bit_frame(m.data() + (fp - mbase)));
                }
            } else {
                a.m_special_or_unknown = true;
            }
        }
        out.actions.push_back(std::move(a));
    }
    return out;
}

std::vector<uint8_t> cook_l3animation(const PbdAnimation& a, L3AnimationStats* stats_out) {
    L3AnimationStats s;
    s.action_slots = a.action_slots; s.populated_actions = static_cast<uint32_t>(a.actions.size());
    for (const auto& x : a.actions) {
        if (x.m_is_frame_table) ++s.frame_table_actions;
        if (x.m_special_or_unknown) ++s.special_m_actions;
        s.s_records += static_cast<uint32_t>(x.s_records.size());
        s.frames += static_cast<uint32_t>(x.angle_frames.size());
    }
    std::vector<uint8_t> o;
    o.reserve(80 + a.actions.size()*32 + size_t(s.s_records)*8 + size_t(s.frames)*80);
    o.insert(o.end(), {'L','3','A','N'}); wr_u16(o,kL3AnimationVersion); wr_u16(o,80); wr_u32(o,1u); // bit0: 12-bit channels expanded to u16
    wr_u32(o,s.action_slots); wr_u32(o,s.populated_actions); wr_u32(o,s.frame_table_actions); wr_u32(o,s.special_m_actions); wr_u32(o,s.s_records); wr_u32(o,s.frames);
    wr_u32(o,kPbdPackedAngleChannels);
    const size_t p_actions=o.size();wr_u32(o,0); const size_t p_s=o.size();wr_u32(o,0); const size_t p_frames=o.size();wr_u32(o,0);
    const size_t p_file=o.size();wr_u32(o,0); const size_t p_crc=o.size();wr_u32(o,0);
    for(int i=0;i<5;++i)wr_u32(o,0);
    if(o.size()!=80)throw std::runtime_error("L3AN header size bug");

    uint32_t s_cursor=0, f_cursor=0;
    const uint32_t off_actions=uint32_t(o.size());
    for (const auto& x : a.actions) {
        wr_u16(o,x.action_index);
        uint16_t flags=0; if(x.has_s_records)flags|=1; if(x.has_m_pointer)flags|=2; if(x.m_is_frame_table)flags|=4; if(x.m_special_or_unknown)flags|=8;
        wr_u16(o,flags); wr_u32(o,s_cursor); wr_u32(o,uint32_t(x.s_records.size())); wr_u32(o,f_cursor); wr_u32(o,uint32_t(x.angle_frames.size()));
        wr_u32(o,x.source_s_offset); wr_u32(o,x.source_m_offset); wr_u32(o,0);
        s_cursor += uint32_t(x.s_records.size()); f_cursor += uint32_t(x.angle_frames.size());
    }
    const uint32_t off_s=uint32_t(o.size());
    for(const auto&x:a.actions)for(const auto&r:x.s_records){wr_u16(o,r.word0);wr_u16(o,r.word1);wr_u32(o,r.word2);}
    const uint32_t off_frames=uint32_t(o.size());
    for(const auto&x:a.actions)for(const auto&fr:x.angle_frames)for(uint16_t v:fr)wr_u16(o,static_cast<uint16_t>(v&0x0FFFu));
    patch_u32(o,p_actions,off_actions);patch_u32(o,p_s,off_s);patch_u32(o,p_frames,off_frames);patch_u32(o,p_file,uint32_t(o.size()));
    patch_u32(o,p_crc,crc32(o.data()+80,o.size()-80));
    if (stats_out) *stats_out = s;
    return o;
}

L3AnimationHeader inspect_l3animation(const std::vector<uint8_t>&b,bool verify){
    if(b.size()<80||b[0]!='L'||b[1]!='3'||b[2]!='A'||b[3]!='N')throw std::runtime_error("not L3AN");
    L3AnimationHeader h;h.version=rd_u16(&b[4]);h.header_bytes=rd_u16(&b[6]);h.flags=rd_u32(&b[8]);size_t q=12;
    h.stats.action_slots=rd_u32(&b[q]);q+=4;h.stats.populated_actions=rd_u32(&b[q]);q+=4;h.stats.frame_table_actions=rd_u32(&b[q]);q+=4;h.stats.special_m_actions=rd_u32(&b[q]);q+=4;h.stats.s_records=rd_u32(&b[q]);q+=4;h.stats.frames=rd_u32(&b[q]);q+=4;
    h.angle_channels_per_frame=rd_u32(&b[q]);q+=4;h.off_actions=rd_u32(&b[q]);q+=4;h.off_s_records=rd_u32(&b[q]);q+=4;h.off_frames=rd_u32(&b[q]);q+=4;h.file_bytes=rd_u32(&b[q]);q+=4;h.payload_crc32=rd_u32(&b[q]);
    if(h.version!=kL3AnimationVersion||h.header_bytes!=80||h.angle_channels_per_frame!=kPbdPackedAngleChannels)throw std::runtime_error("unsupported L3AN");
    if(h.file_bytes!=b.size())throw std::runtime_error("L3AN file size mismatch");
    if(!(h.off_actions>=80&&h.off_actions<=h.off_s_records&&h.off_s_records<=h.off_frames&&h.off_frames<=h.file_bytes))throw std::runtime_error("L3AN bad section offsets");
    const uint64_t need_actions=uint64_t(h.off_actions)+uint64_t(h.stats.populated_actions)*32u;
    const uint64_t need_s=uint64_t(h.off_s_records)+uint64_t(h.stats.s_records)*8u;
    const uint64_t need_f=uint64_t(h.off_frames)+uint64_t(h.stats.frames)*kPbdPackedAngleChannels*2u;
    if(need_actions>h.off_s_records||need_s>h.off_frames||need_f>h.file_bytes)throw std::runtime_error("L3AN section size mismatch");
    if(verify&&crc32(b.data()+80,b.size()-80)!=h.payload_crc32)throw std::runtime_error("L3AN CRC mismatch");
    return h;
}


PbdAnimation load_l3animation(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3animation(b, verify_crc);
    PbdAnimation out;
    out.action_slots = static_cast<uint16_t>(h.stats.action_slots);
    if (h.stats.action_slots > 0xFFFFu) throw std::runtime_error("L3AN action slot count exceeds runtime field");
    out.actions.reserve(h.stats.populated_actions);
    std::vector<uint8_t> seen(h.stats.action_slots, 0u);
    for (uint32_t i = 0; i < h.stats.populated_actions; ++i) {
        const size_t o = size_t(h.off_actions) + size_t(i) * 32u;
        PbdAnimationAction a;
        a.action_index = rd_u16(&b[o]);
        const uint16_t flags = rd_u16(&b[o + 2]);
        if ((flags & ~0x000Fu) != 0u || rd_u32(&b[o + 28]) != 0u) throw std::runtime_error("L3AN action flags/reserved field invalid");
        if (a.action_index >= h.stats.action_slots) throw std::runtime_error("L3AN action index outside slot table");
        if (seen[a.action_index]++) throw std::runtime_error("L3AN duplicate action index");
        const uint32_t sf = rd_u32(&b[o + 4]), sc = rd_u32(&b[o + 8]);
        const uint32_t ff = rd_u32(&b[o + 12]), fc = rd_u32(&b[o + 16]);
        if (uint64_t(sf) + sc > h.stats.s_records || uint64_t(ff) + fc > h.stats.frames)
            throw std::runtime_error("L3AN action data range invalid");
        a.has_s_records = (flags & 1u) != 0; a.has_m_pointer = (flags & 2u) != 0;
        a.m_is_frame_table = (flags & 4u) != 0; a.m_special_or_unknown = (flags & 8u) != 0;
        a.source_s_offset = rd_u32(&b[o + 20]); a.source_m_offset = rd_u32(&b[o + 24]);
        if (!a.has_s_records && sc != 0) throw std::runtime_error("L3AN S-record range without S flag");
        if (!a.m_is_frame_table && fc != 0) throw std::runtime_error("L3AN frames without frame-table flag");
        a.s_records.reserve(sc);
        for (uint32_t j = 0; j < sc; ++j) {
            const size_t q = size_t(h.off_s_records) + size_t(sf + j) * 8u;
            a.s_records.push_back({rd_u16(&b[q]), rd_u16(&b[q + 2]), rd_u32(&b[q + 4])});
        }
        a.angle_frames.reserve(fc);
        for (uint32_t j = 0; j < fc; ++j) {
            const size_t q = size_t(h.off_frames) + size_t(ff + j) * kPbdPackedAngleChannels * 2u;
            std::array<uint16_t, kPbdPackedAngleChannels> fr{};
            for (size_t k = 0; k < fr.size(); ++k) {
                const uint16_t raw = rd_u16(&b[q + k * 2u]);
                if ((raw & 0xF000u) != 0u) throw std::runtime_error("L3AN frame channel exceeds 12 bits");
                fr[k] = raw;
            }
            a.angle_frames.push_back(fr);
        }
        out.actions.push_back(std::move(a));
    }
    return out;
}

PbdMainSkeleton load_l3skeleton(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3skeleton(b, verify_crc);
    PbdMainSkeleton out;
    out.source_special_offset = h.source_special_offset;
    std::array<uint8_t, kPbdMainPosePartCount> seen{};
    for (uint32_t i = 0; i < h.part_count; ++i) {
        const size_t o = size_t(h.off_parts) + size_t(i) * 12u;
        const uint8_t part = b[o];
        const int8_t parent = static_cast<int8_t>(b[o + 1]);
        const uint16_t flags = rd_u16(&b[o + 2]);
        if (part >= kPbdMainPosePartCount || seen[part]++) throw std::runtime_error("L3SK part id invalid/duplicate");
        if ((flags & ~1u) != 0u || ((part == 0u) != ((flags & 1u) != 0u))) throw std::runtime_error("L3SK part flags invalid");
        if ((part == 0u && parent != -1) || (part != 0u && (parent < 0 || parent >= static_cast<int8_t>(part))))
            throw std::runtime_error("L3SK parent topology invalid");
        out.parent[part] = parent;
        out.base_translation[part] = {rd_s16(&b[o + 4]), rd_s16(&b[o + 6]), rd_s16(&b[o + 8])};
        out.source_reserved[part] = rd_s16(&b[o + 10]);
    }
    return out;
}

} // namespace l3
