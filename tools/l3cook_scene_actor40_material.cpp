#include "legends3ds/scene_e0_material.h"
#include "legends3ds/st00_source.h"
#include "legends3ds/source_scene_actor40.h"
#include <algorithm>
#include <fstream>
#include <iostream>

int main(int argc,char** argv) {
    try {
        if(argc<5){std::cerr<<"usage: l3cook_scene_actor40_material PHASE5.l3p5 AREA.l3ebd OUT.l3sm UPLOAD_ORDER_CONTAINER.BIN...\n";return 2;}
        const auto profile=l3::load_source_event_phase5_profile(l3::read_file(argv[1]));
        const auto ebd=l3::load_l3ebd_directory(l3::read_file(argv[2]));
        const auto witness=l3::bind_source_event_phase5_resources(profile,ebd);
        const auto resources=l3::decode_source_scene_actor40_phase5_resources(profile,witness,ebd);
        std::vector<uint32_t> words;
        for(const auto& binding:resources.mesh.bindings)words.push_back(binding.material_word);
        std::sort(words.begin(),words.end());words.erase(std::unique(words.begin(),words.end()),words.end());
        std::vector<l3::IndexedResource> uploads;
        for(int i=4;i<argc;++i) {
            auto rows=l3::scan_mml_indexed_resources(l3::read_file(argv[i]));
            uploads.insert(uploads.end(),std::make_move_iterator(rows.begin()),std::make_move_iterator(rows.end()));
        }
        std::vector<l3::MaterialVramDiagnostics> coverage;
        const auto cache=l3::build_material_cache(words,uploads,&coverage);
        const auto bytes=l3::cook_scene_e0_material_snapshot(profile.ebd_source_digest,cache);
        (void)l3::load_scene_e0_material_snapshot(bytes,profile.ebd_source_digest,words);
        std::ofstream f(argv[3],std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        if(!f)throw std::runtime_error("cannot write scene material snapshot");
        size_t resolved=0;
        for(size_t i=0;i<cache.entries.size();++i) {
            resolved+=cache.entries[i].resolved;
            std::cout<<"word="<<std::hex<<cache.entries[i].material_word<<std::dec<<" resolved="<<cache.entries[i].resolved
                <<" image="<<coverage[i].image_words_written<<'/'<<coverage[i].image_words_total
                <<" clut="<<coverage[i].palette_words_written<<'/'<<coverage[i].palette_words_total<<'\n';
        }
        std::cout<<"entries="<<cache.entries.size()<<" resolved="<<resolved<<" bytes="<<bytes.size()
            <<" snapshot=INITIAL_UPLOAD_BATCH chronology=SOURCE_BACKED_PARTIAL live_palette=UNKNOWN\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
