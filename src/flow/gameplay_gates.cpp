#include "legends3ds/gameplay_gates.h"

namespace l3 {
namespace {
bool is_space(char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
}

GateFileState parse_gate_file(const char* data, std::size_t size, const char* key) noexcept {
    if (!data) return GateFileState::Absent;
    if (!key || size == 0) return GateFileState::Malformed;
    std::size_t i = 0;
    for (; key[i] != '\0'; ++i) {
        if (i >= size || data[i] != key[i]) return GateFileState::Malformed;
    }
    if (i == 0 || i >= size || data[i] != '=') return GateFileState::Malformed;
    ++i;
    while (i < size && (data[i] == ' ' || data[i] == '\t')) ++i; // historical %u accepted this
    if (i >= size || (data[i] != '0' && data[i] != '1')) return GateFileState::Malformed;
    const bool enabled = data[i] == '1';
    for (++i; i < size; ++i) {
        if (!is_space(data[i])) return GateFileState::Malformed;
    }
    return enabled ? GateFileState::Enabled : GateFileState::Disabled;
}

} // namespace l3
