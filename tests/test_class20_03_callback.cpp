#include "legends3ds/class20_03_behavior.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {
using State = l3::Class20_03CallbackState;
using Status = l3::Class20_03CallbackStatus;
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
    return s;
}
size_t source_cases(const char* path) {
    std::ifstream file(path);
    require(static_cast<bool>(file), "source callback fixture unavailable");
    std::string line;
    std::getline(file, line);
    const bool prepared_schema = line.find("prepared12") != std::string::npos;
    size_t count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        const auto kind = number(row);
        l3::Class20_03CallbackInputs input;
        const auto sample = number(row), detection = number(row), range = number(row);
        const auto distance = number(row), packet = number(row), predicate = number(row), target = number(row);
        if (sample >= 0) input.source_random_sample = static_cast<uint16_t>(sample);
        if (detection >= 0) input.detection_8003648c = detection != 0;
        if (range >= 0) input.detection_range_byte2 = static_cast<uint8_t>(range);
        if (distance >= 0) input.action.work_80 = static_cast<uint32_t>(distance);
        if (packet >= 0) input.action.packet_33c = static_cast<uint32_t>(packet);
        if (predicate >= 0) input.action.predicate_8001da58 = predicate != 0;
        if (target >= 0) input.action.target_heading_80031198 = static_cast<uint16_t>(target);
        input.sound_consumer_ready = number(row) != 0;
        input.animation_consumer_ready = number(row) != 0;
        input.effect_consumer_ready = number(row) != 0;
        input.proxy_consumer_ready = number(row) != 0;
        l3::Class20_03InitializeInputs init;
        const auto initial_hp = number(row), initial_profile = number(row), floor = number(row);
        if (initial_hp != 65536) init.source_initial_health = static_cast<int16_t>(initial_hp);
        if (initial_profile != 256) init.source_detection_profile = static_cast<uint8_t>(initial_profile);
        if (floor != 4294967296LL) init.source_floor_word = static_cast<int32_t>(floor);
        if (prepared_schema) {
            const auto present = number(row);
            l3::Class20_03PreparedAction2 prepared;
            for (auto& value : prepared.raw_14) value = static_cast<int16_t>(number(row));
            for (auto& value : prepared.fixed_1c) value = static_cast<int32_t>(number(row));
            prepared.source_floor_word = static_cast<int32_t>(number(row));
            prepared.source_contact_word = static_cast<uint32_t>(number(row));
            const auto cue = number(row);
            if (cue != 256) prepared.source_a6 = static_cast<int8_t>(cue);
            if (present) input.prepared_action2 = prepared;
        }
        auto state = read_state(row);
        const auto expected = read_state(row);
        const auto animation = number(row), effect = number(row), proxy = number(row), sound = number(row);
        const auto secondary_sound = prepared_schema ? number(row) : -1;
        std::string extra;
        require(!(row >> extra), "extra source callback fixture field");
        l3::Class20_03CallbackResult result;
        if (kind == 0) result = l3::class20_03_step_callback(state, input);
        else if (kind == 1) result = l3::class20_03_step_state1(state, input);
        else if (kind == 2) result = l3::class20_03_step_state1_tail(state, input.source_random_sample);
        else if (kind == 3) result.status = l3::class20_03_initialize_scalar_state(state, init);
        else require(false, "unknown source callback operation");
        require(result.status == Status::SourceBackedPartial, "source case unexpectedly held");
        if (values(state) != values(expected)) {
            const auto actual_fields = values(state), expected_fields = values(expected);
            for (size_t i = 0; i < actual_fields.size(); ++i) if (actual_fields[i] != expected_fields[i])
                std::cerr << "case " << count << " kind " << kind << " field " << i
                          << " expected " << expected_fields[i] << " actual " << actual_fields[i] << '\n';
            throw std::runtime_error("original instruction differential mismatch");
        }
        require(static_cast<int>(result.animation) == animation, "animation intent order mismatch");
        require(result.effect_8002c8cc == (effect != 0), "effect intent mismatch");
        require(result.proxy_800341bc == (proxy != 0), "proxy intent mismatch");
        require((result.sound_8001a050 ? *result.sound_8001a050 : -1) == sound, "sound intent mismatch");
        require((result.secondary_sound_8001a050 ? *result.secondary_sound_8001a050 : -1) == secondary_sound,
                "second sound intent mismatch");
        ++count;
    }
    require(count != 0, "empty source callback fixture");
    return count;
}
void guards_and_lifecycle() {
    State s;
    s.behavior.parameter_03 = 0x20;
    s.behavior.phase_0a = 1;
    s.behavior.timer_32c = 2;
    s.aux_0b = 91;
    s.packet_33c = 0xabcdef01;
    l3::Class20_03CallbackInputs in;
    const auto unchanged = values(s);
    require(l3::class20_03_step_callback(s, in).status == Status::NeedsRandomSample,
            "full callback invented BIOS sample");
    require(values(s) == unchanged, "sample hold changed snapshots or state");
    require(l3::class20_03_step_state1(s, in).status == Status::SourceBackedPartial,
            "unused sample incorrectly blocks source-used state1 body");
    require(s.behavior.timer_32c == 1 && s.packet_33c == 0xabcdef01,
            "state1 body incorrectly finalizes outer packet");
    s.behavior.timer_32c = 0;
    const auto selector_before = values(s);
    require(l3::class20_03_step_state1(s, in).status == Status::NeedsRandomSample,
            "source selector invented RNG");
    require(values(s) == selector_before, "tail hold leaked action/timer mutations");
    s.behavior.parameter_03 = 0;
    s.behavior.timer_32c = 4;
    s.detection_phase_353 = 2;
    in.action.work_80 = 0x00f00000;
    in.detection_range_byte2 = 15;
    require(l3::class20_03_step_state1(s, in).status == Status::SourceBackedPartial,
            "original unsigned range equality did not reject");
    require(s.detection_phase_353 == 0 && !(s.behavior.flags_328 & 0x10u),
            "range rejection did not clear persistent detection phase/flag");
    in.action.work_80 = 0x00efffff;
    const auto admitted_before = values(s);
    require(l3::class20_03_step_state1(s, in).status == Status::NeedsDetection,
            "admitted range fabricated detection");
    require(values(s) == admitted_before, "unresolved detection changed state");
    s.behavior.parameter_03 = 0x60;
    s.behavior.flags_328 = 6;
    s.health_6e = 128;
    s.work_00 = 3;
    s.raw_14 = {123, -456, 789, -12345};
    s.fixed_1c = {1234567, -7654321, 2147483647, -2147483647 - 1};
    in.source_random_sample = 0x1234;
    in.action.packet_33c = 0x20000010;
    in.sound_consumer_ready = true;
    in.animation_consumer_ready = true;
    in.effect_consumer_ready = true;
    const auto proxy_hold = values(s);
    const auto proxy_result = l3::class20_03_step_callback(s, in);
    require(proxy_result.status == Status::NeedsProxyConsumer, "missing proxy did not block callback");
    require(values(s) == proxy_hold && !proxy_result.sound_8001a050,
            "late consumer hold leaked health/sound/snapshot mutations");
    in.proxy_consumer_ready = true;
    const auto hit = l3::class20_03_step_callback(s, in);
    require(hit.status == Status::SourceBackedPartial && hit.sound_8001a050 == 162,
            "stat-independent source hit rejected");
    require(s.health_6e == 112 && s.previous_health_b2 == 128 && s.packet_33c == 0,
            "hit and packet clear did not commit together");
    in.action.packet_33c = s.packet_33c;
    require(l3::class20_03_step_callback(s, in).status == Status::SourceBackedPartial,
            "cleared packet callback rejected");
    require(s.health_6e == 112, "cleared packet damaged actor twice");
    in.action.packet_33c = 0x08000001;
    const auto profile_before = values(s);
    require(l3::class20_03_step_callback(s, in).status == Status::NeedsHitConsumer,
            "combat-profile-dependent branch guessed threshold");
    require(values(s) == profile_before, "unsupported hit leaked scalar changes");
    in.action.packet_33c = 0x20000080;
    require(l3::class20_03_step_callback(s, in).status == Status::NeedsHitConsumer,
            "lethal hit guessed defeat lifecycle");
    s.behavior.flags_328 = 0x40;
    s.defeat_timer_360 = 0;
    const auto defeat_before = values(s);
    require(l3::class20_03_step_state1_tail(s, 0).status == Status::NeedsDefeatLifecycle,
            "defeat expiration silently freed or retained actor");
    require(values(s) == defeat_before, "defeat hold leaked common timer writes");
    State init;
    init.raw_14 = {1, 9, 3, 8};
    l3::Class20_03InitializeInputs initial;
    initial.source_floor_word = -1;
    require(l3::class20_03_initialize_scalar_state(init, initial) == Status::SourceBackedPartial,
            "unknown unused combat fields blocked scalar initialization");
    require(!init.health_6e && !init.detection_profile_352 && init.behavior.flags_328 == 14 &&
            init.raw_14[1] == -1 && init.fixed_1c[1] == -65536,
            "scalar initializer invented HP/profile or changed known source fields");
    State missing;
    missing.behavior.parameter_03 = 0x20;
    missing.behavior.flags_328 = 2;
    const auto packet_before = values(missing);
    require(l3::class20_03_step_state1(missing, {}).status == Status::NeedsPacket,
            "unknown hit packet became zero");
    require(values(missing) == packet_before, "unknown packet initialized action prematurely");
    l3::Class20_03CallbackInputs known_packet;
    known_packet.action.packet_33c = 0x20000001;
    require(l3::class20_03_step_state1(missing, known_packet).status == Status::NeedsHealth,
            "nonzero hit invented HP");
    known_packet.action.packet_33c = 1;
    known_packet.sound_consumer_ready = true;
    require(l3::class20_03_step_state1(missing, known_packet).status == Status::SourceBackedPartial,
            "no damage-mask sound route incorrectly required HP/profile");
    missing.behavior.flags_328 = 0;
    missing.behavior.action_09 = 1;
    const auto heading_before = values(missing);
    require(l3::class20_03_step_state1(missing, {}).status == Status::NeedsTargetHeading,
            "state1 invented atan input");
    require(values(missing) == heading_before, "atan hold leaked detection/action mutations");
    missing.behavior.action_09 = 3;
    missing.behavior.phase_0a = 0;
    missing.packet_33c = 0;
    const auto action3_before = values(missing);
    require(l3::class20_03_step_state1(missing, {}).status == Status::NeedsPacket,
            "new source action3 invented packet ownership");
    l3::Class20_03CallbackInputs action3_inputs;
    action3_inputs.action.packet_33c = 0;
    action3_inputs.detection_8003648c = false;
    missing.packet_33c = 0;
    require(l3::class20_03_step_state1(missing, action3_inputs).status == Status::NeedsCombatProfile,
            "new source action3 invented selected combat record");
    require(values(missing) == action3_before,"action3 dependency hold changed live fields");
    // Action4 is translated in01421, but no dependency is manufactured.
    missing.behavior.action_09 = 4;
    missing.behavior.phase_0a = 0;
    const auto action4_before=values(missing);
    require(l3::class20_03_step_state1(missing, {}).status == Status::NeedsSoundConsumer,
            "action4 entry fabricated its source audio consumer");
    auto action4_inputs=l3::Class20_03CallbackInputs{};
    action4_inputs.sound_consumer_ready=true;
    require(l3::class20_03_step_state1(missing, action4_inputs).status == Status::NeedsAction4Motion,
            "action4 entry fabricated its source motion");
    require(values(missing)==action4_before,"action4 dependency hold changed live fields");
    missing.state_08 = 0;
    require(l3::class20_03_step_state1(missing, {}).status == Status::UnknownState,
            "initializer/release became active state1");
    State phase2;
    phase2.behavior.parameter_03 = 0x20;
    phase2.behavior.phase_0a = 2;
    require(l3::class20_03_step_state1(phase2, {}).status == Status::NeedsPredicate,
            "phase2 source predicate invented");
    phase2.behavior.parameter_03 = 0;
    l3::Class20_03CallbackInputs false_detection;
    false_detection.detection_8003648c = false;
    require(l3::class20_03_step_state1(phase2, false_detection).status == Status::NeedsWork80,
            "phase2 distance invented");
    false_detection.action.work_80 = 0x00320000;
    require(l3::class20_03_step_state1(phase2, false_detection).status == Status::NeedsPacket,
            "phase2 far distance invented hit packet");
    State consumers;
    consumers.work_00 = 3;
    consumers.behavior.parameter_03 = 0x20;
    consumers.behavior.flags_328 = 4;
    consumers.behavior.phase_0a = 1;
    consumers.behavior.timer_32c = 8;
    l3::Class20_03CallbackInputs readiness;
    readiness.source_random_sample = 0;
    readiness.action.packet_33c = 0;
    const auto consumer_before = values(consumers);
    require(l3::class20_03_step_callback(consumers, readiness).status == Status::NeedsAnimationConsumer,
            "missing pose consumer silently admitted callback");
    require(values(consumers) == consumer_before, "pose hold leaked source body changes");
    readiness.animation_consumer_ready = true;
    require(l3::class20_03_step_callback(consumers, readiness).status == Status::NeedsEffectConsumer,
            "missing effect consumer silently admitted callback");
    require(values(consumers) == consumer_before, "effect hold leaked source body changes");
    readiness.effect_consumer_ready = true;
    require(l3::class20_03_step_callback(consumers, readiness).status == Status::NeedsProxyConsumer,
            "missing proxy consumer silently admitted callback");
    require(values(consumers) == consumer_before, "proxy hold leaked source body changes");
    consumers.behavior.flags_328 = 2;
    consumers.health_6e = 128;
    readiness.action.packet_33c = 0x20000001;
    const auto sound_before = values(consumers);
    require(l3::class20_03_step_callback(consumers, readiness).status == Status::NeedsSoundConsumer,
            "missing hit sound consumer silently admitted callback");
    require(values(consumers) == sound_before, "sound hold leaked HP changes");
    State missing_floor;
    missing_floor.raw_14[1] = 10;
    const auto floor_before = values(missing_floor);
    require(l3::class20_03_initialize_scalar_state(missing_floor, {}) == Status::NeedsFloor,
            "normal initializer invented a floor");
    require(values(missing_floor) == floor_before, "floor hold leaked initializer stores");
    State action2;
    action2.behavior.parameter_03 = 0x20;
    action2.behavior.action_09 = 2;
    action2.behavior.heading_56 = 4095;
    action2.behavior.turn_66 = 16;
    action2.raw_14 = {1, 2, 3, 4};
    action2.fixed_1c = {65536, 131072, 196608, 77};
    const auto movement_before = values(action2);
    l3::Class20_03CallbackInputs motion;
    require(l3::class20_03_step_state1(action2, motion).status == Status::NeedsAction2Motion,
            "action2 invented wall/floor motion");
    require(values(action2) == movement_before, "unprepared action2 changed phase/heading");
    l3::Class20_03PreparedAction2 prepared;
    prepared.raw_14 = {7, 8, 9, 4};
    prepared.fixed_1c = {458752, 524288, 589824, 77};
    prepared.source_floor_word = 35;
    motion.prepared_action2 = prepared;
    require(l3::class20_03_step_state1(action2, motion).status == Status::NeedsAnimationCue,
            "action2 invented sequence cue byteA6");
    require(values(action2) == movement_before, "unknown cue changed action2");
    motion.prepared_action2->source_a6 = 1;
    require(l3::class20_03_step_state1(action2, motion).status == Status::NeedsSoundConsumer,
            "source action2 sound176 dropped");
    require(values(action2) == movement_before, "sound176 hold leaked prepared movement");
    motion.sound_consumer_ready = true;
    const auto rollback = l3::class20_03_step_state1(action2, motion);
    require(rollback.status == Status::SourceBackedPartial && rollback.sound_8001a050 == 176,
            "prepared source action2 rejected");
    require(action2.raw_14 == std::array<int16_t, 4>{1, 2, 3, 4} &&
            action2.fixed_1c == std::array<int32_t, 4>{65536, 131072, 196608, 77} &&
            action2.behavior.heading_56 == 15 && action2.behavior.action_09 == 1 &&
            action2.aux_0b == 0 && action2.restore_action_0e == 2,
            "33-unit floor rollback lost source heading/restore behavior");
    action2.behavior.action_09 = 2;
    action2.behavior.phase_0a = 2;
    require(l3::class20_03_step_state1(action2, {}).status == Status::SourceBackedPartial,
            "excluded action2 phase requested unused motion/cue");
    action2.behavior.phase_0a = 0;
    action2.behavior.flags_328 = 2;
    action2.health_6e = 128;
    motion.action.packet_33c = 0x20000001;
    motion.prepared_action2->source_floor_word = 2;
    const auto sounds = l3::class20_03_step_state1(action2, motion);
    require(sounds.status == Status::SourceBackedPartial && sounds.sound_8001a050 == 162 &&
            sounds.secondary_sound_8001a050 == 176 && action2.health_6e == 127,
            "hit then action2 cue lost source sound ordering");
}
}

int main(int argc, char** argv) {
    try {
        require(argc <= 2, "at most one optional owner-local source fixture path");
        guards_and_lifecycle();
        if (argc == 2) std::cout << source_cases(argv[1]) << " original-instruction callback/state1/tail/init cases passed\n";
        std::cout << "transaction holds, source-used subset, range boundary and packet lifecycle passed\n";
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
