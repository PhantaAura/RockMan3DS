// 0.0.15.0 M1: exactly one combat owner per session.
#include "legends3ds/runtime_session.h"
#include <cassert>
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
RuntimeSession make_session() {
    RuntimeSession session(make_map(), TutorialRuntimeArea{}, {}, settings());
    session.reset_player({0, -1, 0}, 0, 20, 20);
    session.register_target({7, {0, -1, -200}, 1, 4, true, true});
    return session;
}
RuntimeInputFrame fire_and_lock() { RuntimeInputFrame in{}; in.fire_pressed = true; in.lock_held = true; return in; }
}

int main() {
    // Historical default: candidate projectile + candidate lock-on.
    {
        auto session = make_session();
        assert(session.candidate_combat_enabled());
        assert(session.player_shot_enabled() && !session.player_busy_8004324c());
        session.tick(fire_and_lock());
        assert(session.locked_target_id() && !session.projectiles().empty());
        // Retiring the candidate owner drops its live state immediately.
        session.set_candidate_combat_enabled(false);
        assert(!session.locked_target_id() && session.projectiles().empty());
        const auto frame = session.frame_counter();
        for (int i = 0; i < 8; ++i) session.tick(fire_and_lock());
        assert(session.frame_counter() == frame + 8);
        assert(!session.locked_target_id() && session.projectiles().empty());
        // Source admission still reads the same source player fields.
        assert(session.player_shot_enabled() && !session.player_busy_8004324c());
        // Copies keep the owner choice.
        RuntimeSession copy(session);
        copy.tick(fire_and_lock());
        assert(!copy.candidate_combat_enabled() && copy.projectiles().empty());
        // Re-enabling restores the historical path for A/B comparison.
        session.set_candidate_combat_enabled(true);
        session.tick(fire_and_lock());
        assert(session.locked_target_id() && !session.projectiles().empty());
    }
    std::puts("combat owner PASS: candidate path retired when source combat owns gameplay");
    return 0;
}
