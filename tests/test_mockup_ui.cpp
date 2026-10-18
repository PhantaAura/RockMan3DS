#include "legends3ds/mockup_ui.h"
#include "../platform/3ds/source/mockup_hud.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
constexpr size_t kFramebufferBytes = 320u * 240u * 3u;
constexpr size_t kGuard = 32;
size_t offset(int x, int y) { return (size_t(x)*240u + size_t(239-y))*3u; }
void expect_pixel(const unsigned char* buffer, int x, int y,
                  unsigned char r, unsigned char g, unsigned char b) {
    const size_t index=offset(x,y);
    assert(buffer[index]==b && buffer[index+1]==g && buffer[index+2]==r);
}
void assert_guards(const std::array<unsigned char,kFramebufferBytes+2*kGuard>& buffer) {
    for(size_t i=0;i<kGuard;++i) {
        assert(buffer[i]==0xa5);
        assert(buffer[kGuard+kFramebufferBytes+i]==0xa5);
    }
}
}
int main() {
    l3::MockupUiState state;
    assert(state.tab==l3::MockupTab::Map);
    const auto& layout=l3::mockup_ui_layout();
    for(size_t i=0;i<layout.tabs.size();++i) {
        const auto& tab=layout.tabs[i];
        assert(l3::bottom_ui_rect_in_bounds(tab));
        assert(l3::mockup_ui_touch(state,tab.x,tab.y));
        assert(state.tab==static_cast<l3::MockupTab>(i));
        assert(l3::mockup_ui_touch(state,tab.x+tab.w-1,tab.y+tab.h-1));
        assert(!l3::mockup_ui_touch(state,tab.x+tab.w,tab.y));
        assert(!l3::mockup_ui_touch(state,tab.x,tab.y+tab.h));
        assert(std::strlen(l3::mockup_tab_label(state.tab))*6u<=size_t(tab.w));
    }
    assert(l3::bottom_ui_rect_in_bounds(layout.content));
    assert(l3::bottom_ui_rect_in_bounds(layout.status));
    assert(l3::bottom_ui_rect_in_bounds(layout.map_view));
    const auto last_tab=state.tab;
    for(const auto& point:std::array<std::array<int,2>,6>{{{{-1,16}},{{4,-1}},{{320,16}},{{4,240}},{{151,120}},{{270,150}}}}) {
        assert(!l3::mockup_ui_touch(state,point[0],point[1]));
        assert(state.tab==last_tab);
    }
    // Full hit coverage: every screen pixel selects a tab iff it is within one
    // of the visible tab rectangles. Borders are touchable, gutters are not.
    for(int y=0;y<240;++y) for(int x=0;x<320;++x) {
        bool expected=false;
        for(const auto& rect:layout.tabs) expected|=l3::mockup_ui_rect_contains(rect,x,y);
        assert(l3::mockup_ui_touch(state,x,y)==expected);
    }

    l3::RuntimeMap runtime;
    l3::RuntimeMapArea area;
    area.start_x=-1; area.start_z=-2; area.width=3; area.height=2;
    area.cell_first=1; area.cell_count=6;
    runtime.areas.push_back(area);
    runtime.cells={123,0,1,-1,1,0,1};
    const auto map=l3::make_mockup_map_view(runtime,0,-0x7f00,-0x7f00-0x200);
    assert(l3::mockup_map_available(map));
    assert(map.player_in_area && map.player_x==1 && map.player_z==1);
    assert(l3::sample_mockup_map_cell(map,0,0,3,2)==l3::BottomMapCellKind::Empty);
    assert(l3::sample_mockup_map_cell(map,1,0,3,2)==l3::BottomMapCellKind::Geometry);
    assert(l3::sample_mockup_map_cell(map,2,0,3,2)==l3::BottomMapCellKind::Special);
    assert(l3::sample_mockup_map_cell(map,3,0,3,2)==l3::BottomMapCellKind::Empty);
    assert(l3::sample_mockup_map_cell(map,0,0,0,2)==l3::BottomMapCellKind::Empty);
    assert(!l3::make_mockup_map_view(runtime,0,std::numeric_limits<int32_t>::min(),0).player_in_area);
    assert(!l3::make_mockup_map_view(runtime,0,std::numeric_limits<int32_t>::max(),0).player_in_area);
    assert(!l3::mockup_map_available(l3::make_mockup_map_view(runtime,1,0,0)));
    const auto rect=l3::mockup_map_rect(map);
    assert(rect.w==142 && rect.h==94);
    assert(rect.x>=layout.map_view.x && rect.y>=layout.map_view.y);
    assert(rect.x+rect.w<=layout.map_view.x+layout.map_view.w);
    assert(rect.y+rect.h<=layout.map_view.y+layout.map_view.h);
    auto tall=map; tall.width=1; tall.height=6;
    const auto tall_rect=l3::mockup_map_rect(tall);
    assert(tall_rect.w==28 && tall_rect.h==168);
    auto incomplete=map; incomplete.cell_count=5;
    assert(!l3::mockup_map_available(incomplete));
    assert(l3::mockup_map_rect(incomplete).w==0);
    assert(l3::sample_mockup_map_cell(incomplete,0,0,1,1)==l3::BottomMapCellKind::Empty);
    l3::BottomMapModel classified;
    classified.width=3; classified.height=2;
    classified.cells={l3::BottomMapCellKind::Empty,l3::BottomMapCellKind::Geometry,
        l3::BottomMapCellKind::Special,l3::BottomMapCellKind::Geometry,
        l3::BottomMapCellKind::Empty,l3::BottomMapCellKind::Geometry};
    const auto classified_view=l3::make_mockup_map_view(classified);
    for(uint16_t z=0;z<2;++z) for(uint16_t x=0;x<3;++x)
        assert(l3::sample_mockup_map_cell(map,x,z,3,2)==l3::sample_mockup_map_cell(classified_view,x,z,3,2));

    // Gesture ownership starts on a map touch, cannot be stolen by crossing
    // over a tab, and ends on release or a location/data change.
    l3::MockupUiState gesture;
    assert(!l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,160,120));
    assert(!l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,0,0));
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,150,120));
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,319,239));
    assert(gesture.map_pan_x==0 && gesture.map_pan_y==0); // Entire 1x map stays visible.
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::End,0,0));
    assert(!l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,151,120));
    assert(!l3::mockup_map_set_zoom(gesture,map,3));
    assert(!l3::mockup_map_set_zoom(gesture,incomplete,2));
    assert(l3::mockup_map_set_zoom(gesture,map,2));
    const auto enlarged=l3::mockup_map_projection(map,gesture);
    assert(enlarged.w==2*rect.w && enlarged.h==2*rect.h);
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,150,120));
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,100000,100000));
    assert(gesture.map_pan_x==71 && gesture.map_pan_y==10);
    auto projected=l3::mockup_map_projection(map,gesture);
    assert(projected.x<=layout.map_view.x && projected.y<=layout.map_view.y);
    assert(projected.x+projected.w>=layout.map_view.x+layout.map_view.w);
    assert(projected.y+projected.h>=layout.map_view.y+layout.map_view.h);
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,-100000,-100000));
    assert(gesture.map_pan_x==-71 && gesture.map_pan_y==-10);
    assert(gesture.tab==l3::MockupTab::Map); // A drag crossing tabs cannot select one.
    auto changed_area=map; changed_area.area_index=1;
    assert(!l3::mockup_ui_touch_event(gesture,changed_area,l3::MockupTouchPhase::Move,160,120));
    assert(!gesture.map_dragging);
    assert(l3::mockup_map_set_zoom(gesture,tall,4));
    const auto tall_zoom=l3::mockup_map_projection(tall,gesture);
    assert(tall_zoom.w==112 && tall_zoom.h==672);
    assert(gesture.map_pan_x==0); // A narrow map stays centered even when zoomed.
    assert(l3::mockup_map_set_zoom(gesture,map,4));
    gesture.map_pan_x=std::numeric_limits<int16_t>::max();
    gesture.map_pan_y=std::numeric_limits<int16_t>::min();
    projected=l3::mockup_map_projection(map,gesture); // Defensive bounds on loaded state.
    assert(projected.x==layout.map_view.x);
    assert(projected.y+projected.h==layout.map_view.y+layout.map_view.h);
    uint16_t source_x=99,source_z=99;
    for(int y=layout.map_view.y;y<layout.map_view.y+layout.map_view.h;++y)
        for(int x=layout.map_view.x;x<layout.map_view.x+layout.map_view.w;++x) {
            assert(l3::mockup_map_source_at(map,gesture,x,y,source_x,source_z));
            assert(source_x==uint32_t(x-projected.x)*map.width/projected.w);
            assert(source_z==uint32_t(y-projected.y)*map.height/projected.h);
            assert(source_x<map.width && source_z<map.height);
        }
    assert(!l3::mockup_map_source_at(map,gesture,layout.map_view.x-1,120,source_x,source_z));
    assert(!l3::mockup_map_source_at(map,gesture,layout.map_view.x+layout.map_view.w,120,source_x,source_z));
    gesture.tab=l3::MockupTab::Options;
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,
                                    layout.map_zoom_button.x,layout.map_zoom_button.y));
    assert(gesture.map_zoom==1 && gesture.map_pan_x==0 && gesture.map_pan_y==0);
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,
                                    layout.map_zoom_button.x+1,layout.map_zoom_button.y+1));
    assert(gesture.map_zoom==2);
    assert(!l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,
                                    layout.map_zoom_button.x+layout.map_zoom_button.w,layout.map_zoom_button.y));
    assert(gesture.map_zoom==2);
    assert(l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Begin,
                                    layout.map_reset_button.x,layout.map_reset_button.y));
    assert(gesture.map_zoom==1 && gesture.map_pan_x==0 && gesture.map_pan_y==0);
    assert(!l3::mockup_ui_touch_event(gesture,incomplete,l3::MockupTouchPhase::Begin,
                                    layout.map_zoom_button.x,layout.map_zoom_button.y));
    assert(!l3::mockup_ui_touch_event(gesture,map,l3::MockupTouchPhase::Move,151,120));

    // The same raster is compiled on the host and 3DS. Validate byte order,
    // portrait rotation, all four pages, absent data and maximum-sized stats.
    std::array<unsigned char,kFramebufferBytes+2*kGuard> framebuffer;
    framebuffer.fill(0xa5);
    auto* pixels=framebuffer.data()+kGuard;
    l3::BottomScreenModel status;
    status.hp=3; status.hp_max=4; status.buster_enabled=true;
    state.tab=l3::MockupTab::Map;
    mockup_hud_raster(pixels,state,status,map);
    assert_guards(framebuffer);
    expect_pixel(pixels,0,0,8,18,30);
    expect_pixel(pixels,319,239,8,18,30);
    expect_pixel(pixels,layout.tabs[0].x,layout.tabs[0].y,110,225,230);
    expect_pixel(pixels,layout.tabs[1].x,layout.tabs[1].y,61,112,142);
    const int marker_x=rect.x+int((uint32_t(map.player_x)*2u+1u)*rect.w/(uint32_t(map.width)*2u));
    const int marker_y=rect.y+int((uint32_t(map.player_z)*2u+1u)*rect.h/(uint32_t(map.height)*2u));
    expect_pixel(pixels,marker_x,marker_y,255,221,86);
    auto no_player=map; no_player.player_in_area=false;
    mockup_hud_raster(pixels,state,status,no_player);
    expect_pixel(pixels,marker_x,marker_y,8,18,30);
    auto bad_marker=map; bad_marker.player_x=std::numeric_limits<uint16_t>::max();
    mockup_hud_raster(pixels,state,status,bad_marker);
    assert_guards(framebuffer);
    // A maximally zoomed/panned map and its marker cannot spill into the
    // neighboring status panel, tabs, title or footer.
    state.tab=l3::MockupTab::Map;
    assert(l3::mockup_map_set_zoom(state,map,4));
    state.map_pan_x=32767; state.map_pan_y=-32768;
    mockup_hud_raster(pixels,state,status,map);
    assert_guards(framebuffer);
    expect_pixel(pixels,layout.content.x,layout.content.y,61,112,142);
    expect_pixel(pixels,layout.status.x,layout.status.y,61,112,142);
    expect_pixel(pixels,layout.tabs[0].x,layout.tabs[0].y,110,225,230);
    l3::mockup_map_reset(state);
    MMLRuntimeDiag diag{};
    diag.frame_us=std::numeric_limits<uint32_t>::max();
    diag.draw_calls=std::numeric_limits<uint32_t>::max();
    diag.linear_bytes=std::numeric_limits<uint32_t>::max();
    status.hp=std::numeric_limits<uint16_t>::max(); status.hp_max=1;
    for(size_t tab=0;tab<4;++tab) {
        state.tab=static_cast<l3::MockupTab>(tab);
        mockup_hud_raster(pixels,state,status,map,&diag);
        assert_guards(framebuffer);
        expect_pixel(pixels,layout.tabs[tab].x,layout.tabs[tab].y,110,225,230);
    }
    state.tab=l3::MockupTab::Map; status.hp_max=0;
    mockup_hud_raster(pixels,state,status,incomplete);
    assert_guards(framebuffer);
    mockup_hud_raster(nullptr,state,status,{});
}
