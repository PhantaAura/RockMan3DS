#include "mockup_hud.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
#ifdef __3DS__
#include <3ds.h>
#endif

namespace {
struct Color { unsigned char r, g, b; };
// Deep blue riveted panels with a cyan selection (port presentation).
constexpr Color kBackground{4,10,40}, kPanel{16,40,112}, kBorder{96,150,224};
constexpr Color kAccent{72,210,248}, kWhite{230,240,255}, kMuted{150,170,210};
constexpr Color kGrid{28,52,110}, kGeometry{196,172,132}, kSpecial{214,120,64};
constexpr Color kRivet{170,200,240}, kHeader{8,22,72};
constexpr Color kPlayer{255,221,86};
// Keep only two identities/keys, not copies of the 230,400-byte framebuffers.
// Full key comparison prevents a hash collision from suppressing a needed draw.
using RasterKey = std::array<uint64_t,24>;
struct RasterCacheEntry {
    unsigned char* framebuffer=nullptr;
    uint64_t fingerprint=0;
    RasterKey key{};
};
std::array<RasterCacheEntry,2> raster_cache{};
size_t next_cache_entry=0;
// Tutorial message box overlay (FRONTEND_DIAGNOSTIC presentation of the
// original top-screen window; text and choice state come from the runtime).
struct DialogueOverlay { bool active=false; char text[480]={}; int options=0, choice=0; bool next=false; uint64_t hash=0; };
DialogueOverlay g_dialogue{};
// GAUGE01u.TIM page (materials/hud_gauge.l3ui, cooked from the owner's
// DEMO_DAT.BIN). Empty until mockup_hud_load_gauge succeeds.
struct GaugeArt {
    std::vector<uint8_t> indices; // 256x256, one palette index per pixel
    std::array<uint16_t,128> palette{}; // 8 rows of 16 PS1 BGR555 colors
    bool loaded=false;
};
GaugeArt g_gauge{};
RasterKey raster_key(const l3::MockupUiState& state, const l3::BottomScreenModel& status,
                     const l3::MockupMapView& map) {
    RasterKey key{};
    key[21]=g_dialogue.active?g_dialogue.hash:0;
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
    key[22]=static_cast<uint32_t>(status.money);
    key[23]=g_gauge.loaded;
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
 {'!',{4,4,4,4,4,0,4}}, {'?',{14,17,1,2,4,0,4}}, {',',{0,0,0,0,4,4,8}},
 {'\'',{4,4,8,0,0,0,0}}, {'>',{8,4,2,1,2,4,8}},
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
    // One textured sprite from the gauge page: 0x0000 texels are transparent
    // and the 0x808080 primitive color leaves texels unchanged (PS1 modulation).
    void sprite(int dx, int dy, int u, int v, int w, int h, Color mod={128,128,128}, int row=0) {
        if(!g_gauge.loaded) return;
        for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
            const uint16_t c=g_gauge.palette[size_t(row)*16u+g_gauge.indices[size_t((v+y)&255)*256u+size_t((u+x)&255)]];
            if(!c) continue;
            const auto channel=[](unsigned five, unsigned m){ return (unsigned char)std::min(255u,(five<<3)*m/128u); };
            pixel(dx+x,dy+y,{channel(c&31u,mod.r),channel((c>>5)&31u,mod.g),channel((c>>10)&31u,mod.b)});
        }
    }
    void center(const l3::BottomUiRect& r, int y, const char* text_value, Color c=kWhite) {
        text(r.x+(r.w-int(std::strlen(text_value))*6+1)/2,y,text_value,c);
    }
private:
    unsigned char* buffer_;
    int clip_x0_=0,clip_y0_=0,clip_x1_=320,clip_y1_=240;
};
void draw_map(Raster& raster, const l3::MockupMapView& map, const l3::MockupUiState& state) {
    const auto& layout=l3::mockup_ui_layout();
    char area[24]; std::snprintf(area,sizeof(area),"AREA %lu",static_cast<unsigned long>(map.area_index));
    raster.text(layout.content.x+8,layout.content.y+6,area,kWhite);
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
    // the at-most 212x172 sampling axes once, then fill each source-cell run.
    std::array<uint16_t,212> sample_x{};
    std::array<uint16_t,172> sample_z{};
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
    // North marker in the map's lower-left corner (map rows run north-up).
    const l3::BottomUiRect north{static_cast<int16_t>(view.x+3),static_cast<int16_t>(view.y+view.h-24),13,21};
    raster.fill(north.x,north.y,north.w,north.h,kHeader); raster.border(north,kBorder);
    raster.text(north.x+4,north.y+3,"N",kWhite);
    raster.fill(north.x+6,north.y+12,1,6,kWhite); raster.fill(north.x+5,north.y+13,3,1,kWhite);
    raster.fill(north.x+4,north.y+14,5,1,kWhite);
    char zoom[8]; std::snprintf(zoom,sizeof(zoom),"%uX",unsigned(state.map_zoom));
    raster.text(layout.content.x+layout.content.w-8-int(std::strlen(zoom))*6,layout.content.y+6,zoom,kMuted);
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
// Mega Man's life gauge, composed exactly as ROCK_NEO FUN_8005C9BC does in
// its normal state (piece table 0x8008D37C, cells 0x8008D5E8/0x8008D5FC):
// a top cap raised by the body height 5*(max>>4)-10, a body window of that
// height, the base, the 40x40 ring, one 8x8 cell per 16 HP stepping up 5 px,
// a half cell for a remainder, and a black interior behind them. The PS1
// ordering table draws the last-inserted primitive first, so pieces are
// painted here in reverse call order. Anchor (x,y) is the original (8,0x88)
// screen origin; on the bottom screen it is placed by port presentation.
void draw_life_gauge(Raster& r, int x, int y, unsigned hp, unsigned hp_max, int max_body) {
    const int body=std::clamp(int(hp_max>>4)*5-10,0,max_body);
    const int top=y-body;
    const unsigned full=hp>>4, part=hp&15u;
    r.sprite(x+4,top+0x20,0x70,0x40,0x10,body+16,{0,0,0});        // interior, color 0
    if(part) r.sprite(x+8,y+0x25-int(full)*5,0x98,0,8,8);           // remainder
    for(unsigned i=full;i-->0;) r.sprite(x+8,y+0x25-int(i)*5,0x88,0,8,8);
    r.sprite(x-8,y+0x38,0x00,0x38,0x28,0x28);                     // ring
    // Body window. The original samples v=0x18 for `body` rows; past 16 rows
    // that runs into the ring art, so longer bodies repeat the 16-row segment
    // (port presentation for maxima the original gauge never reaches).
    for(int done=0;done<body;done+=16) r.sprite(x,top+0x28+done,0x00,0x18,0x18,std::min(16,body-done));
    r.sprite(x,y+0x28,0x00,0x28,0x18,0x10);                       // base
    r.sprite(x,top+0x10,0x00,0x00,0x18,0x18);                     // top cap
}
// GAUGE01u digits: 8x8 cells from (96,176), '0'..'9'.
void draw_gauge_number(Raster& r, int right_x, int y, uint32_t value, int min_digits) {
    int x=right_x-8;
    for(int n=0; value || n<min_digits; ++n, value/=10, x-=8) r.sprite(x,y,96+int(value%10)*8,176,8,8);
}
// Riveted panel: fill, border, and a rivet in each corner.
void draw_panel(Raster& r, const l3::BottomUiRect& p, Color fill) {
    r.fill(p.x,p.y,p.w,p.h,fill); r.border(p,kBorder);
    for(const int x:{p.x+3,p.x+p.w-5}) for(const int y:{p.y+3,p.y+p.h-5}) r.fill(x,y,2,2,kRivet);
}
// Status column: the original life gauge on the left with the HP readout
// beside it, then buster / lock-on slots and the screen context.
void draw_status(Raster& r, const l3::BottomScreenModel& status) {
    const auto& p=l3::mockup_ui_layout().status;
    const int text_x=p.x+48;
    if(!g_gauge.loaded) {
        r.text(p.x+8,p.y+40,"GAUGE",kMuted); r.text(p.x+8,p.y+52,"ART",kMuted); r.text(p.x+8,p.y+64,"MISSING",kMuted);
    } else if(status.hp_max) {
        // Cap top to ring bottom is 80+body px; centre it in the top 130 px.
        constexpr int kMaxBody=46;
        const int body=std::clamp(int(status.hp_max>>4)*5-10,0,kMaxBody);
        const int cap_top=p.y+4+(130-80-body)/2;
        draw_life_gauge(r,p.x+12,cap_top+body-16,std::min(status.hp,status.hp_max),status.hp_max,kMaxBody);
    }
    r.text(text_x,p.y+50,"LIFE",kAccent);
    if(status.hp_max) {
        char hp[8], max[8];
        std::snprintf(hp,sizeof(hp),"%u",unsigned(std::min(status.hp,status.hp_max)));
        std::snprintf(max,sizeof(max),"/%u",unsigned(status.hp_max));
        r.text(text_x,p.y+64,hp,kWhite); r.text(text_x,p.y+76,max,kMuted);
    } else r.text(text_x,p.y+64,"--",kMuted);
    const l3::BottomUiRect slots[2]={{static_cast<int16_t>(p.x+5),static_cast<int16_t>(p.y+138),static_cast<int16_t>(p.w-10),18},
                                     {static_cast<int16_t>(p.x+5),static_cast<int16_t>(p.y+158),static_cast<int16_t>(p.w-10),18}};
    for(const auto& slot:slots) { r.fill(slot.x,slot.y,slot.w,slot.h,kHeader); r.border(slot,kBorder); }
    r.text(slots[0].x+4,slots[0].y+6,"BUSTER",kMuted);
    r.text(slots[0].x+slots[0].w-22,slots[0].y+6,status.buster_enabled?"RDY":"OFF",status.buster_enabled?kAccent:kMuted);
    r.text(slots[1].x+4,slots[1].y+6,"LOCK",kMuted);
    r.text(slots[1].x+slots[1].w-22,slots[1].y+6,status.lock_on_active?" ON":"OFF",status.lock_on_active?kPlayer:kMuted);
    r.center(p,p.y+182,context_label(status.context),kMuted);
}
// Footer: GAUGE01u ZENNY plate (0,177 96x14) with the money in its box.
// The plate is original art; showing it permanently here is port presentation.
void draw_zenny(Raster& r, int32_t money) {
    if(!g_gauge.loaded) return;
    r.sprite(218,223,0,177,96,14);
    draw_gauge_number(r,306,226,uint32_t(std::clamp<int32_t>(money,0,999999)),1);
}
void draw_content(Raster& r, const l3::MockupUiState& state,
                  const l3::MockupMapView& map, const MMLRuntimeDiag* diag) {
    const auto& p=l3::mockup_ui_layout().content;
    if(state.tab==l3::MockupTab::Map) { draw_map(r,map,state); return; }
    r.text(p.x+8,p.y+6,l3::mockup_tab_label(state.tab),kWhite);
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
    const auto& zb=layout.map_zoom_button; const auto& rb=layout.map_reset_button; const auto& fb=layout.map_follow_button;
    r.center(zb,zb.y+7,zoom,available?kWhite:kMuted);
    r.center(zb,zb.y+20,"TAP TO CYCLE",kMuted);
    r.center(rb,rb.y+13,"RESET MAP",available?kWhite:kMuted);
    r.center(fb,fb.y+9,state.map_follow_player?"FOLLOW PLAYER ON":"FOLLOW PLAYER OFF",available?kWhite:kMuted);
    const char* controls[]={"HOLD SELECT","L/R TABS  X ZOOM","D-PAD PAN  B CENTER","A SHOW/HIDE MAP"};
    for(int i=0;i<4;++i) r.text(p.x+8,fb.y+fb.h+12+i*14,controls[i]);
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
    const auto& layout=l3::mockup_ui_layout();
    for(size_t i=0;i<layout.tabs.size();++i) {
        const auto tab=static_cast<l3::MockupTab>(i);
        const auto& panel=layout.tabs[i];
        const bool selected=tab==state.tab;
        r.fill(panel.x,panel.y,panel.w,panel.h,selected?kAccent:kHeader);
        r.border(panel,selected?kAccent:kBorder);
        if(selected) r.border({static_cast<int16_t>(panel.x+1),static_cast<int16_t>(panel.y+1),
                               static_cast<int16_t>(panel.w-2),static_cast<int16_t>(panel.h-2)},kWhite);
        r.center(panel,panel.y+6,l3::mockup_tab_label(tab),selected?kBackground:kWhite);
    }
    for(const auto& panel:{layout.content,layout.status}) draw_panel(r,panel,kPanel);
    // Content header strip (area name / page title).
    r.fill(layout.content.x+1,layout.content.y+1,layout.content.w-2,16,kHeader);
    r.fill(layout.content.x+1,layout.content.y+17,layout.content.w-2,1,kBorder);
    draw_content(r,state,map,diagnostics); draw_status(r,status);
    draw_panel(r,{4,222,312,17},kHeader);
    r.text(12,227,"TAP A TAB TO OPEN MENU.",kWhite);
    draw_zenny(r,status.money);
    if(g_dialogue.active) {
        // Word-wrapped uppercase text, 50 columns, 9 rows; '\n' breaks lines.
        const l3::BottomUiRect box{4,130,312,106};
        r.fill(box.x,box.y,box.w,box.h,Color{6,14,40}); r.border(box,kAccent);
        int col=0,row=0; char word[64]; size_t wn=0;
        const auto put=[&](char c){ char one[2]={c,0}; if(row<9) r.text(box.x+6+col*6,box.y+6+row*11,one,kWhite); ++col; };
        const auto flush=[&](){ if(!wn)return; if(col+int(wn)>50){col=0;++row;} for(size_t i=0;i<wn;++i)put(word[i]); wn=0; };
        for(const char* p=g_dialogue.text;;++p) {
            const char c=*p;
            if(!c){flush();break;}
            if(c=='\n'){flush();col=0;++row;continue;}
            if(c==' '){flush();if(col)put(' ');continue;}
            if(wn<sizeof(word)) word[wn++]=char(c>='a'&&c<='z'?c-32:c);
        }
        if(g_dialogue.next) r.text(box.x+box.w-12,box.y+box.h-11,">",kAccent);
        if(g_dialogue.options>1) r.text(box.x+6,box.y+box.h-11,g_dialogue.choice==0?"> YES    NO":"  YES  > NO",kAccent);
    }
}
void mockup_hud_set_dialogue(bool active, const char* text, int options, int choice, bool waiting_next) {
    DialogueOverlay next{};
    next.active=active;
    if(active&&text){std::strncpy(next.text,text,sizeof(next.text)-1);}
    next.options=options;next.choice=choice;next.next=waiting_next;
    uint64_t h=1469598103934665603ULL;
    for(const char* p=next.text;*p;++p){h^=uint8_t(*p);h*=1099511628211ULL;}
    h^=uint64_t(options)<<40^uint64_t(choice)<<48^uint64_t(waiting_next)<<56^uint64_t(active)<<60;
    next.hash=h|1;
    g_dialogue=next;
}
bool mockup_hud_load_gauge(const unsigned char* data, size_t size) {
    g_gauge.loaded=false;
    const auto u16=[&](size_t at){ return uint16_t(data[at]|(data[at+1]<<8)); };
    constexpr size_t kHeader=16, kPalette=128*2, kIndices=256*256;
    if(!data || size!=kHeader+kPalette+kIndices || std::memcmp(data,"L3UI",4)!=0 || u16(4)!=1 ||
       u16(6)!=256 || u16(8)!=256 || u16(10)!=16 || u16(12)!=8) return false;
    for(size_t i=0;i<128;++i) g_gauge.palette[i]=u16(kHeader+i*2);
    g_gauge.indices.assign(data+kHeader+kPalette,data+size);
    for(uint8_t& index:g_gauge.indices) index&=15u;
    g_gauge.loaded=true;
    mockup_hud_invalidate();
    return true;
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
