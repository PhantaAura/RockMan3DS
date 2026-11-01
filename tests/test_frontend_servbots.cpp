// Title Servbot parade (0x80015428 spawner, 0x800153EC callback, 0x80014BCC
// script reader) over a synthetic L3TS image. Scripts and values authored.
#include "legends3ds/frontend_servbots.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace l3;

namespace {
constexpr uint32_t kBase = 0x80080960u, kEnd = 0x80082104u;

ReferenceTables refs() {
    ReferenceTables r;
    for (size_t i = 0; i < r.trig.size(); ++i) {
        const double a = double(i) * 6.283185307179586 / 4096.0;
        r.trig[i] = {int16_t(std::lround(std::sin(a) * 4096)), int16_t(std::lround(std::cos(a) * 4096))};
    }
    return r;
}

struct Image {
    std::vector<uint8_t> data = std::vector<uint8_t>(kEnd - kBase);
    void half(uint32_t at, int16_t v) { data[at - kBase] = uint8_t(v); data[at - kBase + 1] = uint8_t(uint16_t(v) >> 8); }
    void word(uint32_t at, uint32_t v) { half(at, int16_t(v & 0xFFFF)); half(at + 2, int16_t(v >> 16)); }
    void script(uint32_t at, std::initializer_list<int16_t> h) { for (auto v : h) { half(at, v); at += 2; } }
    // v2 appends the cooked 0x80038370 result (target, distance, yaw, pitch).
    std::vector<uint8_t> l3ts(bool v2 = false) const {
        std::vector<uint8_t> payload = data;
        if (v2) for (int32_t v : {0, -224, 512, 1024, 0x1000, 141})
            for (unsigned b = 0; b < 4; ++b) payload.push_back(uint8_t(uint32_t(v) >> (8 * b)));
        std::vector<uint8_t> out(20);
        std::memcpy(out.data(), v2 ? "L3TS0002" : "L3TS0001", 8);
        const uint32_t hdr[3] = {kBase, uint32_t(data.size()), crc32(payload.data(), payload.size())};
        for (unsigned k = 0; k < 3; ++k) for (unsigned b = 0; b < 4; ++b) out[8 + k * 4 + b] = uint8_t(hdr[k] >> (8 * b));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }
};

Image image() {
    Image im;
    const uint32_t table = kBase + 0x100, a = kBase + 0x200, b = kBase + 0x300;
    im.word(0x800820E0u, table);          // wave 0 -> slot table
    im.word(table + 0, a);                // slot 0
    im.word(table + 4, b);                // slot 1; slots 2..15 = 0 (no script)
    // A: position, forward speed, footsteps mode 0, animation 3, wait 3, end.
    im.script(a, {1, 100, -10, 200, 6, 4, 17, 0, 5, 3, 0, 3, 27});
    // B: world motion, x = 50, z speed 2 + 1 per frame, heading 0x100 + 8,
    // then wait 1 / jump back to the wait forever.
    im.script(b, {16, 2, 50, 10, 2, 11, 1, 12, 0x100, 13, 8, 0, 1, 26, 11});
    return im;
}
} // namespace

