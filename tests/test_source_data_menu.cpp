// Data's menu message ops (docs/research_01516/DATA_MENU.md): 0xAC writing main phase 6 (Save),
// 0xB2 recharge, 0xB5 branch and 0xBF jump, on an authored bank shaped like ST03 blocks 0xDC..0xE2.
#include "legends3ds/runtime_session.h"
#include "legends3ds/source_tutorial_triggers.h"
#include <cassert>
#include <iostream>

namespace {
using namespace l3;
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
    MmlMessageFile m; m.blocks.resize(4);
    // 0 (menu): rows Recharge -> 1, Save -> 2, Cancel -> 0xFF; then END.
    m.blocks[0].tokens = {{K::Glyph, 0x25, {}}, {K::Control, 0x96, {0, 0}}, {K::Glyph, 0x26, {}}, {K::Control, 0x96, {0, 0}},
        {K::Glyph, 0x27, {}}, {K::Control, 0x96, {0, 0}}, {K::Control, 0x97, {0x80, 1, 2, 0xFF}}, {K::Control, 0xA9, {}}};
    // 1 (Recharge): CALL close-less text, 0xB5 {ff, 3, 3, 3} -> block 3, which is life only.
    m.blocks[1].tokens = {{K::Control, 0xB5, {0xFF, 3, 3, 3}}, {K::Glyph, 0x30, {}}, {K::Control, 0x9A, {0x77, 0x00}}};
    // 2 (Save): AC 08 06, AC 09 00, AC 0A 00, AC 0B 00, 8A 01 00, HALT.
    m.blocks[2].tokens = {{K::Control, 0xAC, {8, 6}}, {K::Control, 0xAC, {9, 0}}, {K::Control, 0xAC, {10, 0}},
        {K::Control, 0xAC, {11, 0}}, {K::Control, 0x95, {}}};
    // 3 (life only): B2 "text" NEXT BF 00 00 (back to the menu).
    m.blocks[3].tokens = {{K::Control, 0xB2, {}}, {K::Glyph, 0x20, {}}, {K::Control, 0x9A, {0x78, 0x00}},
        {K::Control, 0x9F, {}}, {K::Control, 0xBF, {0, 0}}};
    return m;
}
RuntimeSession make(uint16_t hp) {
    TutorialRuntimeArea area;
    area.flow.objects.records = {object(0xE0, 6)};
    area.stage = build_fresh_stage_runtime_area(area.flow.objects, area.ebd);
    RuntimeSession s(tiny_map(), std::move(area), {bank()}, instant());
    s.reset_player({}, 0, hp, 80);
    return s;
}
} // namespace

int main() {
    RuntimeInputFrame none{}, a{}, down{};
    a.advance_dialogue = true; down.dialogue_right = true;
    {   // Recharge: life +2 per update to the maximum, then the text, NEXT, the jump back to the menu.
        auto s = make(40);
        s.start_source_tutorial_dialogue({0, 0});
        s.tick(none); assert(s.dialogue().waiting_for_choice);
        s.tick(a);                              // Recharge -> block 1 -> 0xB5 -> block 3 -> 0xB2 pending
        for (int i = 0; i < 40 && s.bottom_screen_model().hp < 80; ++i) s.tick(none);
        assert(s.bottom_screen_model().hp == 80);
        assert(s.story_flag(0x77) == false);    // 0xB5 jumped over block 1's tail
        s.tick(none); assert(s.story_flag(0x78));
        s.tick(none); assert(s.dialogue().waiting_for_advance);
        s.tick(a); s.tick(none);                // 0xBF back to block 0: the menu again
        assert(s.dialogue().waiting_for_choice && s.dialogue().ref.block == 0);
    }
    {   // Save: row 1 -> 0xAC 08 06 runs the writer once and the talk window closes; phase 6 lasts a few frames.
        auto s = make(80);
        int writes = 0; RuntimeSaveState last{};
        s.set_save_writer([&](const RuntimeSaveState& st) { ++writes; last = st; return true; });
        s.start_source_tutorial_dialogue({0, 0});
        s.tick(none); s.tick(down); s.tick(a);
        assert(writes == 1 && last.hp == 80 && last.hp_max == 80);
        assert(s.save_screen_active() && !s.dialogue().active && s.save_result() == 1);
        for (int i = 0; i < 4; ++i) s.tick(none);
        assert(!s.save_screen_active() && writes == 1);
    }
    {   // A failing writer is reported.
        auto s = make(80);
        s.set_save_writer([](const RuntimeSaveState&) { return false; });
        s.start_source_tutorial_dialogue({0, 0});
        s.tick(none); s.tick(down); s.tick(a);
        assert(s.save_result() == -1);
    }
    std::cout << "source_data_menu: ok\n";
    return 0;
}
