#include "legends3ds/resource_catalog.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
static std::vector<uint8_t> read_file(const char* p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open input");return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3inspect_resource_catalog resource_catalog.l3rc\n";return 2;}auto c=l3::load_l3resource_catalog(read_file(argv[1]),true);size_t e=0,m=0;for(const auto&r:c.entries){if(r.kind==l3::RuntimeResourceKind::Ebd)++e;else ++m;}std::cout<<"resources="<<c.entries.size()<<" ebd="<<e<<" msg="<<m<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
