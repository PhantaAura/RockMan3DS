#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3AnimationVersion = 1;
constexpr uint32_t kPbdPackedAngleChannels = 40;
constexpr uint32_t kPbdPackedFrameBytes = 60;
constexpr uint32_t kPbdMainPosePartCount = 15;
constexpr uint16_t kL3SkeletonVersion = 1;

struct PbdSRecord {
    uint16_t word0 = 0;
    uint16_t word1 = 0;
    uint32_t word2 = 0;
};

struct PbdAnimationAction {
    uint16_t action_index = 0;
    bool has_s_records = false;
    bool has_m_pointer = false;
    bool m_is_frame_table = false;
    bool m_special_or_unknown = false;
    uint32_t source_s_offset = 0xFFFFFFFFu;
    uint32_t source_m_offset = 0xFFFFFFFFu;
    std::vector<PbdSRecord> s_records;
    std::vector<std::array<uint16_t, kPbdPackedAngleChannels>> angle_frames;
};

struct PbdAnimation {
    uint32_t s_load_base = 0;
    uint32_t m_load_base = 0;
    uint16_t action_slots = 0;
    std::vector<PbdAnimationAction> actions;
};

// Main-body M-frame pose bank proven from ROCK_NEO.EXE 0x800329D4 -> 0x800325D4 -> 0x800331A0.
// Channels 0..2 are signed 12-bit local/root translation. Channels 3..37 feed the rotation
// vectors for player part IDs 0..14; zero components below are explicitly written as zero by the game.
// Channels 38..39 are retained in L3AN and are padding/zero for all regular PL00M00M frames we measured.
struct PbdMainPoseBank {
    std::array<int16_t, 3> root_translation{};
    std::array<std::array<uint16_t, 3>, kPbdMainPosePartCount> part_rotation{};
    std::array<uint16_t, 2> trailing_channels{};
};

struct PbdMainSkeleton {
    uint32_t source_special_offset = 0xFFFFFFFFu;
    std::array<std::array<int16_t, 3>, kPbdMainPosePartCount> base_translation{};
    std::array<int16_t, kPbdMainPosePartCount> source_reserved{};
    std::array<int8_t, kPbdMainPosePartCount> parent{{-1,0,0,2,3,0,5,6,0,8,9,10,0,12,13}};
};

struct L3SkeletonHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t part_count = 0;
    uint32_t source_special_offset = 0;
    uint32_t off_parts = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

struct L3AnimationStats {
    uint32_t action_slots = 0;
    uint32_t populated_actions = 0;
    uint32_t frame_table_actions = 0;
    uint32_t special_m_actions = 0;
    uint32_t s_records = 0;
    uint32_t frames = 0;
};

struct L3AnimationHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    L3AnimationStats stats{};
    uint32_t angle_channels_per_frame = 0;
    uint32_t off_actions = 0;
    uint32_t off_s_records = 0;
    uint32_t off_frames = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

// Proven by the executable's 12-bit unpack path: a 60-byte frame is 480 contiguous bits = forty 12-bit channels.
std::array<uint16_t, kPbdPackedAngleChannels> unpack_pbd_12bit_frame(const uint8_t* sixty_bytes);
std::array<uint8_t, kPbdPackedFrameBytes> pack_pbd_12bit_frame(const std::array<uint16_t, kPbdPackedAngleChannels>& channels);

int16_t sign_extend_pbd_12bit(uint16_t value);
PbdMainPoseBank decode_pbd_main_pose_bank(const std::array<uint16_t, kPbdPackedAngleChannels>& channels);
bool pbd_main_pose_trailing_channels_zero(const std::array<uint16_t, kPbdPackedAngleChannels>& channels);

// Pointer-free translation of the main-body S->M animation sequencing used by
// ROCK_NEO.EXE 0x800320B8 / 0x8003234C / 0x800329D4 for the PL00M00 family.
//
// For request values in the main family, the executable selects S action
// `request`, while the pose pointer table begins one word later in the M file,
// so the matching pose action is `request + 1`.  The current 8-byte S record's
// byte 0 selects the pose frame, byte 1 is its countdown, and signed byte 3
// controls end-of-record flow: -1 repeats the current record, other negative
// values branch to (value & 0x7F), and non-negative values advance sequentially.
// These names intentionally describe structure rather than gameplay meaning.
struct PbdMainBodyTrack {
    uint8_t request = 0;
    uint8_t record_index = 0;
    uint8_t ticks_remaining = 0;
    uint8_t pose_frame = 0;
    bool valid = false;
};

const PbdAnimationAction* find_pbd_animation_action(const PbdAnimation& animation, uint16_t action_index);
// Returns nullptr when a request is outside the proven PL00M00 S->M relationship
// or the corresponding action is not a decoded frame table.
const PbdAnimationAction* pbd_main_body_pose_action_for_request(const PbdAnimation& animation,
                                                                uint8_t request);
// Seed from the same substate/record-index meaning used by 0x800320B8.
PbdMainBodyTrack pbd_main_body_track_begin(const PbdAnimation& animation,
                                           uint8_t request,
                                           uint8_t substate);
// Advance one retail animation update using the control byte behavior at
// 0x8003234C. Returns false if the track becomes structurally invalid.
bool pbd_main_body_track_tick(const PbdAnimation& animation, PbdMainBodyTrack& track);
// Returns the currently selected packed pose frame, or nullptr if unavailable.
const std::array<uint16_t, kPbdPackedAngleChannels>* pbd_main_body_track_frame(
    const PbdAnimation& animation, const PbdMainBodyTrack& track);


// PL00M00M action slot 0 is a non-frame-table block. The player pose routine reads its 8-byte entries
// as base/local translation vectors for parts 1..14. The same function sequence plus its proven matrix
// push/pop helpers establishes the parent chains encoded here.
PbdMainSkeleton parse_pbd_main_skeleton(const std::vector<uint8_t>& m_bytes, uint32_t m_load_base);
std::vector<uint8_t> cook_l3skeleton(const PbdMainSkeleton& skeleton);
L3SkeletonHeader inspect_l3skeleton(const std::vector<uint8_t>& bytes, bool verify_crc = true);

// S/M load bases are taken from the user's DAT resource headers (+0x0C). No hard-coded PS1 addresses are required.
PbdAnimation parse_player_pbd_animation(const std::vector<uint8_t>& s_bytes, uint32_t s_load_base,
                                        const std::vector<uint8_t>& m_bytes, uint32_t m_load_base);
std::vector<uint8_t> cook_l3animation(const PbdAnimation& animation, L3AnimationStats* stats = nullptr);
L3AnimationHeader inspect_l3animation(const std::vector<uint8_t>& bytes, bool verify_crc = true);
// Runtime readers for pointer-free cooked player animation/skeleton data.
PbdAnimation load_l3animation(const std::vector<uint8_t>& bytes, bool verify_crc = true);
PbdMainSkeleton load_l3skeleton(const std::vector<uint8_t>& bytes, bool verify_crc = true);

} // namespace l3
