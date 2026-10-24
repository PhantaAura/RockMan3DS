#include "legends3ds/actor_material.h"
#include "legends3ds/st00_source.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv){
 try{
    if(argc<4){std::cerr<<"usage: l3cook_actor_material_cache AREA.l3ebd OUT.l3mc UPLOAD_ORDER_CONTAINER.BIN...\n";return 2;}
    const auto ebd=l3::load_l3ebd_directory(l3::read_file(argv[1]),true);
    auto words=l3::collect_admitted_actor_material_words(ebd);
    // E0/05 and E0/08 meshes share this per-area cache (same uploads, same
    // last-writer VRAM order).
    const auto e0=l3::collect_admitted_e0_object_material_words(ebd);
    words.insert(words.end(),e0.begin(),e0.end());
    std::sort(words.begin(),words.end());words.erase(std::unique(words.begin(),words.end()),words.end());
    if(words.size()>l3::kActorMaterialEntryLimit)throw std::runtime_error("actor material count policy exceeded");
    std::vector<l3::IndexedResource> uploads;
    for(int i=3;i<argc;++i){
        auto input=l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
        uploads.insert(uploads.end(),std::make_move_iterator(input.begin()),std::make_move_iterator(input.end()));
    }
    std::vector<l3::MaterialVramDiagnostics> coverage;
    const auto cache=l3::build_material_cache(words,uploads,&coverage);
    const auto bytes=l3::cook_l3material_cache(cache);
    // Validate the exact runtime admission policy before emitting owner data.
    (void)l3::load_actor_material_cache(bytes);
    std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    if(!output)throw std::runtime_error("cannot write actor material cache");
    size_t resolved=0;for(size_t i=0;i<cache.entries.size();++i){const auto& e=cache.entries[i];resolved+=e.resolved;
        std::cout<<"word=0x"<<std::hex<<e.material_word<<std::dec<<" resolved="<<e.resolved
          <<" image_written="<<coverage[i].image_words_written<<'/'<<coverage[i].image_words_total
          <<" clut_written="<<coverage[i].palette_words_written<<'/'<<coverage[i].palette_words_total<<'\n';}
    std::cout<<"actor_materials="<<cache.entries.size()<<" resolved="<<resolved<<" bytes="<<bytes.size()
      <<" shade=UNKNOWN upload_order=INHERITED_SOURCE_BACKED_PARTIAL retail_vram_comparison=UNKNOWN\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<"l3cook_actor_material_cache: "<<e.what()<<'\n';return 1;}
}
