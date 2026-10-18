#include "legends3ds/l3map.h"
#include "legends3ds/st00_source.h"
#include <iomanip>
#include <iostream>
using namespace l3;
int main(int argc,char**argv){if(argc!=2){std::cerr<<"usage: l3inspect_map file.l3map\n";return 2;}try{auto b=read_file(argv[1]);auto h=inspect_l3map(b,true);std::cout<<"L3MP version "<<h.version<<" CRC OK\n"<<"areas "<<h.stats.area_count<<" cells "<<h.stats.cell_count<<" (+"<<h.stats.positive_cell_count<<" / -"<<h.stats.negative_cell_count<<")\n"<<"idx "<<h.stats.idx_count<<" descriptors "<<h.stats.descriptor_count<<"\n"<<"contact lists "<<h.stats.contact_list_count<<" groups "<<h.stats.contact_group_count<<" primitives "<<h.stats.contact_primitive_count<<"\n"<<"bytes "<<h.file_bytes<<" crc32 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<h.payload_crc32<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
