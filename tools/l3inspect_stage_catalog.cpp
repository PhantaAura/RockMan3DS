#include "legends3ds/stage_catalog.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
int main(int argc,char**argv){
    if(argc!=2){std::cerr<<"usage: l3inspect_stage_catalog stage_catalog.l3sc\n";return 2;}
    try{std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("cannot open catalog");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});auto c=l3::load_l3stage_catalog(b,true);size_t with_render=0;for(const auto&e:c.entries)if(e.flags&l3::kL3StageCatalogFlagRender)++with_render;std::cout<<"packages="<<c.entries.size()<<" maps="<<c.entries.size()<<" renders="<<with_render<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"l3inspect_stage_catalog: "<<e.what()<<"\n";return 1;}
}
