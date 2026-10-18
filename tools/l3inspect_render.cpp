#include "legends3ds/st00_render.h"
#include "legends3ds/st00_source.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3inspect_render FILE.l3render\n";return 2;}auto b=l3::read_file(argv[1]);auto h=l3::inspect_l3render(b,true);std::cout<<"L3RG v"<<h.version<<" descriptors="<<h.stats.descriptor_count<<" records="<<h.stats.record_count<<" streams="<<h.stats.stream_count<<" refs="<<h.stats.stream_references<<" materials="<<h.stats.material_words<<" primitives="<<h.stats.primitive_count<<" bytes="<<h.file_bytes<<" crc=PASS\n";return 0;}catch(const std::exception&e){std::cerr<<"l3inspect_render: "<<e.what()<<"\n";return 1;}}
