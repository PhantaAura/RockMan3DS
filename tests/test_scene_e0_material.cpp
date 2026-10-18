#include "legends3ds/scene_e0_material.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace {
template<class F>void rejects(F&& f) {
    bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
    assert(rejected);
}
void put(std::vector<uint8_t>& bytes,size_t at,uint32_t v) {
    for(size_t i=0;i<4;++i)bytes[at+i]=uint8_t(v>>(8*i));
}
void crc(std::vector<uint8_t>& bytes) {put(bytes,20,l3::crc32(bytes.data()+64,bytes.size()-64));}
}
int main(int argc,char** argv) {
    std::array<uint8_t,32> digest{};digest[0]=17;digest[31]=42;
    l3::MaterialCache cache;
    l3::MaterialCacheEntry resolved;resolved.material_word=0x78000000u;resolved.bpp=4;
    resolved.resolved=true;resolved.palette_entries=16;resolved.source_width=256;resolved.source_height=256;
    resolved.indices.resize(256*256);for(size_t i=0;i<resolved.indices.size();++i)resolved.indices[i]=uint8_t(i%16);
    resolved.palette_ps1.resize(16);for(size_t i=0;i<16;++i)resolved.palette_ps1[i]=uint16_t(i*512);
    resolved.palette_ps1[1]=0x8001;cache.entries.push_back(resolved);
    auto abr=resolved;abr.material_word=0x78000020u;abr.abr=1;cache.entries.push_back(abr);
    l3::MaterialCacheEntry unknown;unknown.material_word=0x7c000036u;unknown.bpp=4;cache.entries.push_back(unknown);
    const std::vector<uint32_t> words{unknown.material_word,abr.material_word,resolved.material_word};
    const auto bytes=l3::cook_scene_e0_material_snapshot(digest,cache);
    const auto decoded=l3::load_scene_e0_material_snapshot(bytes,digest,words);
    assert(decoded.entries.size()==3&&decoded.entries[0].material_word==0x78000000u&&
        decoded.entries[1].material_word==0x78000020u&&decoded.entries[1].abr==1);
    assert(decoded.entries[0].indices==resolved.indices&&decoded.entries[0].palette_ps1==resolved.palette_ps1);
    assert(!decoded.entries[2].resolved&&decoded.entries[2].indices.empty()&&decoded.entries[2].palette_ps1.empty());
    // Sharing a physical page does not permit aliasing its distinct raw word.
    rejects([&]{(void)l3::load_scene_e0_material_snapshot(bytes,digest,{0x78000000u,0x78000040u,0x7c000036u});});
    auto other=digest;other[0]^=1;rejects([&]{(void)l3::load_scene_e0_material_snapshot(bytes,other,words);});
    rejects([&]{(void)l3::load_scene_e0_material_snapshot(bytes,digest,words,1);});
    rejects([&]{(void)l3::load_scene_e0_material_snapshot(bytes,digest,{});});
    rejects([&]{(void)l3::cook_scene_e0_material_snapshot({},cache);});
    auto duplicate=cache;duplicate.entries.push_back(resolved);
    rejects([&]{(void)l3::cook_scene_e0_material_snapshot(digest,duplicate);});
    for(const size_t at:{size_t(0),size_t(4),size_t(6),size_t(8),size_t(12),size_t(16),size_t(20),size_t(24),size_t(56),size_t(60),size_t(80)}) {
        auto malformed=bytes;malformed[at]^=1;
        rejects([&]{(void)l3::load_scene_e0_material_snapshot(malformed,digest,words);});
    }
    auto shortened=bytes;shortened.pop_back();rejects([&]{(void)l3::load_scene_e0_material_snapshot(shortened,digest,words);});
    auto extended=bytes;extended.push_back(0);rejects([&]{(void)l3::load_scene_e0_material_snapshot(extended,digest,words);});
    auto invalid=cache;invalid.entries[2].palette_ps1={7};
    rejects([&]{(void)l3::cook_scene_e0_material_snapshot(digest,invalid);});
    // Outer CRC cannot bypass the embedded bounded material decoder.
    auto inner=bytes;inner[64]^=1;crc(inner);
    rejects([&]{(void)l3::load_scene_e0_material_snapshot(inner,digest,words);});
    // The retained source classes stay separate, including STP-black8000.
    const auto rgba=l3::material_page_rgba8(decoded.entries[0]);
    assert(rgba[3]==0&&rgba[7]==128&&rgba[11]==255);
    auto black=decoded.entries[0];black.palette_ps1[1]=0x8000u;
    const auto black_rgba=l3::material_page_rgba8(black);
    assert(black_rgba[4]==0&&black_rgba[5]==0&&black_rgba[6]==0&&black_rgba[7]==128);
    using Mask=l3::SceneE0TexelPass;using Factor=l3::SceneE0BlendFactor;
    using Equation=l3::SceneE0BlendEquation;using Status=l3::SceneE0MaterialPlanStatus;
    for(unsigned alpha=0;alpha<256;++alpha){
        assert(l3::scene_e0_texel_in_pass(Mask::Nonzero,uint8_t(alpha))==(alpha==128||alpha==255));
        assert(l3::scene_e0_texel_in_pass(Mask::OpaqueOnly,uint8_t(alpha))==(alpha==255));
        assert(l3::scene_e0_texel_in_pass(Mask::StpOnly,uint8_t(alpha))==(alpha==128));
    }
    for(uint32_t abr_mode=0;abr_mode<4;++abr_mode){
        // Deliberately set every other upper metadata bit. The originalmask
        // discards them; raw/Gouraud are not inferred from metadata bit24/28.
        const auto plan=l3::plan_scene_e0_material(0xff806040u,0x78000000u|(abr_mode<<5),true,true,true);
        assert(plan.status==Status::Ready&&plan.pass_count==2&&plan.semi_transparent&&plan.abr==abr_mode);
        assert(plan.triangle_rgb_code==0x26806040u&&plan.quad_rgb_code==0x2e806040u);
        assert(plan.passes[0].texels==Mask::OpaqueOnly&&plan.passes[0].source==Factor::One&&plan.passes[0].destination==Factor::Zero&&plan.passes[0].alpha_tev_scale==1);
        const auto& semi=plan.passes[1];assert(semi.texels==Mask::StpOnly&&semi.alpha_tev_scale==1);
        assert(semi.source==(abr_mode==0?Factor::Half:abr_mode==3?Factor::Quarter:Factor::One));
        assert(semi.destination==(abr_mode==0?Factor::Half:Factor::One));
        assert(semi.equation==(abr_mode==2?Equation::DestinationMinusSource:Equation::Add));
        const auto opaque=l3::plan_scene_e0_material(0xfd806040u,0x78000000u|(abr_mode<<5),false,false,false);
        assert(opaque.status==Status::Ready&&opaque.pass_count==1&&!opaque.semi_transparent&&opaque.passes[0].alpha_tev_scale==2);
    }
    assert(l3::plan_scene_e0_material(0x02808080u,0,false,true,true).status==Status::AlphaGateDisabled);
    assert(l3::plan_scene_e0_material(0x02808080u,0,true,false,true).status==Status::NeedsRetainedStpMask);
    assert(l3::plan_scene_e0_material(0x02808080u,0,true,true,false).status==Status::NeedsNearestSampling);
    for(unsigned mode=0;mode<3;++mode){const auto held=l3::plan_scene_e0_material(0x02808080u,0,mode!=0,mode!=1,mode!=2);assert(held.pass_count==0);}
    // ABE does not reinterpret ordinary texels as translucent or alias an
    // alternate CLUT. SameTPAGE/ABR with a differentCLUT preserves packetflags.
    const auto clut_a=l3::plan_scene_e0_material(0x02808080u,0x78000020u,true,true,true);
    const auto clut_b=l3::plan_scene_e0_material(0x02808080u,0x7c000020u,true,true,true);
    assert(clut_a.abr==clut_b.abr&&clut_a.triangle_rgb_code==clut_b.triangle_rgb_code);
    size_t original_cases=0;
    if(argc==2){std::ifstream fixture(argv[1]);assert(fixture);std::string header;std::getline(fixture,header);
        uint32_t modulation=0,material=0,triangle=0,quad=0,tpage_uv=0;
        while(fixture>>modulation>>material>>triangle>>quad>>tpage_uv){
            const auto plan=l3::plan_scene_e0_material(modulation,material,true,true,true);
            assert(plan.triangle_rgb_code==triangle&&plan.quad_rgb_code==quad&&plan.abr==((tpage_uv>>21)&3u));++original_cases;
        }
        assert(fixture.eof()&&original_cases>0);
    }else assert(argc==1);
    std::cout<<"scene material snapshot: exact ABR/CLUT roundtrip, unresolved hold, identity, budgets and malformed rejection PASS\n";
    std::cout<<"E0 material: 768texelclass checks, 4source ABR plans, mask/gate/nearest holds; original scalar cases="<<original_cases<<" PASS\n";
}
