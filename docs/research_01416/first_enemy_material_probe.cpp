#include "legends3ds/actor_resource.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Read-only owner-data audit. Output contains scalar metadata; never payloads.
// argv: existing cooked pack, already supplied/extracted CDDATA/DAT directory.
int main(int argc, char** argv) {
 try {
    if(argc!=3) throw std::runtime_error("usage: PROBE EXISTING_PACK OWNER_DAT");
    const std::filesystem::path pack(argv[1]),dat(argv[2]);
    const auto ebd=l3::load_l3ebd_directory(l3::read_file((pack/"ebd/area_01.l3ebd").string()),true);
    const auto rows=l3::find_ebd_entries(ebd,0x20,0x03);
    if(rows.size()!=1) throw std::runtime_error("class20 subtype03 row not unique");
    const auto mesh=l3::decode_ebd_actor_mesh(ebd,rows[0],l3::ActorMeshTier::NEAR);
    std::set<uint32_t> words;
    std::cout<<"{\"format\":\"L3-FIRST-ENEMY-MATERIAL-AUDIT-1\",\"ebd_row\":"<<rows[0]<<",\"parts\":[";
    for(size_t i=0;i<mesh.bindings.size();++i){
      if(i)std::cout<<',';
      const auto& b=mesh.bindings[i];const auto& p=mesh.model.parts[i];
      const uint32_t word=(b.material_word&0xffff0000u)|uint16_t(b.material_word|0x20u);
      words.insert(word);
      size_t mismatches=0; unsigned umin=255,umax=0,vmin=255,vmax=0;
      const auto inspect_uv=[&](const auto& uv){for(size_t j=0;j<uv.size();j+=2){
        umin=std::min(umin,unsigned(uv[j]));umax=std::max(umax,unsigned(uv[j]));
        vmin=std::min(vmin,unsigned(uv[j+1]));vmax=std::max(vmax,unsigned(uv[j+1]));}};
      for(const auto& f:p.triangles){mismatches+=((uint32_t(f.clut)<<16)|f.tpage)!=word;inspect_uv(f.uv);}
      for(const auto& f:p.quads){mismatches+=((uint32_t(f.clut)<<16)|f.tpage)!=word;inspect_uv(f.uv);}
      std::cout<<"{\"draw_slot\":"<<i<<",\"source_part_offset\":"<<b.source_part_offset
        <<",\"raw_material_word\":"<<b.material_word<<",\"decoder_material_word\":"<<word
        <<",\"triangles\":"<<p.triangles.size()<<",\"quads\":"<<p.quads.size()
        <<",\"face_binding_mismatches\":"<<mismatches<<",\"uv_bounds\":["<<umin<<','<<vmin<<','<<umax<<','<<vmax<<"]}";
    }
    std::vector<l3::IndexedResource> uploads;
    const std::vector<std::string> ordered={"INIT_DAT.BIN","HEAD00.BIN","ARM00R.BIN","ARM00L.BIN","LEG00.BIN","ST00.BIN","ST00_01.BIN"};
    std::cout<<"],\"upload_order_source\":\"inherited build_tutorial_pack.py; live retail order not separately measured\",\"containers\":[";
    for(size_t i=0;i<ordered.size();++i){
      if(i)std::cout<<',';auto resources=l3::scan_mml_indexed_resources(l3::read_file((dat/ordered[i]).string()));
      std::cout<<"{\"name\":\""<<ordered[i]<<"\",\"resource_count\":"<<resources.size()<<'}';
      for(auto& r:resources){r.path=ordered[i]+"/"+r.path;uploads.push_back(std::move(r));}
    }
    const auto terrain=l3::load_l3material_cache(l3::read_file((pack/"materials/area_01.l3mc").string()),true);
    const auto player=l3::load_l3material_cache(l3::read_file((pack/"materials/player_area_01.l3mc").string()),true);
    const std::vector<uint32_t> sorted(words.begin(),words.end());
    std::vector<l3::MaterialVramDiagnostics> coverage;
    const auto candidate=l3::build_material_cache(sorted,uploads,&coverage);
    std::cout<<"],\"materials\":[";
    for(size_t i=0;i<sorted.size();++i){
      if(i)std::cout<<',';const auto word=sorted[i];
      const auto t=l3::decode_ps1_tpage(uint16_t(word));const auto c=l3::decode_ps1_clut(uint16_t(word>>16));
      const auto last=l3::resolve_material_word_last_writer(word,uploads);
      const auto* entry=l3::find_material_cache_entry(candidate,word);
      std::cout<<"{\"word\":"<<word<<",\"tpage\":"<<t.raw<<",\"bpp\":"<<unsigned(t.bpp)
       <<",\"abr\":"<<unsigned(t.abr)<<",\"image_x_words\":"<<t.vram_x_words<<",\"image_y\":"<<t.vram_y
       <<",\"clut_raw\":"<<c.raw<<",\"clut_x\":"<<c.vram_x<<",\"clut_y\":"<<c.vram_y
       <<",\"terrain_cache_present\":"<<(l3::find_material_cache_entry(terrain,word)?"true":"false")
       <<",\"player_cache_present\":"<<(l3::find_material_cache_entry(player,word)?"true":"false")
       <<",\"candidate_resolved\":"<<(entry&&entry->resolved?"true":"false")
       <<",\"image_words_written\":"<<coverage[i].image_words_written<<",\"image_words_total\":"<<coverage[i].image_words_total
       <<",\"palette_words_written\":"<<coverage[i].palette_words_written<<",\"palette_words_total\":"<<coverage[i].palette_words_total
       <<",\"legacy_image_resource_index\":"<<last.image_resource<<",\"legacy_palette_resource_index\":"<<last.palette_resource
       <<",\"legacy_palette_row\":"<<last.palette_row<<",\"matching_texture_headers\":[";
      bool first=true;
      for(size_t j=0;j<uploads.size();++j){const auto& r=uploads[j];
       if(r.kind!=l3::IndexedResourceKind::TextureType1||!l3::texture_owns_image_page(r.texture,t.raw))continue;
       if(!first)std::cout<<',';first=false;const auto& x=r.texture;
       auto path=r.path;std::replace(path.begin(),path.end(),'\\','/');
       const bool cooked_matches=l3::cook_l3texture(x)==l3::read_file((pack/"indexed/ST00_01__OBJ__ST00__AR01__EM0300.TIM.l3tex").string());
       std::cout<<"{\"path\":\""<<path<<"\",\"existing_cooked_equal_owner_decoded\":"<<(cooked_matches?"true":"false")
        <<",\"upload_index\":"<<j<<",\"header_offset\":"<<r.header_offset<<",\"width_pixels\":"<<x.width_pixels
        <<",\"height\":"<<x.height<<",\"clut_x\":"<<x.clut_x<<",\"clut_y\":"<<x.clut_y
        <<",\"palette_entries\":"<<x.palette_entries<<",\"palette_rows\":"<<x.palette_rows
        <<",\"row_banked\":"<<(x.palette_row_banked?"true":"false")<<",\"resolved_row\":"<<l3::texture_palette_row(x,c.raw)<<'}';
      }
      std::cout<<"]}";
    }
    std::cout<<"],\"payload_written\":false,\"runtime_edited\":false,\"shade\":\"UNKNOWN\"}\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
