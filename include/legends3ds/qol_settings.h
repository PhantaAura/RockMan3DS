#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Player-facing options are deliberately kept separate from compatibility
 * state. Classic remains the fidelity default; enhanced choices may change
 * presentation/input ergonomics but must not silently alter game rules.
 */
typedef enum L3ControlPreset {
    L3_CONTROL_CLASSIC = 0,
    L3_CONTROL_MODERN = 1,
} L3ControlPreset;

typedef enum L3CameraBehavior {
    L3_CAMERA_CLASSIC = 0,
    L3_CAMERA_SMOOTH = 1,
} L3CameraBehavior;

typedef enum L3LockOnMode {
    L3_LOCK_ON_HOLD = 0,
    L3_LOCK_ON_TOGGLE = 1,
} L3LockOnMode;

typedef enum L3TextSpeed {
    L3_TEXT_SPEED_ORIGINAL = 0,
    L3_TEXT_SPEED_FAST = 1,
    L3_TEXT_SPEED_INSTANT = 2,
} L3TextSpeed;

typedef enum L3BottomScreenMode {
    L3_BOTTOM_SCREEN_SMART = 0,
    L3_BOTTOM_SCREEN_STATUS = 1,
    L3_BOTTOM_SCREEN_MINIMAL = 2,
} L3BottomScreenMode;

enum { L3_QOL_SETTINGS_VERSION = 1 };

typedef struct L3QolSettings {
    uint8_t version;
    uint8_t control_preset;
    uint8_t camera_behavior;
    uint8_t lock_on_mode;
    uint8_t text_speed;
    uint8_t bottom_screen_mode;
    uint8_t dialogue_history;
    uint8_t objective_panel;
    uint8_t quick_restart_confirm;
    uint8_t touch_shortcuts;
    uint8_t invert_camera_x;
    uint8_t invert_camera_y;
    uint16_t camera_sensitivity_percent;
} L3QolSettings;

/* Fidelity-preserving defaults: original behavior first, 3DS conveniences opt-in. */
void l3_qol_settings_defaults(L3QolSettings *out);

/* Normalizes values loaded from a future save/options file without trusting it. */
void l3_qol_settings_sanitize(L3QolSettings *settings);

/* Returns non-zero only when every field is already within the supported range. */
int l3_qol_settings_valid(const L3QolSettings *settings);

#ifdef __cplusplus
}
#endif
