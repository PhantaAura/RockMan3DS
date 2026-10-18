#include "legends3ds/actor_detection_segment.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocation_count = 0;
void* operator new(std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
namespace {
using Status = l3::RuntimeContactStatus;
void require(bool v, const char* why) { if (!v) throw std::runtime_error(why); }
int64_t number(std::istream& row) {
    int64_t v = 0; require(static_cast<bool>(row >> v), "truncated extended segment fixture"); return v;
}
l3::RuntimeMap authored_map(uint16_t code, uint16_t flags, int16_t raw6,
                           const std::array<int16_t,3>& idx, const std::array<int16_t,4>& group,
                           const std::array<int16_t,2>& interval) {
    l3::RuntimeMap map; l3::RuntimeMapArea area;
    area.start_x=62;area.start_z=62;area.width=4;area.height=4;area.cell_count=16;
    map.areas.push_back(area);map.cells.assign(16,1);map.idx.resize(2);
    map.idx[1].x=idx[0];map.idx[1].y=idx[1];map.idx[1].z=idx[2];map.idx[1].raw6=raw6;
    map.idx[1].descriptor_index=0;l3::RuntimeMapDescriptor descriptor;
    descriptor.default_contact_index=0;descriptor.alt_contact_index=0;
    map.descriptors.push_back(descriptor);map.contact_lists.push_back({0,1});
    map.contact_groups.push_back({group[0],group[1],group[2],group[3],0,1});
    map.contact_primitives.push_back({interval[0],interval[1],code,flags});return map;
}
void guards() {
    const l3::RetailPlayerBounds bounds{-25,25,-35,35,-70,10};
    const std::array<int16_t,4> inside{{0,-60,0,0}}, outside{{900,-60,0,0}};
    auto map=authored_map(0x71,0,0,{{0,0,0}},{{-100,100,-100,100}},{{-128,128}});
    auto result=l3::query_actor_detection_segment_80049c54(map,0,inside,inside,bounds);
    require(result.status==Status::Unsupported && result.unsupported_cell_index!=UINT32_MAX &&
        result.unsupported_group_index==0 && result.unsupported_primitive_index==0 && result.unsupported_code==0x71,
        "held family7 query lost its first primitive diagnostics");
    result=l3::query_actor_detection_segment_80049c54(map,0,outside,outside,bounds);
    require(result.status==Status::SourceBackedPartial && !result.result && !result.unsupported_primitives &&
        result.unsupported_code==UINT32_MAX,"original family7 outside rejection still held");
    map.contact_primitives[0].code=8;
    result=l3::query_actor_detection_segment_80049c54(map,0,inside,inside,bounds);
    require(result.status==Status::Unsupported && result.unsupported_code==8,"unknown slope Y semantics invented");
    map.contact_primitives[0].flags=0x8000;
    result=l3::query_actor_detection_segment_80049c54(map,0,inside,inside,bounds);
    require(result.status==Status::SourceBackedPartial && !result.result,"flag-filtered unknown Y was evaluated");
    map.contact_primitives[0].flags=0;map.contact_primitives[0].code=0x61;
    const size_t allocations=allocation_count;
    for(unsigned i=0;i<1536;++i) {
        const auto r=l3::query_actor_detection_segment_80049c54(map,0,inside,inside,bounds);
        if(r.status!=Status::SourceBackedPartial || r.result!=1) throw std::runtime_error("diamond source query mismatch");
    }
    require(allocation_count==allocations,"extended classifier allocated in hot query");
}
void authored(const char* path) {
    std::ifstream file(path);require(static_cast<bool>(file),"missing extended authored fixture");
    std::string line;std::getline(file,line);size_t supported=0,held=0,blocked=0;
    while(std::getline(file,line)) {
        if(line.empty()) continue;
        std::istringstream row(line);
        const auto code=static_cast<uint16_t>(number(row));const auto flags=static_cast<uint16_t>(number(row));
        const auto raw6=static_cast<int16_t>(number(row));
        std::array<int16_t,3> idx{};std::array<int16_t,4> group{},start{},end{};std::array<int16_t,2> interval{};
        for(auto& v:idx) v=static_cast<int16_t>(number(row));
        for(auto& v:group) v=static_cast<int16_t>(number(row));
        for(auto& v:interval) v=static_cast<int16_t>(number(row));
        const auto cls=static_cast<uint32_t>(number(row));
        for(auto& v:start) v=static_cast<int16_t>(number(row));
        for(auto& v:end) v=static_cast<int16_t>(number(row));
        l3::RetailPlayerBounds b;b.x0=static_cast<int16_t>(number(row));b.x1=static_cast<int16_t>(number(row));
        b.z0=static_cast<int16_t>(number(row));b.z1=static_cast<int16_t>(number(row));
        b.y0=static_cast<int16_t>(number(row));b.y1=static_cast<int16_t>(number(row));
        const auto expected=static_cast<int32_t>(number(row));const auto cells=static_cast<uint32_t>(number(row));
        std::string extra;require(!(row>>extra),"extra extended authored field");
        const auto map=authored_map(code,flags,raw6,idx,group,interval);
        const auto r=l3::query_actor_detection_segment_80049c54(map,0,start,end,b,cls);
        if(r.status==Status::SourceBackedPartial) {
            if(r.result!=expected || r.candidate_cells!=cells) {
                std::cerr<<"row="<<supported+held<<" code="<<code<<" point="<<start[0]<<','<<start[1]<<','<<start[2]
                    <<" got="<<r.result<<" want="<<expected<<" cells="<<r.candidate_cells<<'/'<<cells<<'\n';
                throw std::runtime_error("original/native extended segment mismatch");
            }
            ++supported;blocked+=r.result!=0;
        } else {require(r.status==Status::Unsupported,"unexpected extended segment status");++held;}
    }
    require(supported>5000 && held>0 && blocked>100,"insufficient supported/held extended coverage");
    std::cout<<"authored_original_queries="<<supported+held<<" exact_supported="<<supported<<" held="<<held<<" nonzero="<<blocked<<'\n';
}
void stage(const char* path,const char* pack) {
    const std::string root(pack);const auto map_bytes=l3::read_file(root+"/st00.l3map");
    const auto render_bytes=l3::read_file(root+"/st00.l3render");auto map=l3::load_l3map(map_bytes);
    const auto render=l3::load_l3render(render_bytes);const auto init=l3::load_map_initialization(l3::read_file(root+"/st00.l3init"));
    l3::apply_map_initialization(map,render,init,l3::crc32(map_bytes.data(),map_bytes.size()),l3::crc32(render_bytes.data(),render_bytes.size()));
    std::ifstream file(path);require(static_cast<bool>(file),"missing owner-local extended stage fixture");
    std::string line;std::getline(file,line);size_t supported=0,held=0,blocked=0;
    while(std::getline(file,line)) {
        if(line.empty()) continue;
        std::istringstream row(line);const auto cls=static_cast<uint32_t>(number(row));
        std::array<int16_t,4> start{},end{};for(auto& v:start) v=static_cast<int16_t>(number(row));
        for(auto& v:end) v=static_cast<int16_t>(number(row));
        l3::RetailPlayerBounds b;b.x0=static_cast<int16_t>(number(row));b.x1=static_cast<int16_t>(number(row));
        b.z0=static_cast<int16_t>(number(row));b.z1=static_cast<int16_t>(number(row));b.y0=static_cast<int16_t>(number(row));b.y1=static_cast<int16_t>(number(row));
        const auto expected=static_cast<int32_t>(number(row));const auto cells=static_cast<uint32_t>(number(row));
        const auto r=l3::query_actor_detection_segment_80049c54(map,1,start,end,b,cls);
        if(r.status==Status::SourceBackedPartial) {
            if(r.result!=expected || r.candidate_cells!=cells) {
                std::cerr<<"stage row="<<supported+held<<" got="<<r.result<<" want="<<expected<<" code="<<r.unsupported_code<<'\n';
                throw std::runtime_error("materialized owner original/native segment mismatch");
            }
            ++supported;blocked+=r.result!=0;
        } else ++held;
    }
    require(supported>100 && supported+held==294,"missing extended owner stage coverage");
    std::cout<<"owner_stage_original_queries="<<supported+held<<" exact_supported="<<supported<<" held="<<held<<" nonzero="<<blocked<<'\n';
}
}
int main(int argc,char** argv) {
    try {
        guards();if(argc>1)authored(argv[1]);if(argc>3)stage(argv[2],argv[3]);
        std::cout<<"PASS: original classifier/plain-Y extensions and1536 allocation-free queries; no live retail parity\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
