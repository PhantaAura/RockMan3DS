#include "legends3ds/pbd_model.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char**argv){if(argc!=3){std::cerr<<"usage: l3cook_pbd player.PBD out.l3model\n";return 2;}try{auto src=l3::parse_player_pbd_mesh(l3::read_file(argv[1]));l3::L3ModelStats s;auto out=l3::cook_l3model(src,&s);std::ofstream f(argv[2],std::ios::binary);f.write((char*)out.data(),out.size());if(!f)throw std::runtime_error("write failed");std::cout<<"L3MD v1 cooked\nparts="<<s.part_count<<" vertices="<<s.vertex_count<<" triangles="<<s.triangle_count<<" quads="<<s.quad_count<<"\ninferred_ps1_base=0x"<<std::hex<<std::uppercase<<src.inferred_load_base<<std::dec<<"\nbytes="<<out.size()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
