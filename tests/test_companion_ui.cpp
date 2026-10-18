#include "legends3ds/mockup_ui.h"
#include "../platform/3ds/source/mockup_hud.h"
#include <array>
#include <cassert>
#include <cstdint>

namespace {
constexpr size_t kBytes=320u*240u*3u;
constexpr size_t kGuard=16;
using Buffer=std::array<unsigned char,kBytes+2*kGuard>;
size_t pixel_offset(int x,int y){return (size_t(x)*240u+size_t(239-y))*3u;}
bool player_pixel(const unsigned char* p,int x,int y){const size_t i=pixel_offset(x,y);return p[i]==86&&p[i+1]==221&&p[i+2]==255;}
void guard(const Buffer& b){for(size_t i=0;i<kGuard;++i){assert(b[i]==0xA5);assert(b[kGuard+kBytes+i]==0xA5);}}
void follow_and_commands(l3::MockupMapView map) {
    l3::MockupUiState ui;
    const auto& layout=l3::mockup_ui_layout();
    // 4x view is wide enough to center interior cells instead of hitting bounds.
    assert(l3::mockup_map_set_zoom(ui,map,4));
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::CenterPlayer));
    assert(ui.map_follow_player);
    auto projection=l3::mockup_map_projection(map,ui);
    const int marker_x=projection.x+int((uint32_t(map.player_x)*2+1)*projection.w/(uint32_t(map.width)*2));
    const int marker_y=projection.y+int((uint32_t(map.player_z)*2+1)*projection.h/(uint32_t(map.height)*2));
    assert(marker_x==layout.map_view.x+layout.map_view.w/2);
    assert(marker_y==layout.map_view.y+layout.map_view.h/2);
    const int old_pan=ui.map_pan_x;
    ++map.player_x;
    l3::mockup_map_update_follow(ui,map);
    assert(ui.map_pan_x<old_pan);
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::PanLeft));
    assert(!ui.map_follow_player);
    const int stopped_pan=ui.map_pan_x;
    ++map.player_x;
    l3::mockup_map_update_follow(ui,map);
    assert(ui.map_pan_x==stopped_pan);
    ui.tab=l3::MockupTab::Options;
    assert(l3::mockup_ui_touch_event(ui,map,l3::MockupTouchPhase::Begin,layout.map_follow_button.x+1,layout.map_follow_button.y+1));
    assert(ui.map_follow_player);
    assert(l3::mockup_ui_touch_event(ui,map,l3::MockupTouchPhase::Begin,layout.map_follow_button.x+1,layout.map_follow_button.y+1));
    assert(!ui.map_follow_player);
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::NextTab));
    assert(ui.tab==l3::MockupTab::Map);
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::PreviousTab));
    assert(ui.tab==l3::MockupTab::Options);
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::ToggleVisible));
    assert(!ui.companion_visible && !ui.map_dragging);
    assert(l3::mockup_ui_touch_event(ui,map,l3::MockupTouchPhase::Begin,100,100));
    assert(ui.companion_visible);
    const auto source=map.packed_cells;
    const auto cell=source[0];
    assert(l3::mockup_ui_command(ui,map,l3::MockupUiCommand::ZoomMap));
    assert(ui.map_zoom==1);
    assert(source==map.packed_cells && cell==source[0]);
    auto unavailable=map;unavailable.cell_count=0;
    assert(!l3::mockup_ui_command(ui,unavailable,l3::MockupUiCommand::ZoomMap));
    assert(!l3::mockup_map_center_player(ui,unavailable));
    assert(l3::mockup_ui_command(ui,unavailable,l3::MockupUiCommand::NextTab));
}
}

