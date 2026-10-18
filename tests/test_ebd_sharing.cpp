#include "legends3ds/ebd_directory.h"
#include <cassert>
#include <vector>

int main() {
    l3::EbdDirectory original;
    const std::vector<uint8_t> source{10,20,30,40,50,60};
    original.source_payload.assign(source.begin(), source.end());
    l3::EbdDirectoryEntry entry;
    entry.resource_offset[0] = 2;
    original.entries.push_back(entry);
    auto registry = original;
    auto active = registry;
    const auto lifetime = original.source_payload.lifetime();
    assert(original.source_payload.data() == registry.source_payload.data());
    assert(active.source_payload.storage_identity() == registry.source_payload.storage_identity());
    auto window = l3::ebd_resource_span(active, 0, 0, 4);
    assert(window.data == original.source_payload.data()+2 && window.data[3] == 60);
    // Replacement must not truncate or invalidate another registration's data.
    original.source_payload.resize(2);
    assert(original.source_payload.size() == 2 && registry.source_payload == source);
    assert(window.data[3] == 60 && active.source_payload == source);
    registry.source_payload.clear();
    assert(!lifetime.expired() && active.source_payload == source);
    active.source_payload.clear();
    assert(lifetime.expired());
    // Replacing an active payload must detach, not overwrite another owner.
    auto replacement = original;
    replacement.source_payload.assign(source.begin(), source.end());
    assert(original.source_payload.size() == 2 && replacement.source_payload.size() == 6);
}
