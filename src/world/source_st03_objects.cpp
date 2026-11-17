#include "legends3ds/source_st03_objects.h"
#include "legends3ds/actor_animation_bank.h"

namespace l3 {
namespace {

uint32_t u32le(const std::vector<uint8_t>& b, size_t o) {
    return uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24);
}
int16_t s16le(const uint8_t* p) { return int16_t(uint16_t(p[0] | (p[1] << 8))); }

// A byte range [token, token + n) inside one cooked data block.
const uint8_t* bytes_at(const SourceScenePackage& pkg, uint32_t token, size_t n) {
    for (const auto& d : pkg.data)
        if (token >= d.token && token - d.token + n <= d.bytes.size()) return d.bytes.data() + (token - d.token);
    return nullptr;
}

void init_80107bc0(St03Npc& n) noexcept {
    auto& w = n.w;
    w.flags_00 = uint8_t(w.flags_00 | 2u);
    switch (n.object_class) {
    case 0x26: w.word_158 = 0x01D701D7u; w.word_15c = 471; break;
    case 0x24: case 0x30: case 0x42: case 0x53: case 0x5C: case 0x44: w.word_158 = 0x021A021Au; w.word_15c = 538; break;
    case 0x5E: w.word_158 = 0x020001C0u; w.word_15c = 384; break;
    default: w.word_158 = 0x02000200u; w.word_15c = 512; break;
    }
    w.colour_15e = 0x4210;
    n.flags_348 = 2; n.timer_350 = 0;
    n.radius_368 = (w.byte_0d & 0x10u) ? 10000 : 5000;
    n.home_354 = w.raw_14; n.heading_352 = w.rot_54[1];
    w.rot_54[0] = w.rot_54[2] = 0; w.rot_44[0] = w.rot_44[1] = 0; w.word_78 = 0;
    w.state_08 = 1; w.sub_0a = 0; w.byte_0b = 0;
    w.state_09 = uint8_t(w.byte_0d & 7u);
    w.pose_ac = 0;   // 0x800322A8(work, 0, 0)
}

bool in_range_8010a4ec(const St03Npc& n, int16_t px, int16_t pz) noexcept {
    // Offset 0 (a1 = 0): the rotated vector from 0x80030058 is zero.
    const int32_t lim = int32_t(n.radius_368), twice = lim * 2;
    const int32_t dx = int32_t(uint16_t(n.w.raw_14[0] - px + lim));
    const int32_t dz = int32_t(uint16_t(n.w.raw_14[2] - pz + lim));
    return dx < twice && dz < twice;
}

void behaviour0_80107f5c(St03Npc& n, const St03NpcInputs& in, St03NpcTick& out) noexcept {
    auto& w = n.w;
    switch (w.sub_0a) {   // 0x80116E58
    case 0: {
        if (!in.rand || !in.idle_timers) { out.rand_missing = true; return; }
        w.pose_ac = st03_npc_idle_request(n.object_class);
        out.rand_used = true;
        const size_t i = size_t(n.byte_0e & 3u) * 16u + (*in.rand & 15u);
        n.timer_350 = int16_t((*in.idle_timers)[i]);
        ++w.sub_0a;
        return;
    }
    case 1:
        if (--n.timer_350 != 0) return;
        if (n.object_class == 73) n.timer_350 = 120;
        if (n.object_class == 0x53) {
            if (!in.rand) { out.rand_missing = true; ++n.timer_350; return; }
            out.rand_used = true;
        }
        w.pose_ac = st03_npc_fidget_request(n.object_class, in.rand.value_or(0));
        ++w.sub_0a;
        return;
    case 2:
        if (n.timer_350 != 0 && --n.timer_350 != 0) return;
        if (!in.animation_finished) return;
        if (n.object_class == 73) { w.pose_ac = 9; ++w.sub_0a; } else w.sub_0a = 0;
        return;
    case 3:
        if (in.animation_finished) w.sub_0a = 0;
        return;
    default: return;
    }
}

} // namespace

std::vector<St03ObjectRecord> st03_object_set(const SourceScenePackage& package, uint8_t n) {
    std::vector<St03ObjectRecord> out;
    const uint8_t* t = bytes_at(package, kSt03ObjectSetTable + 8u * n, 8);
    if (!t) return out;
    const uint32_t count = uint32_t(t[0] | (t[1] << 8) | (t[2] << 16) | (uint32_t(t[3]) << 24));
    const uint32_t at = uint32_t(t[4] | (t[5] << 8) | (t[6] << 16) | (uint32_t(t[7]) << 24));
    if (count > kSt03ObjectSlots) return out;
    const uint8_t* r = bytes_at(package, at, size_t(count) * 20u);
    if (!r && count) return out;
    for (uint32_t i = 0; i < count; ++i, r += 20) {
        St03ObjectRecord o;
        o.slot = r[0]; o.object_class = r[1]; o.subtype = r[2]; o.family = r[3]; o.byte_04 = r[4];
        o.word_08 = {r[8], r[9], r[10], r[11]};
        o.position = {s16le(r + 12), s16le(r + 14), s16le(r + 16)};
        o.heading = uint16_t(r[18] | (r[19] << 8));
        out.push_back(o);
    }
    (void)u32le;
    return out;
}

