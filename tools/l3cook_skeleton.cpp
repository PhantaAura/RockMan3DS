#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
static uint32_t parse_u32(const char* s){size_t used=0;unsigned long v=std::stoul(s,&used,0);if(used!=std::string(s).size()||v>0xFFFFFFFFul)throw std::runtime_error("invalid 32-bit load base");return static_cast<uint32_t>(v);}
int main(int argc,char**argv){try{if(argc!=4){std::cerr<<"usage: l3cook_skeleton M.PBD M_LOAD_BASE OUT.l3skel\n";return 2;}auto m=l3::read_file(argv[1]);auto s=l3::parse_pbd_main_skeleton(m,parse_u32(argv[2]));auto out=l3::cook_l3skeleton(s);std::ofstream f(argv[3],std::ios::binary);f.write(reinterpret_cast<const char*>(out.data()),static_cast<std::streamsize>(out.size()));if(!f)throw std::runtime_error("failed writing output");std::cout<<"parts="<<l3::kPbdMainPosePartCount<<" special_offset=0x"<<std::hex<<s.source_special_offset<<std::dec<<" bytes="<<out.size()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"l3cook_skeleton: "<<e.what()<<"\n";return 1;}}
