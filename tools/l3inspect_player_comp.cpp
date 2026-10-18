#include "legends3ds/player_composition.h"
#include "legends3ds/st00_source.h"
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: l3inspect_player_comp file.l3pc\n"; return 2; }
    try { const auto h = l3::inspect_l3player_composition(l3::read_file(argv[1]), true); std::cout << "L3PC v" << h.version << " slots=" << h.component_slots << " attachments=" << h.attachment_count << " bytes=" << h.file_bytes << " CRC=PASS\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}
