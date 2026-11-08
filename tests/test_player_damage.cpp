// Player damage on a RuntimeSession: hit handler 0x8004174C, action choice
// 0x80041A44, flinch (action 11) and knockdown (action 12) through to the
// get-up blink or the death state. Values follow the PSXRecomp slot-2
// shockwave hits (packet 0x1000000A: 80 -> 70 -> 60, stun 10 -> 20).
#include "legends3ds/runtime_session.h"
#include "legends3ds/source_effects.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace l3;

namespace {
L3QolSettings settings() { L3QolSettings s{}; l3_qol_settings_defaults(&s); return s; }
RuntimeMap make_map() {
    RuntimeMap map; RuntimeMapArea area{};
    area.start_x = 63; area.start_z = 63; area.width = 2; area.height = 2; area.cell_count = 4;
    map.areas.push_back(area);
    map.cells.assign(4, static_cast<int16_t>(0x8000));
    map.descriptors.resize(1);
    return map;
}
ReferenceTables refs() {
    ReferenceTables r;   // authored trig, not the retail table
    for (size_t i = 0; i < r.trig.size(); ++i) { const double a = double(i) * 6.283185307179586 / 4096.0;
        r.trig[i] = {int16_t(std::lround(std::sin(a) * 4096)), int16_t(std::lround(std::cos(a) * 4096))}; }
    return r;
}
RuntimeSession make_session(uint16_t hp) {
    RuntimeSession session(make_map(), TutorialRuntimeArea{}, {}, settings());
    session.set_reference_tables(refs());
    session.reset_player({0, -1, 0}, 0, hp, 80);
    for (int i = 0; i < 4 && session.compat_player().bytes[8] != 1; ++i) session.tick({});
    return session;
}
uint8_t action(const RuntimeSession& s) { return s.compat_player().bytes[9]; }
uint8_t phase(const RuntimeSession& s) { return s.compat_player().bytes[10]; }
void hit(RuntimeSession& s, uint32_t packet) { s.deliver_player_hit_target(packet, 0, {{0, -86, 100}}, 1); s.tick({}); }
}

int main() {
    auto s = make_session(80);
    assert(s.compat_player().bytes[8] == 1 && s.player_hit_proxy_admitted());
    // First shockwave: 10 damage, stun 10 <= 16 -> flinch (action 11).
    hit(s, 0x1000000Au);
    assert(s.hp() == 70 && s.compat_player().bytes[0x73] == 10);
    assert(action(s) == 11 && phase(s) == 1);
    // Second: stun 20 > (0+1)*16 -> knockdown (action 12, phase 1 then 2).
    hit(s, 0x1000000Au);
    assert(s.hp() == 60 && s.compat_player().bytes[0x73] == 20 && action(s) == 12 && phase(s) == 2);
    assert(s.compat_player().bytes[0x110] == 0xFF && !s.player_hit_proxy_admitted());
    // While knocked down (+0x110 < 0) further packets are ignored.
    hit(s, 0x1000000Au);
    assert(s.hp() == 60);
    // No packet and outside actions 11/12: the stun meter decays by 4.
    auto d = make_session(80);
    d.deliver_player_hit_target(0x10000008u, 0, {{0, 0, 0}}, 1); d.tick({});
    for (int i = 0; i < 40 && action(d) == 11; ++i) d.tick({});
    // A push (bit 0x01000000, no damage) knocks down without HP loss.
    auto p = make_session(80);
    hit(p, 0x01000000u);
    assert(p.hp() == 80 && action(p) == 12);
    // Out of HP: the hit clamps +0x6E to 0 and always knocks down.
    auto k = make_session(5);
    hit(k, 0x1000000Au);
    assert(k.hp() == 0 && action(k) == 12);
    // The boss-defeat lock (0x800B55F9) stops the player's hit box.
    auto l = make_session(80);
    l.set_player_lock_800b55f9();
    assert(!l.player_hit_proxy_admitted());
    // The boss's shockwave (effect 128, parameter 0x42) proxy, 0x80044200.
    SourceEffectPool pool; initialize_source_effect_pool_8002e924(pool);
    const auto id = allocate_source_effect_80031618(pool).source_identity;
    const auto slot = *source_effect_slot(id);
    auto& w = pool.slots[slot];
    w[0] = 5; w[2] = 128; w[3] = 0x42; w[6] |= 1; w[8] = 1; w[0xd] = 0; w[0xe] = 0;
    w[0x38] = 0x00; w[0x39] = 0xFF; w[0x3e] = 0x00; w[0x3f] = 0x01;       // x -256, extent 0x100
    const std::array<uint8_t,4> damage{{10, 12, 16, 8}};                    // authored
    SourceHitProxyTarget shared;
    auto r = source_effect128_proxy_80044200(pool, slot, damage, &shared);
    assert(r && r->a1 == 0x80 && r->a3 == 0xa000 && r->stack18 == 0x1000000Au && r->stack20 == 0x80 && r->target == &shared);
    assert((r->pointed_words[0] & 0xFFFF) == 0xFF00);
    w[3] = 6; assert(!source_effect128_proxy_80044200(pool, slot, damage, &shared));
    w[3] = 0x42; w[0xe] = 2; assert(!source_effect128_proxy_80044200(pool, slot, damage, &shared));
    w[0xe] = 1; r = source_effect128_proxy_80044200(pool, slot, damage, &shared); assert(r && r->a3 == 0x4000);
    std::puts("player damage: ok");
    return 0;
}
