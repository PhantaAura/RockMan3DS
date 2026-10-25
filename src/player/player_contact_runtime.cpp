#include "legends3ds/player_contact_runtime.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>

namespace l3 {
namespace {
int16_t rd(const L3PlayerCompatWork& w, size_t o) {
    return static_cast<int16_t>(uint16_t(w.bytes[o]) | (uint16_t(w.bytes[o+1]) << 8));
}
int16_t short_wrap(int32_t v) { return static_cast<int16_t>(static_cast<uint16_t>(v)); }
int32_t word_wrap(int64_t v) { return static_cast<int32_t>(static_cast<uint32_t>(v)); }
int32_t word_mul(int32_t a,int32_t b) { return word_wrap(int64_t(a)*b); }
std::optional<int32_t> word_div(int32_t a,int32_t b) {
    if(!b || (a==std::numeric_limits<int32_t>::min() && b==-1))return {};
    return a/b;
}
int32_t grid(int32_t v) { return v >= 0 ? v / 512 : -((-v + 511) / 512); }
struct Candidates { std::array<RuntimeCellRef,4> cells{}; uint8_t size = 0; };
Candidates gather(const RuntimeMap& map, size_t area, int16_t x, int16_t z) {
    Candidates out;
    if (area >= map.areas.size()) return out;
    const auto& a = map.areas[area];
    const int32_t nx = (uint16_t(x) & 511u) < 257u ? 0x7E00 : 0x8200;
    const int32_t nz = (uint16_t(z) & 511u) < 257u ? 0x7E00 : 0x8200;
    const std::array<std::array<int32_t,2>,4> offsets{{
        {{0x8000,0x8000}}, {{nx,0x8000}}, {{0x8000,nz}}, {{nx,nz}}
    }};
    for (const auto& o : offsets) {
        const int32_t gx=grid(int32_t(x)+o[0]), gz=grid(int32_t(z)+o[1]);
        const int32_t lx=gx-a.start_x, lz=gz-a.start_z;
        if (lx<0 || lz<0 || lx>=a.width || lz>=a.height) continue;
        const uint64_t ci=uint64_t(a.cell_first)+uint64_t(lz)*a.width+uint64_t(lx);
        if (ci>=map.cells.size()) continue;
        const auto packed=map.cells[static_cast<size_t>(ci)];
        if (!packed) continue;
        bool duplicate=false;
        if (packed>0) for (uint8_t i=0;i<out.size;++i)
            if(out.cells[i].packed_cell==packed) duplicate=true;
        if (duplicate) continue;
        auto& c=out.cells[out.size++];
        c.area_index=area; c.grid_x=static_cast<uint16_t>(gx); c.grid_z=static_cast<uint16_t>(gz);
        c.cell_index=static_cast<uint32_t>(ci); c.packed_cell=packed;
        c.world_origin_x=gx*512-0x7F00; c.world_origin_z=gz*512-0x7F00;
    }
    return out;
}
struct Rect { int16_t x0=0,x1=0,z0=0,z1=0,y0=0,y1=0; };
Rect rect(const RuntimeMapIdx& idx, const RuntimeContactGroup& g,
          const RuntimeContactPrimitive& p, const RetailPlayerBounds& b) {
    // The source copies IDX XYZ, then performs halfword additions. The bounds
    // expansion happens inside the family-0/1 classifier and Y response.
    return {short_wrap(int32_t(idx.x)+g.x0-b.x1), short_wrap(int32_t(idx.x)+g.x1-b.x0),
            short_wrap(int32_t(idx.z)+g.z0-b.z1), short_wrap(int32_t(idx.z)+g.z1-b.z0),
            short_wrap(int32_t(idx.y)+p.y0-b.y1), short_wrap(int32_t(idx.y)+p.y1-b.y0)};
}
bool inside_xz(const Rect& r, int16_t x, int16_t z) {
    return r.x0<x && x<r.x1 && r.z0<z && z<r.z1;
}
bool inside(const Rect& r, const L3RetailContactScratch& p) {
    return inside_xz(r,p.x,p.z) && r.y0<p.y && p.y<r.y1;
}
std::optional<Rect> diagonal_rect(const RuntimeMapIdx& idx,const RuntimeContactGroup& g,
                                  const RuntimeContactPrimitive& p,uint16_t classifier,
                                  const RetailPlayerBounds& b,int16_t x,int16_t z) {
    const unsigned family=(classifier&0xf0u)>>4;
    if(family<2||family>5)return {};
    int16_t x0=short_wrap(int32_t(idx.x)+g.x0),x1=short_wrap(int32_t(idx.x)+g.x1);
    int16_t z0=short_wrap(int32_t(idx.z)+g.z0),z1=short_wrap(int32_t(idx.z)+g.z1);
    const bool special=classifier==uint16_t((family<<4)|5u);
    const bool left=family==2||family==4,bottom=family==2||family==3;
    const bool plus_x=left!=special,plus_z=bottom==special;
    x0=short_wrap(int32_t(x0)+(plus_x?b.x1:-int32_t(b.x1)));
    x1=short_wrap(int32_t(x1)+(plus_x?-int32_t(b.x0):b.x0));
    z0=short_wrap(int32_t(z0)+(plus_z?b.z1:-int32_t(b.z1)));
    z1=short_wrap(int32_t(z1)+(plus_z?-int32_t(b.z0):b.z0));
    const int32_t sx=int32_t(x1)-x0,sz=int32_t(z1)-z0;
    const auto qx=word_div(word_mul(int32_t(z)-z0,sx),sz);
    const auto qz=word_div(word_mul(int32_t(x)-x0,sz),sx);
    if(!qx||!qz)return {};
    int32_t lo_x=x0,hi_x=x1,lo_z=z0,hi_z=z1;
    if(family==2){hi_x=std::min<int32_t>(x1,word_wrap(int64_t(x0)+*qx+4));lo_z=std::max<int32_t>(z0,word_wrap(int64_t(z0)+*qz-4));}
    else if(family==3){lo_x=std::max<int32_t>(x0,word_wrap(int64_t(x1)-*qx-4));lo_z=std::max<int32_t>(z0,word_wrap(int64_t(z1)-*qz-4));}
    else if(family==4){hi_x=std::min<int32_t>(x1,word_wrap(int64_t(x1)-*qx+4));hi_z=std::min<int32_t>(z1,word_wrap(int64_t(z1)-*qz+4));}
    else {lo_x=std::max<int32_t>(x0,word_wrap(int64_t(x0)+*qx-4));hi_z=std::min<int32_t>(z1,word_wrap(int64_t(z0)+*qz+4));}
    Rect out{short_wrap(lo_x),short_wrap(hi_x),short_wrap(lo_z),short_wrap(hi_z),
        short_wrap(int32_t(idx.y)+p.y0-b.y1),short_wrap(int32_t(idx.y)+p.y1-b.y0)};
    if(!inside_xz(out,x,z))return {};
    return out;
}
bool eligible(int16_t correction, int16_t delta, uint8_t blocked, uint8_t positive, uint8_t negative) {
    // 0x80050808/0x800508BC/0x80050970: the correction must fit the
    // per-pass movement magnitude; earlier response-direction bits exclude it.
    return delta!=0 && std::abs(int(correction))<=std::abs(int(delta)) &&
           (blocked & (correction>0 ? positive : negative))==0;
}
// Correction history of 0x8004B6AC, verified against the R3000 code at
// 0x8004BB24..0x8004BC1C. A response records its negated correction in
// 0x1F800100/102/104 (h). Each restart first adds the pending vector
// 0x1F800106/108/10A (u) to the scratch pose, then moves h to u and clears h.
// Responses clear parts of u (same-axis X/Z pushes, floor/ceiling snaps, any
// response returning true), so a previous correction is undone only when the
// next response is on a different axis and does not clear it. The
// 0x1F80010C..116 copies only feed the unported x5 diagonal fallback.
struct History { std::array<int16_t,3> h{}, u{}; };
void clear_pending(History& hist) {hist.u={};}
// 0x80050B74 / 0x80050BB4 / 0x80050BF4: the earlier blocked byte decides
// whether the pending same-axis correction is cleared; then it is replaced.
void block_x(int16_t correction,uint8_t& blocked,History& hist) {
    if(blocked&0x03u)hist.u[0]=0;
    blocked=correction<1?2:1;
}
void block_z(int16_t correction,uint8_t& blocked,History& hist) {
    if(blocked&0x0Cu)hist.u[2]=0;
    blocked=correction<1?8:4;
}
void block_y(int16_t correction,uint8_t& blocked) {blocked=correction>0?16:32;}
// Dispatcher tails after a response: LAB_8004C144 (codes 1/2), case 3,
// LAB_8004C0C8 (0x8004F8BC/FBE8) and LAB_8004E17C (0x8004FF14). A true result
// clears u and, except for case 3, publishes +74 grounded when dy >= 0. The
// diagonal-axis tail also clears u.x/u.z on a false result.
void response_tail(bool result,bool publish_ground,bool clear_xz_on_false,int16_t dy,
                   L3RetailContactScratch& p,History& hist) {
    if(result){clear_pending(hist);if(publish_ground&&dy>=0)p.contact_state=0;}
    else if(clear_xz_on_false){hist.u[0]=0;hist.u[2]=0;}
}
int16_t nearest_face(int16_t lo,int16_t hi,int16_t v) {
    // 0x80050400: |lo-v| < |hi-v| selects lo; a tie selects hi.
    return short_wrap(int32_t(std::abs(int32_t(lo)-v)<std::abs(int32_t(hi)-v)?lo:hi)-v);
}
// Exact 0x80050400 (verified against the original R3000 instructions, not only
// Ghidra). Reached from 0x8004F44C when no per-pass axis correction fits the
// movement delta. Each axis takes its nearest rectangle face; X/Z ties choose
// the high face. With mode bit 2 the Y target is always the top face. Y wins
// only when strictly smaller than both horizontal magnitudes and not blocked
// by 0x30; it records h.y. Otherwise the smaller horizontal magnitude wins.
// The 0x800506B0..0x80050790 branch tree reduces to: an X/Z tie chooses X
// unless X is already blocked (0x03), then Z; the Z-blocked test (0x0C)
// never changes the result. X/Z record no h. The chosen axis moves the
// scratch pose and the pre-move pose (+2C..+30) by the same amount, so the
// per-pass delta is unchanged. Returns the 0x80050BF4 result (Y only).
bool deep_overlap_80050400(const Rect& r,L3RetailContactScratch& p,
                           int16_t& before_x,int16_t& before_y,int16_t& before_z,
                           unsigned mode,uint8_t& blocked,History& hist,
                           RuntimePlayerContact& out) {
    ++out.deep_overlap_recoveries;
    const int16_t cx=nearest_face(r.x0,r.x1,p.x);
    const int16_t cz=nearest_face(r.z0,r.z1,p.z);
    const int16_t cy=(mode&2u)?short_wrap(int32_t(r.y0)-p.y):nearest_face(r.y0,r.y1,p.y);
    const int ax=std::abs(int(cx)),az=std::abs(int(cz));
    if((mode&1u) && std::abs(int(cy))<std::min(ax,az) && (blocked&0x30u)==0){
        p.y=short_wrap(int32_t(p.y)+cy);before_y=short_wrap(int32_t(before_y)+cy);
        hist.h[1]=short_wrap(-int32_t(cy));block_y(cy,blocked);
        if(cy>0)out.ceiling_contact=true;
        return cy<=0;
    }
    const bool x_axis=ax<az||(ax==az&&(blocked&0x03u)==0);
    if(x_axis){p.x=short_wrap(int32_t(p.x)+cx);before_x=short_wrap(int32_t(before_x)+cx);block_x(cx,blocked,hist);}
    else {p.z=short_wrap(int32_t(p.z)+cz);before_z=short_wrap(int32_t(before_z)+cz);block_z(cz,blocked,hist);}
    out.wall_contact=true;
    return false;
}
// 0x8004F44C for dispatcher modes 1 (code 1) and 3 (airborne-upward code 3).
// Mode bit 2 keeps the Y target on the top face even while moving up. Only
// the branch with both horizontal axes eligible records h.x/h.z; every Y
// push records h.y except that branch in mode 3 (0x8004F7B0). Every response
// restarts the candidate scan.
bool generic_rectangle_response(Rect r, L3RetailContactScratch& p,
                                int16_t& before_x,int16_t& before_y,int16_t& before_z,
                                int16_t dx,int16_t dy,int16_t dz,uint8_t& blocked,
                                unsigned mode,History& hist,RuntimePlayerContact& out) {
    const int16_t cx=short_wrap((dx<0?r.x1:r.x0)-p.x);
    const int16_t cy=short_wrap((dy<0&&!(mode&2u)?r.y1:r.y0)-p.y);
    const int16_t cz=short_wrap((dz<0?r.z1:r.z0)-p.z);
    const bool ex=eligible(cx,dx,blocked,2,1);
    const bool ez=eligible(cz,dz,blocked,8,4);
    const bool ey=eligible(cy,dy,blocked,32,16);
    bool result=false;
    if (!ex && !ez && !ey) {
        result=deep_overlap_80050400(r,p,before_x,before_y,before_z,mode,blocked,hist,out);
    } else {
        // Exact 0x8004F44C tie priority: horizontal ties choose Z. With both
        // horizontal axes eligible, Y must be strictly smaller; with only one
        // eligible horizontal axis, Y wins an equal-magnitude tie.
        const bool both=ex&&ez;
        int axis=0;
        if (!ex) axis=ez?2:1;
        else if(!ez) axis=0;
        else axis=std::abs(int(cx))<std::abs(int(cz))?0:2;
        const int horizontal=axis==0?std::abs(int(cx)):std::abs(int(cz));
        if(ey && ((!ex&&!ez)||(both?std::abs(int(cy))<horizontal:std::abs(int(cy))<=horizontal))) axis=1;
        if(axis==0){p.x=short_wrap(int32_t(p.x)+cx);if(both)hist.h[0]=short_wrap(-int32_t(cx));block_x(cx,blocked,hist);out.wall_contact=true;}
        else if(axis==2){p.z=short_wrap(int32_t(p.z)+cz);if(both)hist.h[2]=short_wrap(-int32_t(cz));block_z(cz,blocked,hist);out.wall_contact=true;}
        else {
            p.y=short_wrap(int32_t(p.y)+cy);if(!both||!(mode&2u))hist.h[1]=short_wrap(-int32_t(cy));
            block_y(cy,blocked);result=cy<=0;
            if(!result)out.ceiling_contact=true;
        }
    }
    // 0x8004BDC8 case 1 reaches LAB_8004C144 and returns true after every
    // 0x8004F44C response, so 0x8004B6AC (LAB_8004BB24) restarts the whole
    // candidate scan. Through 0.0.14.51 a wall/ceiling push ended the pass,
    // so a corrected pose never re-tested the floor cell it was pushed into:
    // landing against a wall corner sank Mega Man through the floor.
    response_tail(result,mode==1u,false,dy,p,hist);
    return true;
}
bool diagonal_project_8004ff14(const Rect& r,L3RetailContactScratch& p,
                               int16_t dy,uint8_t& blocked,unsigned mode,
                               History& hist,RuntimePlayerContact& out) {
    int32_t ax=0,az=0;
    if(mode==0){ax=int32_t(r.x1)-short_wrap(int32_t(p.x)-4);az=int32_t(r.z0)-short_wrap(int32_t(p.z)+4);}
    else if(mode==1){ax=int32_t(r.x0)-short_wrap(int32_t(p.x)+4);az=int32_t(r.z0)-short_wrap(int32_t(p.z)+4);}
    else if(mode==2){ax=int32_t(r.x1)-short_wrap(int32_t(p.x)-4);az=int32_t(r.z1)-short_wrap(int32_t(p.z)-4);}
    else if(mode==3){ax=int32_t(r.x0)-short_wrap(int32_t(p.x)+4);az=int32_t(r.z1)-short_wrap(int32_t(p.z)-4);}
    else return false;
    const int32_t denom=word_wrap(int64_t(word_mul(ax,ax))+word_mul(az,az));
    const auto cx32=word_div(word_mul(word_mul(ax,az),az),denom);
    const auto cz32=word_div(word_mul(word_mul(ax,az),ax),denom);
    if(!cx32||!cz32)return false;
    const int16_t cx=short_wrap(*cx32),cz=short_wrap(*cz32);
    const int16_t cy=short_wrap(int32_t(dy>0?r.y0:r.y1)-p.y);
    bool result=false;
    if(dy!=0&&std::abs(int(cy))<=std::abs(int(cx))&&std::abs(int(cy))<=std::abs(int(cz))){
        // 0x8004FF14 stores blocked 0x20 and returns 1 for either Y side.
        p.y=short_wrap(int32_t(p.y)+cy);blocked=0x20;hist.h[1]=short_wrap(-int32_t(cy));result=true;
        if(cy>0)out.ceiling_contact=true;
    }else{
        p.x=short_wrap(int32_t(p.x)+cx);p.z=short_wrap(int32_t(p.z)+cz);
        block_x(cx,blocked,hist);block_z(cz,blocked,hist);out.wall_contact=true;
    }
    response_tail(result,true,false,dy,p,hist);
    return true;
}
// 0x8004FBE8 (x_axis) / 0x8004F8BC (z axis). The eligible branch records h;
// the both-ineligible nearest-face branch shifts the pre-move pose instead.
bool diagonal_axis_response(const Rect& r,L3RetailContactScratch& p,
                            int16_t& before_axis,int16_t& before_y,int16_t delta_axis,int16_t dy,
                            uint8_t& blocked,bool x_axis,bool high_side,
                            History& hist,RuntimePlayerContact& out) {
    const int16_t ca=short_wrap(int32_t(x_axis?(high_side?r.x1:r.x0):(high_side?r.z1:r.z0))-
                                (x_axis?p.x:p.z));
    const int16_t cy=short_wrap(int32_t(dy>=0?r.y0:r.y1)-p.y);
    const bool ea=eligible(ca,delta_axis,blocked,x_axis?2:8,x_axis?1:4);
    const bool ey=eligible(cy,dy,blocked,32,16);
    int16_t& pa=x_axis?p.x:p.z;
    const auto push_axis=[&](int16_t c,bool record){
        pa=short_wrap(int32_t(pa)+c);
        if(record)hist.h[x_axis?0:2]=short_wrap(-int32_t(c));
        else before_axis=short_wrap(int32_t(before_axis)+c);
        if(x_axis)block_x(c,blocked,hist);else block_z(c,blocked,hist);out.wall_contact=true;
    };
    bool result=false;
    if(!ea&&!ey){
        const int16_t near_a=nearest_face(x_axis?r.x0:r.z0,x_axis?r.x1:r.z1,pa);
        const int16_t near_y=nearest_face(r.y0,r.y1,p.y);
        if(std::abs(int(near_y))<std::abs(int(near_a))){
            p.y=short_wrap(int32_t(p.y)+near_y);before_y=short_wrap(int32_t(before_y)+near_y);
            block_y(near_y,blocked);result=near_y<=0;if(!result)out.ceiling_contact=true;
        }
        else push_axis(near_a,false);
    }else if(ey&&std::abs(int(cy))<std::abs(int(ca))){
        p.y=short_wrap(int32_t(p.y)+cy);hist.h[1]=short_wrap(-int32_t(cy));
        block_y(cy,blocked);result=cy<=0;if(!result)out.ceiling_contact=true;
    }
    else push_axis(ca,true);
    response_tail(result,true,true,dy,p,hist);
    return true;
}
bool diagonal_response(uint16_t code,const Rect& r,L3RetailContactScratch& p,
                       int16_t& before_x,int16_t& before_y,int16_t& before_z,
                       int16_t dx,int16_t dy,int16_t dz,
                       int32_t span_x,int32_t span_z,const RetailPlayerBounds& b,
                       uint8_t& blocked,History& hist,RuntimePlayerContact& out) {
    const unsigned family=(code&0xf0u)>>4;
    if(family<2||family>5||((code&0x0fu)!=1u&&(code&0x0fu)!=2u))return false;
    const int32_t adx=std::abs(int(dx)),adz=std::abs(int(dz));
    const int32_t lhs=word_mul(adx+(family==3?-int32_t(b.x0):int32_t(b.x1)),span_x);
    const int32_t rhs=word_mul(adz+((family==4)?int32_t(b.z1):-int32_t(b.z0)),span_z);
    const auto zr=[&](bool high){return diagonal_axis_response(r,p,before_z,before_y,dz,dy,blocked,false,high,hist,out);};
    const auto xr=[&](bool high){return diagonal_axis_response(r,p,before_x,before_y,dx,dy,blocked,true,high,hist,out);};
    if(family==2){if(lhs>rhs&&(blocked&1))return zr(false);if(rhs>lhs&&(blocked&8))return xr(true);}
    else if(family==3){if(lhs>rhs&&(blocked&2))return zr(false);if(rhs>lhs&&(blocked&8))return xr(false);}
    else if(family==4){if(lhs>rhs&&(blocked&1))return zr(true);if(rhs>lhs&&(blocked&4))return xr(true);}
    else {if(lhs>rhs&&(blocked&2))return zr(true);if(rhs>lhs&&(blocked&4))return xr(false);}
    return diagonal_project_8004ff14(r,p,dy,blocked,family-2,hist,out);
}
bool allowed(const RuntimeContactPrimitive& p,const L3PlayerCompatWork& player) {
    // Geometry response 0x8004BA98..BAA8 uses only the 0x8000 filter bit.
    const uint16_t filter=l3_obj_group_bits_8002fe74(player.bytes[1])==0?0:0x8000;
    return (p.flags & filter & 0x8000u)==0;
}
void find_floor(const RuntimeMap& map,size_t area,const L3PlayerCompatWork& w,
                RetailPlayerBounds b,RuntimePlayerContact& out) {
    const auto c=gather(map,area,out.scratch.x,out.scratch.z);
    int32_t best=32767;
    const int32_t dy=int32_t(rd(w,0x16))-rd(w,0x2E);
    // Source floor query repeats candidates until its minimum is stable.
    // The only dependency is the current minimum, so at most four passes are
    // needed for the bounded supported subset; no vectors/triangles allocated.
    for(unsigned pass=0;pass<4;++pass){
        const int32_t previous=best;
        for(uint8_t rev=c.size;rev>0;--rev){
            const auto& cell=c.cells[rev-1];
            if(cell.packed_cell<0){
                const auto containing=runtime_map_find_cell(map,area,out.scratch.x,out.scratch.z);
                if(!containing || containing->cell_index!=cell.cell_index)continue;
                const int32_t v=-int32_t((uint16_t(cell.packed_cell)&0x1F00u)>>1)-1-b.y1;
                if(v<best){best=v;out.floor_cell_index=cell.cell_index;out.floor_primitive_index.reset();}
                continue;
            }
            const auto* idx=runtime_map_cell_idx(map,cell);
            const auto* list=runtime_map_cell_contact_list(map,cell);
            if(!idx||!list)continue;
            for(uint32_t gi=0;gi<list->group_count;++gi){
                const auto gix=uint64_t(list->group_first)+gi;
                if(gix>=map.contact_groups.size())continue;
                const auto& g=map.contact_groups[static_cast<size_t>(gix)];
                if(!g.primitive_count)continue;
                if(g.primitive_first>=map.contact_primitives.size())continue;
                const auto& first=map.contact_primitives[g.primitive_first];
                if(first.code>=256u || (first.code&0xF0u)>=0x20u)continue;
                for(uint32_t pi=0;pi<g.primitive_count;++pi){
                    const auto pix=uint64_t(g.primitive_first)+pi;
                    if(pix>=map.contact_primitives.size())continue;
                    const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
                    if(!allowed(p,w))continue;
                    const auto interval=runtime_contact_floor_interval_8004a060(
                        *idx,g,p,b,out.scratch.x,out.scratch.z);
                    if(!interval)continue;
                    const int32_t compare=dy<0?out.scratch.y:std::min<int32_t>(out.scratch.y,best);
                    if(interval->y0<best && compare<interval->y1){
                        best=interval->y0;out.floor_cell_index=cell.cell_index;
                        out.floor_primitive_index=static_cast<uint32_t>(pix);
                    }
                }
            }
        }
        if(previous==best)break;
    }
    if(best!=32767){out.floor_y=short_wrap(best);out.grounded=out.scratch.contact_state==0&&out.scratch.y==*out.floor_y;}
}
} // namespace

std::optional<RuntimeContactFloorInterval> runtime_contact_floor_interval_8004a060(
    const RuntimeMapIdx& idx, const RuntimeContactGroup& group,
    const RuntimeContactPrimitive& primitive, RetailPlayerBounds bounds,
    int16_t query_x, int16_t query_z) {
    const auto code=primitive.code;
    if(code!=1u && code!=3u && code!=15u && (code<8u || code>11u))return {};
    auto r=rect(idx,group,primitive,bounds);
    if(!inside_xz(r,query_x,query_z))return {};
    if(code<8u || code==15u)return RuntimeContactFloorInterval{r.y0,r.y1};

    // MIPS MULT retains its low signed 32 bits. AD(D)U also wraps before the
    // subsequent signed division. Preserve those operations explicitly rather
    // than allowing signed-overflow UB or widening the source arithmetic.
    const auto wrap32=[](int64_t v){return static_cast<int32_t>(static_cast<uint32_t>(v));};
    const auto multiply=[&](int32_t a,int32_t b){return wrap32(int64_t(a)*b);};
    const auto divide=[](int32_t a,int32_t b)->std::optional<int32_t>{
        if(!b || (a==std::numeric_limits<int32_t>::min() && b==-1))return {};
        return a/b;
    };
    const int32_t raw_y0=short_wrap(int32_t(idx.y)+primitive.y0);
    const int32_t raw_y1=short_wrap(int32_t(idx.y)+primitive.y1);
    const bool along_z=code==8u || code==9u;
    const int32_t span=along_z?int32_t(r.z1)-r.z0:int32_t(r.x1)-r.x0;
    const int32_t bounds_delta=along_z?int32_t(bounds.z0)-bounds.z1:
                                          int32_t(bounds.x0)-bounds.x1;
    const auto expansion=divide(multiply(raw_y1-raw_y0,bounds_delta),span);
    if(!expansion)return {};
    const int32_t adjusted_y0=wrap32(int64_t(raw_y0)-*expansion);
    const int32_t distance=code==8u?int32_t(r.z1)-query_z:
                           code==9u?int32_t(query_z)-r.z0:
                           code==10u?int32_t(query_x)-r.x0:int32_t(r.x1)-query_x;
    const auto interpolation=divide(multiply(wrap32(int64_t(raw_y1)-adjusted_y0),distance),span);
    if(!interpolation)return {};
    const int32_t floor=std::max(raw_y0,wrap32(int64_t(adjusted_y0)+*interpolation));
    return RuntimeContactFloorInterval{short_wrap(wrap32(int64_t(floor)-bounds.y1)),r.y1};
}

RuntimePlayerContact runtime_player_floor_query(const RuntimeMap& map,size_t area,
                                               const L3PlayerCompatWork& w,RetailPlayerBounds b) {
    RuntimePlayerContact out;
    out.scratch={rd(w,0x14),rd(w,0x16),rd(w,0x18),w.bytes[0x74]};
    out.candidate_cells=gather(map,area,out.scratch.x,out.scratch.z).size;
    if(out.candidate_cells)out.status=RuntimeContactStatus::SourceBackedPartial;
    find_floor(map,area,w,b,out);
    return out;
}

RuntimePlayerContact runtime_player_contact_step(const RuntimeMap& map,size_t area,
                                                const L3PlayerCompatWork& w,RetailPlayerBounds b) {
    RuntimePlayerContact out;
    const L3RetailContactScratch requested{rd(w,0x14),rd(w,0x16),rd(w,0x18),w.bytes[0x74]};
    out.scratch=requested;
    int16_t before_x=rd(w,0x2C),before_z=rd(w,0x30);
    int16_t before_y=rd(w,0x2E);
    const auto candidates=gather(map,area,requested.x,requested.z);
    out.candidate_cells=candidates.size;
    if(!candidates.size)return out;
    out.status=RuntimeContactStatus::SourceBackedPartial;
    uint8_t blocked=0;
    History hist;
    for(unsigned pass=0;pass<9;++pass){
        out.iterations=static_cast<uint8_t>(pass+1);
        bool repeat=false;
        for(uint8_t rev=candidates.size;rev>0&&!repeat;--rev){
            const auto& cell=candidates.cells[rev-1];
            const auto dx=short_wrap(int32_t(out.scratch.x)-before_x);
            const auto dy=short_wrap(int32_t(out.scratch.y)-before_y);
            const auto dz=short_wrap(int32_t(out.scratch.z)-before_z);
            if(cell.packed_cell<0){
                const auto containing=runtime_map_find_cell(map,area,out.scratch.x,out.scratch.z);
                if(!containing||containing->cell_index!=cell.cell_index)continue;
                const int32_t top=-int32_t((uint16_t(cell.packed_cell)&0x1F00u)>>1);
                const int16_t floor=short_wrap(top-1-b.y1);
                if(floor<out.scratch.y){out.scratch.y=floor;if(dy>0)out.scratch.contact_state=0;clear_pending(hist);repeat=true;}
                else if((uint16_t(cell.packed_cell)&0x6000u)!=0u){
                    const int32_t bands=(uint16_t(cell.packed_cell)&0x6000u)>>13;
                    const int16_t ceiling=short_wrap(top-bands*384+1-b.y0);
                    if(out.scratch.y<ceiling){out.scratch.y=ceiling;out.ceiling_contact=true;clear_pending(hist);repeat=true;}
                }
                continue;
            }
            const auto* idx=runtime_map_cell_idx(map,cell);
            const auto* list=runtime_map_cell_contact_list(map,cell);
            if(!idx||!runtime_map_cell_descriptor(map,cell)){++out.unsupported_primitives;continue;}
            // 0x8004B9B8..: the selected descriptor list (+8 default, +4 alt) is
            // walked only while its first group word at +8 is nonzero. A null
            // list pointer (cooked 0xFFFF) contributes no geometry; it is not
            // an unsupported primitive. Premise: the PS1 word read through the
            // null list (kernel RAM 0x00000008) is zero, as every ST00 cell with
            // a null selected list relies on it (694 of 994 IDX entries).
            if(!list)continue;
            for(uint32_t gi=0;gi<list->group_count&&!repeat;++gi){
                const auto gix=uint64_t(list->group_first)+gi;
                if(gix>=map.contact_groups.size()){++out.unsupported_primitives;continue;}
                const auto& g=map.contact_groups[static_cast<size_t>(gix)];
                ++out.groups_tested;
                if(!g.primitive_count)continue;
                if(g.primitive_first>=map.contact_primitives.size()){++out.unsupported_primitives;continue;}
                const auto first_code=map.contact_primitives[g.primitive_first].code;
                const unsigned first_family=(first_code&0xf0u)>>4;
                if(first_family>=2&&first_family<=5){
                    if(g.x0==g.x1||g.z0==g.z1){++out.unsupported_primitives;continue;}
                    const auto classified=diagonal_rect(*idx,g,map.contact_primitives[g.primitive_first],
                        first_code,b,out.scratch.x,out.scratch.z);
                    if(!classified)continue;
                }
                for(uint32_t pi=0;pi<g.primitive_count&&!repeat;++pi){
                    const auto pix=uint64_t(g.primitive_first)+pi;
                    if(pix>=map.contact_primitives.size()){++out.unsupported_primitives;continue;}
                    const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
                    ++out.primitives_tested;
                    if(!allowed(p,w))continue;
                    const auto diagonal=first_family>=2&&first_family<=5;
                    const auto clipped=diagonal?diagonal_rect(*idx,g,p,first_code,b,out.scratch.x,out.scratch.z):std::optional<Rect>{};
                    const auto r=clipped?*clipped:rect(*idx,g,p,b);
                    if(!inside(r,out.scratch))continue;
                    if(first_code>=256u)continue; // exact dispatcher admission
                    if(diagonal){
                        if(diagonal_response(p.code,r,out.scratch,before_x,before_y,before_z,dx,dy,dz,
                            int32_t(g.x1)-g.x0,int32_t(g.z1)-g.z0,b,blocked,hist,out))repeat=true;
                        else {++out.unsupported_primitives;if(!out.unsupported_code)out.unsupported_code=uint16_t(p.code|0x100u);}
                        continue;
                    }
                    if((first_code&0xF0u)>=0x20u || (p.code!=1u&&p.code!=3u&&p.code!=15u)){
                        ++out.unsupported_primitives;if(!out.unsupported_code)out.unsupported_code=p.code;continue;
                    }
                    if(p.code==3u){
                        if(dy>=0 || out.scratch.contact_state==0){out.scratch.y=r.y0;out.scratch.contact_state=0;clear_pending(hist);repeat=true;}
                        else repeat=generic_rectangle_response(r,out.scratch,before_x,before_y,before_z,
                                                                dx,dy,dz,blocked,3u,hist,out);
                    }else if(p.code==15u){out.scratch.y=r.y1;out.ceiling_contact=true;clear_pending(hist);repeat=true;}
                    else if(out.scratch.contact_state==0 && int32_t(out.scratch.y)-r.y0<8 && !blocked){
                        out.scratch.y=r.y0;clear_pending(hist);repeat=true;
                    }else repeat=generic_rectangle_response(r,out.scratch,before_x,before_y,before_z,
                                                          dx,dy,dz,blocked,1u,hist,out);
                }
            }
        }
        if(!repeat)break;
        if(pass==8){out.status=RuntimeContactStatus::IterationLimit;out.result=static_cast<int32_t>(0x80000000u);return out;}
        // LAB_8004BB24: apply the pending undo, then promote this response's h.
        out.scratch.x=short_wrap(int32_t(out.scratch.x)+hist.u[0]);
        out.scratch.y=short_wrap(int32_t(out.scratch.y)+hist.u[1]);
        out.scratch.z=short_wrap(int32_t(out.scratch.z)+hist.u[2]);
        hist.u=hist.h;hist.h={};
    }
    out.result=l3_collision_no_contact_result_8004bc2c(requested.x,requested.y,requested.z,
                                                     out.scratch.x,out.scratch.y,out.scratch.z);
    find_floor(map,area,w,b,out);
    if(out.unsupported_primitives){out.status=RuntimeContactStatus::Unsupported;out.grounded.reset();}
    out.landed=out.grounded.value_or(false)&&w.bytes[0x74]!=0;
    return out;
}

RuntimeEventPrimitiveQuery runtime_player_event_primitive_800514fc(
    const RuntimeMap& map,size_t area,const L3PlayerCompatWork& w,
    const ReferenceTables* trig,RetailPlayerBounds b) {
    RuntimeEventPrimitiveQuery out;
    const int16_t px=rd(w,0x14),py=rd(w,0x16),pz=rd(w,0x18),heading=rd(w,0x56);
    const auto c=gather(map,area,px,pz);
    out.candidate_cells=c.size;
    if(!c.size)return out;
    out.status=RuntimeEventQueryStatus::Miss;
    // 0x800515A4..0x800515F4: RotMatrixY(heading) then ApplyMatrixSV on
    // (0,0,-64). The GTE result is (m*v)>>12 (arithmetic), m02=sin, m22=cos.
    const auto front=[&](int16_t& fx,int16_t& fz){
        const auto& t=trig->trig[uint16_t(heading)&0x0FFFu];
        fx=short_wrap(int32_t(px)+((int32_t(t.sin_q12)*-64)>>12));
        fz=short_wrap(int32_t(pz)+((int32_t(t.cos_q12)*-64)>>12));
    };
    for(uint8_t rev=c.size;rev>0;--rev){
        const auto& cell=c.cells[rev-1];
        // 0x80048CEC returns the packed cell; zero/negative cells are skipped.
        if(cell.packed_cell<=0)continue;
        const auto* idx=runtime_map_cell_idx(map,cell);
        const auto* list=runtime_map_cell_contact_list(map,cell);
        if(!idx||!list)continue;
        for(uint32_t gi=0;gi<list->group_count;++gi){
            const auto gix=uint64_t(list->group_first)+gi;
            if(gix>=map.contact_groups.size())continue;
            const auto& g=map.contact_groups[static_cast<size_t>(gix)];
            if(!g.primitive_count||g.primitive_first>=map.contact_primitives.size())continue;
            const auto& first=map.contact_primitives[g.primitive_first];
            if(first.code<256u)continue;
            ++out.event_groups_tested;
            int16_t qx=px,qz=pz;
            if(first.flags&0x8000u){
                if(!trig){out.status=RuntimeEventQueryStatus::NeedsTrig;out.unsupported_code=first.code;return out;}
                front(qx,qz);
            }
            // Dispatcher 0x8008AA24: families 0/1 are 0x8004E304, whose
            // nonzero result is exactly the strict expanded-rectangle test.
            if((first.code&0xF0u)>=0x20u){
                out.status=RuntimeEventQueryStatus::Unsupported;out.unsupported_code=first.code;return out;
            }
            const Rect r=rect(*idx,g,first,b);
            if(!inside_xz(r,qx,qz))continue;
            for(uint32_t pi=0;pi<g.primitive_count;++pi){
                const auto pix=uint64_t(g.primitive_first)+pi;
                if(pix>=map.contact_primitives.size())break;
                const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
                const unsigned low=p.code&0xFFu;
                if(low>=8u&&low<=85u){
                    out.status=RuntimeEventQueryStatus::Unsupported;out.unsupported_code=p.code;return out;
                }
                // 0x800518A4..C4 subtract the bounds, then the default
                // 0x8004AC54..88 tail of 0x8004A060 subtracts them again.
                const Rect pr=rect(*idx,g,p,b);
                const int16_t lo=short_wrap(int32_t(pr.y0)-b.y1),hi=short_wrap(int32_t(pr.y1)-b.y0);
                if(!(lo<py&&py<hi))continue;
                RuntimeEventPrimitiveSample s;
                s.x=short_wrap(int32_t(idx->x)+(int32_t(g.x0)+g.x1)/2);
                s.y=short_wrap(int32_t(idx->y)+(int32_t(p.y0)+p.y1)/2);
                s.z=short_wrap(int32_t(idx->z)+(int32_t(g.z0)+g.z1)/2);
                s.heading=uint16_t(heading);s.type=uint16_t(p.code>>8);s.flags=p.flags;
                s.cell_index=cell.cell_index;s.primitive_index=static_cast<uint32_t>(pix);
                out.sample=s;out.status=RuntimeEventQueryStatus::Hit;return out;
            }
        }
    }
    return out;
}

RuntimePointEmbeddingResult runtime_point_embedded_80049c54(const RuntimeMap& map,
    size_t area_index, int16_t x, int16_t y, int16_t z, RetailPlayerBounds b,
    bool skip_flag_8000) noexcept {
    RuntimePointEmbeddingResult out;
    if(area_index>=map.areas.size())return out;
    const auto& a=map.areas[area_index];
    // 0x80048C60: the packed cell under the point; none or zero = 0x40000000.
    const auto cell_at=[&](int32_t px,int32_t pz)->std::optional<uint32_t>{
        const uint32_t gx=uint32_t((px+0x8000)>>9)-uint32_t(uint16_t(a.start_x));
        const uint32_t gz=uint32_t((pz+0x8000)>>9)-uint32_t(uint16_t(a.start_z));
        if(gx>=a.width||gz>=a.height)return {};
        const uint64_t ci=uint64_t(a.cell_first)+uint64_t(gz)*a.width+gx;
        if(ci>=map.cells.size())return {};
        return static_cast<uint32_t>(ci);
    };
    const auto here=cell_at(x,z);
    if(!here||map.cells[*here]==0){out.status=RuntimePointEmbedding::Embedded;out.raw=0x40000000u;return out;}
    // 0x8004B1E0 mode 1: four probes, stop at the first one off the grid,
    // positive cells deduplicated by value, empty and negative cells kept.
    std::array<uint32_t,4> candidates{};uint8_t count=0;
    const int32_t nx=(uint16_t(x)&0x1ffu)>0x100u?0x8200:0x7e00;
    const int32_t nz=(uint16_t(z)&0x1ffu)>0x100u?0x8200:0x7e00;
    const std::array<std::array<int32_t,2>,4> offsets{{{{0x8000,0x8000}},{{nx,0x8000}},{{0x8000,nz}},{{nx,nz}}}};
    for(const auto& o:offsets){
        const uint32_t gx=uint32_t((int32_t(x)+o[0])>>9)-uint32_t(uint16_t(a.start_x));
        if(gx>=a.width)break;
        const uint32_t gz=uint32_t((int32_t(z)+o[1])>>9)-uint32_t(uint16_t(a.start_z));
        if(gz>=a.height)break;
        const uint64_t ci=uint64_t(a.cell_first)+uint64_t(gz)*a.width+gx;
        if(ci>=map.cells.size())break;
        const int16_t packed=map.cells[static_cast<size_t>(ci)];
        bool duplicate=false;
        if(packed>0)for(uint8_t i=0;i<count;++i)if(map.cells[candidates[i]]==packed)duplicate=true;
        if(!duplicate)candidates[count++]=static_cast<uint32_t>(ci);
    }
    for(uint8_t rev=count;rev>0;--rev){
        const uint32_t ci=candidates[rev-1];
        const int16_t packed=map.cells[ci];
        if(packed<1){
            if(ci!=*here)continue;
            const int32_t top=-int32_t((uint16_t(packed)&0x1F00u)>>1);
            const int32_t bands=(uint16_t(packed)&0x6000u)>>13;
            if(int32_t(y)<=top-1-b.y1&&(bands==0||top-bands*0x180+1-b.y0<=int32_t(y)))continue;
            out.raw=uint16_t(packed);
            out.status=out.raw?RuntimePointEmbedding::Embedded:RuntimePointEmbedding::Free;
            return out;
        }
        RuntimeCellRef cell;cell.area_index=area_index;cell.cell_index=ci;cell.packed_cell=packed;
        const auto* idx=runtime_map_cell_idx(map,cell);
        const auto* list=runtime_map_cell_contact_list(map,cell);
        if(!idx||!list)continue;
        for(uint32_t gi=0;gi<list->group_count;++gi){
            const auto gix=uint64_t(list->group_first)+gi;
            if(gix>=map.contact_groups.size())return out;
            const auto& g=map.contact_groups[static_cast<size_t>(gix)];
            if(!g.primitive_count||g.primitive_first>=map.contact_primitives.size())continue;
            const auto& first=map.contact_primitives[g.primitive_first];
            if(first.code>=0x100u)continue;
            const unsigned family=(first.code&0xf0u)>>4;
            bool classified=false;
            if(family<=1)classified=inside_xz(rect(*idx,g,first,b),x,z);
            else if(family<=5){
                if(g.x0==g.x1||g.z0==g.z1)return out;
                classified=diagonal_rect(*idx,g,first,first.code,b,x,z).has_value();
            } else return out;
            if(!classified)continue;
            for(uint32_t pi=0;pi<g.primitive_count;++pi){
                const auto pix=uint64_t(g.primitive_first)+pi;
                if(pix>=map.contact_primitives.size())return out;
                const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
                if(skip_flag_8000&&(p.flags&0x8000u))continue;
                const unsigned code=p.code&0xffu;
                int32_t y0=0,y1=0;
                if(code>=8u&&code<=11u){
                    const auto interval=runtime_contact_floor_interval_8004a060(*idx,g,p,b,x,z);
                    if(!interval)return out;
                    y0=interval->y0;y1=interval->y1;
                } else if((code>=0x0cu&&code<=0x0eu)||(code>=0x10u&&code<=0x13u)||(code>=0x18u&&code<=0x1bu)||
                          code==0x24u||code==0x34u||code==0x44u||code==0x54u||
                          code==0x25u||code==0x35u||code==0x45u||code==0x55u){
                    return out; // unported 0x8004A060 case
                } else {
                    // 0x8004A060 default: subtract the bounds from the Y range.
                    y0=short_wrap(int32_t(idx->y)+p.y0-b.y1);
                    y1=short_wrap(int32_t(idx->y)+p.y1-b.y0);
                }
                if(y0<y&&y<y1){
                    out.status=RuntimePointEmbedding::Embedded;
                    out.raw=uint32_t(uint16_t(packed))|(uint32_t(p.flags&0xfu)<<16);
                    return out;
                }
            }
        }
    }
    out.status=RuntimePointEmbedding::Free;out.raw=0;
    return out;
}

std::optional<int32_t> runtime_point_floor_8004920c(const RuntimeMap& map,
    size_t area_index, int16_t x, int16_t y, int16_t z) noexcept {
    if(area_index>=map.areas.size())return {};
    const auto& a=map.areas[area_index];
    // 0x80048C60: off the grid reads as cell 0.
    int16_t packed=0;std::optional<uint32_t> index;
    const uint32_t gx=uint32_t((int32_t(x)+0x8000)>>9)-uint32_t(uint16_t(a.start_x));
    const uint32_t gz=uint32_t((int32_t(z)+0x8000)>>9)-uint32_t(uint16_t(a.start_z));
    if(gx<a.width&&gz<a.height){
        const uint64_t ci=uint64_t(a.cell_first)+uint64_t(gz)*a.width+gx;
        if(ci>=map.cells.size())return {};
        index=uint32_t(ci);packed=map.cells[static_cast<size_t>(ci)];
    }
    if(packed<1)return ~(int32_t(uint16_t(packed)&0x1F00u)>>1);
    RuntimeCellRef cell;cell.area_index=area_index;cell.cell_index=*index;cell.packed_cell=packed;
    const auto* idx=runtime_map_cell_idx(map,cell);
    const auto* list=runtime_map_cell_contact_list(map,cell);
    if(!idx||!list)return {};
    // Scratch bounds 0x1F800124..2C are zeroed before the scan.
    const RetailPlayerBounds zero{0,0,0,0,0,0};
    int32_t best=0x7fff;
    for(uint32_t gi=0;gi<list->group_count;++gi){
        const auto gix=uint64_t(list->group_first)+gi;
        if(gix>=map.contact_groups.size())return {};
        const auto& g=map.contact_groups[static_cast<size_t>(gix)];
        if(!g.primitive_count||g.primitive_first>=map.contact_primitives.size())continue;
        const auto& first=map.contact_primitives[g.primitive_first];
        // Dispatcher 0x8008AA24 on the first code: families 0/1 are the
        // rectangle test; others are not ported here.
        if((first.code&0xF0u)>=0x20u)return {};
        if(!inside_xz(rect(*idx,g,first,zero),x,z))continue;
        for(uint32_t pi=0;pi<g.primitive_count;++pi){
            const auto pix=uint64_t(g.primitive_first)+pi;
            if(pix>=map.contact_primitives.size())return {};
            const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
            const unsigned code=p.code&0xffu;
            int32_t top=0,bottom=0;
            if(code>=8u&&code<=11u){
                const auto interval=runtime_contact_floor_interval_8004a060(*idx,g,p,zero,x,z);
                if(!interval)return {};
                top=interval->y0;bottom=interval->y1;
            } else if((code>=0x0cu&&code<=0x0eu)||(code>=0x10u&&code<=0x13u)||(code>=0x18u&&code<=0x1bu)||
                      code==0x24u||code==0x34u||code==0x44u||code==0x54u||
                      code==0x25u||code==0x35u||code==0x45u||code==0x55u){
                return {}; // unported 0x8004A060 case
            } else {
                top=short_wrap(int32_t(idx->y)+p.y0);bottom=short_wrap(int32_t(idx->y)+p.y1);
            }
            if(top<best&&y<bottom)best=top;
        }
    }
    return best;
}

const char* runtime_contact_status_name(RuntimeContactStatus status) {
    switch(status){
        case RuntimeContactStatus::NoData:return "UNKNOWN";
        case RuntimeContactStatus::SourceBackedPartial:return "SOURCE_BACKED_PARTIAL";
        case RuntimeContactStatus::Unsupported:return "UNKNOWN";
        case RuntimeContactStatus::IterationLimit:return "UNKNOWN";
    }
    return "UNKNOWN";
}
} // namespace l3
