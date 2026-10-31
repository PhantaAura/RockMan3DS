#pragma once
#include "legends3ds/dialogue_history.h"
#include "legends3ds/qol_settings.h"
#include <cstdint>
#include <vector>

namespace l3 {

constexpr int kBottomScreenWidth = 320;
constexpr int kBottomScreenHeight = 240;

enum class BottomScreenContext : uint8_t {
    Gameplay,
    Dialogue,
    Cinematic,
    Pause,
    Save,
    Loading,
};

enum class BottomUiCommandKind : uint8_t {
    Background,
    Panel,
    Gauge,
    Label,
    RuntimeText,
    Icon,
    TouchTarget,
    CinematicShade,
};

/* Player-facing semantic roles. There are intentionally no debug/telemetry roles. */
enum class BottomUiRole : uint8_t {
    None,
    AreaName,
    ObjectiveHeader,
    ObjectiveBody,
    HealthLabel,
    HealthGauge,
    BusterLabel,
    BusterReady,
    BusterDisabled,
    LockOnActive,
    DialogueHistory,
    QuickMenu,
    SaveStatus,
    LoadingMark,
    CutsceneSkip,
};

struct BottomUiRect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
};

struct BottomUiCommand {
    BottomUiCommandKind kind = BottomUiCommandKind::Panel;
    BottomUiRole role = BottomUiRole::None;
    BottomUiRect rect{};
    uint16_t value = 0;
    uint16_t maximum = 0;
    MessageRef message{};
    bool has_message = false;
};

struct BottomScreenModel {
    BottomScreenContext context = BottomScreenContext::Gameplay;
    uint16_t area_name_id = 0; /* resolved by the runtime localization/string layer */
    uint16_t hp = 0;
    uint16_t hp_max = 0;
    bool buster_enabled = false;
    bool lock_on_active = false;
    bool objective_available = false;
    MessageRef objective{};
    size_t dialogue_history_count = 0;
    bool save_available = false;
    bool cutscene_skip_available = false;
    uint16_t cutscene_skip_hold = 0;
    uint16_t cutscene_skip_hold_required = 60;
    int32_t money = 0; /* zenny, ROCK_NEO 0x800C1B2C */
    /* Last enemy hit, for the target life gauge (ROCK_NEO 0x8005CEFC). */
    bool target_visible = false;
    uint16_t target_hp = 0;
    uint16_t target_hp_max = 0;
};

struct BottomScreenLayout {
    std::vector<BottomUiCommand> commands;
    bool player_facing_only = true;
};

/*
 * Produces a backend-neutral 320x240 UI command stream. Citro2D/Citro3D can
 * skin these semantic roles later without gameplay code knowing about fonts,
 * textures, or debug instrumentation.
 */
BottomScreenLayout build_bottom_screen_layout(const BottomScreenModel& model,
                                               const L3QolSettings& settings);

bool bottom_ui_rect_in_bounds(const BottomUiRect& rect);

} // namespace l3
