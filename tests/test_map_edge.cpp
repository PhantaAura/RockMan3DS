// 0.0.15.0 M2: map-edge / collision regressions for the Area 0/1 escape.
//
// Synthetic authored geometry only (no retail payload). Each case pins one of
// the root causes found with the owner-local tutorial pack:
//  1. a wall push must restart the candidate scan (0x8004BDC8 case 1 ->
//     LAB_8004C144 returns true -> 0x8004B6AC LAB_8004BB24), otherwise a
//     falling player pushed out of a block never re-tests the floor cell and
//     sinks through it;
//  2. leaving the floor selects the 0x8003C4EC fall branch (no-floor sentinel
//     32767) instead of walking on air;
//  3. an untranslated primitive response applies the 0x8004B414 failure
//     contract (restore the +2C pre-move pose) instead of free motion;
//  4. a null selected contact list contributes no geometry and is not an
//     unsupported primitive.
#include "legends3ds/runtime_session.h"
#include <cassert>
#include <cstdio>

namespace {
void put(L3PlayerCompatWork& w, size_t o, int16_t v) { w.bytes[o] = uint8_t(v); w.bytes[o + 1] = uint8_t(uint16_t(v) >> 8); }

l3::ReferenceTables refs() {
    l3::ReferenceTables r; for (auto& t : r.trig) t = {0, 4096};
    r.trig[0x400] = {4096, 0}; r.trig[0x800] = {0, -4096}; r.trig[0xC00] = {-4096, 0};
    return r;
}
l3::RuntimeSession session(l3::RuntimeMap m, l3::RuntimeVec3i at) {
    l3::TutorialRuntimeArea a; a.area_index = 0; L3QolSettings q{}; l3_qol_settings_defaults(&q);
    l3::RuntimeSession s(std::move(m), std::move(a), {}, q); s.set_reference_tables(refs());
    s.reset_player(at, 0, 100, 100); return s;
}
// 2x2 cells at grid 63..64: x/z in [-512,0) for 63 and [0,512) for 64.
l3::RuntimeMap grid(std::array<int16_t, 4> cells) {
    l3::RuntimeMap m; l3::RuntimeMapArea a; a.start_x = 63; a.start_z = 63; a.width = 2; a.height = 2;
    a.cell_count = 4; m.areas.push_back(a); m.cells.assign(cells.begin(), cells.end());
    m.idx.resize(2); return m;
}
// Positive cell 1: tall code-1 block covering world x [0,512] (IDX at 256,-256).
void add_block(l3::RuntimeMap& m) {
    m.idx[1] = {256, 0, -256, 0, 0};
    l3::RuntimeMapDescriptor d; d.default_contact_index = 0; m.descriptors.push_back(d);
    m.contact_lists.push_back({0, 1});
    m.contact_groups.push_back({-256, 256, -1024, 1024, 0, 1});
    m.contact_primitives.push_back({-2048, 512, 1, 0});
}

void corner_landing_restarts_scan() {
    // Column 63 = packed floor (top 0 -> floor -1), column 64 = block cell.
    auto m = grid({int16_t(-32768), 1, int16_t(-32768), 1}); add_block(m);
    L3PlayerCompatWork w{};
    // Falling (+74=1): prior (-40,-10), requested (0,+20) lies in the block
    // cell and below the floor top. The candidate order visits the packed floor
    // cell first (not yet containing), then the block: only a restarted scan
    // can land the pushed pose (-32) on the floor cell it now occupies.
    put(w, 0x14, 0); put(w, 0x16, 20); put(w, 0x18, -256);
    put(w, 0x2C, -40); put(w, 0x2E, -10); put(w, 0x30, -256);
    w.bytes[0x74] = 1;
    const auto c = l3::runtime_player_contact_step(m, 0, w);
    assert(c.status == l3::RuntimeContactStatus::SourceBackedPartial);
    assert(c.scratch.x == -32 && c.wall_contact);      // pushed out of the block
    assert(c.scratch.y == -1 && c.scratch.contact_state == 0); // and landed in the same pass set
    assert(c.iterations >= 2);
}

void leaving_the_floor_falls() {
    // Column 63 floor, column 64 empty.
    auto m = grid({int16_t(-32768), 0, int16_t(-32768), 0});
    auto on = session(m, {-256, -1, -256});
    for (int i = 0; i < 30; ++i) on.tick({});
    assert(on.player_contact().grounded == true && on.player_position().y == -1);
    assert(on.compat_player().bytes[0x74] == 0);
    auto off = session(m, {256, -1, -256});
    off.tick({});
    assert(off.compat_player().bytes[0x74] == 1);
    assert(off.compat_player().bytes[0x09] == 7 && off.compat_player().bytes[0x0A] == 4);
    const int y0 = off.player_position().y;
    for (int i = 0; i < 20; ++i) off.tick({});
    assert(off.player_position().y > y0 + 32); // gravity applies; no walking on air
}

void unsupported_response_holds() {
    // One positive cell list: a floor slab plus a diagonal (0x21) group ahead.
    auto m = grid({1, 1, 1, 1});
    m.idx[1] = {0, 0, 0, 0, 0};
    l3::RuntimeMapDescriptor d; d.default_contact_index = 0; m.descriptors.push_back(d);
    m.contact_lists.push_back({0, 2});
    m.contact_groups.push_back({-512, 512, -512, 512, 0, 1});      // floor
    m.contact_groups.push_back({-512, 512, -512, -300, 1, 1});     // diagonal family ahead (-z)
    m.contact_primitives.push_back({0, 1024, 3, 0});
    m.contact_primitives.push_back({-2048, 512, 0x21, 0});
    auto s = session(m, {0, 0, 0});
    s.tick({});
    l3::RuntimeInputFrame forward; forward.held_psx_mask = 0x0010; // heading 0 -> -z
    bool held = false; int16_t stop_z = 0;
    for (int i = 0; i < 60; ++i) {
        s.tick(forward);
        if (s.player_contact().status == l3::RuntimeContactStatus::Unsupported && !held) {
            held = true; stop_z = int16_t(s.player_position().z);
        }
    }
    assert(held);
    // Never enters the untranslated primitive's footprint (-300+32 expanded edge).
    assert(s.player_position().z >= -300 + 32 && s.player_position().z == stop_z);
    assert(s.player_position().y == 0);
}

void null_list_is_not_unsupported() {
    // Cell 1 has a descriptor whose selected list is null (cooked 0xFFFF);
    // cells 0/2 are packed floor. Standing on the floor near it stays supported.
    auto m = grid({int16_t(-32768), 1, int16_t(-32768), 1});
    m.idx[1] = {256, 0, -256, 0, 0};
    l3::RuntimeMapDescriptor d; m.descriptors.push_back(d); // both lists 0xFFFF
    L3PlayerCompatWork w{};
    put(w, 0x14, -30); put(w, 0x16, -1); put(w, 0x18, -256);
    put(w, 0x2C, -40); put(w, 0x2E, -1); put(w, 0x30, -256);
    const auto c = l3::runtime_player_contact_step(m, 0, w);
    assert(c.status == l3::RuntimeContactStatus::SourceBackedPartial);
    assert(c.unsupported_primitives == 0 && c.grounded == true);
}
} // namespace

int main() {
    corner_landing_restarts_scan();
    leaving_the_floor_falls();
    unsupported_response_holds();
    null_list_is_not_unsupported();
    std::puts("map edge PASS: corner landing, edge fall, unsupported hold, null list");
    return 0;
}
