#include "legends3ds/st00_source.h"
#include "legends3ds/l3map.h"
#include <fstream>
#include <iostream>
using namespace l3;
int main(int argc,char**argv){
    if(argc!=6&&argc!=7){std::cerr<<"usage: l3cook_st00 MDT IDX STG HED out.l3map [HD2]\n";return 2;}
    try{auto src=parse_st00(read_file(argv[1]),read_file(argv[2]),read_file(argv[3]),read_file(argv[4]),argc==7?read_file(argv[6]):std::vector<uint8_t>{});CookStats s;auto cooked=cook_st00_map(src,&s);std::ofstream f(argv[5],std::ios::binary);f.write((char*)cooked.data(),cooked.size());if(!f)throw std::runtime_error("write failed");
        std::cout<<"L3MP v1 cooked\nareas="<<s.area_count<<" cells="<<s.cell_count<<" positive="<<s.positive_cell_count<<" negative="<<s.negative_cell_count<<"\nidx="<<s.idx_count<<" descriptors="<<s.descriptor_count<<" contact_lists="<<s.contact_list_count<<" groups="<<s.contact_group_count<<" primitives="<<s.contact_primitive_count<<"\nbytes="<<cooked.size()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
