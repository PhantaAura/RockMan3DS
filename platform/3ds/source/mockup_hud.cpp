#include "mockup_hud.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#ifdef __3DS__
#include <3ds.h>
#endif

namespace {
struct Color { unsigned char r, g, b; };
constexpr Color kBackground{8,18,30}, kPanel{13,30,48}, kBorder{61,112,142};
constexpr Color kAccent{110,225,230}, kWhite{226,242,250}, kMuted{139,167,185};
constexpr Color kGrid{20,46,66}, kGeometry{41,110,123}, kSpecial{161,116,74};
constexpr Color kPlayer{255,221,86}, kHealth{61,220,133};
// Keep only two identities/keys, not copies of the 230,400-byte framebuffers.
// Full key comparison prevents a hash collision from suppressing a needed draw.
using RasterKey = std::array<uint64_t,21>;
struct RasterCacheEntry {
    unsigned char* framebuffer=nullptr;
    uint64_t fingerprint=0;
    RasterKey key{};
};
std::array<RasterCacheEntry,2> raster_cache{};
size_t next_cache_entry=0;
RasterKey raster_key(const l3::MockupUiState& state, const l3::BottomScreenModel& status,
                     const l3::MockupMapView& map) {
    RasterKey key{};
    key[0]=state.companion_visible;
    if(!state.companion_visible) return key;
    key[1]=static_cast<unsigned>(state.tab);
    const bool map_page=state.tab==l3::MockupTab::Map;
    const bool options_page=state.tab==l3::MockupTab::Options;
    if(map_page || options_page) key[2]=state.map_zoom;
    if(options_page) key[3]=state.map_follow_player;
    if(map_page) {
        key[4]=static_cast<uint16_t>(state.map_pan_x);
        key[5]=static_cast<uint16_t>(state.map_pan_y);
        key[6]=reinterpret_cast<uintptr_t>(map.packed_cells);
        key[7]=reinterpret_cast<uintptr_t>(map.classified_cells);
        key[8]=map.cell_count; key[9]=map.area_index;
        key[10]=map.width; key[11]=map.height;
        key[12]=map.player_in_area && map.player_x<map.width && map.player_z<map.height;
        if(key[12]) {
            key[13]=map.player_x; key[14]=map.player_z;
            key[15]=((map.player_yaw+256u)&0xFFFu)/512u;
        }
    } else if(options_page) key[8]=l3::mockup_map_available(map);
    key[16]=status.hp; key[17]=status.hp_max;
    key[18]=status.buster_enabled; key[19]=status.lock_on_active;
    key[20]=static_cast<unsigned>(status.context);
    return key;
}
uint64_t raster_fingerprint(const RasterKey& key) {
    uint64_t hash=14695981039346656037ULL;
    for(uint64_t value:key) for(unsigned byte=0;byte<8;++byte) {
        hash^=(value>>(byte*8))&255u;
        hash*=1099511628211ULL;
    }
    return hash;
}
struct Glyph { char character; unsigned char rows[7]; };
// Original 5x7 bitmap glyphs expressed in code, independent of supplied art.
constexpr Glyph kGlyphs[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {'/',{1,1,2,4,8,16,16}}, {'-',{0,0,0,31,0,0,0}},
 {':',{0,4,4,0,4,4,0}}, {'.',{0,0,0,0,0,4,4}},
 {'+',{0,4,4,31,4,4,0}}, {'|',{4,4,4,4,4,4,4}}
};
class Raster {
public:
    explicit Raster(unsigned char* buffer) : buffer_(buffer) {}
    void pixel(int x, int y, Color c) {
        if (x < clip_x0_ || x >= clip_x1_ || y < clip_y0_ || y >= clip_y1_) return;
        // libctru portrait framebuffer: landscape (x,y) maps to column x,
        // with inverted scanline 239-y; BGR8 stores blue, green, red.
        const size_t i = (size_t(x) * 240u + size_t(239-y)) * 3u;
        buffer_[i] = c.b; buffer_[i+1] = c.g; buffer_[i+2] = c.r;
    }
    void fill(int x, int y, int w, int h, Color c) {
        const int end_x = std::min(clip_x1_, x+w), end_y = std::min(clip_y1_, y+h);
        for (int px = std::max(clip_x0_,x); px < end_x; ++px)
            for (int py = std::max(clip_y0_,y); py < end_y; ++py) pixel(px,py,c);
    }
    void clip(const l3::BottomUiRect& r) {
        clip_x0_=std::clamp<int>(r.x,0,320); clip_y0_=std::clamp<int>(r.y,0,240);
        clip_x1_=std::clamp<int>(r.x+r.w,clip_x0_,320);
        clip_y1_=std::clamp<int>(r.y+r.h,clip_y0_,240);
    }
    void clear_clip() { clip_x0_=0; clip_y0_=0; clip_x1_=320; clip_y1_=240; }
    void border(const l3::BottomUiRect& r, Color c) {
        fill(r.x,r.y,r.w,1,c); fill(r.x,r.y+r.h-1,r.w,1,c);
        fill(r.x,r.y,1,r.h,c); fill(r.x+r.w-1,r.y,1,r.h,c);
    }
    void text(int x, int y, const char* str, Color c = kWhite) {
        for (const char* p = str; *p; ++p, x+=6) {
            for (const auto& g : kGlyphs) {
                if (g.character != *p) continue;
                for (int row=0; row<7; ++row)
                    for (int col=0; col<5; ++col)
                        if (g.rows[row] & (1u<<(4-col))) pixel(x+col,y+row,c);
                break;
            }
        }
    }
    void center(const l3::BottomUiRect& r, int y, const char* text_value, Color c=kWhite) {
        text(r.x+(r.w-int(std::strlen(text_value))*6+1)/2,y,text_value,c);
    }
private:
    unsigned char* buffer_;
    int clip_x0_=0,clip_y0_=0,clip_x1_=320,clip_y1_=240;
};
void draw_tab_icon(Raster& r, l3::MockupTab tab, int x, int y, Color color) {
    switch(tab) {
    case l3::MockupTab::Map:
        r.border({static_cast<int16_t>(x-9),static_cast<int16_t>(y),18,16},color);
        r.fill(x-3,y+1,1,14,color); r.fill(x+3,y+1,1,14,color);
        r.fill(x-8,y+7,16,1,color); break;
    case l3::MockupTab::Items:
        r.border({static_cast<int16_t>(x-9),static_cast<int16_t>(y+4),18,12},color);
        r.fill(x-7,y+1,14,2,color); r.fill(x-2,y+7,4,3,color); break;
    case l3::MockupTab::Weapons:
        r.border({static_cast<int16_t>(x-7),static_cast<int16_t>(y+2),14,12},color);
        r.fill(x-1,y,2,16,color); r.fill(x-10,y+7,20,2,color); break;
    case l3::MockupTab::Options:
        r.fill(x-10,y+2,20,1,color); r.fill(x-10,y+8,20,1,color);
        r.fill(x-10,y+14,20,1,color); r.fill(x-5,y,3,5,color);
        r.fill(x+3,y+6,3,5,color); r.fill(x-2,y+12,3,5,color); break;
    }
}
void draw_map(Raster& raster, const l3::MockupMapView& map, const l3::MockupUiState& state) {
    const auto& layout=l3::mockup_ui_layout();
    char area[24]; std::snprintf(area,sizeof(area),"AREA %lu",static_cast<unsigned long>(map.area_index));
    raster.text(layout.content.x+6,layout.content.y+8,area,kAccent);
    if (!l3::mockup_map_available(map)) {
        raster.center(layout.content,88,"MAP DATA",kWhite);
        raster.center(layout.content,102,"UNAVAILABLE",kMuted);
        return;
    }
    const auto projection=l3::mockup_map_projection(map,state);
    const auto& view=layout.map_view;
    const int draw_x=std::max<int>(view.x,projection.x), draw_y=std::max<int>(view.y,projection.y);
    const int draw_w=std::min<int>(view.x+view.w,projection.x+projection.w)-draw_x;
    const int draw_h=std::min<int>(view.y+view.h,projection.y+projection.h)-draw_y;
    const l3::BottomUiRect bounds{static_cast<int16_t>(draw_x),static_cast<int16_t>(draw_y),
                                 static_cast<int16_t>(draw_w),static_cast<int16_t>(draw_h)};
    raster.clip(view);
    // The software backend on Old3DS has no hardware integer divide. Build
    // the at-most 142x168 sampling axes once, then fill each source-cell run.
    std::array<uint16_t,142> sample_x{};
    std::array<uint16_t,168> sample_z{};
    for(int x=0;x<bounds.w;++x)
        sample_x[x]=static_cast<uint16_t>(uint32_t(bounds.x+x-projection.x)*map.width/projection.w);
    for(int z=0;z<bounds.h;++z)
        sample_z[z]=static_cast<uint16_t>(uint32_t(bounds.y+z-projection.y)*map.height/projection.h);
    const bool grid=projection.w/map.width>=4 && projection.h/map.height>=4;
    for(int z=0;z<bounds.h;) {
        int next_z=z+1;
        while(next_z<bounds.h && sample_z[next_z]==sample_z[z]) ++next_z;
        const size_t row=size_t(sample_z[z])*map.width;
        for(int x=0;x<bounds.w;) {
            int next_x=x+1;
            while(next_x<bounds.w && sample_x[next_x]==sample_x[x]) ++next_x;
            const size_t i=row+sample_x[x];
            const auto cell=map.classified_cells?map.classified_cells[i]:
                (map.packed_cells[i]>0?l3::BottomMapCellKind::Geometry:
                 (map.packed_cells[i]<0?l3::BottomMapCellKind::Special:l3::BottomMapCellKind::Empty));
            const Color color=cell==l3::BottomMapCellKind::Geometry?kGeometry:
                              (cell==l3::BottomMapCellKind::Special?kSpecial:kBackground);
            raster.fill(bounds.x+x,bounds.y+z,next_x-x,next_z-z,color);
            if(grid) {
                raster.fill(bounds.x+x,bounds.y+z,next_x-x,1,kGrid);
                raster.fill(bounds.x+x,bounds.y+z,1,next_z-z,kGrid);
            }
            x=next_x;
        }
        z=next_z;
    }
    if (map.player_in_area && map.player_x<map.width && map.player_z<map.height) {
        const int x=projection.x+int((uint32_t(map.player_x)*2u+1u)*projection.w/(uint32_t(map.width)*2u));
        const int y=projection.y+int((uint32_t(map.player_z)*2u+1u)*projection.h/(uint32_t(map.height)*2u));
        raster.fill(x-3,y-3,7,7,kBackground);
        raster.fill(x-2,y-2,5,5,kPlayer);
        const unsigned heading=((map.player_yaw+256u)&0xFFFu)/512u;
        constexpr int dx[8]={0,-1,-1,-1,0,1,1,1};
        constexpr int dy[8]={-1,-1,0,1,1,1,0,-1};
        raster.pixel(x+dx[heading]*4,y+dy[heading]*4,kPlayer);
    }
    raster.clear_clip();
    char zoom[24]; std::snprintf(zoom,sizeof(zoom),"DRAG MAP  %uX",unsigned(state.map_zoom));
    raster.text(layout.content.x+6,209,zoom,kMuted);
}
const char* context_label(l3::BottomScreenContext context) {
    switch(context) {
    case l3::BottomScreenContext::Gameplay: return "PLAYING";
    case l3::BottomScreenContext::Dialogue: return "DIALOGUE";
    case l3::BottomScreenContext::Cinematic: return "CINEMATIC";
    case l3::BottomScreenContext::Pause: return "PAUSED";
    case l3::BottomScreenContext::Save: return "SAVE";
    case l3::BottomScreenContext::Loading: return "LOADING";
    }
    return "PLAYING";
}
void draw_status(Raster& r, const l3::BottomScreenModel& status) {
    const auto& p=l3::mockup_ui_layout().status;
    r.center(p,24,"PLAYER",kAccent); r.fill(p.x+6,38,p.w-12,1,kBorder);
    r.text(p.x+6,49,"HP");
    if (status.hp_max) {
        char hp[24]; std::snprintf(hp,sizeof(hp),"%u/%u",unsigned(status.hp),unsigned(status.hp_max));
        r.text(p.x+6,63,hp);
        r.fill(p.x+6,77,p.w-12,9,kBackground);
        const int bar=int(uint32_t(std::min(status.hp,status.hp_max))*(p.w-12)/status.hp_max);
        r.fill(p.x+6,77,bar,9,kHealth);
    } else r.text(p.x+6,63,"NO DATA",kMuted);
    r.text(p.x+6,101,"BUSTER",kMuted);
    r.text(p.x+6,115,status.buster_enabled?"READY":"OFF",status.buster_enabled?kAccent:kMuted);
    r.text(p.x+6,141,"LOCK",kMuted);
    r.text(p.x+6,155,status.lock_on_active?"ACTIVE":"OFF",status.lock_on_active?kPlayer:kMuted);
    r.fill(p.x+6,179,p.w-12,1,kBorder);
    r.center(p,194,context_label(status.context),kMuted);
}
void draw_content(Raster& r, const l3::MockupUiState& state,
                  const l3::MockupMapView& map, const MMLRuntimeDiag* diag) {
    const auto& p=l3::mockup_ui_layout().content;
    if(state.tab==l3::MockupTab::Map) { draw_map(r,map,state); return; }
    r.text(p.x+6,p.y+8,l3::mockup_tab_label(state.tab),kAccent);
    if(state.tab==l3::MockupTab::Items || state.tab==l3::MockupTab::Weapons) {
        r.center(p,84,state.tab==l3::MockupTab::Items?"INVENTORY":"LOADOUT");
        r.center(p,101,"UNAVAILABLE",kMuted);
        return;
    }
    (void)diag; // Runtime telemetry remains in the technical log.
    const auto& layout=l3::mockup_ui_layout();
    const bool available=l3::mockup_map_available(map);
    for(const auto& button:{layout.map_zoom_button,layout.map_reset_button,layout.map_follow_button}) {
        r.fill(button.x,button.y,button.w,button.h,kBackground);
        r.border(button,available?kAccent:kBorder);
    }
    char zoom[24]; std::snprintf(zoom,sizeof(zoom),"MAP ZOOM %uX",unsigned(state.map_zoom));
    r.center(layout.map_zoom_button,49,zoom,available?kWhite:kMuted);
    r.center(layout.map_zoom_button,62,"TAP TO CYCLE",kMuted);
    r.center(layout.map_reset_button,94,"RESET MAP",available?kWhite:kMuted);
    r.center(layout.map_follow_button,127,state.map_follow_player?"FOLLOW PLAYER ON":"FOLLOW PLAYER OFF",available?kWhite:kMuted);
    const char* controls[]={"HOLD SELECT","L/R TABS  X ZOOM","D-PAD PAN B CENTER","A SHOW/HIDE MAP"};
    for(int i=0;i<4;++i) r.text(p.x+6,151+i*14,controls[i]);
}
} // namespace

