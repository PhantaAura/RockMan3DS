#include "legends3ds/actor_animation_bank.h"
#include "legends3ds/endian_io.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>

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
constexpr uint32_t base = 0x8016c000;
constexpr std::array<uint8_t, 9> authored_requests{{0, 1, 2, 3, 4, 6, 21, 22, 255}};
constexpr size_t cap = 512 * 1024;
void pointer(std::vector<uint8_t>& bytes, size_t at, size_t target) {
    l3::patch_u32(bytes, at, base + static_cast<uint32_t>(target));
}
void packed_channel(std::vector<uint8_t>& bytes, size_t pose, size_t channel, uint16_t value) {
    const size_t bit = channel * 12, byte = pose + bit / 8;
    const auto shifted = static_cast<uint32_t>(value & 0xfffu) << (bit % 8);
    bytes.at(byte) |= static_cast<uint8_t>(shifted);
    bytes.at(byte + 1) |= static_cast<uint8_t>(shifted >> 8);
}
// Authored sparse tables include request255 to exercise the generic byte domain.
// They are diagnostic input, not original-game bytes or animation parity data.
std::vector<uint8_t> fixture() {
    std::vector<uint8_t> bytes(0x4000);
    l3::patch_u32(bytes, 0, 1);
    bytes[4] = 0x20; bytes[5] = 3;
    pointer(bytes, 8, 0x40); pointer(bytes, 12, 0x400); pointer(bytes, 16, 0x900);
    bytes[0x51] = 2; // two draw slots; zero-face authored meshes are sufficient.
    bytes[0x54] = 1; bytes[0x55] = 0; bytes[0x56] = 1;
    for (size_t i = 0; i < 3; ++i) pointer(bytes, 0xb0 + i * 4, 0x200);
    pointer(bytes, 0x400, 0xe00);
    bytes[0xe08] = 8; bytes[0xe0a] = 16; bytes[0xe0c] = 24;
    for (size_t i = 0; i < authored_requests.size(); ++i) {
        const auto request = authored_requests[i];
        const size_t records = 0x1000 + i * 0x40, frames = 0x1300 + i * 0x40;
        pointer(bytes, 0x900 + size_t(request) * 4, records);
        pointer(bytes, 0x404 + size_t(request) * 4, frames);
        for (size_t j = 0; j < 2; ++j) {
            const size_t record = records + j * 8;
            bytes[record] = static_cast<uint8_t>(j);
            bytes[record + 1] = request == 6 ? 0 : 1;
            bytes[record + 2] = static_cast<uint8_t>(17 + i * 2 + j);
            bytes[record + 3] = request == 22 ? 255 : (j ? 128 : 0);
            bytes[record + 4] = static_cast<uint8_t>(101 + i);
            const size_t pose = 0x1800 + i * 0x300 + j * 0x100;
            pointer(bytes, frames + j * 4, pose);
            packed_channel(bytes, pose, 0, static_cast<uint16_t>(100 + i * 10 + j));
            packed_channel(bytes, pose, 4, static_cast<uint16_t>(300 + i * 10 + j));
        }
    }
    return bytes;
}
template<class F> void rejects(F&& f) {
    bool rejected = false;
    try { f(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
bool same_cursor(const l3::ActorSequenceCursor& a, const l3::ActorSequenceCursor& b) {
    return a.record_index == b.record_index && a.frame_a4 == b.frame_a4 &&
        a.countdown_a5 == b.countdown_a5 && a.opaque_a6 == b.opaque_a6 &&
        a.opaque_a7 == b.opaque_a7;
}
bool same_pose(const l3::ActorPoseResource& a, const l3::ActorPoseResource& b) {
    return a.request == b.request && a.record_index == b.record_index &&
        a.frame_index == b.frame_index && a.countdown == b.countdown &&
        a.next_control == b.next_control && a.sequencing_record == b.sequencing_record &&
        a.root_translation == b.root_translation && a.rotations == b.rotations &&
        a.base_translation == b.base_translation &&
        a.sequence_source_offset == b.sequence_source_offset &&
        a.pose_source_offset == b.pose_source_offset &&
        a.translations_source_offset == b.translations_source_offset;
}
void decode_and_budget_checks() {
    const auto bytes = fixture();
    const auto ebd = l3::parse_ebd_directory(bytes, base);
    const auto mesh = l3::decode_ebd_actor_mesh(ebd, 0);
    auto bank = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), cap);
    assert(bank.source_row == 0 && bank.requests.size() == authored_requests.size());
    size_t expected_bytes = sizeof(l3::ActorAnimationBank) +
        bank.requests.capacity() * sizeof(l3::ActorAnimationRequestCache);
    for (size_t i = 0; i < bank.requests.size(); ++i) {
        const auto* cache = l3::find_actor_animation_request(bank, authored_requests[i]);
        assert(cache && cache->request == authored_requests[i]);
        assert(cache->sequence.poses.size() == (cache->request == 22 ? 1 : 2));
        assert(cache->sequence.poses[0].request == cache->request);
        assert(cache->sequence.poses[0].root_translation[0] == 100 + static_cast<int>(i) * 10);
        assert(cache->sequence.poses[0].pose_source_offset == 0x1800 + i * 0x300);
        expected_bytes += cache->sequence.decoded_bytes - sizeof(l3::ActorPoseSequenceCache);
    }
    assert(bank.decoded_bytes == expected_bytes && bank.decoded_bytes <= cap);
    const auto exact = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), bank.decoded_bytes);
    assert(exact.decoded_bytes == bank.decoded_bytes);
    rejects([&] { l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), bank.decoded_bytes - 1); });
    rejects([&] { l3::decode_ebd_actor_animation_bank(ebd, 0, mesh, nullptr, 1, cap); });
    rejects([&] { l3::decode_ebd_actor_animation_bank(ebd, 0, mesh, authored_requests.data(), 0, cap); });
    rejects([&] { l3::decode_ebd_actor_animation_bank(ebd, 0, mesh, authored_requests.data(), 257, cap); });
    const std::array<uint8_t, 2> duplicate{{1, 1}}, missing{{0, 5}};
    rejects([&] { l3::decode_ebd_actor_animation_bank(ebd, 0, mesh, duplicate.data(), 2, cap); });
    const auto before = bank.decoded_bytes;
    rejects([&] { bank = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh, missing.data(), 2, cap); });
    assert(bank.decoded_bytes == before && bank.requests.size() == authored_requests.size());
    auto bad_bytes = bytes;
    // Request1 decodes only after request0; a late bad pose must not replace bank.
    pointer(bad_bytes, 0x1340, bad_bytes.size() - l3::kActorPackedPoseBytes + 4);
    const auto bad = l3::parse_ebd_directory(bad_bytes, base);
    rejects([&] { bank = l3::decode_ebd_actor_animation_bank(bad, 0, mesh,
        authored_requests.data(), authored_requests.size(), cap); });
    assert(bank.decoded_bytes == before && bank.requests[1].sequence.poses[0].request == 1);
    auto resource = std::make_shared<const l3::ActorAnimationBank>(std::move(bank));
    std::weak_ptr<const l3::ActorAnimationBank> life = resource;
    auto second_actor = resource;
    assert(resource == second_actor);
    resource.reset(); assert(!life.expired());
    second_actor.reset(); assert(life.expired());
}
void request_transaction_checks() {
    const auto ebd = l3::parse_ebd_directory(fixture(), base);
    const auto mesh = l3::decode_ebd_actor_mesh(ebd, 0);
    auto bank = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), cap);
    auto live = l3::decode_ebd_actor_pose(ebd, 0, mesh, 0, 0);
    auto cursor = l3::actor_sequence_begin(0, live.sequencing_record);
    const auto storage = live.base_translation.data();
    auto result = l3::actor_animation_reset(bank, 1, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::SourceBackedPartial && result.request_changed);
    assert(live.request == 1 && live.record_index == 0 && live.root_translation[0] == 110);
    assert(cursor.opaque_a6 == 19 && live.base_translation.data() == storage);
    result = l3::actor_animation_step(bank, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::SourceBackedPartial && result.record_changed);
    assert(live.record_index == 1 && live.root_translation[0] == 111 && cursor.opaque_a6 == 20);
    result = l3::actor_animation_reset(bank, 21, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::SourceBackedPartial && live.request == 21);
    assert(live.record_index == 0 && live.root_translation[0] == 160);
    const auto before_pose = live;
    const auto before_cursor = cursor;
    assert(l3::actor_animation_request_ready(bank, 5, live) == l3::ActorAnimationStatus::MissingRequest);
    result = l3::actor_animation_reset(bank, 5, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::MissingRequest);
    assert(same_pose(live, before_pose) && same_cursor(cursor, before_cursor));
    bank.requests[1].sequence.poses[0].base_translation.push_back({});
    result = l3::actor_animation_reset(bank, 1, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::PartCountMismatch);
    assert(same_pose(live, before_pose) && same_cursor(cursor, before_cursor));
    auto* selected = &bank.requests[6];
    selected->sequence.records[0][3] = 130; // next index2 is absent.
    selected->sequence.poses[0].sequencing_record[3] = 130;
    result = l3::actor_animation_step(bank, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::InvalidCache);
    assert(same_pose(live, before_pose) && same_cursor(cursor, before_cursor));
    selected->sequence.poses.pop_back(); // records/pose count disagreement.
    result = l3::actor_animation_reset(bank, 21, live, cursor);
    assert(result.status == l3::ActorAnimationStatus::InvalidCache);
    assert(same_pose(live, before_pose) && same_cursor(cursor, before_cursor));
    assert(l3::actor_sequence_reset(cursor, nullptr, 1) == l3::ActorSequenceStatus::MissingRecord);
    assert(l3::actor_sequence_reset(cursor, selected->sequence.records.data(), 257) ==
        l3::ActorSequenceStatus::MissingRecord);
    assert(same_cursor(cursor, before_cursor));
}
void preview_preserves_retained_live_root() {
    const auto ebd = l3::parse_ebd_directory(fixture(), base);
    const auto mesh = l3::decode_ebd_actor_mesh(ebd, 0);
    const auto bank = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), cap);
    auto live = l3::decode_ebd_actor_pose(ebd, 0, mesh, 22, 0);
    auto cursor = l3::actor_sequence_begin(0, live.sequencing_record);
    // Terminal source control retains the current record. The established
    // animation consumer preserves the live root on this branch, including an
    // explicitly modified retained root, rather than reloading its cache.
    live.root_translation = {{321, -654, 987}};
    const auto before_pose = live;
    const auto before_cursor = cursor;
    const auto allocations_before = allocation_count;
    const auto preview = l3::actor_animation_preview(bank, 22, live, cursor);
    assert(preview.result.status == l3::ActorAnimationStatus::SourceBackedPartial &&
        !preview.result.record_changed && !preview.result.request_changed && preview.pose == &live);
    assert(same_pose(live, before_pose) && same_cursor(cursor, before_cursor));
    const auto step = l3::actor_animation_step(bank, live, cursor);
    assert(step.status == preview.result.status && !step.record_changed &&
        same_cursor(cursor, preview.cursor) && live.root_translation == before_pose.root_translation);
    // A changed request does select the resident cache root, so preserving the
    // retained live root is bounded to the actual no-record-change branch.
    const auto restart = l3::actor_animation_preview(bank, 1, live, cursor);
    assert(restart.result.status == l3::ActorAnimationStatus::SourceBackedPartial &&
        restart.result.request_changed && restart.pose != &live && restart.pose->root_translation[0] == 110);
    const auto reset = l3::actor_animation_reset(bank, 1, live, cursor);
    assert(reset.status == restart.result.status && live.root_translation == restart.pose->root_translation &&
        same_cursor(cursor, restart.cursor) && allocation_count == allocations_before);
}

