#include "legends3ds/frontend_servbots.h"
#include "legends3ds/l3map.h"
#include <cstring>
#include <stdexcept>

namespace l3 {

std::optional<int16_t> TitleServbotScripts::halfword(uint32_t address) const noexcept {
    if (address < base || (address & 1u) || address - base + 2u > data.size()) return std::nullopt;
    const size_t at = address - base;
    return int16_t(uint16_t(data[at] | (data[at + 1] << 8)));
}
std::optional<uint32_t> TitleServbotScripts::word(uint32_t address) const noexcept {
    const auto lo = halfword(address), hi = halfword(address + 2);
    if (!lo || !hi || (address & 3u)) return std::nullopt;
    return uint32_t(uint16_t(*lo)) | (uint32_t(uint16_t(*hi)) << 16);
}
std::optional<uint32_t> TitleServbotScripts::script(uint8_t wave, uint8_t index) const noexcept {
    // 0x80014AC0: *(*(0x800820E0 + (s8)wave * 4) + index * 4).
    const auto table = word(0x800820E0u + uint32_t(int8_t(wave)) * 4u);
    if (!table) return std::nullopt;
    return word(*table + uint32_t(index) * 4u);
}

TitleServbotScripts load_title_servbot_scripts(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 20) throw std::runtime_error("not L3TS");
    const bool v2 = std::memcmp(bytes.data(), "L3TS0002", 8) == 0;
    if (!v2 && std::memcmp(bytes.data(), "L3TS0001", 8) != 0) throw std::runtime_error("not L3TS");
    const auto rd = [&](size_t at) { return uint32_t(bytes[at]) | (uint32_t(bytes[at + 1]) << 8) |
                                             (uint32_t(bytes[at + 2]) << 16) | (uint32_t(bytes[at + 3]) << 24); };
    TitleServbotScripts out;
    out.base = rd(8);
    const size_t size = rd(12), camera_bytes = v2 ? 24 : 0;
    if (size > bytes.size() || bytes.size() != 20u + size + camera_bytes) throw std::runtime_error("L3TS size mismatch");
    if (crc32(bytes.data() + 20, size + camera_bytes) != rd(16)) throw std::runtime_error("L3TS CRC mismatch");
    out.data.assign(bytes.begin() + 20, bytes.begin() + 20 + std::ptrdiff_t(size));
    if (v2) {
        TitleServbotCamera c;
        const size_t at = 20 + size;
        for (unsigned a = 0; a < 3; ++a) c.target[a] = int32_t(rd(at + a * 4));
        c.distance = int32_t(rd(at + 12)); c.yaw = int32_t(rd(at + 16)); c.pitch = int32_t(rd(at + 20));
        out.camera = c;
    }
    return out;
}

void title_servbot_spawn_80015428(TitleServbotParade& p, uint8_t frontend_state,
                                  int16_t fade_80098a72, TitleServbotFrame& out) {
    if (p.phase_0a == 1) {
        if (p.timer_06 != 0) { --p.timer_06; return; }
        // 0x8002EE90(2, 0x80): bit 2 sweeps the class-20 pool; flag 0x80
        // unlinks (0x80052528) and releases (0x80031824) every live slot.
        for (auto& s : p.slots) if (s.active) { s.active = false; ++out.released_8002ee90; }
        if (++p.wave_08 >= 9) p.wave_08 = 0;
        ++p.phase_0a;
        return;
    }
    if (p.phase_0a == 2) {
        if (fade_80098a72 != 0) return;
        // sltiu (state - 4) < 2, or state == 6: menu, CONTINUE, OPTION.
        if (uint8_t(frontend_state - 4u) < 2u || int8_t(frontend_state) == 6) p.phase_0a = 0;
        return;
    }
    if (p.phase_0a != 0 || fade_80098a72 != 0) return;
    for (uint8_t i = 0; i < 16; ++i) {
        // 0x80031418: first free slot of the 16-entry class-20 pool.
        TitleServbot* slot = nullptr;
        for (auto& s : p.slots) if (!s.active) { slot = &s; break; }
        if (!slot) continue;
        // The spawner writes +0/+2..+4/+0x0C only and 0x80014AC0 leaves the
        // position: a reused slot keeps its previous occupant's +0x14/+0x1C
        // (oracle: an unscripted slot reports it in its one state-2 update).
        const auto raw = slot->raw; const auto fixed = slot->fixed;
        *slot = TitleServbot{};
        slot->raw = raw; slot->fixed = fixed;
        // Oracle: the title setup's spawn (state 1, 0x800135DC) first runs
        // its callbacks on the next update; later spawns (states 2/4 through
        // 0x800137D4) run them in the same update. CANDIDATE mechanism.
        slot->active = true; slot->fresh = frontend_state == 1; slot->index = i; slot->state = 0;
        slot->wave = uint8_t(p.wave_08);
    }
    p.timer_06 = 300;
    ++p.phase_0a;
}

