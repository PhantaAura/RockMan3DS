// ST00 E0/06 tutorial message triggers (0x80114424) and the inline tutorial
// dialogue mode. Catalogs and message banks are authored, not retail data.
#include "legends3ds/runtime_session.h"
#include "legends3ds/source_tutorial_triggers.h"
#include <cassert>
#include <iostream>
#include <set>

namespace {
using namespace l3;

struct FakeHost final : TutorialTriggerHost {
    std::set<uint16_t> flags; std::vector<int8_t> messages; std::vector<uint32_t> voices;
    bool active = false; uint32_t pause = 0; uint16_t pause_mask = 0; uint8_t bd = 0;
    bool flag(uint16_t id) const override { return flags.count(id) != 0; }
    void set_flag(uint16_t id) override { flags.insert(id); }
    void clear_flag(uint16_t id) override { flags.erase(id); }
    void request_message(int8_t b) override { messages.push_back(b); active = true; }
    bool message_active() const override { return active; }
    void voice(uint32_t c) override { voices.push_back(c); }
    void pause_objects(uint16_t mask, uint32_t mode) override { pause_mask = mask; pause = mode; }
    uint8_t& player_bd() override { return bd; }
};
TutorialTriggerRecord rec(uint8_t type, int8_t msg, int8_t x, int8_t z, uint16_t w = 1, uint16_t d = 1) {
    TutorialTriggerRecord r; r.type = type; r.message = msg; r.cell_x = x; r.cell_z = z; r.width = w; r.depth = d; return r;
}
TutorialTriggerWorld at(int16_t x, int16_t z, int16_t y = -1, uint16_t heading = 0) {
    TutorialTriggerWorld w; w.area = 1; w.player_x = x; w.player_y = y; w.player_z = z; w.player_heading = heading; return w;
}

void controller() {
    TutorialTriggerCatalog c;
    c.areas[1] = {rec(0, 0, 5, -12), rec(1, 0, 5, -12), rec(2, 2, 5, -19), rec(4, 4, -7, -20, 1, 2),
                  rec(6, 6, -8, -10, 3, 1), rec(16, 11, 4, 23), rec(15, 10, -1, 29)};
    // L3TR round trip and CRC guard.
    const auto bytes = cook_l3tutorial_triggers(c);
    const auto loaded = load_l3tutorial_triggers(bytes);
    assert(loaded.areas[1].size() == 7 && loaded.areas[1][3].depth == 2 && loaded.areas[1][4].cell_x == -8);
    auto bad = bytes; bad.back() ^= 1; bool threw = false;
    try { (void)load_l3tutorial_triggers(bad); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    SourceTutorialTriggers t; t.bind(&loaded); FakeHost h;
    assert(t.tick(h, at(2822, -4833)).status == TutorialTriggerStatus::SourceBackedPartial && t.controller_state() == 1);
    // Start cell (5,-10): nothing fires.
    auto r = t.tick(h, at(2822, -4833));
    assert(h.messages.empty() && h.voices.empty() && !r.unsupported_type);
    // z -5643 >> 9 = -12: type 0 voices 0 and type 1 opens message 0 frozen.
    (void)t.tick(h, at(2822, -5643));
    assert(t.cell_z() == -12 && h.voices == std::vector<uint32_t>{0});
    assert(h.messages == std::vector<int8_t>{0} && h.bd == 0x40 && h.pause == 1 && h.pause_mask == 0x27E);
    // Still talking: type 0 keeps voicing every frame (state < 2), no completion.
    (void)t.tick(h, at(2822, -5643));
    assert(h.voices.size() == 2 && !h.flag(0x610) && h.bd == 0x40);
    // Message done: voice 5, flag 1552, unfreeze, resume.
    h.active = false; (void)t.tick(h, at(2822, -5643));
    assert(h.flag(0x610) && h.bd == 0 && h.pause == 2 && h.voices.back() == 5);
    const size_t voiced = h.voices.size();
    (void)t.tick(h, at(2822, -5643));
    assert(h.messages.size() == 1 && h.voices.size() == voiced); // both gated by 0x610 now
    // Type 2: message without freeze, then flag 0x612 and voice 6.
    (void)t.tick(h, at(2822, -9500));
    assert(h.messages.back() == 2 && h.bd == 0);
    h.active = false; (void)t.tick(h, at(2822, -9500));
    assert(h.flag(0x612) && h.voices.back() == 6);
    // Type 4: heading selects msg (facing) or msg+1.
    (void)t.tick(h, at(-3500, -10000, -1, 0x900));
    assert(h.messages.back() == 5 && h.bd == 0x40);
    h.active = false; (void)t.tick(h, at(-3500, -10000));
    assert(h.flag(0x614) && h.bd == 0);
    // Type 6 needs 0x607; then waits until no class-20 object is in its region.
    (void)t.tick(h, at(-3000, -4800));
    assert(h.messages.back() == 5);           // gated: 0x607 clear
    h.set_flag(0x607); (void)t.tick(h, at(-3000, -4800));
    assert(h.messages.back() == 6 && h.flag(0x615));
    h.active = false; auto world = at(-3000, -4800); world.class20_xz = {{{-3922, -4860}}};
    (void)t.tick(h, world);
    assert(!h.flag(0x61F));                   // a Reaverbot is still there
    world.class20_xz.clear(); (void)t.tick(h, world);
    assert(h.flag(0x61F) && h.messages.back() == 7);
    // Type 15 ends the tutorial: 0x622 set, every handler inert afterwards.
    h.active = false; (void)t.tick(h, at(-256, 15000));
    assert(h.messages.back() == 10);
    h.active = false; (void)t.tick(h, at(-256, 15000));
    assert(h.flag(0x61E) && h.flag(0x622));
    h.flags.erase(0x610); const size_t n = h.messages.size();
    (void)t.tick(h, at(2822, -5643));
    assert(h.messages.size() == n);           // 0x622 blocks type 1 even with 0x610 clear
    // Type 16 ignores 0x622: the box's 0x620 pulse in the region freezes,
    // waits 60 frames, then opens message 11; done -> 0x621, resume 0x276.
    h.flags.insert(0x620); (void)t.tick(h, at(-3000, -4800));
    assert(!h.flag(0x620) && h.messages.size() == n);   // outside the region: pulse consumed
    h.flags.insert(0x620); (void)t.tick(h, at(2304, 11904));
    assert(h.bd == 0x40 && h.pause_mask == 0x256 && !h.flag(0x620));
    for (int i = 0; i < 60; ++i) (void)t.tick(h, at(2304, 11904));
    assert(h.messages.size() == n);
    (void)t.tick(h, at(2304, 11904));
    assert(h.messages.back() == 11 && h.pause_mask == 0x20);
    h.active = false; (void)t.tick(h, at(2304, 11904));
    assert(h.flag(0x621) && h.bd == 0 && h.pause_mask == 0x276 && h.pause == 2);
}

L3QolSettings instant() { L3QolSettings s{}; l3_qol_settings_defaults(&s); s.text_speed = L3_TEXT_SPEED_INSTANT; return s; }
RuntimeMap tiny_map() {
    RuntimeMap m; RuntimeMapArea a{}; a.start_x = 63; a.start_z = 63; a.width = 2; a.height = 2; a.cell_count = 4;
    m.areas.push_back(a); m.cells.assign(4, int16_t(0x8000)); m.descriptors.resize(1); return m;
}
StageObjectRecord object(uint8_t cls, uint8_t sub) {
    StageObjectRecord r{}; r.raw[1] = cls; r.raw[2] = sub; r.object_class = cls; r.words_02_to_0a[0] = sub; return r;
}
MmlMessageFile bank() {
    using K = MmlMessageTokenKind;
    MmlMessageFile m; m.blocks.resize(2);
    // Block 0: "Hi" NEXT CLEAR "W" CALL(0,1) END. Block 1: "Q" SEL "Y" SEL "N" MULTI(80,01,FF) END.
    m.blocks[0].tokens = {{K::Glyph, 0x1C, {}}, {K::Glyph, 0x38, {}}, {K::Control, 0x9F, {}}, {K::Control, 0x87, {0, 0}},
        {K::Glyph, 0x2C, {}}, {K::Control, 0x99, {0, 1}}, {K::Control, 0xA9, {}}, {K::Control, 0x84, {2, 0}}};
    m.blocks[1].tokens = {{K::Glyph, 0x25, {}}, {K::Control, 0x96, {1, 0}}, {K::Glyph, 0x2E, {}},
        {K::Control, 0x96, {1, 0x11}}, {K::Glyph, 0x22, {}}, {K::Control, 0x97, {0x80, 1, 0xFF}}, {K::Control, 0xA9, {}}};
    return m;
}
void dialogue() {
    TutorialRuntimeArea area;
    area.flow.objects.records = {object(0xE0, 6)};
    area.stage = build_fresh_stage_runtime_area(area.flow.objects, area.ebd);
    RuntimeSession s(tiny_map(), std::move(area), {bank()}, instant());
    s.start_source_tutorial_dialogue({0, 0});
    assert(s.dialogue().active && !s.dialogue().gates_gameplay);
    RuntimeInputFrame none{}; RuntimeInputFrame a{}; a.advance_dialogue = true;
    RuntimeInputFrame right{}; right.dialogue_right = true;
    s.tick(none);
    assert(s.dialogue().waiting_for_advance && s.dialogue().visible_glyphs.size() == 2);
    s.tick(a);  // clear, "W", call block 1, options, MULTI
    assert(s.dialogue().waiting_for_choice && s.dialogue().ref.block == 1 && s.dialogue().option_glyph_at.size() == 2);
    // Yes (choice 0) restarts block 1: the instructions repeat.
    s.tick(a); s.tick(none);
    assert(s.dialogue().waiting_for_choice && s.dialogue().ref.block == 1);
    // No (choice 1 = 0xFF) continues to END, returns to block 0 and ends.
    s.tick(right); assert(s.dialogue().choice == 1);
    s.tick(a);
    assert(s.dialogue().active && s.dialogue().closing);   // CLOSE holds 2 frames
    s.tick(none); assert(s.dialogue().active);
    s.tick(none); assert(!s.dialogue().active);
    // The bound catalog activates only because the area holds an E0/06.
    s.bind_tutorial_triggers(std::make_shared<const TutorialTriggerCatalog>());
    assert(s.tutorial_triggers_active());
}
} // namespace

int main() {
    controller();
    dialogue();
    std::cout << "PASS: L3TR, E0/06 handlers 0/1/2/4/6/15/16, freeze/pause/flags, tutorial dialogue CALL/MULTI\n";
}
