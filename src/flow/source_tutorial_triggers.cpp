#include "legends3ds/source_tutorial_triggers.h"
#include "legends3ds/l3map.h"
#include <stdexcept>

namespace l3 {
namespace {
constexpr uint16_t kTutorialDone = 0x622;    // 1570: set by type 15, disables every handler
constexpr uint16_t kPauseMask = 0x27E, kPauseMask11 = 0x256;
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t rd32(const uint8_t* p) { return uint32_t(rd16(p)) | (uint32_t(rd16(p + 2)) << 16); }
void wr16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); }
void wr32(std::vector<uint8_t>& b, uint32_t v) { wr16(b, uint16_t(v)); wr16(b, uint16_t(v >> 16)); }
// (s16 << 16) >> 25: the arithmetic X/Z >> 9 the walker and type 6 use.
int16_t cell_of(int16_t v) { return int16_t(int32_t(v) >> 9); }
}

std::vector<uint8_t> cook_l3tutorial_triggers(const TutorialTriggerCatalog& c) {
    std::vector<uint8_t> payload; uint32_t records = 0;
    for (const auto& area : c.areas) {
        if (area.size() > kTutorialTriggerMaxRecords) throw std::invalid_argument("too many tutorial triggers");
        wr16(payload, uint16_t(area.size()));
        for (const auto& r : area) {
            payload.push_back(r.type); payload.push_back(uint8_t(r.message));
            payload.push_back(uint8_t(r.cell_x)); payload.push_back(uint8_t(r.cell_z));
            wr16(payload, r.width); wr16(payload, r.depth); ++records;
        }
    }
    std::vector<uint8_t> out{'L', '3', 'T', 'R'};
    wr16(out, 1); wr16(out, 32); wr32(out, uint32_t(32 + payload.size()));
    wr32(out, uint32_t(kTutorialTriggerAreas)); wr32(out, records);
    wr32(out, crc32(payload.data(), payload.size()));
    out.resize(32, 0); out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

TutorialTriggerCatalog load_l3tutorial_triggers(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < 32 || b[0] != 'L' || b[1] != '3' || b[2] != 'T' || b[3] != 'R')
        throw std::runtime_error("not L3TR");
    if (rd16(&b[4]) != 1 || rd16(&b[6]) != 32 || rd32(&b[8]) != b.size() ||
        rd32(&b[12]) != kTutorialTriggerAreas) throw std::runtime_error("unsupported L3TR");
    for (size_t i = 24; i < 32; ++i) if (b[i]) throw std::runtime_error("L3TR reserved header nonzero");
    if (verify_crc && crc32(b.data() + 32, b.size() - 32) != rd32(&b[20]))
        throw std::runtime_error("L3TR CRC mismatch");
    TutorialTriggerCatalog out; size_t at = 32; uint32_t records = 0;
    for (auto& area : out.areas) {
        if (at + 2 > b.size()) throw std::runtime_error("L3TR truncated");
        const uint16_t n = rd16(&b[at]); at += 2;
        if (n > kTutorialTriggerMaxRecords || at + size_t(n) * 8 > b.size()) throw std::runtime_error("L3TR bad count");
        area.resize(n);
        for (auto& r : area) {
            r.type = b[at]; r.message = int8_t(b[at + 1]); r.cell_x = int8_t(b[at + 2]); r.cell_z = int8_t(b[at + 3]);
            r.width = rd16(&b[at + 4]); r.depth = rd16(&b[at + 6]); at += 8; ++records;
        }
    }
    if (at != b.size() || records != rd32(&b[16])) throw std::runtime_error("L3TR size mismatch");
    return out;
}

void SourceTutorialTriggers::reset() noexcept { state_ = 0; cell_x_ = 0; cell_z_ = 0; }

uint8_t SourceTutorialTriggers::record_state(size_t area, size_t index) const {
    return work_.at(area).at(index).state;
}

bool SourceTutorialTriggers::in_region(const TutorialTriggerRecord& r) const noexcept {
    // 0x801159F8: unsigned 16-bit offsets from the record's corner.
    return uint16_t(cell_x_ - int16_t(r.cell_x)) < r.width &&
           uint16_t(cell_z_ - int16_t(r.cell_z)) < r.depth;
}