namespace {
// 0x80014BCC: commands until a 0 (wait) or 26 (end); the wait stores its
// count - 1 and steps past it.
void read_script_80014bcc(TitleServbot& s, const TitleServbotScripts& scripts) {
    const auto arg = [&](unsigned k) -> int16_t {
        return scripts.halfword(s.script + uint32_t(s.pc + k) * 2u).value_or(0);
    };
    for (unsigned guard = 0; guard < 4096; ++guard) {
        const auto op = scripts.halfword(s.script + uint32_t(s.pc) * 2u);
        if (!op) { s.state = 2; return; }
        if (*op == 0) {
            s.delay = int16_t(arg(1) - 1);
            s.pc = uint16_t(s.pc + 2);
            return;
        }
        switch (*op) {
            case 1:
                for (unsigned a = 0; a < 3; ++a) { s.raw[a] = arg(1 + a); s.fixed[a] = int32_t(uint32_t(uint16_t(s.raw[a])) << 16); }
                s.pc = uint16_t(s.pc + 4); break;
            case 2: case 3: case 4: {
                const unsigned a = unsigned(*op - 2);
                s.raw[a] = arg(1); s.fixed[a] = int32_t(uint32_t(uint16_t(s.raw[a])) << 16);
                s.pc = uint16_t(s.pc + 2); break;
            }
            case 5: s.anim_ac = uint8_t(arg(1)); s.pc = uint16_t(s.pc + 2); break;
            case 6: s.velocity[0] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 7: s.acceleration[0] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 8: s.velocity[1] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 9: s.acceleration[1] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 10: s.velocity[2] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 11: s.acceleration[2] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 12: s.heading = uint16_t(arg(1)); s.pc = uint16_t(s.pc + 2); break;
            case 13: s.heading_step = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 14: s.byte_13 = uint8_t(arg(1)); s.pc = uint16_t(s.pc + 2); break;
            case 15: s.world_motion = 0; s.pc = uint16_t(s.pc + 1); break;
            case 16: s.world_motion = 1; s.pc = uint16_t(s.pc + 1); break;
            case 17: s.steps_on = 1; s.step_timer = 0; s.step_mode = uint8_t(arg(1)); s.pc = uint16_t(s.pc + 2); break;
            case 18: s.steps_on = 0; s.pc = uint16_t(s.pc + 1); break;
            case 19:
                for (unsigned a = 0; a < 3; ++a) s.scale[a] = arg(1 + a);
                s.pc = uint16_t(s.pc + 4); break;
            case 20: case 21: case 22: s.scale[unsigned(*op - 20)] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 23: case 24: case 25: s.scale_step[unsigned(*op - 23)] = arg(1); s.pc = uint16_t(s.pc + 2); break;
            case 26: s.pc = uint16_t(arg(1)); break;      // 0x80015074: jump
            case 27: s.state = 2; return;                  // 0x80015068: end
            default: s.state = 2; return;                  // outside the 27-entry table
        }
    }
    s.state = 2;
}

void run_800150e0(TitleServbot& s, const TitleServbotScripts& scripts, const ReferenceTables& refs,
                  size_t slot, TitleServbotFrame& out) {
    s.anim_ad = s.anim_ac;
    s.byte_14 = s.byte_13;
    // No state test follows the reader: the frame that reads 27 still moves.
    if (s.delay == 0) read_script_80014bcc(s, scripts);
    else --s.delay;
    for (unsigned a = 0; a < 3; ++a) s.velocity[a] = int16_t(uint16_t(s.velocity[a] + s.acceleration[a]));
    if (s.world_motion) {
        for (unsigned a = 0; a < 3; ++a) {
            s.fixed[a] = int32_t(uint32_t(s.fixed[a]) + (uint32_t(int32_t(s.velocity[a])) << 12));
            s.raw[a] = int16_t(uint16_t(uint32_t(s.fixed[a]) >> 16));
        }
    } else {
        // 0x8002FEA4(work, +0x44, +0x46, +0x48): heading-relative, then 0x80030150.
        const auto d = reference_player_motion_delta(refs, uint16_t(s.heading & 0xFFFu),
                                                     s.velocity[0], s.velocity[1], s.velocity[2]);
        const std::array<int32_t, 3> delta{{d.dx_fixed, d.dy_fixed, d.dz_fixed}};
        for (unsigned a = 0; a < 3; ++a) {
            s.fixed[a] = int32_t(uint32_t(s.fixed[a]) + uint32_t(delta[a]));
            s.raw[a] = reference_fixed_to_short_80030150(s.fixed[a]);
        }
    }
    s.heading = uint16_t(s.heading + s.heading_step);
    for (unsigned a = 0; a < 3; ++a) s.scale[a] = int16_t(uint16_t(s.scale[a] + s.scale_step[a]));
    if (s.steps_on) {
        const auto start = [&](uint8_t id, int8_t timer) {
            if (s.step_timer != 0) { --s.step_timer; return; }
            out.sounds.push_back(id); s.step_timer = timer;
        };
        switch (s.step_mode) {
            case 0: start(214, 18); break;
            case 1: start(215, 10); break;
            case 2: start(215, 8); break;
            case 3: out.sounds.push_back(218); s.steps_on = 0; break;
            case 4: out.sounds.push_back(217); s.steps_on = 0; break;
            default: break;
        }
    }
    if (s.byte_13 != s.byte_14) s.byte_110 = s.byte_13;
    out.animation_restart[slot] = s.anim_ad != s.anim_ac;
}
} // namespace

