#pragma once
#include "legends3ds/bottom_map.h"
#include "legends3ds/bottom_screen.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace l3 {

// A compact original HUD layout informed by the reference's three columns.
// The reference's images, map art, portrait and unrecovered stats are not used.
enum class MockupTab : uint8_t { Map, Items, Weapons, Options };
enum class MockupTouchPhase : uint8_t { Begin, Move, End };
enum class MockupUiCommand : uint8_t {
    PreviousTab, NextTab, ZoomMap, CenterPlayer, PanLeft, PanRight,
    PanUp, PanDown, ToggleVisible
};
struct MockupUiState {
    bool companion_visible = true;
    bool map_follow_player = false;
    MockupTab tab = MockupTab::Map;
    uint8_t map_zoom = 1; // Only 1, 2 and 4 are supported.
    int16_t map_pan_x = 0;
    int16_t map_pan_y = 0;
    bool map_dragging = false;
    int16_t last_touch_x = 0;
    int16_t last_touch_y = 0;
    size_t drag_area_index = 0;
    uint16_t drag_width = 0;
    uint16_t drag_height = 0;
};

struct MockupUiLayout {
    std::array<BottomUiRect, 4> tabs;
    BottomUiRect content;
    BottomUiRect status;
    BottomUiRect map_view;
    BottomUiRect map_zoom_button;
    BottomUiRect map_reset_button;
    BottomUiRect map_follow_button;
};

const MockupUiLayout& mockup_ui_layout();
const char* mockup_tab_label(MockupTab tab);
// Tabs deliberately remain selectable even when inventory is not connected.
// Touch coordinates use the normal landscape 320x240 screen coordinate space.
bool mockup_ui_touch(MockupUiState& state, int x, int y);
bool mockup_ui_rect_contains(const BottomUiRect& rect, int x, int y);

// Borrowed cell storage: rendering and player position lookup allocate nothing.
// The view must not survive a map/area change or a change to its owning vectors.
struct MockupMapView {
    const int16_t* packed_cells = nullptr;
    const BottomMapCellKind* classified_cells = nullptr;
    size_t cell_count = 0;
    size_t area_index = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    bool player_in_area = false;
    uint16_t player_x = 0;
    uint16_t player_z = 0;
    uint16_t player_yaw = 0;
};
MockupMapView make_mockup_map_view(const BottomMapModel& model);
MockupMapView make_mockup_map_view(const RuntimeMap& map, size_t area_index,
                                 int32_t player_world_x, int32_t player_world_z,
                                 uint16_t player_yaw = 0);
bool mockup_map_available(const MockupMapView& map);
BottomMapCellKind sample_mockup_map_cell(const MockupMapView& map,
                                         uint16_t x, uint16_t z,
                                         uint16_t width, uint16_t height);
// Fits the real area grid to the panel without stretching its aspect ratio.
BottomUiRect mockup_map_rect(const MockupMapView& map);
// Zoomed map projection in screen coordinates, with bounded pan. Drawing must
// clip this rectangle to map_view; cell data and top camera remain unchanged.
BottomUiRect mockup_map_projection(const MockupMapView& map, const MockupUiState& state);
bool mockup_map_source_at(const MockupMapView& map, const MockupUiState& state,
                         int screen_x, int screen_y, uint16_t& cell_x, uint16_t& cell_z);
bool mockup_map_set_zoom(MockupUiState& state, const MockupMapView& map, uint8_t zoom);
void mockup_map_reset(MockupUiState& state);
bool mockup_map_center_player(MockupUiState& state, const MockupMapView& map);
void mockup_map_update_follow(MockupUiState& state, const MockupMapView& map);
// Platform-neutral button equivalents. These only change companion-screen state.
bool mockup_ui_command(MockupUiState& state, const MockupMapView& map, MockupUiCommand command);
// Begin selects tabs, activates the two Options buttons, or starts a map drag.
// Move pans only a drag that began inside the map; End releases that gesture.
// No touch action mutates source map cells or top-screen camera/gameplay state.
bool mockup_ui_touch_event(MockupUiState& state, const MockupMapView& map,
                          MockupTouchPhase phase, int x, int y);

} // namespace l3
