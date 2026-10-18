#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3inspect_skeleton FILE.l3skel\n";return 2;}auto b=l3::read_file(argv[1]);auto h=l3::inspect_l3skeleton(b,true);std::cout<<"L3SK v"<<h.version<<" parts="<<h.part_count<<" special_offset=0x"<<std::hex<<h.source_special_offset<<std::dec<<" bytes="<<h.file_bytes<<" crc=PASS\n";return 0;}catch(const std::exception&e){std::cerr<<"l3inspect_skeleton: "<<e.what()<<"\n";return 1;}}
