#include "legends3ds/mml_message.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
static std::vector<uint8_t> read_file(const char* p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open input");return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
int main(int argc,char**argv){try{if(argc!=3){std::cerr<<"usage: l3cook_msg RAW_MSG_PAYLOAD OUT.l3msg\n";return 2;}auto msg=l3::parse_mml_message_payload(read_file(argv[1]));auto stats=l3::inspect_mml_message(msg);auto out=l3::cook_l3message(msg);std::ofstream f(argv[2],std::ios::binary);f.write(reinterpret_cast<const char*>(out.data()),static_cast<std::streamsize>(out.size()));if(!f)throw std::runtime_error("write failed");std::cout<<"L3MS v1 blocks="<<stats.blocks<<" tokens="<<(stats.glyph_bytes+stats.newline_bytes+stats.control_commands+stats.unknown_control_bytes)<<" audio="<<stats.audio_commands<<" bytes="<<out.size()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"l3cook_msg: "<<e.what()<<"\n";return 1;}}
