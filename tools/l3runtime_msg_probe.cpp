#include "legends3ds/mml_message.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<uint8_t> read_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error(std::string("cannot open ") + path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: l3runtime_msg_probe message.l3msg\n"; return 2; }
        const auto msg = l3::load_l3message(read_file(argv[1]), true);
        size_t glyph=0,newline=0,audio=0,wait=0,next=0,end=0,sel=0,multi=0,control=0,unknown=0;
        for (size_t b=0;b<msg.blocks.size();++b) {
            l3::MmlMessageRuntimeCursor c(msg); c.start_block(b); l3::MmlRuntimeEvent e{};
            while (c.next(e)) {
                switch(e.kind) {
                    case l3::MmlRuntimeEventKind::Glyph: ++glyph; break;
                    case l3::MmlRuntimeEventKind::Newline: ++newline; break;
                    case l3::MmlRuntimeEventKind::Audio: ++audio; break;
                    case l3::MmlRuntimeEventKind::Wait: ++wait; break;
                    case l3::MmlRuntimeEventKind::Next: ++next; break;
                    case l3::MmlRuntimeEventKind::End: ++end; break;
                    case l3::MmlRuntimeEventKind::Selection: ++sel; break;
                    case l3::MmlRuntimeEventKind::Multi: ++multi; break;
                    case l3::MmlRuntimeEventKind::Control: ++control; break;
                    case l3::MmlRuntimeEventKind::UnknownControl: ++unknown; break;
                    case l3::MmlRuntimeEventKind::BlockEnd: break;
                }
            }
        }
        std::cout << "blocks="<<msg.blocks.size()<<" glyph="<<glyph<<" newline="<<newline
                  <<" audio="<<audio<<" wait="<<wait<<" next="<<next<<" end="<<end
                  <<" sel="<<sel<<" multi="<<multi<<" other_control="<<control
                  <<" unknown="<<unknown<<"\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"error: "<<e.what()<<"\n"; return 1; }
}
