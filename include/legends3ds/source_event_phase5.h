#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "legends3ds/source_event_consumer.h"
#include "legends3ds/ebd_directory.h"

namespace l3 {
struct SourceEventWork;
template<size_t N> struct SourceEventPhase5Bytes {
    std::array<uint8_t,N> raw{},known{};
};
template<size_t Count,size_t Bytes> struct SourceEventPhase5Pool {
    std::array<SourceEventPhase5Bytes<Bytes>,Count> slots{};
    std::array<std::optional<uint32_t>,(Count+31)/32> used{};
};
constexpr size_t kSourceEventPhase5Actors=16;
struct SourceEventPhase5Record {
    std::array<uint8_t,20> raw{};
    // An original virtual address is an identity token, never a native pointer.
    uint32_t resource_token=0,resource_directory_index=0xffffffffu;
    uint8_t resource_variant=0;
};
struct SourceEventPhase5Profile {
    bool initialized=false;
    uint32_t directory_token=0,callback_table_token=0;
    // Retained L3P5 header identity; validated against resident EBD bytes.
    std::array<uint8_t,32> ebd_source_digest{};
    std::array<SourceEventPhase5Record,kSourceEventPhase5Actors> records{};
};
// L3P5v1 retains neutral spawn records and selected EBD identities only.
// Supported overlay/EBD digests and all coverage/budget domains are checked.
SourceEventPhase5Profile load_source_event_phase5_profile(const std::vector<uint8_t>& bytes);
// Load-time admission checks the immutable retained payload, not its pathname.
// This generic proof preserves exact SHA-256, source directory bytes, first-match
// order and selected pointer identities. It does not infer a resource format.
enum class SourceEventPhase5ResourceStatus : uint8_t {
    Admitted,NeedsProfile,UnsupportedProfile,NeedsPayload,PayloadBudgetExceeded,
    PayloadDigestMismatch,SourceDirectoryMismatch,RowKeyMismatch,
    NotFirstMatch,ResourceOffsetMismatch,ResourceOutsidePayload
};
class SourceEventPhase5ResourceBinding {
public:
    SourceEventPhase5ResourceStatus status() const noexcept { return status_; }
    uint8_t binding_count() const noexcept { return admitted() ? 15 : 0; }
    bool admitted() const noexcept;
    bool matches(const SourceEventPhase5Profile& profile) const noexcept;
    bool resident_in(const EbdDirectory& directory) const noexcept;
private:
    SourceEventPhase5ResourceStatus status_=SourceEventPhase5ResourceStatus::NeedsPayload;
    SourceEventPhase5Profile profile_{};
    const void* payload_identity_=nullptr;
    std::array<EbdDirectoryEntry,14> directory_entries_{};
    uint32_t load_base_=0;
    std::weak_ptr<const SharedBytePayload::Storage> payload_;
    friend SourceEventPhase5ResourceBinding bind_source_event_phase5_resources(
        const SourceEventPhase5Profile&,const EbdDirectory&) noexcept;
};
SourceEventPhase5ResourceBinding bind_source_event_phase5_resources(
    const SourceEventPhase5Profile& profile,const EbdDirectory& directory) noexcept;
struct SourceEventPhase5Context {
    SourceEventPhase5Pool<4,1444> pool40;
    SourceEventPhase5Pool<64,72> pool80;
    // Original deferred family80 releases800988C8/CC. Unknown until the
    // explicit pool reset; allocation bits survive until80031AA4 cleanup.
    std::array<std::optional<uint32_t>,2> pool80_pending_release_800988c8{};
    SourceEventPhase5Pool<32,80> pool_e0;
    // Original8002E924 also resets familyC0 at800A2D08.
    SourceEventPhase5Pool<16,36> pool_c0;
    std::optional<uint32_t> word_80098818;
    SourceEventPhase5Bytes<188> camera_800bc738;
    std::optional<std::array<uint32_t,3>> initial_eye_800a39a0;
    std::optional<std::array<uint32_t,3>> initial_rotation_800a3964;
    std::optional<uint8_t> mode_800a395c,byte_80122428;
};
std::array<uint8_t,188> source_event_phase5_camera_snapshot(const SourceEventPhase5Context& context) noexcept;
// Exact cold/reset slices8002E88C,8002E924,8002E9CC including800177CC.
// The requested family60 allocator uses the original family80 pool.
// Call only at the explicit source pool-reset lifecycle, not every area load.
void initialize_source_event_phase5_pools_8002e88c(SourceEventPhase5Context& context) noexcept;
// Exact initializer stores within80038674..800387B8, requiring source-owned
// player position bytes. This is a prefix, not a gameplay camera update.
bool initialize_source_event_phase5_camera_80038674(SourceEventPhase5Context& context,
    const L3PlayerCompatWork& player,const SourceEventPlayerKnown& known) noexcept;
enum class SourceEventPhase5Status : uint8_t {
    Consumed,NeedsEventWork,WrongEventPhase,NeedsProfile,UnsupportedProfile,
    NeedsCameraFields,NeedsPoolBitmap,NeedsPoolHeader,PoolCapacityExceeded,NeedsResourceOwnership
};
struct SourceEventPhase5Result {
    SourceEventPhase5Status status=SourceEventPhase5Status::NeedsEventWork;
    bool committed=false;
    uint8_t constructed_actors=0,resource_bindings=0;
    std::optional<uint16_t> missing_offset;
};
// Complete bounded80100EFC constructor and the caller's phase5->6 store.
// Preflight occurs before any mutation; no hot-loop allocations or copies of
// the large pools. This establishes logical scripted records, not actor draw,
// script callback execution, dialogue, scene completion or hardware parity.
SourceEventPhase5Result consume_source_event1_phase5_80100efc(SourceEventWork& work,
    SourceEventPhase5Context& context,const SourceEventPhase5Profile& profile,
    const SourceEventPhase5ResourceBinding* resources=nullptr) noexcept;
} // namespace l3
