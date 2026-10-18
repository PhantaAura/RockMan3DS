#include "legends3ds/tutorial_runtime.h"
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
        if (argc != 4) {
            std::cerr << "usage: l3runtime_pack_probe st00.l3flow area_XX.l3ebd area_index\n";
            return 2;
        }
        const auto flow = read_file(argv[1]);
        const auto ebd = read_file(argv[2]);
        const size_t area = static_cast<size_t>(std::stoul(argv[3], nullptr, 0));
        const auto r = l3::load_tutorial_runtime_area(flow, ebd, area, true);
        size_t works = 0, unique = 0, narrowed = 0, ambiguous = 0, none = 0;
        for (const auto& i : r.stage.instances) {
            if (i.work) ++works;
            switch (i.ebd.state) {
                case l3::StageEbdBindingState::UniqueClassSubtype: ++unique; break;
                case l3::StageEbdBindingState::AmbiguousNarrowedBySourceByte3: ++narrowed; break;
                case l3::StageEbdBindingState::AmbiguousClassSubtype: ++ambiguous; break;
                case l3::StageEbdBindingState::NoLocalCandidate: ++none; break;
            }
        }
        std::cout << "area=" << r.area_index
                  << " objects=" << r.flow.objects.records.size()
                  << " aux=" << r.flow.aux.records.size()
                  << " ebd_rows=" << r.ebd.entries.size()
                  << " work_slots=" << works
                  << " ebd_unique=" << unique
                  << " ebd_narrowed=" << narrowed
                  << " ebd_ambiguous=" << ambiguous
                  << " ebd_none=" << none << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
