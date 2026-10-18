#include "legends3ds/actor_sequence.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::array<l3::ActorSequenceRecord, 256> authored_records() {
    std::array<l3::ActorSequenceRecord, 256> records{};
    for (size_t i = 0; i < records.size(); ++i)
        records[i] = {static_cast<uint8_t>(i * 3), static_cast<uint8_t>(i * 7),
                      static_cast<uint8_t>(i ^ 0x55), 0, 1, 2, 3, 4};
    return records;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "source sequencer fixture path required");
        std::ifstream file(argv[1]);
        require(static_cast<bool>(file), "source fixture unavailable");
        std::string line;
        std::getline(file, line);
        size_t cases = 0;
        while (std::getline(file, line)) {
            std::istringstream row(line);
            uint16_t index, count, control;
            std::array<uint16_t, 5> expected{};
            require(static_cast<bool>(row >> index >> count >> control), "missing source input");
            for (auto& value : expected) require(static_cast<bool>(row >> value), "missing expected field");
            auto records = authored_records();
            records[index][3] = static_cast<uint8_t>(control);
            l3::ActorSequenceCursor cursor{index, 42, static_cast<uint8_t>(count), 77, 13};
            require(l3::actor_sequence_step(cursor, records.data(), records.size()).status ==
                    l3::ActorSequenceStatus::SourceBackedPartial, "valid source sequence rejected");
            require(std::array<uint16_t, 5>{cursor.record_index, cursor.frame_a4, cursor.countdown_a5,
                                            cursor.opaque_a6, cursor.opaque_a7} == expected,
                    "original source sequencing mismatch");
            ++cases;
        }
        require(cases != 0, "empty source fixture");
        auto records = authored_records();
        l3::ActorSequenceCursor cursor{255, 42, 1, 77, 13};
        const auto before = cursor;
        require(l3::actor_sequence_step(cursor, records.data(), records.size()).status ==
                l3::ActorSequenceStatus::MissingRecord, "unbounded next record accepted");
        require(cursor.record_index == before.record_index && cursor.countdown_a5 == before.countdown_a5,
                "failed sequence step modified cursor");
        require(l3::actor_sequence_step(cursor, nullptr, 256).status ==
                l3::ActorSequenceStatus::MissingRecord, "null record cache accepted");
        require(l3::actor_sequence_step(cursor, records.data(), 257).status ==
                l3::ActorSequenceStatus::MissingRecord, "over-budget record cache accepted");
        std::cout << cases << " original sequence cases x5 fields; malformed-cache guards passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