TutorialTriggerTick SourceTutorialTriggers::tick(TutorialTriggerHost& host, const TutorialTriggerWorld& world) {
    TutorialTriggerTick out;
    if (!catalog_) { out.status = TutorialTriggerStatus::NoCatalog; return out; }
    if (state_ == 0) {
        // 0x80114460: clear +8..+B of every record of every area.
        for (auto& area : work_) for (auto& w : area) w = {};
        state_ = 1; out.status = TutorialTriggerStatus::SourceBackedPartial; return out;
    }
    if (state_ != 1) return out;   // state 2 (0x801145D4) releases the object
    if (world.area >= kTutorialTriggerAreas) { out.status = TutorialTriggerStatus::AreaOutOfRange; return out; }
    cell_x_ = cell_of(world.player_x); cell_z_ = cell_of(world.player_z);
    out.status = TutorialTriggerStatus::SourceBackedPartial;
    const auto& list = catalog_->areas[world.area];
    for (size_t i = 0; i < list.size() && i < kTutorialTriggerMaxRecords; ++i) {
        if (!run(list[i].type, list[i], work_[world.area][i], host, world)) {
            if (!out.unsupported_type) out.unsupported_type = list[i].type;
            out.status = TutorialTriggerStatus::UnsupportedType;
            continue;
        }
        ++out.handlers_run;
    }
    return out;
}

