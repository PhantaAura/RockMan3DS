#include "legends3ds/source_camera.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

namespace {bool count_allocations=false;std::size_t allocations=0;}
void* operator new(std::size_t size){if(count_allocations)++allocations;auto p=std::malloc(size);if(!p)throw std::bad_alloc();return p;}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

namespace {
void require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
std::vector<std::vector<long long>> read_rows(const char* path,std::size_t width){
    std::ifstream file(path);require(static_cast<bool>(file),"fixture unavailable");
    std::vector<std::vector<long long>> rows;std::string line;
    while(std::getline(file,line)){std::istringstream stream(line);std::vector<long long> row;long long value;
        while(stream>>value)row.push_back(value);
        require(row.size()==width,"fixture width");rows.push_back(row);}
    require(!rows.empty(),"empty fixture");return rows;
}
bool same(const l3::SourceProjectionControls& a,const l3::SourceProjectionControls& b){
    return a.ofx==b.ofx && a.ofy==b.ofy && a.h==b.h && a.dqa==b.dqa && a.dqb==b.dqb;
}
void camera_fixture(const std::vector<std::vector<long long>>& rows){
    l3::ReferenceTables reference;
    std::size_t number=0;
    for(const auto& row:rows){
        const auto yaw=static_cast<std::int32_t>(row[0]),pitch=static_cast<std::int32_t>(row[1]);
        const std::array<std::size_t,3> indexes{{(0U-static_cast<std::uint32_t>(pitch))&4095U,
            static_cast<std::uint32_t>(yaw)&4095U,0}};
        for(std::size_t i=0;i<3;++i)reference.trig[indexes[i]]={static_cast<std::int16_t>(row[6+2*i]),static_cast<std::int16_t>(row[7+2*i])};
        l3::SourceCameraDirectInputs input;input.reference=&reference;input.yaw=yaw;input.pitch=pitch;
        input.eye=std::array<std::int32_t,3>{{static_cast<std::int32_t>(row[2]),static_cast<std::int32_t>(row[3]),static_cast<std::int32_t>(row[4])}};
        input.retained_matrix_padding_1f8000f2=static_cast<std::uint16_t>(row[5]);
        const auto result=l3::produce_source_camera_800384f0(input);
        require(result.status==l3::SourceCameraStatus::SourceBackedPartial,"camera admission");
        for(std::size_t i=0;i<3;++i)require(result.rotation_800a3a10[i]==row[12+i],"camera rotation");
        for(std::size_t i=0;i<8;++i)if((*result.scratch_camera_1f800008)[i]!=static_cast<std::uint32_t>(row[15+i])){
            std::cerr<<"camera row "<<number<<" word "<<i<<" got "<<(*result.scratch_camera_1f800008)[i]<<" expected "<<row[15+i]<<'\n';std::exit(1);}
        for(std::size_t i=0;i<3;++i){require((*result.eye_800a39f0)[i]==row[23+i],"camera eye mirror");
            require((*result.scratch_eye_1f800028)[i]==row[23+i],"camera scratch eye");}
        ++number;
    }
}
void projection_fixture(const std::vector<std::vector<long long>>& rows){
    for(const auto& row:rows){
        l3::SourceProjectionControls owner;std::array<std::optional<std::uint32_t>*,5> fields{{&owner.ofx,&owner.ofy,&owner.h,&owner.dqa,&owner.dqb}};
        for(std::size_t i=0;i<5;++i)*fields[i]=0x13579bdfU+static_cast<std::uint32_t>(24+i)*0x10101U;
        const auto a=static_cast<std::uint32_t>(row[1]),b=static_cast<std::uint32_t>(row[2]);
        switch(row[0]){
        case 0:l3::source_projection_offset_8007b118(owner,a,b);break;
        case 1:l3::source_projection_h_8007b138(owner,a);break;
        case 2:l3::source_projection_dqa_8007b0c0(owner,a);break;
        case 3:l3::source_projection_dqb_8007b0cc(owner,a);break;
        case 4:require(l3::source_projection_fog_8007a1cc(owner,a,b),"fog admission");break;
        default:require(false,"projection fixture kind");
        }
        for(std::size_t i=0;i<5;++i)require(**fields[i]==static_cast<std::uint32_t>(row[3+i]),"projection source differential");
        l3::SourceGteRtpsState state;state.control.fill(0xa5a55a5aU);state.data.fill(0x5a5aa5a5U);
        require(l3::bind_source_projection_controls(state,owner),"projection binding");
        require(state.projection_controls_bound && !state.matrix_translation_bound,"separate control ownership");
        for(std::size_t i=0;i<32;++i){require(state.data[i]==0x5a5aa5a5U,"data preserve");
            require(state.control[i]==(i>=24 && i<=28 ? **fields[i-24] : 0xa5a55a5aU),"unrelated control preserve");}
    }
}
void holds_and_hot_loop(){
    using Status=l3::SourceCameraStatus;
    l3::ReferenceTables reference;reference.trig[0]={0,4096};
    l3::SourceCameraDirectInputs input;
    const auto check=[&](Status status){const auto result=l3::produce_source_camera_800384f0(input);
        require(result.status==status,"camera missing premise status");
        require(!result.scratch_camera_1f800008 && !result.scratch_eye_1f800028 && !result.eye_800a39f0,"camera unavailable output hold");};
    check(Status::NeedsReference);input.reference=&reference;check(Status::NeedsEye);
    input.eye=std::array<std::int32_t,3>{{11,-22,33}};check(Status::NeedsAngles);
    input.yaw=0;check(Status::NeedsAngles);input.pitch=0;check(Status::NeedsRetainedMatrixPadding);
    input.retained_matrix_padding_1f8000f2=0x8123U;
    l3::SourceProjectionControls owner;
    l3::SourceGteRtpsState state;state.control.fill(0x87654321U);state.data.fill(0x12345678U);
    const auto before=state;
    const auto hold=[&](){require(!l3::bind_source_projection_controls(state,owner),"missing projection premise");
        require(state.control==before.control && state.data==before.data && !state.projection_controls_bound && !state.matrix_translation_bound,"atomic projection hold");};
    hold();l3::source_projection_offset_8007b118(owner,160,120);hold();l3::source_projection_h_8007b138(owner,320);hold();
    l3::source_projection_dqa_8007b0c0(owner,0x1234);hold();
    const auto retained=owner;
    require(!l3::source_projection_fog_8007a1cc(owner,15,0) && same(owner,retained),"zero division atomic hold");
    require(!l3::source_projection_fog_8007a1cc(owner,0x02000000U,0xffffffffU) && same(owner,retained),"source division break atomic hold");
    l3::source_projection_dqb_8007b0cc(owner,99);
    count_allocations=true;
    for(std::size_t tick=0;tick<4096;++tick){
        input.eye=std::array<std::int32_t,3>{{static_cast<std::int32_t>(tick),-22,33}};
        const auto result=l3::produce_source_camera_800384f0(input);
        require(result.status==Status::SourceBackedPartial,"hot camera");
        require((*result.scratch_camera_1f800008)[4]==0x81231000U,"retained padding");
        require(l3::source_projection_fog_8007a1cc(owner,static_cast<std::uint32_t>(tick),384),"hot fog");
        require(l3::bind_source_projection_controls(state,owner),"hot control bind");
        require(!state.matrix_translation_bound,"projection cannot bind current camera");
    }
    count_allocations=false;require(allocations==0,"hot camera allocation");
}
}
int main(int argc,char** argv){
    require(argc==3,"usage: test_source_camera camera-direct.tsv projection-control.tsv");
    const auto cameras=read_rows(argv[1],26),projection=read_rows(argv[2],8);
    camera_fixture(cameras);projection_fixture(projection);holds_and_hot_loop();
    std::cout<<"source camera: "<<cameras.size()<<" authored camera cases, "<<projection.size()<<" authored projection cases; holds and4096 allocation-free ticks passed\n";
}