void allocation_and_countdown_checks() {
    const auto ebd = l3::parse_ebd_directory(fixture(), base);
    const auto mesh = l3::decode_ebd_actor_mesh(ebd, 0);
    const auto bank = l3::decode_ebd_actor_animation_bank(ebd, 0, mesh,
        authored_requests.data(), authored_requests.size(), cap);
    auto live = l3::decode_ebd_actor_pose(ebd, 0, mesh, 0, 0);
    auto cursor = l3::actor_sequence_begin(0, live.sequencing_record);
    const auto allocations_before = allocation_count;
    for (size_t tick = 0; tick < 480; ++tick) {
        const auto request = authored_requests[tick % authored_requests.size()];
        assert(l3::actor_animation_request_ready(bank, request, live) ==
            l3::ActorAnimationStatus::SourceBackedPartial);
        assert(l3::actor_animation_reset(bank, request, live, cursor).status ==
            l3::ActorAnimationStatus::SourceBackedPartial);
        for (size_t step = 0; step < 4; ++step) {
            assert(l3::actor_animation_step(bank, live, cursor).status ==
                l3::ActorAnimationStatus::SourceBackedPartial);
            assert(live.request == request && cursor.record_index == live.record_index);
        }
        assert(l3::actor_animation_reset(bank, 5, live, cursor).status ==
            l3::ActorAnimationStatus::MissingRequest);
    }
    assert(l3::actor_animation_reset(bank, 6, live, cursor).status ==
        l3::ActorAnimationStatus::SourceBackedPartial);
    assert(l3::actor_animation_step(bank, live, cursor).status ==
        l3::ActorAnimationStatus::SourceBackedPartial);
    assert(cursor.countdown_a5 == 255 && cursor.record_index == 0 && live.countdown == 255);
    assert(l3::actor_animation_reset(bank, 22, live, cursor).status ==
        l3::ActorAnimationStatus::SourceBackedPartial);
    const auto terminal_word = cursor;
    assert(!l3::actor_animation_step(bank, live, cursor).record_changed);
    assert(same_cursor(cursor, terminal_word)); // -1 only reloads the countdown.
    assert(allocation_count == allocations_before);
}
} // namespace

int main() {
    decode_and_budget_checks();
    request_transaction_checks();
    preview_preserves_retained_live_root();
    allocation_and_countdown_checks();
    std::cout << "sparse source request tables, transactional reset/step, decoded cap and no-allocation playback passed\n";
}
