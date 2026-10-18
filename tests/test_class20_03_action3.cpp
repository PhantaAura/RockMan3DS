#include "legends3ds/class20_03_action3.h"
#include <cstdlib>
#include <new>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

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
using State = l3::Class20_03CallbackState;
using Status = l3::Class20_03Action3Status;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::vector<int64_t> values(const State& s) {
    const auto& b = s.behavior;
    std::vector<int64_t> out{b.parameter_03, b.action_09, b.phase_0a, b.animation_ac,
        b.flags_328, b.timer_32c, b.work_330, b.heading_56, b.turn_66, b.work_36c,
        s.work_00, s.state_08, s.aux_0b, s.flags_0c, s.restore_action_0e, s.previous_animation_ad};
    const auto add = [&](const auto& a) { for (auto v : a) out.push_back(v); };
    add(s.raw_14); add(s.fixed_1c); add(s.angle_54); add(s.previous_raw_2c);
    add(s.previous_fixed_34); add(s.previous_angle_5c); add(s.proxy_position_180);
    add(s.previous_proxy_position_334);
    out.insert(out.end(), {s.health_6e ? *s.health_6e : 65536,
        s.initial_health_b0 ? *s.initial_health_b0 : 65536,
        s.previous_health_b2 ? *s.previous_health_b2 : 65536, s.work_15e,
        s.random_32a, s.variant_32e, s.work_332, s.packet_33c,
        s.detection_profile_352 ? *s.detection_profile_352 : 256,
        s.detection_phase_353, s.detection_offset_35b, s.work_35c, s.work_35e,
        s.defeat_timer_360, s.work_362, s.work_370, s.child_374});
    out.push_back(s.work_64 ? *s.work_64 : 65536);
    out.push_back(s.source_delay_0d ? *s.source_delay_0d : 256);
    return out;
}
int64_t number(std::istream& row) {
    int64_t value;
    require(static_cast<bool>(row >> value), "missing source fixture field");
    return value;
}
State read_state(std::istream& row) {
    State s;
    auto& b = s.behavior;
    const auto read = [&](auto& field) { field = static_cast<std::decay_t<decltype(field)>>(number(row)); };
    read(b.parameter_03); read(b.action_09); read(b.phase_0a); read(b.animation_ac);
    read(b.flags_328); read(b.timer_32c); read(b.work_330); read(b.heading_56);
    read(b.turn_66); read(b.work_36c); read(s.work_00); read(s.state_08);
    read(s.aux_0b); read(s.flags_0c); read(s.restore_action_0e); read(s.previous_animation_ad);
    const auto array = [&](auto& a) { for (auto& v : a) read(v); };
    array(s.raw_14); array(s.fixed_1c); array(s.angle_54); array(s.previous_raw_2c);
    array(s.previous_fixed_34); array(s.previous_angle_5c); array(s.proxy_position_180);
    array(s.previous_proxy_position_334);
    const auto hp = [&]() -> std::optional<int16_t> {
        const auto n = number(row);
        return n == 65536 ? std::nullopt : std::optional<int16_t>(static_cast<int16_t>(n));
    };
    s.health_6e = hp(); s.initial_health_b0 = hp(); s.previous_health_b2 = hp();
    read(s.work_15e); read(s.random_32a); read(s.variant_32e); read(s.work_332); read(s.packet_33c);
    const auto profile = number(row);
    if (profile != 256) s.detection_profile_352 = static_cast<uint8_t>(profile);
    read(s.detection_phase_353); read(s.detection_offset_35b); read(s.work_35c); read(s.work_35e);
    read(s.defeat_timer_360); read(s.work_362); read(s.work_370); read(s.child_374);
    const auto limit = number(row), delay = number(row);
    if (limit != 65536) s.work_64 = static_cast<int16_t>(limit);
    if (delay != 256) s.source_delay_0d = static_cast<int8_t>(delay);
    return s;
}
size_t source_cases(const char* path) {
    std::ifstream file(path);
    require(static_cast<bool>(file), "source action3 fixture unavailable");
    std::string line;
    std::getline(file, line);
    size_t count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        l3::Class20_03Action3Inputs input;
        input.packet_33c = static_cast<uint32_t>(number(row));
        input.combat_record_byte_a = static_cast<uint8_t>(number(row));
        const auto target = number(row);
        if (target >= 0) input.target_heading_80031198 = static_cast<uint16_t>(target);
        input.sound_consumer_ready = number(row) != 0;
        input.release_consumer_ready = number(row) != 0;
        const auto present = number(row);
        l3::Class20_03PreparedAction3 motion;
        for (auto& v : motion.raw_14) v = static_cast<int16_t>(number(row));
        for (auto& v : motion.fixed_1c) v = static_cast<int32_t>(number(row));
        motion.source_floor_word = static_cast<int32_t>(number(row));
        motion.source_contact_word = static_cast<uint32_t>(number(row));
        motion.heading_after_turn = static_cast<uint16_t>(number(row));
        if (present) input.prepared_motion = motion;
        auto state = read_state(row);
        const auto expected = read_state(row);
        const auto sound = number(row), release = number(row);
        std::string extra;
        require(!(row >> extra), "extra source action3 fixture field");
        const auto result = l3::class20_03_step_action3(state, input);
        require(result.status == Status::SourceBackedPartial, "source action3 case unexpectedly held");
        if (values(state) != values(expected)) {
            const auto actual = values(state), want = values(expected);
            for (size_t i = 0; i < actual.size(); ++i) if (actual[i] != want[i])
                std::cerr << "case " << count << " field " << i << " expected " << want[i]
                          << " actual " << actual[i] << '\n';
            throw std::runtime_error("original action3 instruction differential mismatch");
        }
        require((result.sound_8001a050 ? *result.sound_8001a050 : -1) == sound, "source action3 sound intent mismatch");
        require(result.release_80031824 == (release != 0), "source pool release intent mismatch");
        ++count;
    }
    require(count != 0, "empty source action3 fixture");
    return count;
}
void guards_and_allocation() {
    State s;
    s.behavior.action_09 = 3;
    s.behavior.work_330 = 19;
    s.aux_0b = 91;
    l3::Class20_03Action3Inputs in;
    const auto unknown = values(s);
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsPacket,
            "action3 invented packet ownership");
    require(values(s) == unknown, "missing packet leaked state writes");
    in.packet_33c = 0x02000000;
    require(l3::class20_03_step_action3(s, in).status == Status::StalePacket,
            "inconsistent packet was not rejected");
    require(values(s) == unknown, "stale packet leaked source mask store");
    in.packet_33c = 0;
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsCombatProfile,
            "phase0 invented selected combat record byteA");
    require(values(s) == unknown, "missing combat record leaked entry phase/request");
    in.combat_record_byte_a = 0;
    require(l3::class20_03_step_action3(s, in).status == Status::SourceBackedPartial,
            "source zero combat byte rejected");
    require(s.behavior.phase_0a == 1 && s.behavior.animation_ac == 2 && s.work_64 == 0 &&
            s.behavior.timer_32c == 4 && s.behavior.work_330 == 0,
            "phase0 source stores/fallthrough were not retained");
    s.behavior.phase_0a = 2; s.behavior.timer_32c = 1;
    s.behavior.flags_328 = 0x20;
    const auto sound_before = values(s);
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsSoundConsumer,
            "source sound was silently dropped");
    require(values(s) == sound_before, "sound hold leaked timer/phase changes");
    in.sound_consumer_ready = true;
    const auto delay_hold = l3::class20_03_step_action3(s, in);
    require(delay_hold.status == Status::NeedsDelay && !delay_hold.sound_8001a050,
            "missing signed byte0D fabricated timer or leaked sound intent");
    require(values(s) == sound_before, "delay hold leaked state");
    s.source_delay_0d = -128;
    const auto sound = l3::class20_03_step_action3(s, in);
    require(sound.status == Status::SourceBackedPartial && sound.sound_8001a050 == 180 &&
            s.behavior.phase_0a == 3 && s.behavior.timer_32c == -128 && !(s.behavior.flags_328 & 0x20u),
            "original signed source delay or sound intent changed");
    s.behavior.phase_0a = 3; s.behavior.timer_32c = 4; s.work_64.reset();
    const auto limit_before = values(s);
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsTurnLimit,
            "external phase3 entry invented source work64");
    require(values(s) == limit_before, "limit hold leaked decrement");
    s.work_64 = 16;
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsTargetHeading,
            "phase3 invented target angle");
    in.target_heading_80031198 = 1;
    const auto moving_before = values(s);
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsMotion,
            "phase3 fabricated movement/contact");
    require(values(s) == moving_before, "missing motion leaked turn/timer");
    l3::Class20_03PreparedAction3 prepared;
    prepared.raw_14 = {1, 10, 3, 4}; prepared.fixed_1c = {5, 6, 7, 8};
    prepared.source_floor_word = 10; prepared.heading_after_turn = 7;
    in.prepared_motion = prepared;
    require(l3::class20_03_step_action3(s, in).status == Status::StaleMotion,
            "wrong prepared heading accepted");
    require(values(s) == moving_before, "stale motion leaked turn/body");
    s.previous_raw_2c = {21, 10, 23, 24}; s.previous_fixed_34 = {25, 26, 27, 28};
    prepared.heading_after_turn = 1; in.prepared_motion = prepared;
    require(l3::class20_03_step_action3(s, in).status == Status::SourceBackedPartial,
            "source prepared flat motion rejected");
    require(s.raw_14 == prepared.raw_14 && s.fixed_1c[1] == 10 * 65536 &&
            s.behavior.heading_56 == 1 && s.behavior.timer_32c == 3,
            "source motion/turn/floor not committed together");
    // Rollback uses complete previous callback snapshots, not current raw Y.
    s.behavior.phase_0a = 3; s.behavior.timer_32c = 4;
    prepared.source_floor_word = 43; in.prepared_motion = prepared;
    require(l3::class20_03_step_action3(s, in).status == Status::SourceBackedPartial,
            "source floor rollback rejected");
    require(s.behavior.action_09 == 8 && s.behavior.phase_0a == 0 && s.aux_0b == 0 &&
            (s.behavior.parameter_03 & 0x40u) && s.raw_14 == s.previous_raw_2c &&
            s.fixed_1c == s.previous_fixed_34 && s.behavior.timer_32c == -1,
            "source 33-unit rollback changed full snapshots or action8 stores");
    s.behavior.action_09 = 3; s.behavior.phase_0a = 3; s.behavior.timer_32c = 0;
    s.behavior.animation_ac = 255; s.behavior.parameter_03 = 0;
    in.target_heading_80031198.reset(); in.prepared_motion.reset();
    require(l3::class20_03_step_action3(s, in).status == Status::SourceBackedPartial &&
            s.behavior.phase_0a == 5 && s.behavior.animation_ac == 0 &&
            s.behavior.timer_32c == 9 && s.behavior.work_330 == 0,
            "phase3 completion did not fall through to phase5 timer body");
    s.behavior.parameter_03 = 0x20; s.behavior.phase_0a = 5; s.work_00 = 3;
    const auto release_before = values(s);
    require(l3::class20_03_step_action3(s, in).status == Status::NeedsReleaseConsumer,
            "source pool release was silently omitted");
    require(values(s) == release_before, "release hold leaked scalar or timer");
    in.release_consumer_ready = true;
    const auto release = l3::class20_03_step_action3(s, in);
    require(release.status == Status::SourceBackedPartial && release.release_80031824 && s.work_00 == 0,
            "release acknowledgement did not retain original work00 clear");
    State idle; idle.behavior.action_09 = 3;
    l3::Class20_03Action3Inputs entry; entry.packet_33c = 0; entry.combat_record_byte_a = 29;
    const auto before_allocations = allocation_count;
    bool all_ready = true;
    for (int i = 0; i < 480; ++i) {
        auto copy = idle;
        all_ready &= l3::class20_03_step_action3(copy, entry).status == Status::SourceBackedPartial;
    }
    require(all_ready && allocation_count == before_allocations, "action3 hot loop allocated");
}
} // namespace
int main(int argc, char** argv) {
    try {
        guards_and_allocation();
        const auto cases = argc > 1 ? source_cases(argv[1]) : 0;
        std::cout << "class20/03 action3 guards and 480 allocation-free entries PASS; original scalar cases "
                  << cases << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n'; return 1;
    }
}
