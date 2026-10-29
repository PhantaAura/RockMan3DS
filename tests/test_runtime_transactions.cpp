#include "legends3ds/runtime_session.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace {
// Fail each successive allocation in a transaction, including nested stage
// work copies. Reset before throwing so observations after rejection can allocate.
long allocations_before_failure = -1;
void* checked_allocate(std::size_t bytes) {
    if (allocations_before_failure == 0) {
        allocations_before_failure = -1;
        throw std::bad_alloc();
    }
    if (allocations_before_failure > 0) --allocations_before_failure;
    if (void* result = std::malloc(bytes ? bytes : 1)) return result;
    throw std::bad_alloc();
}
}

void* operator new(std::size_t bytes) { return checked_allocate(bytes); }
void* operator new[](std::size_t bytes) { return checked_allocate(bytes); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
using namespace l3;

L3QolSettings settings(uint8_t speed = L3_TEXT_SPEED_INSTANT) {
    L3QolSettings result{};
    l3_qol_settings_defaults(&result);
    result.text_speed = speed;
    return result;
}

RuntimeMap make_map(uint32_t tag) {
    RuntimeMap map;
    RuntimeMapArea area{};
    area.start_x = 63; area.start_z = 63;
    area.width = 2; area.height = 2; area.cell_count = 4;
    map.areas.push_back(area);
    // Synthetic packed negative cells provide the already supported floor -1.
    map.cells.assign(4, static_cast<int16_t>(0x8000));
    map.descriptors.resize(1);
    map.descriptors.front().word0 = tag;
    return map;
}

StageObjectRecord object(uint8_t source_class, uint8_t subtype, uint8_t id) {
    StageObjectRecord result{};
    result.raw[0] = id; result.raw[1] = source_class; result.raw[2] = subtype;
    result.id = id; result.object_class = source_class;
    result.words_02_to_0a[0] = subtype;
    return result;
}

TutorialRuntimeArea make_area(uint8_t tag) {
    TutorialRuntimeArea area;
    area.flow.objects.records = {object(0xC0, 0x21, tag), object(0x20, 3, tag + 1)};
    area.flow.aux.records.resize(1);
    area.stage = build_fresh_stage_runtime_area(area.flow.objects, area.ebd);
    St00SpawnRegion region;
    region.id = tag; region.qualifier = 1;
    region.min_x = -1; region.min_z = -1; region.width = 2; region.depth = 2;
    region.records = {{object(0x20, 3, 3), 1599}, {object(0x20, 3, 4), 1600}};
    area.conditional_spawns = St00SpawnArea{{region}};
    return area;
}

MmlMessageFile queued_message() {
    MmlMessageFile message;
    message.blocks.resize(1);
    message.blocks[0].tokens = {
        {MmlMessageTokenKind::Glyph, 0x21, {}},
        {MmlMessageTokenKind::UnknownControl, 0xFE, {5}},
        {MmlMessageTokenKind::Glyph, 0x22, {}},
        {MmlMessageTokenKind::Control, 0x8F, {7, 0, 0, 0}},
        {MmlMessageTokenKind::Glyph, 0x23, {}}
    };
    return message;
}

RuntimeSession populated_session() {
    RuntimeSession session(make_map(11), make_area(11), {queued_message()}, settings());
    session.register_location(77, make_map(77), 0, make_area(77));
    session.reset_player({0, -1, 0}, 0, 63, 80);
    session.set_story_flag(17, true);
    session.set_story_flag(1600, true);
    session.tick({}); session.tick({});
    assert(session.conditional_spawn_objects().records.size() == 1);
    session.register_target({7, {0, -1, -200}, 1, 4, true, true});
    RuntimeInputFrame fire{}; fire.fire_pressed = true; fire.lock_held = true;
    session.tick(fire);
    assert(session.locked_target_id() && !session.projectiles().empty());
    session.start_dialogue({0, 0}); session.tick({});
    assert(session.dialogue().waiting_for_script);
    session.resume_dialogue_script_wait(); session.tick({});
    assert(session.dialogue().waiting_for_audio);
    assert(session.dialogue().script_requests.size() == 1);
    assert(session.dialogue().audio_requests.size() == 1);
    assert(!session.projectiles().empty());
    session.submit_contact_sample({0, 0, 0, 0, 1, 0});
    session.begin_cutscene(9);
    session.start_script_camera({{400, -300, 500}, {50, -40, 30}, 0x123}, 100);
    return session;
}

bool same_vector(RuntimeVec3i a, RuntimeVec3i b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
bool same_pose(const RuntimeCameraPose& a, const RuntimeCameraPose& b) {
    return same_vector(a.eye, b.eye) && same_vector(a.look_at, b.look_at) && a.yaw == b.yaw;
}
void assert_same_camera(const RuntimeCameraState& a, const RuntimeCameraState& b) {
    assert(a.owner == b.owner && a.transition_frame == b.transition_frame);
    assert(a.transition_frames == b.transition_frames);
    assert(same_pose(a.current, b.current) && same_pose(a.start, b.start) && same_pose(a.target, b.target));
}
void assert_same_player(const L3PlayerCompatWork& a, const L3PlayerCompatWork& b) {
    assert(std::equal(std::begin(a.bytes), std::end(a.bytes), std::begin(b.bytes)));
}
void assert_same_scene(RuntimeSession& observed, RuntimeSession& expected, bool check_time = true) {
    assert(&observed.active_map() == &expected.active_map());
    assert(observed.area_index() == expected.area_index());
    assert(observed.map_area_index() == expected.map_area_index());
    assert_same_player(observed.compat_player(), expected.compat_player());
    auto observed_save = observed.make_save_state(1);
    auto expected_save = expected.make_save_state(1);
    if (!check_time) observed_save.play_frames = expected_save.play_frames;
    assert(cook_l3save(observed_save) == cook_l3save(expected_save));
    assert(observed.active_area().flow.objects.records.front().id == expected.active_area().flow.objects.records.front().id);
    assert(observed.active_area().stage.instances.front().work->bytes == expected.active_area().stage.instances.front().work->bytes);
    assert(observed.targets().size() == expected.targets().size());
    assert(observed.targets().front().hp == expected.targets().front().hp);
    assert(observed.projectiles().size() == expected.projectiles().size());
    assert(observed.projectiles().front().serial == expected.projectiles().front().serial);
    assert(same_vector(observed.projectiles().front().position, expected.projectiles().front().position));
    assert(observed.projectiles().front().age == expected.projectiles().front().age);
    assert(observed.locked_target_id() == expected.locked_target_id());
    assert(observed.dialogue().active == expected.dialogue().active);
    assert(observed.dialogue().waiting_for_audio == expected.dialogue().waiting_for_audio);
    assert(observed.dialogue().waiting_for_script == expected.dialogue().waiting_for_script);
    assert(observed.dialogue().visible_glyphs == expected.dialogue().visible_glyphs);
    assert(observed.dialogue().audio_requests.size() == expected.dialogue().audio_requests.size());
    assert(observed.dialogue().script_requests.size() == expected.dialogue().script_requests.size());
    assert(observed.cutscene_active() == expected.cutscene_active());
    assert_same_camera(observed.camera(), expected.camera());
    assert(observed.conditional_spawn_revision() == expected.conditional_spawn_revision());
    assert(observed.conditional_spawn_qualifier() == expected.conditional_spawn_qualifier());
    assert(observed.conditional_spawn_objects().records.size() == expected.conditional_spawn_objects().records.size());
    // Drain only copied observations so the actual transaction fixture stays intact.
    auto observed_copy = observed;
    auto expected_copy = expected;
    const auto observed_triggers = observed_copy.take_aux_triggers();
    const auto expected_triggers = expected_copy.take_aux_triggers();
    assert(observed_triggers.size() == expected_triggers.size() && observed_triggers.size() == 1);
    assert(observed_triggers.front().record_index == expected_triggers.front().record_index);
}

void test_message_copy_lifetime() {
    MmlMessageFile message; message.blocks.resize(1);
    message.blocks[0].tokens = {{MmlMessageTokenKind::Glyph, 0x31, {}},
                               {MmlMessageTokenKind::Glyph, 0x32, {}},
                               {MmlMessageTokenKind::Glyph, 0x33, {}}};
    auto source = std::make_unique<RuntimeSession>(make_map(1), make_area(1),
        std::vector<MmlMessageFile>{message}, settings(L3_TEXT_SPEED_ORIGINAL));
    source->start_dialogue({0, 0}); source->tick({});
    RuntimeSession constructed = *source;
    RuntimeSession assigned(make_map(2), make_area(2), {}, settings());
    assigned = *source;
    MmlMessageFile previous_message = message;
    previous_message.blocks[0].tokens[0].opcode = 0x51;
    previous_message.blocks[0].tokens[1].opcode = 0x52;
    bool assignment_committed = false;
    size_t failed_assignments = 0;
    for (long failure = 0; failure < 128; ++failure) {
        RuntimeSession destination(make_map(2), make_area(2), {previous_message}, settings(L3_TEXT_SPEED_ORIGINAL));
        destination.start_dialogue({0, 0}); destination.tick({});
        const auto* previous_map = &destination.active_map();
        const auto previous_work = destination.compat_player();
        allocations_before_failure = failure;
        try {
            destination = *source;
            allocations_before_failure = -1;
            assert(&destination.active_map() == &source->active_map());
            assert(destination.dialogue().visible_glyphs == source->dialogue().visible_glyphs);
            assignment_committed = true;
            break;
        } catch (const std::bad_alloc&) {
            allocations_before_failure = -1;
            ++failed_assignments;
            assert(&destination.active_map() == previous_map);
            assert_same_player(destination.compat_player(), previous_work);
            assert(destination.dialogue().visible_glyphs == std::vector<uint8_t>({0x51}));
            destination.tick({});
            assert(destination.dialogue().visible_glyphs == std::vector<uint8_t>({0x51, 0x52}));
        }
    }
    assert(assignment_committed && failed_assignments >= 3);
    source->tick({});
    constructed.tick({}); constructed.tick({});
    assert(source->dialogue().visible_glyphs == std::vector<uint8_t>({0x31, 0x32}));
    assert(constructed.dialogue().visible_glyphs == std::vector<uint8_t>({0x31, 0x32, 0x33}));
    assert(assigned.dialogue().visible_glyphs == std::vector<uint8_t>({0x31}));
    source.reset();
    assigned.tick({}); assigned.tick({}); assigned.tick({});
    assert(!assigned.dialogue().active);
    assert(assigned.dialogue().visible_glyphs == std::vector<uint8_t>({0x31, 0x32, 0x33}));
    constructed.tick({});
    assert(!constructed.dialogue().active);
    assigned.start_dialogue({0, 0}); assigned.tick({});
    assert(assigned.dialogue().visible_glyphs == std::vector<uint8_t>({0x31}));
}

RuntimeSaveState destination_save() {
    RuntimeSaveState save;
    save.checkpoint_id = 42; save.area_id = 77; save.player_yaw = 0xABC;
    save.player_x = 110; save.player_y = -3; save.player_z = -40;
    save.hp = 44; save.hp_max = 120; save.play_frames = 888;
    save.qol = settings(L3_TEXT_SPEED_FAST);
    runtime_story_flag_set(save, 23, true);
    return save;
}

void test_save_restore_transaction() {
    const auto save = destination_save();
    size_t rejected_allocations = 0;
    bool committed = false;
    for (long failure = 0; failure < 64; ++failure) {
        auto session = populated_session();
        auto before = session;
        allocations_before_failure = failure;
        try {
            session.apply_save_state(save);
            allocations_before_failure = -1;
            assert(cook_l3save(session.make_save_state(save.checkpoint_id)) == cook_l3save(save));
            assert(session.active_area().flow.objects.records.front().id == 77);
            assert(session.active_map().descriptors.front().word0 == 77);
            assert(session.targets().empty() && session.projectiles().empty() && !session.locked_target_id());
            assert(!session.cutscene_active() && session.camera().owner == RuntimeCameraOwner::Gameplay);
            assert(session.camera().transition_frame == 0 && session.camera().transition_frames == 0);
            assert(!session.dialogue().active && session.dialogue().visible_glyphs.empty());
            assert(session.take_audio_requests().empty() && session.take_script_requests().empty());
            assert(session.take_aux_triggers().empty());
            assert(session.conditional_spawn_objects().records.empty());
            assert(session.conditional_spawn_qualifier() == -1);
            assert(session.conditional_spawn_revision() == before.conditional_spawn_revision() + 1);
            committed = true;
            break;
        } catch (const std::bad_alloc&) {
            allocations_before_failure = -1;
            ++rejected_allocations;
            assert_same_scene(session, before);
        }
    }
    assert(committed && rejected_allocations >= 3);
    auto session = populated_session();
    auto before = session;
    auto absent = save; absent.area_id = 999;
    bool rejected = false;
    try { session.apply_save_state(absent); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected); assert_same_scene(session, before);
}

void test_source_event_player_position() {
    RuntimeSession blocked(make_map(11), make_area(11), {queued_message()}, settings());
    blocked.reset_player({0, -1, 0}, 0, 63, 80);
    blocked.start_script_camera({{400, -300, 500}, {50, -40, 30}, 0x123}, 100);
    RuntimeInputFrame input{}; input.held_psx_mask = 0x10;
    blocked.tick(input);
    assert(blocked.compat_player().bytes[0x11C] == 0);
    assert(blocked.player_position().z == 0);

    RuntimeSession event_owned(make_map(11), make_area(11), {queued_message()}, settings());
    event_owned.reset_player({0, -1, 0}, 0, 63, 80);
    event_owned.start_script_camera({{400, -300, 500}, {50, -40, 30}, 0x123}, 100);
    input.source_event_player_position=RuntimeVec3i{0,-1,16};
    event_owned.tick(input);
    assert(event_owned.compat_player().bytes[0x11C] == 0);
    assert(event_owned.player_position().z == 16);
}

void write16(L3PlayerCompatWork& work, size_t at, int16_t value) {
    const uint16_t bits = static_cast<uint16_t>(value);
    work.bytes[at] = static_cast<uint8_t>(bits);
    work.bytes[at + 1] = static_cast<uint8_t>(bits >> 8);
}
void write32(L3PlayerCompatWork& work, size_t at, int32_t value) {
    const uint32_t bits = static_cast<uint32_t>(value);
    for (unsigned byte = 0; byte < 4; ++byte)
        work.bytes[at + byte] = static_cast<uint8_t>(bits >> (byte * 8));
}
L3PlayerCompatWork recovered_work(L3PlayerCompatWork work, RuntimeVec3i position) {
    const int32_t coordinates[] = {position.x, position.y, position.z};
    for (size_t axis = 0; axis < 3; ++axis) {
        write16(work, 0x14 + axis * 2, static_cast<int16_t>(coordinates[axis]));
        write32(work, 0x1C + axis * 4, coordinates[axis] * 65536);
    }
    std::memcpy(&work.bytes[0x2C], &work.bytes[0x14], 8);
    std::memcpy(&work.bytes[0x34], &work.bytes[0x1C], 16);
    return work;
}

void test_diagnostic_recovery_preserves_scene() {
    auto session = populated_session();
    auto before = session;
    const RuntimeVec3i recovered{200, -42, 100};
    const auto expected_player = recovered_work(before.compat_player(), recovered);
    allocations_before_failure = 0;
    const bool accepted = session.recover_player_position_diagnostic(recovered);
    allocations_before_failure = -1;
    assert(accepted); assert_same_player(session.compat_player(), expected_player);
    assert(same_vector(session.player_position(), recovered));
    // Position is the only difference in persistent state. Restore this copied
    // observation's mirrors to compare every outgoing scene owner and queue.
    auto observed = session;
    assert(observed.recover_player_position_diagnostic(before.player_position()));
    assert_same_scene(observed, before);
    assert(session.player_contact().status == RuntimeContactStatus::NoData);
    assert(!session.player_contact().grounded.has_value());
    const auto stable_work = session.compat_player();
    const auto stable_save = cook_l3save(session.make_save_state(42));
    for (const auto position : {RuntimeVec3i{32768, 0, 0}, RuntimeVec3i{0, -32769, 0}, RuntimeVec3i{700, 0, 0}}) {
        assert(!session.recover_player_position_diagnostic(position));
        assert_same_player(session.compat_player(), stable_work);
        assert(cook_l3save(session.make_save_state(42)) == stable_save);
    }

    RuntimePlayerReplaySeed seed;
    seed.position = {0, -80, 0}; seed.fixed_position = {0, -80 * 65536, 0};
    seed.state = {{1, 8, 4}}; seed.velocity_work = {{7, -32, 9}};
    seed.contact_state = 1; seed.animation_request = 12; seed.animation_pose = 3;
    seed.animation_countdown = 4; seed.animation_record_byte2 = 5;
    seed.animation_signed_control = -1; seed.work_0b = 2;
    assert(session.seed_player_replay_diagnostic(seed));
    const auto airborne_before = session.compat_player();
    const auto spawn_revision = session.conditional_spawn_revision();
    assert(session.recover_player_position_diagnostic({100, -90, 100}));
    assert_same_player(session.compat_player(), recovered_work(airborne_before, {100, -90, 100}));
    assert(session.conditional_spawn_revision() == spawn_revision);
    assert(session.conditional_spawn_objects().records.size() == 1);
}

void test_cutscene_completion_transaction() {
    auto session = populated_session();
    RuntimeCutsceneCompletionContract completion;
    completion.id = 9; completion.flags_on = {23}; completion.flags_off = {17};
    completion.destination_location = 999; completion.destination_position = {20, -1, 30};
    completion.destination_yaw = 0x456; completion.buster_enabled = false;
    session.register_cutscene_completion(completion);
    auto invalid = completion; invalid.flags_on.push_back(kL3StoryFlagBytes * 8u);
    bool rejected = false;
    try { session.register_cutscene_completion(std::move(invalid)); }
    catch (const std::out_of_range&) { rejected = true; }
    assert(rejected && session.cutscene_skippable());
    RuntimeInputFrame skip{}; skip.skip_held = true;
    for (unsigned frame = 0; frame < 59; ++frame) session.tick(skip);
    // Projectile age and camera interpolation naturally advanced during these
    // ticks. The completion itself must publish no partial mutation.
    auto before = session;
    rejected = false;
    try { session.tick(skip); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    // Long cutscene ticks expire the synthetic projectile, so inspect relevant
    // completion-owned state directly instead of the populated scene helper.
    assert(&session.active_map() == &before.active_map() && session.area_index() == before.area_index());
    assert_same_player(session.compat_player(), before.compat_player());
    assert(session.story_flag(17) && !session.story_flag(23));
    assert(l3_pl00_shot_is_enabled(&session.compat_player()));
    assert(session.dialogue().active && session.dialogue().audio_requests.size() == 1);
    assert(session.dialogue().script_requests.size() == 1 && session.cutscene_active());
    assert_same_camera(session.camera(), before.camera());

    session.register_location(999, make_map(99), 0, make_area(99));
    // Two contract-vector allocations precede destination stage/work copying.
    allocations_before_failure = 2;
    rejected = false;
    try { session.tick(skip); }
    catch (const std::bad_alloc&) { rejected = true; }
    allocations_before_failure = -1;
    assert(rejected && session.area_index() == before.area_index());
    assert(session.story_flag(17) && !session.story_flag(23));
    assert(l3_pl00_shot_is_enabled(&session.compat_player()));
    assert(session.cutscene_active() && session.dialogue().audio_requests.size() == 1);
    session.tick(skip);
    assert(session.area_index() == 999 && same_vector(session.player_position(), completion.destination_position));
    assert(session.player_yaw() == completion.destination_yaw);
    assert(!session.story_flag(17) && session.story_flag(23));
    assert(!l3_pl00_shot_is_enabled(&session.compat_player()));
    assert(!session.cutscene_active() && !session.dialogue().active);
    assert(session.take_audio_requests().empty() && session.take_script_requests().empty());
    assert(session.camera().transition_frames == 0);

    session.begin_cutscene(123);
    session.start_script_camera({{500, -700, 900}, {100, 200, 300}, 0x234}, 100);
    session.tick({});
    assert(session.camera().transition_frame == 1);
    session.end_cutscene(); session.tick({});
    assert(session.camera().owner == RuntimeCameraOwner::Gameplay);
    assert(session.camera().transition_frame == 0 && session.camera().transition_frames == 0);
    assert(same_pose(session.camera().current, session.camera().target));
}
}

int main() {
    test_message_copy_lifetime();
    test_save_restore_transaction();
    test_source_event_player_position();
    test_diagnostic_recovery_preserves_scene();
    test_cutscene_completion_transaction();
    std::cout << "runtime transactions: copied dialogue lifetime, atomic save/scene cleanup, diagnostic recovery and cutscene completion PASS\n";
}
