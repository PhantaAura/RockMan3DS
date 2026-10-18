#include "legends3ds/qol_settings.h"

static uint8_t bool_u8(uint8_t value) { return value ? 1u : 0u; }

void l3_qol_settings_defaults(L3QolSettings *out) {
    if (!out) return;
    out->version = L3_QOL_SETTINGS_VERSION;
    out->control_preset = L3_CONTROL_CLASSIC;
    out->camera_behavior = L3_CAMERA_CLASSIC;
    out->lock_on_mode = L3_LOCK_ON_HOLD;
    out->text_speed = L3_TEXT_SPEED_ORIGINAL;
    out->bottom_screen_mode = L3_BOTTOM_SCREEN_SMART;
    out->dialogue_history = 1;
    out->objective_panel = 1;
    out->quick_restart_confirm = 1;
    out->touch_shortcuts = 1;
    out->invert_camera_x = 0;
    out->invert_camera_y = 0;
    out->camera_sensitivity_percent = 100;
}

void l3_qol_settings_sanitize(L3QolSettings *s) {
    if (!s) return;
    if (s->version != L3_QOL_SETTINGS_VERSION) {
        l3_qol_settings_defaults(s);
        return;
    }
    if (s->control_preset > L3_CONTROL_MODERN) s->control_preset = L3_CONTROL_CLASSIC;
    if (s->camera_behavior > L3_CAMERA_SMOOTH) s->camera_behavior = L3_CAMERA_CLASSIC;
    if (s->lock_on_mode > L3_LOCK_ON_TOGGLE) s->lock_on_mode = L3_LOCK_ON_HOLD;
    if (s->text_speed > L3_TEXT_SPEED_INSTANT) s->text_speed = L3_TEXT_SPEED_ORIGINAL;
    if (s->bottom_screen_mode > L3_BOTTOM_SCREEN_MINIMAL) s->bottom_screen_mode = L3_BOTTOM_SCREEN_SMART;
    s->dialogue_history = bool_u8(s->dialogue_history);
    s->objective_panel = bool_u8(s->objective_panel);
    s->quick_restart_confirm = bool_u8(s->quick_restart_confirm);
    s->touch_shortcuts = bool_u8(s->touch_shortcuts);
    s->invert_camera_x = bool_u8(s->invert_camera_x);
    s->invert_camera_y = bool_u8(s->invert_camera_y);
    if (s->camera_sensitivity_percent < 25) s->camera_sensitivity_percent = 25;
    if (s->camera_sensitivity_percent > 200) s->camera_sensitivity_percent = 200;
}

int l3_qol_settings_valid(const L3QolSettings *s) {
    if (!s || s->version != L3_QOL_SETTINGS_VERSION) return 0;
    if (s->control_preset > L3_CONTROL_MODERN || s->camera_behavior > L3_CAMERA_SMOOTH) return 0;
    if (s->lock_on_mode > L3_LOCK_ON_TOGGLE || s->text_speed > L3_TEXT_SPEED_INSTANT) return 0;
    if (s->bottom_screen_mode > L3_BOTTOM_SCREEN_MINIMAL) return 0;
    if (s->dialogue_history > 1 || s->objective_panel > 1 || s->quick_restart_confirm > 1 || s->touch_shortcuts > 1) return 0;
    if (s->invert_camera_x > 1 || s->invert_camera_y > 1) return 0;
    if (s->camera_sensitivity_percent < 25 || s->camera_sensitivity_percent > 200) return 0;
    return 1;
}