void title_servbot_update_800153ec(TitleServbotParade& p, const TitleServbotScripts& scripts,
                                   const ReferenceTables& refs, TitleServbotFrame& out) {
    for (size_t i = 0; i < p.slots.size(); ++i) {
        auto& s = p.slots[i];
        if (!s.active) continue;
        if (s.fresh) { s.fresh = false; continue; }
        switch (s.state) {
            case 0: {
                // 0x80014AC0: scale 0x400 x3, zero motion, the wave's script.
                s.scale = {{0x400, 0x400, 0x400}};
                // +0x44..+0x5B (velocity, acceleration, heading) and +0x10 cleared;
                // the position keeps its value until the script sets it.
                s.velocity = {}; s.acceleration = {}; s.heading = 0;
                const auto script = scripts.script(s.wave, s.index);
                if (!script || *script == 0) { s.state = 2; break; }
                s.script = *script; s.pc = 0; s.delay = 0; s.heading_step = 0; s.scale_step = {};
                s.world_motion = 0; s.anim_ac = 0xFF; s.byte_13 = 0xFF;
                s.state = 1;
                run_800150e0(s, scripts, refs, i, out);
                break;
            }
            case 1: run_800150e0(s, scripts, refs, i, out); break;
            case 2: s.active = false; break;      // 0x800153CC -> 0x80031824
            default: break;
        }
    }
}

} // namespace l3