int main() {
    std::array<int16_t,100> cells{};
    cells.fill(1);
    l3::MockupMapView map;
    map.packed_cells=cells.data();map.cell_count=cells.size();map.width=10;map.height=10;
    map.player_in_area=true;map.player_x=4;map.player_z=4;
    follow_and_commands(map);
    l3::MockupUiState ui;
    l3::BottomScreenModel status;
    status.hp=3;status.hp_max=4;status.buster_enabled=true;
    Buffer first{},second{},third{},oracle{};
    for(auto* b:{&first,&second,&third,&oracle}) b->fill(0xA5);
    auto* p=first.data()+kGuard;auto* q=second.data()+kGuard;auto* r=third.data()+kGuard;
    mockup_hud_init();
    assert(!mockup_hud_raster_cached(nullptr,ui,status,map));
    assert(mockup_hud_raster_cached(p,ui,status,map));
    assert(mockup_hud_raster_cached(q,ui,status,map));
    assert(first==second);
    for(int i=0;i<60;++i) {
        assert(!mockup_hud_raster_cached(p,ui,status,map));
        assert(!mockup_hud_raster_cached(q,ui,status,map));
    }
    // Draw/flush dirty state in both alternating buffers, not just the first.
    status.hp=2;
    assert(mockup_hud_raster_cached(p,ui,status,map));
    assert(mockup_hud_raster_cached(q,ui,status,map));
    assert(first==second);
    assert(!mockup_hud_raster_cached(p,ui,status,map));
    ui.last_touch_x=100;ui.last_touch_y=100;ui.map_dragging=true;
    MMLRuntimeDiag diag{};diag.frame_us=999999;
    assert(!mockup_hud_raster_cached(p,ui,status,map,&diag));
    // A new area identity or borrowed storage identity must redraw.
    ++map.area_index;
    assert(mockup_hud_raster_cached(p,ui,status,map));
    std::array<int16_t,100> replacement=cells;
    map.packed_cells=replacement.data();
    assert(mockup_hud_raster_cached(p,ui,status,map));
    // In-place map edits require explicit invalidation, which dirties both.
    replacement.fill(-1);
    mockup_hud_invalidate();
    assert(mockup_hud_raster_cached(p,ui,status,map));
    assert(mockup_hud_raster_cached(q,ui,status,map));
    mockup_hud_raster(oracle.data()+kGuard,ui,status,map);
    assert(first==oracle && second==oracle);
    // Map cardinal directions follow the runtime's -sin(yaw),-cos(yaw) travel.
    const auto fit=l3::mockup_map_projection(map,ui);
    const int x=fit.x+int((uint32_t(map.player_x)*2+1)*fit.w/(uint32_t(map.width)*2));
    const int y=fit.y+int((uint32_t(map.player_z)*2+1)*fit.h/(uint32_t(map.height)*2));
    constexpr std::array<int,4> dx{{0,-1,0,1}},dy{{-1,0,1,0}};
    for(unsigned direction=0;direction<4;++direction) {
        map.player_yaw=static_cast<uint16_t>(direction*1024);
        (void)mockup_hud_raster_cached(p,ui,status,map);
        assert(player_pixel(p,x+dx[direction]*4,y+dy[direction]*4));
        assert(!player_pixel(p,x-dx[direction]*4,y-dy[direction]*4));
    }
    assert(!mockup_hud_raster_cached(p,ui,status,map));
    map.player_yaw+=1; // Same 8-direction raster marker: no unnecessary redraw.
    assert(!mockup_hud_raster_cached(p,ui,status,map));
    // Hidden pages and unavailable inventory don't redraw for moving map data.
    ui.companion_visible=false;
    assert(mockup_hud_raster_cached(p,ui,status,map));
    ++map.player_x;--status.hp;
    assert(!mockup_hud_raster_cached(p,ui,status,map));
    ui.companion_visible=true;ui.tab=l3::MockupTab::Items;
    assert(mockup_hud_raster_cached(p,ui,status,map));
    ++map.player_x;
    assert(!mockup_hud_raster_cached(p,ui,status,map));
    status.lock_on_active=true;
    assert(mockup_hud_raster_cached(p,ui,status,map));
    // Third unexpected framebuffer evicts bounded storage; it never needs a
    // heap buffer and regenerates the correct pixels when reused.
    assert(mockup_hud_raster_cached(r,ui,status,map));
    mockup_hud_raster(oracle.data()+kGuard,ui,status,map);
    assert(third==oracle);
    (void)mockup_hud_raster_cached(q,ui,status,map);
    assert(second==oracle);
    mockup_hud_init();
    assert(mockup_hud_raster_cached(p,ui,status,map));
    assert(mockup_hud_raster_cached(q,ui,status,map));
    for(const auto* b:{&first,&second,&third,&oracle}) guard(*b);
}
