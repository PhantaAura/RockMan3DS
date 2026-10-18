#include "legends3ds/ps1_texture.h"
#include "legends3ds/st00_source.h"
#include <iostream>
int main(int argc,char**argv){
    try{
        if(argc!=2){std::cerr<<"usage: l3inspect_texture FILE.l3tex\n";return 2;}
        auto b=l3::read_file(argv[1]);auto h=l3::inspect_l3texture(b,true);
        std::cout<<"L3TX v"<<h.version<<" "<<h.width_pixels<<"x"<<h.height<<" "<<unsigned(h.bpp)<<"bpp"
                 <<" palettes="<<h.palette_rows<<" image_vram=("<<h.image_x_words<<","<<h.image_y<<")"
                 <<" clut=("<<h.clut_x<<","<<h.clut_y<<") bytes="<<h.file_bytes<<" crc=PASS\n";
        return 0;
    }catch(const std::exception&e){std::cerr<<"l3inspect_texture: "<<e.what()<<"\n";return 1;}
}
