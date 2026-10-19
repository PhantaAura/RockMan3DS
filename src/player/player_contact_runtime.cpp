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
bool eligible(int16_t correction, int16_t delta, uint8_t blocked, uint8_t positive, uint8_t negative) {
    // 0x80050808/0x800508BC/0x80050970: the correction must fit the
    // per-pass movement magnitude; earlier response-direction bits exclude it.
    return delta!=0 && std::abs(int(correction))<=std::abs(int(delta)) &&
           (blocked & (correction>0 ? positive : negative))==0;
}
bool generic_rectangle_response(Rect r, L3RetailContactScratch& p,
                                int16_t dx,int16_t dy,int16_t dz,uint8_t& blocked,
                                RuntimePlayerContact& out) {
    const int16_t cx=short_wrap((dx<0?r.x1:r.x0)-p.x);
    const int16_t cy=short_wrap((dy<0?r.y1:r.y0)-p.y);
    const int16_t cz=short_wrap((dz<0?r.z1:r.z0)-p.z);
    const bool ex=eligible(cx,dx,blocked,2,1);
    const bool ez=eligible(cz,dz,blocked,8,4);
    const bool ey=eligible(cy,dy,blocked,32,16);
    if (!ex && !ez && !ey) {
        // The source invokes the still-untranslated 0x80050400 here. Never
        // replace that deep-overlap recovery with a guessed shortest-axis push.
        ++out.unsupported_primitives;
        return false;
    }
    // Exact 0x8004F44C tie priority: horizontal ties choose Z. With both
    // horizontal axes eligible, Y must be strictly smaller; with only one
    // eligible horizontal axis, Y wins an equal-magnitude tie.
    int axis=0;
    if (!ex) axis=ez?2:1;
    else if(!ez) axis=0;
    else axis=std::abs(int(cx))<std::abs(int(cz))?0:2;
    const int horizontal=axis==0?std::abs(int(cx)):std::abs(int(cz));
    if(ey && ((!ex&&!ez)||((ex&&ez)?std::abs(int(cy))<horizontal:std::abs(int(cy))<=horizontal))) axis=1;
    if(axis==0){p.x=short_wrap(int32_t(p.x)+cx);blocked=cx>0?1:2;out.wall_contact=true;}
    else if(axis==2){p.z=short_wrap(int32_t(p.z)+cz);blocked=cz>0?4:8;out.wall_contact=true;}
    else {
        p.y=short_wrap(int32_t(p.y)+cy);blocked=cy>0?16:32;
        if(cy<=0){p.contact_state=0;return true;}
        out.ceiling_contact=true;
    }
    // 0x8004BDC8 case 1 reaches LAB_8004C144 and returns true after every
    // 0x8004F44C response, so 0x8004B6AC (LAB_8004BB24) restarts the whole
    // candidate scan. Through 0.0.14.51 a wall/ceiling push ended the pass,
    // so a corrected pose never re-tested the floor cell it was pushed into:
    // landing against a wall corner sank Mega Man through the floor.
    return true;
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
    const int16_t before_x=rd(w,0x2C),before_y=rd(w,0x2E),before_z=rd(w,0x30);
    const auto candidates=gather(map,area,requested.x,requested.z);
    out.candidate_cells=candidates.size;
    if(!candidates.size)return out;
    out.status=RuntimeContactStatus::SourceBackedPartial;
    uint8_t blocked=0;
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
                if(floor<out.scratch.y){out.scratch.y=floor;if(dy>0)out.scratch.contact_state=0;repeat=true;}
                else if((uint16_t(cell.packed_cell)&0x6000u)!=0u){
                    const int32_t bands=(uint16_t(cell.packed_cell)&0x6000u)>>13;
                    const int16_t ceiling=short_wrap(top-bands*384+1-b.y0);
                    if(out.scratch.y<ceiling){out.scratch.y=ceiling;out.ceiling_contact=true;repeat=true;}
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
                for(uint32_t pi=0;pi<g.primitive_count&&!repeat;++pi){
                    const auto pix=uint64_t(g.primitive_first)+pi;
                    if(pix>=map.contact_primitives.size()){++out.unsupported_primitives;continue;}
                    const auto& p=map.contact_primitives[static_cast<size_t>(pix)];
                    ++out.primitives_tested;
                    if(!allowed(p,w))continue;
                    const auto r=rect(*idx,g,p,b);
                    if(!inside(r,out.scratch))continue;
                    if(first_code>=256u)continue; // exact dispatcher admission
                    if((first_code&0xF0u)>=0x20u || (p.code!=1u&&p.code!=3u&&p.code!=15u)){
                        ++out.unsupported_primitives;continue;
                    }
                    if(p.code==3u){
                        if(dy>=0 || out.scratch.contact_state==0){out.scratch.y=r.y0;out.scratch.contact_state=0;repeat=true;}
                        else {++out.unsupported_primitives;}
                    }else if(p.code==15u){out.scratch.y=r.y1;out.ceiling_contact=true;repeat=true;}
                    else if(out.scratch.contact_state==0 && int32_t(out.scratch.y)-r.y0<8 && !blocked){
                        out.scratch.y=r.y0;repeat=true;
                    }else repeat=generic_rectangle_response(r,out.scratch,dx,dy,dz,blocked,out);
                }
            }
        }
        if(!repeat)break;
        if(pass==8){out.status=RuntimeContactStatus::IterationLimit;out.result=static_cast<int32_t>(0x80000000u);return out;}
    }
    out.result=l3_collision_no_contact_result_8004bc2c(requested.x,requested.y,requested.z,
                                                     out.scratch.x,out.scratch.y,out.scratch.z);
    find_floor(map,area,w,b,out);
    if(out.unsupported_primitives){out.status=RuntimeContactStatus::Unsupported;out.grounded.reset();}
    out.landed=out.grounded.value_or(false)&&w.bytes[0x74]!=0;
    return out;
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
