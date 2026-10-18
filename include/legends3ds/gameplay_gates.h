#pragma once
#include <cstddef>
#include <cstdint>

// 0.0.15.0 default-gameplay gate resolution.
//
// Through 0.0.14.51 every source-backed gameplay owner (actor state fields,
// actor proxies/Buster, actor textures, candidate OpenBIOS RNG) defaulted OFF
// and had to be enabled with a separate SD file. The ordinary build therefore
// played the older candidate projectile path. From 0.0.15.0 the source-backed
// combat path is the default; each SD file still overrides its own gate, and
// one master file restores the historical all-OFF defaults for A/B tests.
//
// Evidence labels are unchanged: enabling a gate by default does not promote
// any CANDIDATE/UNKNOWN owner to VERIFIED_1TO1.
namespace l3 {

enum class GateFileState : uint8_t { Absent, Enabled, Disabled, Malformed };

// Parses exactly "<key>=<0|1>" followed only by optional ASCII whitespace.
// data==nullptr means the file is absent. Empty, truncated, embedded-NUL,
// other values or trailing tokens are Malformed.
GateFileState parse_gate_file(const char* data, std::size_t size, const char* key) noexcept;

// Enabled/Disabled are explicit. Malformed fails closed (false), matching the
// historical loader. Absent uses the profile default.
constexpr bool resolve_gate(GateFileState state, bool default_on) noexcept {
    return state == GateFileState::Enabled ||
           (state == GateFileState::Absent && default_on);
}

// Master file sdmc:/3ds/Legends3DS/gameplay_profile.ini, key SOURCE_COMBAT_DEFAULT.
// Absent => true (0.0.15.0 default). Disabled or Malformed => historical OFF defaults.
constexpr const char* kGameplayProfileKey = "SOURCE_COMBAT_DEFAULT";

struct GameplayGateDefaults {
    bool actor_state_fields = false;   // actor_state_01418.cfg   ACTOR_STATE_FIELDS
    bool source_actor_proxy = false;   // source_actor_proxy.ini  SOURCE_ACTOR_PROXY
    bool actor_textures = false;       // actor_render_01416.cfg  ACTOR_TEXTURES
    bool actor_rng_candidate = false;  // actor_rng.ini           ACTOR_RNG_CANDIDATE
};

// Research/replay/scene gates (SOURCE_EVENT_BOOT, SOURCE_SCENE_*, SOURCE_LOOP_REPLAY,
// RETAIL_ACTOR_ANIMATION, ...) are deliberately not part of this profile.
constexpr GameplayGateDefaults gameplay_gate_defaults(GateFileState master) noexcept {
    const bool on = resolve_gate(master, true);
    return GameplayGateDefaults{on, on, on, on};
}

// Native frontend firing admission for the source Buster producer. The Y edge
// itself is a CANDIDATE owner for player+174 bits 10/20; the shot-enable bit
// (+175 bit0) and the 0x8004324C busy predicate are the same source fields the
// historical candidate projectile path already respected.
constexpr bool source_buster_fire_admitted(bool source_combat_active, bool fire_edge,
                                           bool shot_enabled, bool player_busy) noexcept {
    return source_combat_active && fire_edge && shot_enabled && !player_busy;
}

} // namespace l3
