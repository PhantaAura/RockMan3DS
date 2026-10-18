#include "legends3ds/pbd_model.h"
#include "legends3ds/st00_source.h"
#include <iomanip>
#include <iostream>
int main(int argc,char**argv){if(argc!=2){std::cerr<<"usage: l3inspect_model file.l3model\n";return 2;}try{auto h=l3::inspect_l3model(l3::read_file(argv[1]),true);std::cout<<"L3MD version "<<h.version<<" CRC OK\nparts "<<h.stats.part_count<<" vertices "<<h.stats.vertex_count<<" triangles "<<h.stats.triangle_count<<" quads "<<h.stats.quad_count<<"\nbytes "<<h.file_bytes<<" crc32 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<h.payload_crc32<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
