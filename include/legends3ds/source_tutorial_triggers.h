#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace l3 {

// ST00 E0/06 tutorial message-trigger controller (0x80114424). Records come
// from the per-area lists 0x8011CC9C[area] (16 bytes, 0xFF-terminated), cooked
// owner-locally into neutral L3TR/1 fields by tools/cook_source_tutorial_triggers.py.
constexpr size_t kTutorialTriggerAreas = 5;
constexpr size_t kTutorialTriggerMaxRecords = 32;

struct TutorialTriggerRecord {
    uint8_t type = 0;      // +0 handler index into 0x8011CB24
    int8_t message = 0;    // +1 message block in the area's bank
    int8_t cell_x = 0;     // +2 (player X >> 9)
    int8_t cell_z = 0;     // +3
    uint16_t width = 0;    // +4 cells
    uint16_t depth = 0;    // +6 cells
};
struct TutorialTriggerCatalog {
    std::array<std::vector<TutorialTriggerRecord>, kTutorialTriggerAreas> areas{};
};
std::vector<uint8_t> cook_l3tutorial_triggers(const TutorialTriggerCatalog&);
// Validates magic, version, sizes, counts and CRC before decoding.
TutorialTriggerCatalog load_l3tutorial_triggers(const std::vector<uint8_t>& bytes,
                                                bool verify_crc = true);

// Everything a handler reads or calls outside its own record.
class TutorialTriggerHost {
public:
    virtual ~TutorialTriggerHost() = default;
    virtual bool flag(uint16_t id) const = 0;               // 0x8001DA58
    virtual void set_flag(uint16_t id) = 0;                 // 0x8001DA8C
    virtual void clear_flag(uint16_t id) = 0;               // 0x8001DCD0
    virtual void request_message(int8_t block) = 0;         // 0x80053A08(0, block)
    virtual bool message_active() const = 0;                // 0x80098A58 & 0x08000000
    virtual void voice(uint32_t clip) = 0;                  // 0x8001D648 (CANDIDATE: voice line)
    virtual void pause_objects(uint16_t mask, uint32_t mode) = 0; // 0x8002EE90
    virtual uint8_t& player_bd() = 0;                       // player +0xBD (0x40 = freeze)
};

struct TutorialTriggerWorld {
    size_t area = 0;                                        // 0x800C1B61
    int16_t player_x = 0, player_y = 0, player_z = 0;       // 0x800B51C4/C6/C8
    uint16_t player_heading = 0;                            // 0x800B5206
    std::optional<int32_t> player_floor_y;                  // 0x80048E6C(player)
    std::vector<std::array<int16_t, 2>> class20_xz;         // live (+0 bit 0) class-0x20 +14/+18
};

enum class TutorialTriggerStatus : uint8_t {
    SourceBackedPartial, Inactive, NoCatalog, AreaOutOfRange, UnsupportedType, NeedsFloor
};
struct TutorialTriggerTick {
    TutorialTriggerStatus status = TutorialTriggerStatus::Inactive;
    uint8_t handlers_run = 0;
    std::optional<uint8_t> unsupported_type;
};

// One E0/06 object. State 0 (0x80114460) clears every record's +8..+B in all
// areas; state 1 (0x80114504) computes the player cell and runs each record's
// handler in list order. A record with an unported type holds only itself.
class SourceTutorialTriggers {
public:
    void bind(const TutorialTriggerCatalog* catalog) noexcept { catalog_ = catalog; reset(); }
    void reset() noexcept;   // a new E0/06 object (state 0 pending)
    TutorialTriggerTick tick(TutorialTriggerHost& host, const TutorialTriggerWorld& world);
    uint8_t controller_state() const noexcept { return state_; }
    int16_t cell_x() const noexcept { return cell_x_; }
    int16_t cell_z() const noexcept { return cell_z_; }
    uint8_t record_state(size_t area, size_t index) const;
private:
    struct RecordWork { uint8_t state = 0; int16_t timer = 0; };
    bool in_region(const TutorialTriggerRecord& r) const noexcept;
    bool run(uint8_t type, const TutorialTriggerRecord& r, RecordWork& w,
             TutorialTriggerHost& host, const TutorialTriggerWorld& world);
    const TutorialTriggerCatalog* catalog_ = nullptr;
    std::array<std::array<RecordWork, kTutorialTriggerMaxRecords>, kTutorialTriggerAreas> work_{};
    uint8_t state_ = 0;
    int16_t cell_x_ = 0, cell_z_ = 0;
};

} // namespace l3
