#include "legends3ds/actor_animation_bank.h"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <set>
#include <stdexcept>
#include <string>

static size_t allocation_count = 0;
void* operator new(std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    ++allocation_count;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
std::string quoted(const std::string& text) {
    std::string out = "\"";
    for (const char c : text) {
        if (c == '\\' || c == '"') out += '\\';
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out + '"';
}
bool equal_cursor(const l3::ActorSequenceCursor& a, const l3::ActorSequenceCursor& b) {
    return a.record_index == b.record_index && a.frame_a4 == b.frame_a4 &&
        a.countdown_a5 == b.countdown_a5 && a.opaque_a6 == b.opaque_a6 &&
        a.opaque_a7 == b.opaque_a7;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected existing cooked owner EBD");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("owner EBD unavailable");
        const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),
                                       std::istreambuf_iterator<char>()};
        const auto ebd = l3::load_l3ebd_directory(bytes);
        const auto rows = l3::find_ebd_entries(ebd, 0x20, 3);
        if (rows.empty()) throw std::runtime_error("owner EBD lacks class20/03 row");
        const auto row = rows.front(); // proved initializer first-match lookup.
        const auto mesh = l3::decode_ebd_actor_mesh(ebd, row);
        // Explicit source call sites: initializer/action0 select0; actions1/2/9
        // select1; action3 selects2 then increments to3; actions5/10 select4; action6 selects6;
        // action4 selects21 then increments to22. No names/semantics are inferred.
        constexpr std::array<uint8_t, 8> requests{{0, 1, 2, 3, 4, 6, 21, 22}};
        constexpr size_t cap = 512 * 1024;
        std::cout << "{\"schema\":\"actor-animation-bank-owner-probe/1\","
                     "\"status\":\"SOURCE_BACKED_PARTIAL\",\"source_row\":" << row
                  << ",\"requests\":[";
        bool first = true;
        for (const auto request : requests) {
            if (!first) std::cout << ',';
            first = false;
            std::cout << "{\"request\":" << unsigned(request);
            try {
                const auto bank = l3::decode_ebd_actor_animation_bank(ebd, row, mesh, &request, 1, cap);
                const auto& sequence = bank.requests[0].sequence;
                std::set<uint32_t> windows;
                size_t distinct = 0;
                for (size_t i = 0; i < sequence.poses.size(); ++i) {
                    const auto& pose = sequence.poses[i];
                    windows.insert(pose.pose_source_offset);
                    bool unique = true;
                    for (size_t j = 0; j < i; ++j) {
                        const auto& previous = sequence.poses[j];
                        if (pose.root_translation == previous.root_translation &&
                            pose.rotations == previous.rotations &&
                            pose.base_translation == previous.base_translation)
                            unique = false;
                    }
                    distinct += unique;
                }
                std::cout << ",\"supported\":true,\"reachable_records\":" << sequence.records.size()
                          << ",\"distinct_pose_windows\":" << windows.size()
                          << ",\"distinct_decoded_poses\":" << distinct
                          << ",\"decoded_bank_elements\":" << bank.decoded_bytes;
            } catch (const std::exception& error) {
                std::cout << ",\"supported\":false,\"reason\":" << quoted(error.what());
            }
            std::cout << '}';
        }
        std::cout << "],\"combined_bank\":{";
        try {
            const auto all = l3::decode_ebd_actor_animation_bank(ebd, row, mesh,
                requests.data(), requests.size(), cap - mesh.footprint.decoded_bytes);
            std::cout << "\"supported\":true,\"request_count\":" << all.requests.size()
                      << ",\"decoded_bank_elements\":" << all.decoded_bytes
                      << ",\"decoded_mesh_and_bank_elements\":"
                      << mesh.footprint.decoded_bytes + all.decoded_bytes;
        } catch (const std::exception& error) {
            std::cout << "\"supported\":false,\"reason\":" << quoted(error.what());
        }
        std::cout << "},\"request0_1_playback\":{";
        try {
            constexpr std::array<uint8_t, 2> minimal{{0, 1}};
            const auto bank = l3::decode_ebd_actor_animation_bank(ebd, row, mesh,
                minimal.data(), minimal.size(), cap - mesh.footprint.decoded_bytes);
            auto live = bank.requests[0].sequence.poses[0];
            auto cursor = l3::actor_sequence_begin(0, live.sequencing_record);
            const auto storage = live.base_translation.data();
            const auto allocations_before = allocation_count;
            uint32_t resets = 0, pose_changes = 0;
            for (uint32_t tick = 0; tick < 960; ++tick) {
                if (tick % 120 == 0) {
                    const uint8_t request = static_cast<uint8_t>((tick / 120) & 1u);
                    const auto reset = l3::actor_animation_reset(bank, request, live, cursor);
                    if (reset.status != l3::ActorAnimationStatus::SourceBackedPartial ||
                        live.request != request || live.record_index != 0)
                        throw std::runtime_error("owner request reset failed");
                    ++resets;
                }
                const auto stepped = l3::actor_animation_step(bank, live, cursor);
                if (stepped.status != l3::ActorAnimationStatus::SourceBackedPartial)
                    throw std::runtime_error("owner bank playback failed");
                pose_changes += stepped.record_changed;
            }
            const auto before = cursor;
            const auto pose_offset = live.pose_source_offset;
            const auto selected_request = live.request;
            const auto missing = l3::actor_animation_reset(bank, 255, live, cursor);
            const bool missing_held = missing.status == l3::ActorAnimationStatus::MissingRequest &&
                equal_cursor(cursor, before) && live.pose_source_offset == pose_offset &&
                live.request == selected_request;
            const bool no_allocations = allocation_count == allocations_before;
            if (!missing_held || !no_allocations || live.base_translation.data() != storage)
                throw std::runtime_error("owner allocation/unsupported-request hold failed");
            std::cout << "\"supported\":true,\"ticks\":960,\"resets\":" << resets
                      << ",\"pose_changes\":" << pose_changes
                      << ",\"decoded_bank_elements\":" << bank.decoded_bytes
                      << ",\"no_frame_allocations\":true,\"missing_request_holds\":true";
        } catch (const std::exception& error) {
            std::cout << "\"supported\":false,\"reason\":" << quoted(error.what());
        }
        std::cout << "},\"cadence\":\"CANDIDATE\",\"complete_request_reset\":\"UNKNOWN\","
                     "\"full_ai\":false,\"retail_payload_exported\":false}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
