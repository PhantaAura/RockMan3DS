#include "legends3ds/ebd_directory.h"
#include "legends3ds/mml_message.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open: " + path.string());
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3script_pack_probe SCRIPT_DIR\n";
            return 2;
        }
        const fs::path root = fs::path(argv[1]);
        if (!fs::is_directory(root)) throw std::runtime_error("script directory not found");

        size_t ebd_files = 0;
        size_t msg_files = 0;
        uint64_t ebd_rows = 0;
        uint64_t message_blocks = 0;
        uint64_t message_tokens = 0;
        uint64_t runtime_events = 0;
        uint64_t audio_events = 0;
        uint64_t script_events = 0;

        for (const auto& de : fs::recursive_directory_iterator(root)) {
            if (!de.is_regular_file()) continue;
            const auto ext = de.path().extension().string();
            if (ext == ".l3ebd") {
                const auto dir = l3::load_l3ebd_directory(read_file(de.path()), true);
                ++ebd_files;
                ebd_rows += dir.entries.size();
            } else if (ext == ".l3msg") {
                const auto msg = l3::load_l3message(read_file(de.path()), true);
                ++msg_files;
                message_blocks += msg.blocks.size();
                for (const auto& block : msg.blocks) message_tokens += block.tokens.size();
                for (size_t block = 0; block < msg.blocks.size(); ++block) {
                    l3::MmlMessageRuntimeCursor cursor(msg);
                    cursor.start_block(block);
                    l3::MmlRuntimeEvent ev{};
                    while (cursor.next(ev)) {
                        ++runtime_events;
                        if (ev.kind == l3::MmlRuntimeEventKind::Audio) ++audio_events;
                        if (ev.kind == l3::MmlRuntimeEventKind::Wait ||
                            ev.kind == l3::MmlRuntimeEventKind::Selection ||
                            ev.kind == l3::MmlRuntimeEventKind::Multi ||
                            ev.kind == l3::MmlRuntimeEventKind::Control ||
                            ev.kind == l3::MmlRuntimeEventKind::UnknownControl) {
                            ++script_events;
                        }
                    }
                }
            }
        }

        if (ebd_files == 0 || msg_files == 0) throw std::runtime_error("no cooked EBD/MSG resources found");
        std::cout << "ebd_files=" << ebd_files
                  << " ebd_rows=" << ebd_rows
                  << " msg_files=" << msg_files
                  << " msg_blocks=" << message_blocks
                  << " msg_tokens=" << message_tokens
                  << " runtime_events=" << runtime_events
                  << " audio_events=" << audio_events
                  << " script_events=" << script_events << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
