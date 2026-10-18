#include "legends3ds/player_composition.h"
#include "legends3ds/st00_source.h"
#include <array>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "usage: l3cook_player_comp BODY.PBD HEAD.PBD RIGHT.PBD LEFT.PBD FEET.PBD output.l3pc\n";
        return 2;
    }
    try {
        std::array<l3::PbdModel, l3::kPlayerComponentSlots> components;
        for (size_t i = 0; i < components.size(); ++i) components[i] = l3::parse_player_pbd_mesh(l3::read_file(argv[1 + i]));
        const auto c = l3::build_baseline_player_composition(components);
        const auto bytes = l3::cook_l3player_composition(c);
        std::ofstream f(argv[6], std::ios::binary); f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!f) throw std::runtime_error("failed to write output");
        std::cout << "attachments=" << c.attachments.size() << " bytes=" << bytes.size() << "\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
}
