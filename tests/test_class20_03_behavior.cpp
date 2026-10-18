#include "legends3ds/class20_03_behavior.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using Values = std::array<int64_t, 10>;
Values values(const l3::Class20_03BehaviorState& s) {
    return {s.parameter_03, s.action_09, s.phase_0a, s.animation_ac,
            s.flags_328, s.timer_32c, s.work_330, s.heading_56, s.turn_66, s.work_36c};
}
void require(bool check, const char* why) { if (!check) throw std::runtime_error(why); }
}

int main(int argc, char** argv) {
    try {
        require(argc == 3, "action and distance source fixture paths required");
        std::ifstream file(argv[1]);
        require(static_cast<bool>(file), "source fixture unavailable");
        std::string line;
        std::getline(file, line);
        size_t cases = 0;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream row(line);
            int kind;
            Values before{}, after{};
            uint32_t distance, packet;
            uint16_t target, random;
            bool predicate;
            require(static_cast<bool>(row >> kind), "missing operation");
            for (auto& value : before) require(static_cast<bool>(row >> value), "missing before field");
            require(static_cast<bool>(row >> distance >> packet >> predicate >> target >> random), "missing inputs");
            for (auto& value : after) require(static_cast<bool>(row >> value), "missing output field");
            std::string extra;
            require(!(row >> extra), "unexpected fixture field");
            l3::Class20_03BehaviorState state{
                static_cast<uint8_t>(before[0]), static_cast<uint8_t>(before[1]),
                static_cast<uint8_t>(before[2]), static_cast<uint8_t>(before[3]),
                static_cast<uint16_t>(before[4]), static_cast<int16_t>(before[5]),
                static_cast<int16_t>(before[6]), static_cast<uint16_t>(before[7]),
                static_cast<int16_t>(before[8]), static_cast<int16_t>(before[9])};
            if (kind == 0) {
                l3::Class20_03ActionInputs input{distance, packet, predicate, target};
                require(l3::class20_03_step_action(state, input) == l3::Class20_03ActionStatus::SourceBackedPartial,
                        "supported source action rejected");
            } else if (kind == 1) l3::class20_03_select_action(state, random);
            else require(false, "unknown operation");
            if (values(state) != after) {
                std::cerr << "first divergent case " << cases << '\n';
                return 1;
            }
            ++cases;
        }
        require(cases != 0, "empty fixture");
        std::ifstream distances(argv[2]);
        require(static_cast<bool>(distances), "distance source fixture unavailable");
        std::getline(distances, line);
        size_t distance_cases = 0;
        while (std::getline(distances, line)) {
            if (line.empty()) continue;
            std::istringstream row(line);
            int16_t ax, az, px, pz;
            uint32_t expected;
            require(static_cast<bool>(row >> ax >> az >> px >> pz >> expected), "malformed distance case");
            require(l3::class20_03_distance_work80(ax, az, px, pz) == expected, "distance source mismatch");
            ++distance_cases;
        }
        require(distance_cases != 0, "empty distance fixture");
        l3::Class20_03BehaviorState blocked{};
        blocked.phase_0a = 2;
        const auto before = values(blocked);
        require(l3::class20_03_step_action(blocked, {}) == l3::Class20_03ActionStatus::NeedsWork80,
                "unknown distance must not become zero");
        require(values(blocked) == before, "missing evidence changed state");
        l3::Class20_03ActionInputs far_input;
        far_input.work_80 = UINT32_MAX;
        require(l3::class20_03_step_action(blocked, far_input) == l3::Class20_03ActionStatus::NeedsPacket,
                "unknown packet must not become zero");
        require(values(blocked) == before, "missing packet changed state");
        blocked.parameter_03 = 0x20;
        const auto predicate_before = values(blocked);
        require(l3::class20_03_step_action(blocked, {}) == l3::Class20_03ActionStatus::NeedsPredicate,
                "unknown predicate must remain unknown");
        require(values(blocked) == predicate_before, "missing predicate changed state");
        blocked.action_09 = 1;
        blocked.phase_0a = 0;
        require(l3::class20_03_step_action(blocked, {}) == l3::Class20_03ActionStatus::NeedsTargetHeading,
                "unproved atan must remain unknown");
        blocked.action_09 = 3;
        require(l3::class20_03_step_action(blocked, {}) == l3::Class20_03ActionStatus::UnknownAction,
                "unported action must remain unsupported");
        std::cout << cases << " source action/selector cases x10 fields; missing-input guards passed\n";
        std::cout << distance_cases << " source distance store cases passed\n";
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
