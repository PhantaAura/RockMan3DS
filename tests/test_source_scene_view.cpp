#include "legends3ds/source_scene_view.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
namespace {bool counting=false;std::size_t allocations=0;}
void* operator new(std::size_t n){if(counting)++allocations;auto p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
void require(bool value,const char* why){if(!value){std::cerr<<why<<'\n';std::exit(1);}}
void put(std::uint8_t* p,std::uint32_t v,std::size_t n){for(std::size_t i=0;i<n;++i)p[i]=std::uint8_t(v>>(i*8));}
void camera_put(l3::SourceEventPhase5Context& scene,std::size_t off,std::uint32_t v,std::size_t n){put(scene.camera_800bc738.raw.data()+off,v,n);for(std::size_t i=0;i<n;++i)scene.camera_800bc738.known[off+i]=1;}
std::uint32_t crc(const std::vector<std::uint8_t>& bytes){std::uint32_t c=0xffffffffu;for(std::size_t i=64;i<bytes.size();++i){c^=bytes[i];for(unsigned b=0;b<8;++b)c=(c>>1u)^(0xedb88320u&(0u-(c&1u)));}return c^0xffffffffu;}
void fix_crc(std::vector<std::uint8_t>& bytes){put(bytes.data()+24,crc(bytes),4);}
std::vector<std::uint8_t> profile_bytes(){
    std::vector<std::uint8_t> bytes(l3::kSourceSceneViewProfileBytes,0);bytes[0]='L';bytes[1]='3';bytes[2]='S';bytes[3]='V';
    put(bytes.data()+4,1,2);put(bytes.data()+6,64,2);put(bytes.data()+8,1,4);put(bytes.data()+12,0x80092348,4);put(bytes.data()+16,1025,4);put(bytes.data()+20,std::uint32_t(bytes.size()),4);
    const char* digest="10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0";
    const auto hex=[](char c){return c<='9'?c-'0':c-'a'+10;};for(std::size_t i=0;i<32;++i)bytes[32+i]=std::uint8_t(hex(digest[i*2])*16+hex(digest[i*2+1]));
    put(bytes.data()+64,160u<<16u,4);put(bytes.data()+68,120u<<16u,4);put(bytes.data()+72,384,4);
    for(std::size_t i=0;i<1025;++i)put(bytes.data()+76+i*2,std::uint32_t(i*4),2);fix_crc(bytes);return bytes;
}
void malformed(){
    const auto good=profile_bytes();const auto profile=l3::load_source_scene_view_profile(good);require(profile.initialized&&profile.scalar_sine[1024]==4096,"profile decode");
    const auto rejects=[](const auto& bytes){bool rejected=false;try{(void)l3::load_source_scene_view_profile(bytes);}catch(const std::exception&){rejected=true;}require(rejected,"malformed profile accepted");};
    for(const std::size_t off:{std::size_t(0),std::size_t(4),std::size_t(6),std::size_t(8),std::size_t(12),std::size_t(16),std::size_t(20),std::size_t(24),std::size_t(28),std::size_t(32),std::size_t(64),std::size_t(68),std::size_t(72)}){auto bad=good;bad[off]^=1;if(off>=64)fix_crc(bad);rejects(bad);}
    auto bad=good;bad.pop_back();rejects(bad);bad=good;bad.push_back(0);rejects(bad);
    for(const std::uint32_t value:{4097u,65535u}){bad=good;put(bad.data()+76+200*2,value,2);fix_crc(bad);rejects(bad);}
    for(const std::size_t off:{std::size_t(76),std::size_t(76+1024*2)}){bad=good;put(bad.data()+off,4,2);fix_crc(bad);rejects(bad);}
    l3::SourceProjectionControls projection;projection.dqa=123;projection.dqb=456;require(l3::initialize_source_scene_projection_80011f48(projection,profile),"fresh projection admission");
    require(projection.ofx==160u<<16u&&projection.ofy==120u<<16u&&projection.h==384u&&projection.dqa==123u&&projection.dqb==456u,"fresh projection retained fog");
    const auto before=projection;require(!l3::initialize_source_scene_projection_80011f48(projection,{}),"unowned profile admitted");require(projection.ofx==before.ofx&&projection.ofy==before.ofy&&projection.h==before.h&&projection.dqa==before.dqa&&projection.dqb==before.dqb,"fresh projection atomic hold");
}
void differentials(const char* path){
    std::ifstream file(path);require(bool(file),"fixture unavailable");std::string line;std::size_t count=0;
    l3::SourceEventPhase5Context scene;scene.mode_800a395c=3;
    l3::SourceEventPhase6Context script;script.byte_800bc733=0;
    l3::ReferenceTables refs;auto profile=l3::load_source_scene_view_profile(profile_bytes());
    require(l3::initialize_source_scene_projection_80011f48(script.projection,profile),"fresh profile init");
    while(std::getline(file,line)){std::istringstream stream(line);std::array<long long,32> row{};for(auto& value:row)require(bool(stream>>value),"fixture width");long long extra=0;require(!(stream>>extra),"extra fixture column");
        const auto seed=std::uint32_t(row[0]);for(std::size_t i=1;i<1024;++i)profile.scalar_sine[i]=std::int16_t((i*31+seed*17)%4097);
        camera_put(scene,0,0,1);for(std::size_t axis=0;axis<3;++axis){camera_put(scene,8+axis*4,std::uint32_t(row[1+axis]),4);camera_put(scene,152+axis*4,std::uint32_t(row[4+axis]),4);}
        camera_put(scene,0x42,std::uint32_t(row[7]),2);camera_put(scene,0x3a,std::uint32_t(row[8]),2);camera_put(scene,0x3e,std::uint32_t(row[9]),2);script.byte_1f800074=std::uint8_t(row[10]);
        const std::array<std::size_t,3> indexes{{(0u-std::uint32_t(row[9]))&4095u,std::uint32_t(row[8])&4095u,0}};
        for(std::size_t i=0;i<3;++i)refs.trig[indexes[i]]={std::int16_t(row[11+i*2]),std::int16_t(row[12+i*2])};
        const auto camera_before=scene.camera_800bc738;
        const auto result=l3::produce_source_scene_view_8001e550({&profile,&refs,&scene,&script,true});require(result.status==l3::SourceSceneViewStatus::SourceBackedPartial&&result.frame.has_value(),"scene view admission");const auto& frame=*result.frame;
        for(std::size_t i=0;i<9;++i)require(frame.rotation_q12[i]==row[17+i],"source rotation differential");
        for(std::size_t i=0;i<3;++i){if(frame.translation[i]!=row[26+i]){std::cerr<<"row "<<seed<<" translation "<<i<<" actual "<<frame.translation[i]<<" expected "<<row[26+i]<<'\n';require(false,"source translation differential");}require(frame.eye[i]==row[29+i],"source eye differential");}
        require(scene.camera_800bc738.raw==camera_before.raw&&scene.camera_800bc738.known==camera_before.known,"producer mutated camera");
        require(frame.focal_pixels==384&&frame.center_x_pixels==200&&frame.center_y_pixels==120,"widescreen optical centers");
        require(std::abs(frame.vertical_fov_degrees-34.70805f)<0.001f,"source vertical framing");
        // The native transform conjugates the source axis conventions while
        // preserving the actual fixed matrix coefficients and shifted translation.
        const std::array<float,3> raw{{137.0f,-219.0f,431.0f}},native{{raw[0]/512.0f,-raw[1]/512.0f,raw[2]/512.0f}};
        const float scale=512.0f*float(1u<<frame.source_shift);const std::array<float,3> sign{{1,-1,-1}};
        for(std::size_t axis=0;axis<3;++axis){float source=float(frame.translation[axis])/scale;float actual=frame.native_view_row_major[axis*4+3];for(std::size_t c=0;c<3;++c){source+=float(frame.rotation_q12[axis*3+c])*raw[c]/(4096.0f*512.0f);actual+=frame.native_view_row_major[axis*4+c]*native[c];}require(std::abs(actual-sign[axis]*source)<=0.00001f*std::max(1.0f,std::abs(source)),"native coordinate adaptation");}
        ++count;
    }
    require(count==384,"fixture case count");
}
void holds_and_hot_loop(){
    using Status=l3::SourceSceneViewStatus;l3::SourceEventPhase5Context scene;l3::SourceEventPhase6Context script;l3::ReferenceTables refs;auto profile=l3::load_source_scene_view_profile(profile_bytes());
    l3::SourceSceneViewInputs input;
    const auto held=[&](Status status){const auto result=l3::produce_source_scene_view_8001e550(input);require(result.status==status&&!result.frame,"missing owner atomic hold");};
    held(Status::NeedsActiveResources);input.active_resources_admitted=true;held(Status::NeedsProfile);input.profile=&profile;held(Status::NeedsReference);input.reference=&refs;held(Status::NeedsCameraMode);input.scene=&scene;held(Status::NeedsCameraMode);scene.mode_800a395c=1;held(Status::GameplayCameraOwned);scene.mode_800a395c=3;held(Status::NeedsScriptControl);input.script=&script;held(Status::NeedsScriptControl);script.byte_800bc733=0;held(Status::NeedsSourceShift);script.byte_1f800074=3;held(Status::UnsupportedSourceShift);script.byte_1f800074=0;held(Status::NeedsCameraBytes);camera_put(scene,0,1,1);held(Status::NeedsTargetPointCamera);camera_put(scene,0,0,1);held(Status::NeedsCameraBytes);
    for(std::size_t axis=0;axis<3;++axis){camera_put(scene,8+axis*4,0,4);camera_put(scene,152+axis*4,0,4);}for(const std::size_t off:{std::size_t(0x42),std::size_t(0x3a),std::size_t(0x3e)})camera_put(scene,off,0,2);held(Status::NeedsProjectionControls);
    require(l3::initialize_source_scene_projection_80011f48(script.projection,profile),"projection init");refs.trig[0]={0,4096};
    const auto good=l3::produce_source_scene_view_8001e550(input);require(good.frame.has_value(),"view readiness");
    for(const auto h:{0u,32768u,65535u}){script.projection.h=h;held(Status::UnsupportedOptics);}script.projection.h=384;
    script.projection.ofx=(160u<<16u)|1u;held(Status::UnsupportedOptics);script.projection.ofx=160u<<16u;
    script.projection.ofy=(120u<<16u)|1u;held(Status::UnsupportedOptics);script.projection.ofy=120u<<16u;
    for(const std::size_t off:{std::size_t(0),std::size_t(8),std::size_t(19),std::size_t(152),std::size_t(163),std::size_t(0x3a),std::size_t(0x3f),std::size_t(0x43)}){scene.camera_800bc738.known[off]=0;held(Status::NeedsCameraBytes);scene.camera_800bc738.known[off]=1;}
    input.active_resources_admitted=false;held(Status::NeedsActiveResources);input.active_resources_admitted=true;
    counting=true;for(std::size_t i=0;i<4096;++i){const auto result=l3::produce_source_scene_view_8001e550(input);require(result.status==Status::SourceBackedPartial&&result.frame.has_value(),"hot view");}counting=false;require(allocations==0,"view hot-loop allocation");
}
}
int main(int argc,char** argv){require(argc==2,"usage: test_source_scene_view fixture.tsv");malformed();differentials(argv[1]);holds_and_hot_loop();std::cout<<"source scene view:384 original radial differentials, malformed/owner holds, optical adaptation and4096 allocation-free frames passed\n";}
