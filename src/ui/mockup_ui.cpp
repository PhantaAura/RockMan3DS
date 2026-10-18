#include "legends3ds/mockup_ui.h"
#include <algorithm>

namespace l3 {
namespace {
constexpr MockupUiLayout kLayout{
    {{{4, 16, 66, 48}, {4, 68, 66, 48}, {4, 120, 66, 48}, {4, 172, 66, 48}}},
    {74, 16, 154, 204}, {232, 16, 84, 204}, {80, 38, 142, 168},
    {80, 42, 142, 32}, {80, 82, 142, 32}, {80,118,142,24}
};
}
const MockupUiLayout& mockup_ui_layout() { return kLayout; }
const char* mockup_tab_label(MockupTab tab) {
    switch (tab) {
    case MockupTab::Map: return "MAP";
    case MockupTab::Items: return "ITEMS";
    case MockupTab::Weapons: return "WEAPONS";
    case MockupTab::Options: return "OPTIONS";
    }
    return "MAP";
}
bool mockup_ui_rect_contains(const BottomUiRect& r, int x, int y) {
    return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y &&
           x < int(r.x) + r.w && y < int(r.y) + r.h;
}
bool mockup_ui_touch(MockupUiState& state, int x, int y) {
    for (size_t i = 0; i < kLayout.tabs.size(); ++i) {
        if (mockup_ui_rect_contains(kLayout.tabs[i], x, y)) {
            state.tab = static_cast<MockupTab>(i);
            state.map_dragging = false;
            return true;
        }
    }
    return false;
}
MockupMapView make_mockup_map_view(const BottomMapModel& model) {
    MockupMapView out;
    out.classified_cells = model.cells.data();
    out.cell_count = model.cells.size();
    out.area_index = model.area_index;
    out.width = model.width; out.height = model.height;
    out.player_in_area = model.player_in_area;
    out.player_x = model.player_x; out.player_z = model.player_z;
    return out;
}
MockupMapView make_mockup_map_view(const RuntimeMap& map, size_t area_index,
                                 int32_t player_world_x, int32_t player_world_z,
                                 uint16_t player_yaw) {
    MockupMapView out;
    out.area_index = area_index;
    out.player_yaw = player_yaw & 0x0FFFu;
    if (area_index >= map.areas.size()) return out;
    const auto& area = map.areas[area_index];
    out.width = area.width; out.height = area.height;
    if (area.cell_first < map.cells.size()) {
        out.packed_cells = map.cells.data() + area.cell_first;
        out.cell_count = std::min<size_t>(size_t(area.width) * area.height,
                                        map.cells.size() - area.cell_first);
    }
    const auto player = runtime_map_find_cell(map, area_index, player_world_x, player_world_z);
    if (player) {
        const size_t local = size_t(player->cell_index) - area.cell_first;
        if (area.width && local < size_t(area.width) * area.height) {
            out.player_in_area = true;
            out.player_x = static_cast<uint16_t>(local % area.width);
            out.player_z = static_cast<uint16_t>(local / area.width);
        }
    }
    return out;
}
bool mockup_map_available(const MockupMapView& map) {
    return map.width != 0 && map.height != 0 &&
           (map.packed_cells || map.classified_cells) &&
           map.cell_count >= size_t(map.width) * map.height;
}
BottomMapCellKind sample_mockup_map_cell(const MockupMapView& map,
                                         uint16_t x, uint16_t z,
                                         uint16_t width, uint16_t height) {
    if (!mockup_map_available(map) || !width || !height || x >= width || z >= height)
        return BottomMapCellKind::Empty;
    const size_t sx = size_t(x) * map.width / width;
    const size_t sz = size_t(z) * map.height / height;
    const size_t i = sz * map.width + sx;
    if (map.classified_cells) return map.classified_cells[i];
    return map.packed_cells[i] > 0 ? BottomMapCellKind::Geometry :
           (map.packed_cells[i] < 0 ? BottomMapCellKind::Special : BottomMapCellKind::Empty);
}
BottomUiRect mockup_map_rect(const MockupMapView& map) {
    if (!mockup_map_available(map)) return {};
    const auto& v = kLayout.map_view;
    int w = v.w, h = v.h;
    if (uint32_t(map.width) * v.h > uint32_t(map.height) * v.w)
        h = std::max(1, int(uint32_t(v.w) * map.height / map.width));
    else
        w = std::max(1, int(uint32_t(v.h) * map.width / map.height));
    return {static_cast<int16_t>(v.x + (v.w-w)/2),
            static_cast<int16_t>(v.y + (v.h-h)/2),
            static_cast<int16_t>(w), static_cast<int16_t>(h)};
}
namespace {
uint8_t valid_zoom(const MockupUiState& state) {
    return state.map_zoom==2 || state.map_zoom==4 ? state.map_zoom : 1;
}
int clamped_pan(int pan, int size, int viewport_size) {
    // When the scaled map is smaller than the viewport it stays centered.
    // Larger maps always cover the viewport in this axis, even at the limits.
    const int limit=std::max(0,(size-viewport_size)/2);
    return std::clamp(pan,-limit,limit);
}
void clamp_pan(MockupUiState& state, const MockupMapView& map) {
    const auto fit=mockup_map_rect(map);
    const int zoom=valid_zoom(state);
    state.map_pan_x=static_cast<int16_t>(clamped_pan(state.map_pan_x,fit.w*zoom,kLayout.map_view.w));
    state.map_pan_y=static_cast<int16_t>(clamped_pan(state.map_pan_y,fit.h*zoom,kLayout.map_view.h));
}
}
BottomUiRect mockup_map_projection(const MockupMapView& map, const MockupUiState& state) {
    const auto fit=mockup_map_rect(map);
    if(!fit.w || !fit.h) return {};
    const auto& v=kLayout.map_view;
    const int zoom=valid_zoom(state), w=fit.w*zoom, h=fit.h*zoom;
    return {static_cast<int16_t>(v.x+(v.w-w)/2+clamped_pan(state.map_pan_x,w,v.w)),
            static_cast<int16_t>(v.y+(v.h-h)/2+clamped_pan(state.map_pan_y,h,v.h)),
            static_cast<int16_t>(w),static_cast<int16_t>(h)};
}
bool mockup_map_source_at(const MockupMapView& map, const MockupUiState& state,
                         int screen_x, int screen_y, uint16_t& cell_x, uint16_t& cell_z) {
    const auto projection=mockup_map_projection(map,state);
    if(!mockup_ui_rect_contains(kLayout.map_view,screen_x,screen_y) ||
       !mockup_ui_rect_contains(projection,screen_x,screen_y)) return false;
    cell_x=static_cast<uint16_t>(uint32_t(screen_x-projection.x)*map.width/projection.w);
    cell_z=static_cast<uint16_t>(uint32_t(screen_y-projection.y)*map.height/projection.h);
    return true;
}
bool mockup_map_set_zoom(MockupUiState& state, const MockupMapView& map, uint8_t zoom) {
    if((zoom!=1 && zoom!=2 && zoom!=4) || !mockup_map_available(map)) return false;
    const uint8_t old_zoom=valid_zoom(state);
    state.map_pan_x=static_cast<int16_t>(std::clamp(int(state.map_pan_x)*zoom/old_zoom,-1024,1024));
    state.map_pan_y=static_cast<int16_t>(std::clamp(int(state.map_pan_y)*zoom/old_zoom,-1024,1024));
    state.map_zoom=zoom;
    state.map_dragging=false;
    clamp_pan(state,map);
    return true;
}
void mockup_map_reset(MockupUiState& state) {
    state.map_zoom=1; state.map_pan_x=0; state.map_pan_y=0;
    state.map_dragging=false;
    state.map_follow_player=false;
}
bool mockup_map_center_player(MockupUiState& state, const MockupMapView& map) {
    if(!mockup_map_available(map) || !map.player_in_area ||
       map.player_x>=map.width || map.player_z>=map.height) return false;
    const auto fit=mockup_map_rect(map);
    const int w=fit.w*valid_zoom(state), h=fit.h*valid_zoom(state);
    state.map_pan_x=static_cast<int16_t>(w/2-int((uint32_t(map.player_x)*2+1)*w/(2u*map.width)));
    state.map_pan_y=static_cast<int16_t>(h/2-int((uint32_t(map.player_z)*2+1)*h/(2u*map.height)));
    clamp_pan(state,map);
    state.map_dragging=false;
    return true;
}
void mockup_map_update_follow(MockupUiState& state, const MockupMapView& map) {
    if(state.map_follow_player) (void)mockup_map_center_player(state,map);
}
bool mockup_ui_command(MockupUiState& state, const MockupMapView& map, MockupUiCommand command) {
    if(command==MockupUiCommand::ToggleVisible) { state.companion_visible=!state.companion_visible;state.map_dragging=false;return true; }
    state.companion_visible=true; state.map_dragging=false;
    if(command==MockupUiCommand::PreviousTab || command==MockupUiCommand::NextTab) {
        const unsigned tab=static_cast<unsigned>(state.tab);
        state.tab=static_cast<MockupTab>((tab+(command==MockupUiCommand::NextTab?1:3))%4);
        return true;
    }
    if(command==MockupUiCommand::ZoomMap) {
        const uint8_t zoom=valid_zoom(state);
        const bool result=mockup_map_set_zoom(state,map,zoom==1?2:zoom==2?4:1);
        mockup_map_update_follow(state,map); return result;
    }
    if(command==MockupUiCommand::CenterPlayer) {
        state.map_follow_player=true; return mockup_map_center_player(state,map);
    }
    if(!mockup_map_available(map)) return false;
    int dx=0,dy=0;
    switch(command) {
    case MockupUiCommand::PanLeft: dx=12; break;
    case MockupUiCommand::PanRight: dx=-12; break;
    case MockupUiCommand::PanUp: dy=12; break;
    case MockupUiCommand::PanDown: dy=-12; break;
    default: return false;
    }
    state.map_follow_player=false;
    state.map_pan_x=static_cast<int16_t>(std::clamp(int(state.map_pan_x)+dx,-1024,1024));
    state.map_pan_y=static_cast<int16_t>(std::clamp(int(state.map_pan_y)+dy,-1024,1024));
    clamp_pan(state,map); return true;
}
bool mockup_ui_touch_event(MockupUiState& state, const MockupMapView& map,
                          MockupTouchPhase phase, int x, int y) {
    if(phase==MockupTouchPhase::End) {
        const bool handled=state.map_dragging;
        state.map_dragging=false;
        return handled;
    }
    if(phase==MockupTouchPhase::Begin) {
        if(!state.companion_visible) { state.companion_visible=true;return true; }
        state.map_dragging=false;
        if(mockup_ui_touch(state,x,y)) return true;
        if(!mockup_map_available(map)) return false;
        if(state.tab==MockupTab::Options) {
            if(mockup_ui_rect_contains(kLayout.map_zoom_button,x,y)) {
                const uint8_t current=valid_zoom(state);
                return mockup_map_set_zoom(state,map,current==1?2:current==2?4:1);
            }
            if(mockup_ui_rect_contains(kLayout.map_reset_button,x,y)) {
                mockup_map_reset(state);
                return true;
            }
            if(mockup_ui_rect_contains(kLayout.map_follow_button,x,y)) {
                state.map_follow_player=!state.map_follow_player;
                mockup_map_update_follow(state,map);return true;
            }
        }
        if(state.tab!=MockupTab::Map || !mockup_ui_rect_contains(kLayout.map_view,x,y)) return false;
        clamp_pan(state,map);
        state.map_dragging=true;
        state.map_follow_player=false;
        state.last_touch_x=static_cast<int16_t>(x); state.last_touch_y=static_cast<int16_t>(y);
        state.drag_area_index=map.area_index;
        state.drag_width=map.width; state.drag_height=map.height;
        return true;
    }
    if(phase!=MockupTouchPhase::Move || !state.map_dragging) return false;
    if(state.tab!=MockupTab::Map || !mockup_map_available(map) ||
       state.drag_area_index!=map.area_index || state.drag_width!=map.width || state.drag_height!=map.height) {
        state.map_dragging=false;
        return false;
    }
    x=std::clamp(x,0,319); y=std::clamp(y,0,239);
    state.map_pan_x=static_cast<int16_t>(std::clamp(int(state.map_pan_x)+x-state.last_touch_x,-1024,1024));
    state.map_pan_y=static_cast<int16_t>(std::clamp(int(state.map_pan_y)+y-state.last_touch_y,-1024,1024));
    clamp_pan(state,map);
    state.last_touch_x=static_cast<int16_t>(x); state.last_touch_y=static_cast<int16_t>(y);
    return true;
}
} // namespace l3