int main() {
    const auto scripts = load_title_servbot_scripts(image().l3ts());
    assert(scripts.base == kBase && scripts.script(0, 0) == kBase + 0x200 && scripts.script(0, 2) == 0u);
    assert(!scripts.camera);
    {   // v2 carries the title camera.
        const auto v2 = load_title_servbot_scripts(image().l3ts(true));
        assert(v2.camera && v2.camera->target[1] == -224 && v2.camera->distance == 1024 && v2.camera->pitch == 141);
        assert(v2.data == scripts.data);
    }
    {   // A corrupted payload is refused.
        auto bad = image().l3ts(); bad.back() ^= 1;
        bool threw = false;
        try { (void)load_title_servbot_scripts(bad); } catch (const std::exception&) { threw = true; }
        assert(threw);
    }

    // Spawner: 16 slots, 300-frame wait, then the next wave and phase 2.
    TitleServbotParade parade;
    TitleServbotFrame f;
    title_servbot_spawn_80015428(parade, 2, 1, f);           // fade running: nothing
    assert(!parade.slots[0].active && parade.phase_0a == 0);
    title_servbot_spawn_80015428(parade, 1, 0, f);           // the title setup's spawn
    for (uint8_t i = 0; i < 16; ++i) assert(parade.slots[i].active && parade.slots[i].fresh && parade.slots[i].index == i && parade.slots[i].wave == 0);
    assert(parade.timer_06 == 300 && parade.phase_0a == 1);

    const auto r = refs();
    // The spawn update: the new slots are not live for callbacks yet.
    f = {};
    title_servbot_update_800153ec(parade, scripts, r, f);
    assert(parade.slots[0].state == 0 && parade.slots[0].active && f.sounds.empty());
    // Frame 1: slot 0 runs its script up to the wait; slots 2..15 have none.
    f = {};
    title_servbot_update_800153ec(parade, scripts, r, f);
    auto& sa = parade.slots[0];
    auto& sb = parade.slots[1];
    assert(sa.state == 1 && sa.delay == 2 && sa.anim_ac == 3 && sa.scale[0] == 0x400);
    const auto d = reference_player_motion_delta(r, 0, 4, 0, 0);
    assert(sa.fixed[0] == int32_t(uint32_t(100) << 16) + d.dx_fixed);
    assert(sa.fixed[2] == int32_t(uint32_t(200) << 16) + d.dz_fixed);
    assert(sa.raw[1] == reference_fixed_to_short_80030150(sa.fixed[1]));
    assert(f.sounds.size() == 1 && f.sounds[0] == 214 && sa.step_timer == 18);
    assert(f.animation_restart[0] && !f.animation_restart[1]);
    assert(parade.slots[2].state == 2 && parade.slots[2].active);
    assert(sb.world_motion == 1 && sb.raw[0] == 50 && sb.velocity[2] == 3 && sb.heading == 0x108);

    // Frames 2..5: A waits out its count, steps keep counting down.
    for (int k = 0; k < 2; ++k) { f = {}; title_servbot_update_800153ec(parade, scripts, r, f); assert(f.sounds.empty()); }
    assert(sa.delay == 0 && sa.step_timer == 16 && !parade.slots[2].active);
    const int32_t x_before_end = sa.fixed[0];
    f = {};
    title_servbot_update_800153ec(parade, scripts, r, f);    // reads 27: end, still moves
    assert(sa.state == 2 && sa.active && sa.fixed[0] == x_before_end + d.dx_fixed);
    f = {};
    title_servbot_update_800153ec(parade, scripts, r, f);
    assert(!sa.active);
    // B after five frames: z speed 3..7, sum 25 << 12, raw z 1.
    assert(sb.velocity[2] == 7 && sb.fixed[2] == 25 * 4096 && sb.raw[2] == 1 && sb.heading == 0x100 + 5 * 8);

    // Spawner: the 300-frame wait, then 0x8002EE90 releases every live slot
    // (B still runs), wave 1, phase 2; the menu (state 4) re-arms it.
    for (int k = 0; k < 300; ++k) { f = {}; title_servbot_spawn_80015428(parade, 2, 0, f); assert(f.released_8002ee90 == 0); }
    assert(sb.active);
    f = {};
    title_servbot_spawn_80015428(parade, 2, 0, f);
    assert(f.released_8002ee90 == 1 && !sb.active && parade.wave_08 == 1 && parade.phase_0a == 2);
    title_servbot_spawn_80015428(parade, 2, 0, f);
    assert(parade.phase_0a == 2);                           // title: stays
    title_servbot_spawn_80015428(parade, 4, 0, f);
    assert(parade.phase_0a == 0);
    // The wave counter wraps after 8.
    parade.wave_08 = 8; parade.phase_0a = 1; parade.timer_06 = 0;
    title_servbot_spawn_80015428(parade, 2, 0, f);
    assert(parade.wave_08 == 0);
    // A menu spawn (phase 0 again) is live in the same update.
    parade.phase_0a = 0;
    title_servbot_spawn_80015428(parade, 4, 0, f);
    assert(parade.slots[0].active && !parade.slots[0].fresh);

    std::cout << "frontend_servbots: OK\n";
}