void mockup_hud_raster(unsigned char* buffer, const l3::MockupUiState& state,
                       const l3::BottomScreenModel& status, const l3::MockupMapView& map,
                       const MMLRuntimeDiag* diagnostics) {
    if (!buffer) return;
    Raster r(buffer);
    r.fill(0,0,320,240,kBackground);
    if(!state.companion_visible) {
        r.text(103,111,"COMPANION OFF",kMuted);
        r.text(91,128,"TOUCH TO RESTORE",kAccent);
        return;
    }
    r.text(5,4,"LEGENDS3DS",kMuted);
    const auto& layout=l3::mockup_ui_layout();
    for(size_t i=0;i<layout.tabs.size();++i) {
        const auto tab=static_cast<l3::MockupTab>(i);
        const auto& panel=layout.tabs[i];
        const bool selected=tab==state.tab;
        r.fill(panel.x,panel.y,panel.w,panel.h,selected?Color{23,66,80}:kPanel);
        r.border(panel,selected?kAccent:kBorder);
        if(selected) r.fill(panel.x+1,panel.y+1,2,panel.h-2,kAccent);
        draw_tab_icon(r,tab,panel.x+panel.w/2,panel.y+7,selected?kAccent:kMuted);
        r.center(panel,panel.y+33,l3::mockup_tab_label(tab),selected?kWhite:kMuted);
    }
    for(const auto& panel:{layout.content,layout.status}) {
        r.fill(panel.x,panel.y,panel.w,panel.h,kPanel); r.border(panel,kBorder);
    }
    draw_content(r,state,map,diagnostics); draw_status(r,status);
    r.text(5,228,"TOUCH TABS | HOLD SELECT",kAccent);
}
void mockup_hud_init() {
    mockup_hud_invalidate();
#ifdef __3DS__
    gfxSetScreenFormat(GFX_BOTTOM,GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_BOTTOM,true);
#endif
}
void mockup_hud_invalidate() {
    raster_cache={};
    next_cache_entry=0;
}
bool mockup_hud_raster_cached(unsigned char* buffer, const l3::MockupUiState& state,
                              const l3::BottomScreenModel& status, const l3::MockupMapView& map,
                              const MMLRuntimeDiag* diagnostics) {
    if(!buffer) return false;
    const auto key=raster_key(state,status,map);
    const uint64_t fingerprint=raster_fingerprint(key);
    RasterCacheEntry* entry=nullptr;
    for(auto& cached:raster_cache) if(cached.framebuffer==buffer) {entry=&cached;break;}
    if(entry && entry->fingerprint==fingerprint && entry->key==key) return false;
    mockup_hud_raster(buffer,state,status,map,diagnostics);
    if(!entry) {
        entry=&raster_cache[next_cache_entry];
        next_cache_entry=(next_cache_entry+1)%raster_cache.size();
    }
    *entry={buffer,fingerprint,key};
    return true;
}
void mockup_hud_draw(const l3::MockupUiState& state, const l3::BottomScreenModel& status,
                     const l3::MockupMapView& map, const MMLRuntimeDiag* diagnostics) {
#ifdef __3DS__
    u16 width=0,height=0;
    unsigned char* buffer=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    if(!buffer || width!=240 || height!=320 || gfxGetScreenFormat(GFX_BOTTOM)!=GSP_BGR8_OES) return;
    if(!mockup_hud_raster_cached(buffer,state,status,map,diagnostics)) return;
    // Flush just our CPU-written bottom framebuffer; top is a GPU render target.
    GSPGPU_FlushDataCache(buffer,320u*240u*3u);
#else
    (void)state; (void)status; (void)map; (void)diagnostics;
#endif
}
