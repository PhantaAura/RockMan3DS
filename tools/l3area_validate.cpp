#include "legends3ds/l3map.h"
#include "legends3ds/area_package.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/mml_message.h"
#include "legends3ds/stage_object_catalog.h"
#include "legends3ds/render/negative_materials.h"
extern "C" {
#include "mml_package_v2.h"
}
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if(argc!=2)return 2;
        auto d=l3::read_file(argv[1]);
        if(!mml_package_v2_validate(d.data(),d.size()))throw std::runtime_error("invalid area envelope");
        const auto u32=[](const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);};
        const unsigned count=d[6]|(unsigned(d[7])<<8);
        for(unsigned i=0;i<count;++i) {
            const auto* row=d.data()+20+16*i;const uint32_t kind=u32(row);
            const uint8_t* p=d.data()+u32(row+4);const size_t n=u32(row+8);
            if(kind==1)continue;
            if(kind==l3::kAreaRuntimeBindingsSection || kind==l3::kAreaSpawnsSection) {(void)l3::load_area_package(argv[1],l3::AreaPackageLimits{});continue;}
            std::vector<uint8_t> payload(p,p+n);
            if(kind==2) (void)l3::load_l3map(payload,true);
            else if(kind==3) (void)l3::load_l3render(payload,true);
            else if(kind==4) (void)l3::load_l3stage_object_catalog(payload,true);
            else if(kind==0x20) (void)l3::load_l3negative_materials(payload,true);
            else if(n>=4&&p[0]=='L'&&p[1]=='3'&&p[2]=='E'&&p[3]=='B') (void)l3::load_l3ebd_directory(payload,true);
            else if(n>=4&&p[0]=='L'&&p[1]=='3'&&p[2]=='M'&&p[3]=='S') (void)l3::load_l3message(payload,true);
            else throw std::runtime_error("unsupported payload type");
        }
        std::cout<<"PASS\n";return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
