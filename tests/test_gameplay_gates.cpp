#include "legends3ds/gameplay_gates.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

using namespace l3;

static GateFileState parse(const char* text, const char* key = "SOURCE_ACTOR_PROXY") {
    return parse_gate_file(text, std::strlen(text), key);
}

int main() {
    // Explicit files.
    assert(parse("SOURCE_ACTOR_PROXY=1") == GateFileState::Enabled);
    assert(parse("SOURCE_ACTOR_PROXY=1\n") == GateFileState::Enabled);
    assert(parse("SOURCE_ACTOR_PROXY=1\r\n") == GateFileState::Enabled);
    assert(parse("SOURCE_ACTOR_PROXY= 1") == GateFileState::Enabled);
    assert(parse("SOURCE_ACTOR_PROXY=0") == GateFileState::Disabled);
    assert(parse("SOURCE_ACTOR_PROXY=0\n") == GateFileState::Disabled);
    // Absent file.
    assert(parse_gate_file(nullptr, 0, "SOURCE_ACTOR_PROXY") == GateFileState::Absent);
    // Malformed files fail closed.
    assert(parse("") == GateFileState::Malformed);
    assert(parse("SOURCE_ACTOR_PROXY=") == GateFileState::Malformed);
    assert(parse("SOURCE_ACTOR_PROXY=2") == GateFileState::Malformed);
    assert(parse("SOURCE_ACTOR_PROXY=1 x") == GateFileState::Malformed);
    assert(parse("SOURCE_ACTOR_PROXY=10") == GateFileState::Malformed);
    assert(parse("ACTOR_TEXTURES=1") == GateFileState::Malformed);
    assert(parse("SOURCE_ACTOR_PROX=1") == GateFileState::Malformed);
    {
        const char embedded[] = {'S','O','U','R','C','E','_','A','C','T','O','R','_','P','R','O','X','Y','=','1','\0','x'};
        assert(parse_gate_file(embedded, sizeof(embedded), "SOURCE_ACTOR_PROXY") == GateFileState::Malformed);
    }
    // Resolution.
    assert(resolve_gate(GateFileState::Enabled, false));
    assert(!resolve_gate(GateFileState::Disabled, true));
    assert(!resolve_gate(GateFileState::Malformed, true));
    assert(resolve_gate(GateFileState::Absent, true));
    assert(!resolve_gate(GateFileState::Absent, false));
    // Master profile: absent => source combat ON by default.
    {
        const auto d = gameplay_gate_defaults(GateFileState::Absent);
        assert(d.actor_state_fields && d.source_actor_proxy && d.actor_textures && d.actor_rng_candidate);
    }
    for (const auto master : {GateFileState::Disabled, GateFileState::Malformed}) {
        const auto d = gameplay_gate_defaults(master);
        assert(!d.actor_state_fields && !d.source_actor_proxy && !d.actor_textures && !d.actor_rng_candidate);
    }
    assert(gameplay_gate_defaults(parse("SOURCE_COMBAT_DEFAULT=1", kGameplayProfileKey)).source_actor_proxy);
    assert(!gameplay_gate_defaults(parse("SOURCE_COMBAT_DEFAULT=0", kGameplayProfileKey)).source_actor_proxy);
    // Fire admission keeps the historical shot-enable/busy source fields.
    assert(source_buster_fire_admitted(true, true, true, false));
    assert(!source_buster_fire_admitted(false, true, true, false));
    assert(!source_buster_fire_admitted(true, false, true, false));
    assert(!source_buster_fire_admitted(true, true, false, false));
    assert(!source_buster_fire_admitted(true, true, true, true));
    std::puts("gameplay gate defaults PASS");
    return 0;
}
