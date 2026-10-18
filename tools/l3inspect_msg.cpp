#include "legends3ds/mml_message.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<uint8_t> read_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input");
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3inspect_msg RAW_MSG_PAYLOAD\n";
            return 2;
        }
        const auto msg = l3::parse_mml_message_payload(read_file(argv[1]));
        const auto s = l3::inspect_mml_message(msg);
        std::cout << "blocks=" << s.blocks
                  << " empty_blocks=" << s.empty_blocks
                  << " glyph_bytes=" << s.glyph_bytes
                  << " newline_bytes=" << s.newline_bytes
                  << " controls=" << s.control_commands
                  << " unknown_high_bytes=" << s.unknown_control_bytes
                  << " audio=" << s.audio_commands
                  << " wait=" << s.wait_commands
                  << " next=" << s.next_commands
                  << " sel=" << s.selection_commands
                  << " multi=" << s.multi_commands
                  << " camera=" << s.camera_commands << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "l3inspect_msg: " << e.what() << "\n";
        return 1;
    }
}