bool st03_generic_npc_class(uint8_t c, uint8_t byte_04) noexcept {
    return byte_04 == 0 && (c == 0x05 || c == 0x25 || c == 0x11 || c == 0x26 || c == 0x24);
}

St03Npc spawn_st03_object(const St03ObjectRecord& r) {
    St03Npc n;
    auto& w = n.w;
    w.live = true; w.slot = r.slot; w.subtype = r.object_class;
    n.object_class = r.object_class; n.family = r.family;
    w.byte_03 = r.byte_04;
    w.byte_0c = r.word_08[0]; w.byte_0d = r.word_08[1]; n.byte_0e = r.word_08[2]; n.byte_0f = r.word_08[3];
    for (size_t a = 0; a < 3; ++a) { w.raw_14[a] = r.position[a]; w.fixed_1c[a] = int32_t(uint32_t(uint16_t(r.position[a])) << 16); }
    w.rot_54[1] = r.heading;
    w.state_08 = 0;
    n.generic = r.family == 0x20 && st03_generic_npc_class(r.object_class, r.byte_04);
    (void)r.subtype;
    return n;
}

uint8_t st03_npc_idle_request(uint8_t c) noexcept { return c == 0x11 ? 1 : 0; }

uint8_t st03_npc_fidget_request(uint8_t c, uint16_t rand) noexcept {
    if (c == 42) return 9;
    if (c < 43) { if (c == 0) return 0; if (c == 17) return 1; return 8; }
    if (c == 80) return 0;
    if (c < 81) return c == 57 ? 0 : 8;
    if (c == 83) return uint8_t((rand & 1u) | 8u);
    return 8;
}

St03NpcTick step_st03_npc(St03Npc& n, const St03NpcInputs& in) noexcept {
    St03NpcTick out;
    auto& w = n.w;
    if (!w.live || !n.generic) return out;
    bool fresh = false;
    if (w.state_08 == 0) { init_80107bc0(n); fresh = true; out.anim_request = 0; }
    if (w.state_08 != 1) return out;
    if (!in_range_8010a4ec(n, in.player_x, in.player_z)) return out;   // the init request stays
    out.in_range = true;
    n.pose_ad = w.pose_ac;   // +0xAD = +0xAC
    w.colour_15e = 0x4210;
    if (n.flags_348 & 2u) out.talk_unported = true;   // 0x8010A344
    if (w.state_09 == 0) behaviour0_80107f5c(n, in, out); else out.behaviour_unported = true;
    if (!(w.byte_0d & 0x20u) && in.floor_y) { w.raw_14[1] = *in.floor_y; w.fixed_1c[1] = int32_t(uint32_t(uint16_t(*in.floor_y)) << 16); }
    // +0xAD (saved before the behaviour) against +0xAC.
    if (n.pose_ad != w.pose_ac) out.anim_request = w.pose_ac; else out.anim_advance = true;
    (void)fresh;
    return out;
}

St02ActorModel decode_st03_npc_model(const EbdDirectory& ebd, uint8_t object_class, size_t cap) noexcept {
    // The generic NPC requests 0 / 1 (idle) and 8 / 9 (fidget); a row whose
    // bank lacks some keeps the ones it has (request 0 at least).
    static const uint8_t all[] = {0, 1, 8, 9};
    St02ActorModel out;
    try {
        const auto rows = find_ebd_entries(ebd, 0x20, object_class);
        if (rows.empty() || ebd.source_payload.empty()) return out;
        auto mesh = decode_ebd_actor_mesh(ebd, rows[0], ActorMeshTier::NEAR, cap);
        const size_t mesh_bytes = mesh.footprint.decoded_bytes;
        if (mesh_bytes > cap) return out;
        std::vector<uint8_t> have;
        for (const uint8_t r : all) {
            const uint8_t one[] = {r};
            try { auto b = decode_ebd_actor_animation_bank(ebd, rows[0], mesh, one, 1, cap - mesh_bytes);
                  if (b.requests.size() == 1) have.push_back(r); } catch (...) {}
        }
        if (have.empty()) return out;
        auto bank = decode_ebd_actor_animation_bank(ebd, rows[0], mesh, have.data(), have.size(), cap - mesh_bytes);
        if (bank.requests.size() != have.size()) return out;
        out.decoded_bytes = mesh_bytes + bank.decoded_bytes;
        out.mesh = std::make_shared<const ActorMeshResource>(std::move(mesh));
        out.bank = std::make_shared<const ActorAnimationBank>(std::move(bank));
    } catch (...) { return St02ActorModel{}; }
    return out;
}

} // namespace l3
