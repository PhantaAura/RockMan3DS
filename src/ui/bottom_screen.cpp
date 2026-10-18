#include "legends3ds/bottom_screen.h"
#include <algorithm>

namespace l3 {
namespace {
void add(std::vector<BottomUiCommand>& out, BottomUiCommandKind kind, BottomUiRole role,
         BottomUiRect rect, uint16_t value = 0, uint16_t maximum = 0) {
    out.push_back(BottomUiCommand{kind, role, rect, value, maximum, {}, false});
}

void add_message(std::vector<BottomUiCommand>& out, BottomUiRole role, BottomUiRect rect,
                 MessageRef message) {
    BottomUiCommand cmd;
    cmd.kind = BottomUiCommandKind::RuntimeText;
    cmd.role = role;
    cmd.rect = rect;
    cmd.message = message;
    cmd.has_message = true;
    out.push_back(cmd);
}

void add_common_header(std::vector<BottomUiCommand>& out, uint16_t area_name_id) {
    add(out, BottomUiCommandKind::Panel, BottomUiRole::AreaName, {8, 8, 304, 28});
    add(out, BottomUiCommandKind::Label, BottomUiRole::AreaName, {16, 13, 288, 18}, area_name_id);
}

void add_health(std::vector<BottomUiCommand>& out, const BottomScreenModel& m, int y) {
    add(out, BottomUiCommandKind::Panel, BottomUiRole::HealthLabel, {8, static_cast<int16_t>(y), 304, 36});
    add(out, BottomUiCommandKind::Label, BottomUiRole::HealthLabel, {16, static_cast<int16_t>(y + 8), 42, 18});
    const uint16_t max_hp = m.hp_max ? m.hp_max : 1;
    add(out, BottomUiCommandKind::Gauge, BottomUiRole::HealthGauge,
        {62, static_cast<int16_t>(y + 9), 238, 16}, std::min(m.hp, max_hp), max_hp);
}
} // namespace

bool bottom_ui_rect_in_bounds(const BottomUiRect& r) {
    if (r.w < 0 || r.h < 0 || r.x < 0 || r.y < 0) return false;
    return int(r.x) + int(r.w) <= kBottomScreenWidth && int(r.y) + int(r.h) <= kBottomScreenHeight;
}

BottomScreenLayout build_bottom_screen_layout(const BottomScreenModel& m,
                                               const L3QolSettings& raw_settings) {
    L3QolSettings s = raw_settings;
    l3_qol_settings_sanitize(&s);

    BottomScreenLayout layout;
    auto& out = layout.commands;
    add(out, BottomUiCommandKind::Background, BottomUiRole::None, {0, 0, 320, 240});

    if (m.context == BottomScreenContext::Cinematic) {
        /* Cutscenes own the top screen. Keep the lower display quiet. Skip is only shown
         * when a proven completion contract exists for the active scene. */
        add(out, BottomUiCommandKind::CinematicShade, BottomUiRole::None, {0, 0, 320, 240});
        if (m.cutscene_skip_available) {
            add(out, BottomUiCommandKind::Panel, BottomUiRole::CutsceneSkip, {72, 202, 176, 28});
            add(out, BottomUiCommandKind::Label, BottomUiRole::CutsceneSkip, {80, 207, 160, 18}, m.cutscene_skip_hold, m.cutscene_skip_hold_required);
        }
        return layout;
    }
    if (m.context == BottomScreenContext::Loading) {
        add(out, BottomUiCommandKind::CinematicShade, BottomUiRole::None, {0, 0, 320, 240});
        add(out, BottomUiCommandKind::Icon, BottomUiRole::LoadingMark, {144, 104, 32, 32});
        return layout;
    }

    add_common_header(out, m.area_name_id);

    if (s.bottom_screen_mode == L3_BOTTOM_SCREEN_MINIMAL) {
        add_health(out, m, 196);
        if (m.lock_on_active) add(out, BottomUiCommandKind::Icon, BottomUiRole::LockOnActive, {280, 44, 24, 24});
        return layout;
    }

    if (m.context == BottomScreenContext::Gameplay &&
        s.bottom_screen_mode == L3_BOTTOM_SCREEN_STATUS) {
        /* Clean retail status view: no objective/log wall, only information that
         * naturally belongs beside the original HUD. */
        add(out, BottomUiCommandKind::Panel, BottomUiRole::BusterLabel, {8, 44, 144, 108});
        add(out, BottomUiCommandKind::Label, BottomUiRole::BusterLabel, {16, 52, 128, 18});
        add(out, BottomUiCommandKind::Icon,
            m.buster_enabled ? BottomUiRole::BusterReady : BottomUiRole::BusterDisabled,
            {56, 80, 48, 48});
        if (m.lock_on_active) {
            add(out, BottomUiCommandKind::Icon, BottomUiRole::LockOnActive, {120, 120, 24, 24});
        }
        add(out, BottomUiCommandKind::Panel, BottomUiRole::QuickMenu, {160, 44, 152, 108});
        add(out, BottomUiCommandKind::Label, BottomUiRole::QuickMenu, {168, 52, 136, 18});
        if (s.touch_shortcuts) {
            add(out, BottomUiCommandKind::TouchTarget, BottomUiRole::QuickMenu, {160, 44, 152, 108});
        }
        add_health(out, m, 160);
        return layout;
    }

    if (m.context == BottomScreenContext::Gameplay) {
        if (s.objective_panel && m.objective_available) {
            add(out, BottomUiCommandKind::Panel, BottomUiRole::ObjectiveHeader, {8, 44, 196, 108});
            add(out, BottomUiCommandKind::Label, BottomUiRole::ObjectiveHeader, {16, 52, 180, 18});
            add_message(out, BottomUiRole::ObjectiveBody, {16, 76, 180, 68}, m.objective);
        } else {
            add(out, BottomUiCommandKind::Panel, BottomUiRole::ObjectiveHeader, {8, 44, 196, 108});
        }

        add(out, BottomUiCommandKind::Panel, BottomUiRole::BusterLabel, {212, 44, 100, 108});
        add(out, BottomUiCommandKind::Label, BottomUiRole::BusterLabel, {220, 52, 84, 18});
        add(out, BottomUiCommandKind::Icon,
            m.buster_enabled ? BottomUiRole::BusterReady : BottomUiRole::BusterDisabled,
            {238, 80, 48, 48});
        if (m.lock_on_active) {
            add(out, BottomUiCommandKind::Icon, BottomUiRole::LockOnActive, {280, 120, 24, 24});
        }
        add_health(out, m, 160);

        if (s.dialogue_history && m.dialogue_history_count) {
            add(out, BottomUiCommandKind::Panel, BottomUiRole::DialogueHistory, {8, 204, 146, 28});
            add(out, BottomUiCommandKind::Label, BottomUiRole::DialogueHistory, {16, 209, 130, 18});
            if (s.touch_shortcuts) {
                add(out, BottomUiCommandKind::TouchTarget, BottomUiRole::DialogueHistory, {8, 204, 146, 28});
            }
        }
        add(out, BottomUiCommandKind::Panel, BottomUiRole::QuickMenu, {162, 204, 150, 28});
        add(out, BottomUiCommandKind::Label, BottomUiRole::QuickMenu, {170, 209, 134, 18});
        if (s.touch_shortcuts) {
            add(out, BottomUiCommandKind::TouchTarget, BottomUiRole::QuickMenu, {162, 204, 150, 28});
        }
        return layout;
    }

    if (m.context == BottomScreenContext::Dialogue) {
        /* Do not mirror the active dialogue and compete with the top screen. */
        add(out, BottomUiCommandKind::Panel, BottomUiRole::DialogueHistory, {8, 44, 304, 132});
        add(out, BottomUiCommandKind::Icon, BottomUiRole::DialogueHistory, {136, 68, 48, 48});
        add(out, BottomUiCommandKind::Label, BottomUiRole::DialogueHistory, {48, 128, 224, 22});
        if (s.dialogue_history && m.dialogue_history_count && s.touch_shortcuts) {
            add(out, BottomUiCommandKind::TouchTarget, BottomUiRole::DialogueHistory, {8, 44, 304, 132});
        }
        add_health(out, m, 196);
        return layout;
    }

    if (m.context == BottomScreenContext::Pause) {
        add(out, BottomUiCommandKind::Panel, BottomUiRole::QuickMenu, {8, 44, 304, 144});
        add(out, BottomUiCommandKind::Label, BottomUiRole::QuickMenu, {20, 54, 280, 22});
        if (s.dialogue_history && m.dialogue_history_count) {
            add(out, BottomUiCommandKind::Icon, BottomUiRole::DialogueHistory, {32, 92, 48, 48});
            if (s.touch_shortcuts) add(out, BottomUiCommandKind::TouchTarget, BottomUiRole::DialogueHistory, {20, 82, 72, 72});
        }
        if (m.save_available) {
            add(out, BottomUiCommandKind::Icon, BottomUiRole::SaveStatus, {112, 92, 48, 48});
        }
        add_health(out, m, 196);
        return layout;
    }

    if (m.context == BottomScreenContext::Save) {
        add(out, BottomUiCommandKind::Panel, BottomUiRole::SaveStatus, {8, 44, 304, 144});
        add(out, BottomUiCommandKind::Icon, BottomUiRole::SaveStatus, {136, 76, 48, 48});
        add(out, BottomUiCommandKind::Label, BottomUiRole::SaveStatus, {48, 136, 224, 22});
        add_health(out, m, 196);
        return layout;
    }

    return layout;
}

} // namespace l3
