#include "legends3ds/audio_catalog.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3inspect_audio_catalog FILE.l3ac\n";return 2;}std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("cannot open");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});auto c=l3::load_l3audio_catalog(b,true);size_t xa=0,str=0,aligned=0;for(auto&e:c.entries){xa+=e.kind==l3::RuntimeAudioSourceKind::Xa;str+=e.kind==l3::RuntimeAudioSourceKind::Str;aligned+=e.aligned_2048;}std::cout<<"sources="<<c.entries.size()<<" xa="<<xa<<" str="<<str<<" aligned_2048="<<aligned<<" routing_proven="<<(c.routing_proven?1:0)<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
