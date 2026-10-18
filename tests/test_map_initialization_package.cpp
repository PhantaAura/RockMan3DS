#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/endian_io.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

using Section = std::pair<uint32_t, std::vector<uint8_t>>;
static std::vector<uint8_t> envelope(const std::vector<Section>& sections) {
    std::vector<uint8_t> out;
    l3::wr_u32(out,0x324c4d4d); l3::wr_u16(out,2); l3::wr_u16(out,uint16_t(sections.size()));
    l3::wr_u32(out,0); l3::wr_u32(out,0); l3::wr_u32(out,0);
    uint32_t offset=20+uint32_t(sections.size())*16;
    for(const auto& s:sections) {
        l3::wr_u32(out,s.first); l3::wr_u32(out,offset); l3::wr_u32(out,uint32_t(s.second.size()));
        l3::wr_u32(out,l3::crc32(s.second.data(),s.second.size())); offset+=uint32_t(s.second.size());
    }
    l3::patch_u32(out,8,offset); l3::patch_u32(out,16,l3::crc32(out.data()+20,out.size()-20));
    for(const auto& s:sections) out.insert(out.end(),s.second.begin(),s.second.end());
    return out;
}
int main() {
    l3::St00Source source;
    source.areas.push_back({63,63,1,1,{1}}); source.idx.resize(2);
    source.idx[1].descriptor_ptr=l3::kSt00StgBase;
    l3::SourceDescriptor descriptor; descriptor.source_ptr=l3::kSt00StgBase;
    source.descriptors.push_back(descriptor);
    l3::St00RenderSource render; render.descriptors.resize(1);
    render.descriptors[0].record_indices[0]=0;
    render.records.resize(1); render.records[0].byte2=1;
    const auto map_bytes=l3::cook_st00_map(source), render_bytes=l3::cook_l3render(render);
    l3::MapInitialization init;
    init.idx_count=2; init.descriptor_count=1; init.render_record_count=1;
    init.map_crc32=l3::crc32(map_bytes.data(),map_bytes.size());
    init.render_crc32=l3::crc32(render_bytes.data(),render_bytes.size());
    init.entries.push_back({1,0,0,0,1});
    const auto metadata=l3::cook_map_initialization(init);
    const auto directory=std::filesystem::temp_directory_path()/
        ("l3-init-package-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path=directory/"test.mmlpkg";
    const auto publish=[&](const std::vector<Section>& sections) {
        const auto bytes=envelope(sections); std::ofstream f(path,std::ios::binary|std::ios::trunc);
        f.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size())); assert(bool(f));
    };
    const auto reject=[&](const std::vector<Section>& sections,const l3::AreaPackageLimits& limits) {
        publish(sections); bool failed=false;
        try { (void)l3::load_area_package(path.string(),limits); } catch(const std::runtime_error&) {failed=true;}
        assert(failed);
    };
    l3::AreaPackageLimits limits;
    const std::vector<Section> base{{2,map_bytes},{3,render_bytes}};
    publish(base); auto legacy=l3::load_area_package(path.string(),limits);
    assert(legacy.map.idx[1].raw6==0 && legacy.diagnostics.map_initialization_profile==0);
    auto sections=base; sections.push_back({l3::kAreaMapInitializationSection,metadata});
    publish(sections); auto loaded=l3::load_area_package(path.string(),limits);
    assert(loaded.map.idx[0].raw6==0 && loaded.map.idx[1].raw6==1);
    assert(loaded.diagnostics.map_initialization_writes==1 && loaded.diagnostics.map_initialization_profile==1);
    assert(loaded.diagnostics.map_initialization_temporary_bytes>=metadata.size()+sizeof(l3::MapInitialization));
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    l3::RuntimeSession session(std::move(loaded.map),{},{},settings);
    loaded.unload(); assert(session.active_map().idx[1].raw6==1);
    assert(loaded.diagnostics.map_initialization_writes==0 && loaded.directory.capacity()==0);
    auto corrupt=sections;
    l3::patch_u32(corrupt.back().second,28,init.map_crc32^1);reject(corrupt,limits);
    corrupt=sections; corrupt.back().second[48+8]=2;
    l3::patch_u32(corrupt.back().second,44,l3::crc32(corrupt.back().second.data()+48,16));reject(corrupt,limits);
    corrupt=sections; corrupt.back().second[48+6]=1;
    l3::patch_u32(corrupt.back().second,44,l3::crc32(corrupt.back().second.data()+48,16));reject(corrupt,limits);
    corrupt=sections;corrupt.push_back(corrupt.back());reject(corrupt,limits);
    corrupt=sections;corrupt.back().second.resize(l3::kMapInitializationMaxFileBytes+1);reject(corrupt,limits);
    auto low=limits;low.max_load_peak_bytes=1;reject(sections,low);
    assert(session.active_map().idx[1].raw6==1 && legacy.map.idx[1].raw6==0);
    std::filesystem::remove_all(directory);
    std::cout<<"PASS: optional initializer package/session integration, malformed rejection, legacy preservation, unload\n";
}
