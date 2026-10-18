#include "legends3ds/actor_action3_contact.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/map_initialization.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
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
using Status = l3::ActorAction3ContactStatus;
constexpr l3::RetailPlayerBounds authored_bounds{-60, 60, -60, 60, -128, 0};
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
l3::RuntimeMap rectangle_map(uint16_t code = 3, uint16_t flags = 0, int16_t raw6 = 0) {
    l3::RuntimeMap map;
    l3::RuntimeMapArea area;
    area.start_x = 62; area.start_z = 62; area.width = 4; area.height = 4; area.cell_count = 16;
    map.areas.push_back(area); map.cells.assign(16, 1); map.idx.resize(2);
    map.idx[1].descriptor_index = 0; map.idx[1].raw6 = raw6;
    l3::RuntimeMapDescriptor descriptor;
    descriptor.default_contact_index = 0; descriptor.alt_contact_index = 0;
    map.descriptors.push_back(descriptor); map.contact_lists.push_back({0, 1});
    map.contact_groups.push_back({-100, 100, -100, 100, 0, 1});
    map.contact_primitives.push_back({-128, 128, code, flags});
    return map;
}
l3::Class20_03CallbackState callback(const l3::ActorContactWork& motion, uint8_t phase = 3,
                                    int16_t timer = 4, int16_t limit = 16, uint16_t heading = 0) {
    l3::Class20_03CallbackState state;
    state.behavior.action_09 = 3; state.behavior.phase_0a = phase;
    state.behavior.timer_32c = timer; state.behavior.heading_56 = heading;
    state.work_64 = limit;
    state.raw_14 = motion.raw; state.fixed_1c = motion.fixed;
    state.previous_raw_2c = motion.previous_raw; state.previous_fixed_34 = motion.previous_fixed;
    state.angle_54[1] = heading;
    return state;
}
void guards() {
    auto map = rectangle_map();
    l3::ActorContactWork motion;
    motion.raw = {{0, -128, 0, 1234}};
    for (size_t i = 0; i < 4; ++i) motion.fixed[i] = int32_t(motion.raw[i]) * 65536 + (i == 1 ? 0 : 12345);
    motion.previous_raw = motion.raw; motion.previous_fixed = motion.fixed;
    motion.velocity = {{321, -654, 987}}; motion.state74 = 1;
    auto state = callback(motion);
    l3::ReferenceTables refs; refs.trig.fill({0, 4096});
    auto p = l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0);
    require(p.status == Status::SourceBackedPartial && p.inputs && p.inputs->heading_after_turn == 0 &&
            p.inputs->raw_14[2] == -48 && p.inputs->fixed_1c[2] == motion.fixed[2] - 4096 * 768 &&
            p.inputs->source_floor_word == -128, "source forward768 flat preparation mismatch");
    require(state.raw_14 == motion.raw && state.fixed_1c == motion.fixed && state.behavior.timer_32c == 4,
            "read-only preparation modified active state");
    require(p.before.previous_raw == motion.raw && p.before.velocity == motion.velocity &&
            p.initial_action == 3 && p.initial_phase == 3 && p.initial_timer == 4 &&
            p.initial_work64 == 16 && p.initial_packet == 0 && p.target_heading == 0,
            "strong stale snapshot omitted source/contact fields");
    state.work_64.reset();
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::NeedsTurnLimit,
            "unknown source work64 was guessed");
    state.behavior.timer_32c = 3;
    p = l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 1024);
    require(p.status == Status::SourceBackedPartial && p.effective_turn_limit == 256 && p.source_turn == 256 &&
            p.inputs->heading_after_turn == 256, "phase3 threshold did not prepare original work64=256");
    state.behavior.phase_0a = 4;
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::NeedsTurnLimit,
            "phase4 incorrectly repeated phase3 limit initialization");
    state.work_64 = 16;
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, std::nullopt).status == Status::NeedsTargetHeading,
            "unknown original target angle was guessed");
    auto stale = motion; ++stale.fixed[0];
    require(l3::prepare_actor_action3_contact(state, stale, map, 0, refs, authored_bounds, 0).status == Status::StaleWork,
            "stale fixed mirror accepted");
    stale = motion; ++stale.previous_raw[3];
    require(l3::prepare_actor_action3_contact(state, stale, map, 0, refs, authored_bounds, 0).status == Status::StaleWork,
            "stale fourth previous raw lane accepted");
    state.behavior.timer_32c = 0;
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::NotMovingBranch,
            "phase completion fabricated movement");
    state.behavior.timer_32c = 4; state.packet_33c = 0x02000000;
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::NotMovingBranch,
            "source packet action6 branch fabricated movement");
    state.packet_33c = 0; map = rectangle_map(8);
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::UnsupportedWall,
            "unknown slope/contact accepted");
    map = rectangle_map(); map.cells.clear();
    require(l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::UnsupportedBoundary,
            "malformed/unknown boundary accepted");
    map = rectangle_map();
    const auto allocations = allocation_count;
    bool ready = true;
    for (int i = 0; i < 640; ++i) ready &= l3::prepare_actor_action3_contact(state, motion, map, 0, refs, authored_bounds, 0).status == Status::SourceBackedPartial;
    require(ready && allocation_count == allocations, "action3 contact preparation allocated");
    const auto words = l3::actor_source_sound_position_words({{-32768, 32767, -1, -12345}});
    require(words[0] == 0x7fff8000u && words[1] == 0xcfc7ffffu,
            "original sound two-word payload lost signed/fourthlane bits");
}
int64_t number(std::istream& row) {
    int64_t n; require(static_cast<bool>(row >> n), "truncated action3 contact fixture"); return n;
}
struct Case {
    uint8_t phase = 0;
    int16_t timer = 0, limit = 0;
    uint16_t heading = 0, target = 0;
    l3::RetailPlayerBounds bounds;
    l3::ActorContactWork before, after;
    int32_t wall = 0, floor = 0;
    uint16_t moved_heading = 0;
    int16_t turn = 0;
};
void read_tail(std::istream& row, Case& c) {
    c.bounds.x0 = static_cast<int16_t>(number(row)); c.bounds.x1 = static_cast<int16_t>(number(row));
    c.bounds.z0 = static_cast<int16_t>(number(row)); c.bounds.z1 = static_cast<int16_t>(number(row));
    c.bounds.y0 = static_cast<int16_t>(number(row)); c.bounds.y1 = static_cast<int16_t>(number(row));
    for (auto& v : c.before.raw) v = static_cast<int16_t>(number(row));
    for (auto& v : c.before.fixed) v = static_cast<int32_t>(number(row));
    for (auto& v : c.before.velocity) v = static_cast<int16_t>(number(row));
    c.before.state74 = static_cast<uint8_t>(number(row));
    c.before.previous_raw = c.before.raw; c.before.previous_fixed = c.before.fixed;
    c.wall = static_cast<int32_t>(number(row)); c.floor = static_cast<int32_t>(number(row));
    c.after = c.before;
    for (auto& v : c.after.raw) v = static_cast<int16_t>(number(row));
    for (auto& v : c.after.fixed) v = static_cast<int32_t>(number(row));
    for (auto& v : c.after.velocity) v = static_cast<int16_t>(number(row));
    c.after.state74 = static_cast<uint8_t>(number(row));
    c.moved_heading = static_cast<uint16_t>(number(row)); c.turn = static_cast<int16_t>(number(row));
}
void compare(const l3::ActorAction3ContactPreparation& p, const Case& c) {
    require(p.inputs.has_value(), "admitted source preparation missing inputs");
    if (p.wall.result != c.wall || p.floor.result != c.floor || p.after_wall.raw != c.after.raw ||
        p.after_wall.fixed != c.after.fixed || p.after_wall.velocity != c.after.velocity ||
        p.after_wall.state74 != c.after.state74 || p.inputs->heading_after_turn != c.moved_heading || p.source_turn != c.turn)
        throw std::runtime_error("original/native action3 preparation mismatch");
}
void authored_cases(const char* path) {
    std::ifstream file(path); require(static_cast<bool>(file), "authored action3 fixture unavailable");
    std::string line; std::getline(file, line);
    size_t supported = 0, held = 0, corrected = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line); Case c;
        c.phase = static_cast<uint8_t>(number(row)); c.timer = static_cast<int16_t>(number(row));
        c.limit = static_cast<int16_t>(number(row)); c.heading = static_cast<uint16_t>(number(row));
        c.target = static_cast<uint16_t>(number(row));
        const auto sin = static_cast<int16_t>(number(row)), cos = static_cast<int16_t>(number(row));
        const auto code = static_cast<uint16_t>(number(row)), flags = static_cast<uint16_t>(number(row));
        const auto raw6 = static_cast<int16_t>(number(row));
        read_tail(row, c); std::string extra; require(!(row >> extra), "extra action3 fixture fields");
        const auto map = rectangle_map(code, flags, raw6);
        const auto state = callback(c.before, c.phase, c.timer, c.limit, c.heading);
        l3::ReferenceTables refs; refs.trig.fill({sin, cos});
        const auto p = l3::prepare_actor_action3_contact(state, c.before, map, 0, refs, c.bounds, c.target);
        if (p.status == Status::SourceBackedPartial) {
            try { compare(p, c); }
            catch (...) { std::cerr << "source case " << supported + held << " code " << code << '\n'; throw; }
            ++supported; corrected += c.wall != 0;
        } else { require(!p.inputs, "held preparation published motion"); ++held; }
    }
    require(supported > 400 && held > 0 && corrected > 20, "insufficient original action3 contact coverage");
    std::cout << "original action3 contact cases=" << supported + held << " supported=" << supported
              << " held=" << held << " corrected=" << corrected << '\n';
}
void sound_cases(const char* path) {
    std::ifstream file(path); require(static_cast<bool>(file), "sound producer fixture unavailable");
    std::string line; std::getline(file, line); size_t count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        const auto action = number(row), full = number(row);
        std::array<int16_t, 4> raw{}, after{};
        for (auto& v : raw) v = static_cast<int16_t>(number(row));
        const auto type = number(row), id = number(row);
        const auto word0 = static_cast<uint32_t>(number(row)), word1 = static_cast<uint32_t>(number(row));
        const auto queue_pointer = static_cast<uint32_t>(number(row));
        for (auto& v : after) v = static_cast<int16_t>(number(row));
        const auto words = l3::actor_source_sound_position_words(raw);
        if (!full) {
            require(words[0] == word0 && words[1] == word1 && type == 1 && id == (action == 2 ? 176 : 180) &&
                    queue_pointer == 0x9014, "original pointed producer/callee copy mismatch");
            if (action == 2) require(l3::actor_source_sound_position_words(after) != words,
                                    "sound fixture failed to distinguish pre/postmotion point");
        } else require(type == 0x78 && id == 0x1234 && queue_pointer == 0x800be6d8,
                       "original full queue suppression mismatch");
        ++count;
    }
    require(count == 12, "missing source sound producer cases");
    std::cout << "original sound176/180 producer+queuewriter cases=" << count << '\n';
}
void owner_stage(const char* path, const char* owner_pack) {
    const std::string pack(owner_pack);
    const auto map_bytes = l3::read_file(pack + "/st00.l3map"), render_bytes = l3::read_file(pack + "/st00.l3render");
    auto map = l3::load_l3map(map_bytes); const auto render = l3::load_l3render(render_bytes);
    const auto init = l3::load_map_initialization(l3::read_file(pack + "/st00.l3init"));
    l3::apply_map_initialization(map, render, init, l3::crc32(map_bytes.data(), map_bytes.size()),
                               l3::crc32(render_bytes.data(), render_bytes.size()));
    const auto refs = l3::load_l3reference(l3::read_file(pack + "/reference.l3ref"));
    std::ifstream file(path); require(static_cast<bool>(file), "owner stage action3 fixture unavailable");
    std::string line; std::getline(file, line); size_t supported = 0, held = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line); Case c;
        c.phase = static_cast<uint8_t>(number(row)); c.timer = static_cast<int16_t>(number(row));
        c.limit = static_cast<int16_t>(number(row)); c.heading = static_cast<uint16_t>(number(row));
        c.target = static_cast<uint16_t>(number(row)); read_tail(row, c);
        const auto state = callback(c.before, c.phase, c.timer, c.limit, c.heading);
        const auto p = l3::prepare_actor_action3_contact(state, c.before, map, 1, refs, c.bounds, c.target);
        if (p.status == Status::SourceBackedPartial) { compare(p, c); ++supported; }
        else { require(!p.inputs, "held owner preparation published motion"); ++held; }
    }
    require(supported > 0 && supported + held == 24, "missing owner action3 preparation coverage");
    std::cout << "materialized source action3 contact cases=" << supported + held
              << " supported=" << supported << " held=" << held << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try {
        guards();
        if (argc > 1) authored_cases(argv[1]);
        if (argc > 2) sound_cases(argv[2]);
        if (argc > 4) owner_stage(argv[3], argv[4]);
        std::cout << "PASS: original action3 contact and pointed soundpayload;640 allocation-free preparations\n";
        return 0;
    } catch (const std::exception& ex) { std::cerr << ex.what() << '\n'; return 1; }
}
