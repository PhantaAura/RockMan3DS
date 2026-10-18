#include "legends3ds/st00_flow.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
int main(int argc,char**argv){
    try{
        if(argc!=2){std::cerr<<"usage: l3inspect_st00_flow file.l3flow\n";return 2;}
        std::ifstream f(argv[1],std::ios::binary); if(!f) throw std::runtime_error("cannot open input");
        std::vector<uint8_t>b{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        const auto h=l3::inspect_l3st00_flow(b,true);
        std::cout<<"L3SF v"<<h.version<<" areas="<<h.area_count<<" objects="<<h.object_count<<" aux="<<h.aux_count
                 <<" bytes="<<h.file_bytes<<" crc=PASS\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}
}
