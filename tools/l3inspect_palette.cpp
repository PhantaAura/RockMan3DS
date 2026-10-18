#include "legends3ds/ps1_texture.h"
#include "legends3ds/st00_source.h"
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3inspect_palette FILE.l3pal\n";return 2;}auto b=l3::read_file(argv[1]);auto h=l3::inspect_l3palette(b,true);std::cout<<"L3PL v"<<h.version<<" "<<unsigned(h.bpp)<<"bpp palettes="<<h.palette_rows<<" clut=("<<h.clut_x<<","<<h.clut_y<<") bytes="<<h.file_bytes<<" crc=PASS\n";return 0;}catch(const std::exception&e){std::cerr<<"l3inspect_palette: "<<e.what()<<"\n";return 1;}}