bool SourceTutorialTriggers::run(uint8_t type, const TutorialTriggerRecord& r, RecordWork& w,
                                 TutorialTriggerHost& h, const TutorialTriggerWorld& world) {
    auto& bd = h.player_bd();
    const auto freeze_start = [&](int8_t message, uint16_t mask = kPauseMask) {
        w.state = 1; h.request_message(message); bd |= 0x40; h.pause_objects(mask, 1);
    };
    const auto freeze_end = [&](uint16_t own, std::optional<uint32_t> clip, uint16_t mask = kPauseMask) {
        if (clip) h.voice(*clip);
        h.set_flag(own); bd &= 0xBF; h.pause_objects(mask, 2);
    };
    // The common frozen-message handler (types 1, 4, 7, 8, 12, 13 shapes).
    const auto frozen = [&](uint16_t own, std::optional<uint32_t> clip, int8_t message) {
        if (h.flag(kTutorialDone) || h.flag(own)) return;
        if (w.state == 0) { if (in_region(r)) freeze_start(message); }
        else if (w.state == 1 && !h.message_active()) freeze_end(own, clip);
    };
    const auto talking = [&](uint16_t own, std::optional<uint32_t> clip) {   // types 2, 3: no freeze
        if (h.flag(kTutorialDone) || h.flag(own)) return;
        if (w.state == 0) { if (in_region(r)) { w.state = 1; h.request_message(r.message); } }
        else if (w.state == 1 && !h.message_active()) { if (clip) h.voice(*clip); h.set_flag(own); }
    };
    const auto zone_voice = [&](uint16_t gate, uint32_t clip) {              // types 0, 5
        if (!h.flag(gate) && int8_t(w.state) < 2 && int8_t(w.state) >= 0 && in_region(r)) { w.state = 1; h.voice(clip); }
    };
    switch (type) {
    case 0: zone_voice(0x610, 0); return true;                                  // 0x801145F4
    case 1: frozen(0x610, 5u, r.message); return true;                          // 0x8011466C
    case 2: talking(0x612, 6u); return true;                                    // 0x80114774
    case 3: talking(0x613, std::nullopt); return true;                          // 0x80114838
    case 4: {                                                                   // 0x801148F4
        const bool facing = ((uint32_t(world.player_heading) - 0x200u) & 0xFFFu) < 0x400u;
        frozen(0x614, std::nullopt, int8_t(facing ? r.message : r.message + 1)); return true;
    }
    case 5: zone_voice(0x615, 10); return true;                                 // 0x80114A34
    case 6: {                                                                   // 0x80114AAC
        if (h.flag(0x60A)) return true;
        const int8_t st = int8_t(w.state);
        if (st == 1) {
            if (!h.message_active() || h.flag(kTutorialDone)) {
                bool occupied = false;
                for (const auto& p : world.class20_xz)
                    if (uint32_t(int32_t(cell_of(p[0])) - r.cell_x) < uint32_t(r.width) &&
                        uint32_t(int32_t(cell_of(p[1])) - r.cell_z) < uint32_t(r.depth)) { occupied = true; break; }
                if (!occupied) {
                    w.state = 2;
                    if (!h.flag(kTutorialDone)) h.request_message(int8_t(r.message + 1));
                    h.set_flag(0x61F);
                }
            }
        } else if (st < 2) {
            if (st == 0 && in_region(r) && h.flag(0x607)) {
                w.state = 1; w.timer = 0; h.set_flag(0x615);
                if (!h.flag(kTutorialDone)) h.request_message(r.message);
            }
        } else if (st == 2 && (!h.message_active() || h.flag(kTutorialDone))) w.state = 3;
        return true;
    }
    case 7: frozen(0x616, 14u, r.message); return true;                         // 0x80114C88
    case 8: frozen(0x617, 7u, r.message); return true;                          // 0x80114D90
    case 9: {                                                                   // 0x80114E98
        if (h.flag(kTutorialDone) || h.flag(0x618)) return true;
        if (w.state == 0) {
            if (!in_region(r) || world.player_y != -0xC1) { w.timer = 0; return true; }
            if (w.timer < 0x5A) { ++w.timer; return true; }
            freeze_start(r.message);
        } else if (w.state == 1 && !h.message_active()) freeze_end(0x618, 21u);
        return true;
    }
    case 10:                                                                    // 0x80114FE0
        if (h.flag(kTutorialDone) || h.flag(0x619)) return true;
        if (w.state == 0) {
            if (in_region(r)) { w.state = 1; h.request_message(r.message); h.set_flag(0x618); h.set_flag(0x60B); bd |= 0x40; }
        } else if (w.state == 1 && !h.message_active()) { h.set_flag(0x619); bd &= 0xBF; }
        return true;
    case 11:                                                                    // 0x801150CC
        if (h.flag(kTutorialDone) || h.flag(0x61A)) return true;
        if (w.state == 1) {
            if (!world.player_floor_y) return false;
            if (int32_t(world.player_y) == *world.player_floor_y) { w.state = 2; h.request_message(r.message); }
        } else if (w.state == 0) {
            if (in_region(r)) { w.state = 1; bd |= 0x40; h.pause_objects(kPauseMask11, 1); }
        } else if (w.state == 2 && !h.message_active()) { h.set_flag(0x61A); bd &= 0xBF; h.pause_objects(kPauseMask11, 2); }
        return true;
    case 12:                                                                    // 0x80115210
        if (h.flag(0x608) && h.flag(0x609)) return true;
        frozen(0x61B, std::nullopt, r.message); return true;
    case 13: {                                                                  // 0x80115330
        if (h.flag(kTutorialDone) || h.flag(0x61C)) return true;
        if (w.state == 0) {
            if (!in_region(r) || (h.flag(0x608) && h.flag(0x609))) return true;
            freeze_start(int8_t(h.flag(0x61B) ? r.message : r.message + 1));
        } else if (w.state == 1 && !h.message_active()) freeze_end(0x61C, std::nullopt);
        return true;
    }
    case 14:                                                                    // 0x80115480
        if (h.flag(kTutorialDone) || h.flag(0x61D)) return true;
        if (w.state == 0) { if (h.flag(0x60E)) { freeze_start(r.message); h.clear_flag(0x60E); } }
        else if (w.state == 1 && !h.message_active()) freeze_end(0x61D, std::nullopt);
        return true;
    case 15:                                                                    // 0x801155A4
        if (h.flag(kTutorialDone) || h.flag(0x61E)) return true;
        if (w.state == 0) { if (in_region(r)) freeze_start(r.message); }
        else if (w.state == 1 && !h.message_active()) { freeze_end(0x61E, std::nullopt); h.set_flag(kTutorialDone); }
        return true;
    default:
        return false;   // type 16 (0x801156C8) is not decompiled yet
    }
}

} // namespace l3
