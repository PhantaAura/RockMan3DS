#include "legends3ds/ebd_directory.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
constexpr uint32_t base = 0x8016c000u;
void put32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value) {
    assert(offset <= bytes.size() && bytes.size() - offset >= 4);
    l3::patch_u32(bytes, offset, value);
}
template<class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
std::vector<uint8_t> source_fixture() {
    std::vector<uint8_t> bytes(96, 0);
    put32(bytes, 0, 2);
    bytes[4] = 0xe0; bytes[5] = 1; bytes[6] = 2; bytes[7] = 0x81;
    put32(bytes, 8, base + 48); put32(bytes, 12, base + 48); // Exact aliases.
    bytes[20] = 0x20; bytes[21] = 0x35; bytes[22] = 3;
    put32(bytes, 24, base + 52); // Overlaps the caller's first resource window.
    put32(bytes, 28, base); // In-payload directory pointer is legal, not typed.
    put32(bytes, 32, base + 95); // Last byte.
    for (size_t i = 48; i < bytes.size(); ++i) bytes[i] = uint8_t(i ^ 0xa5u);
    return bytes;
}
}
int main() {
    const auto source = source_fixture();
    auto directory = l3::parse_ebd_directory(source, base);
    assert(directory.source_payload == source);
    assert(directory.entries.size() == 2);
    const auto a = l3::ebd_resource_span(directory, 0, 0, 8);
    const auto alias = l3::ebd_resource_span(directory, 0, 1, 8);
    const auto overlap = l3::ebd_resource_span(directory, 1, 0, 4);
    assert(a.status == l3::EbdResourceSpanStatus::AVAILABLE && a.size == 8);
    assert(a.source_offset == 48 && a.data == directory.source_payload.data() + 48);
    assert(alias.data == a.data && overlap.data == a.data + 4);
    for (size_t i = 0; i < a.size; ++i) assert(a.data[i] == source[48 + i]);
    const auto null = l3::ebd_resource_span(directory, 0, 2, 100);
    assert(null.status == l3::EbdResourceSpanStatus::NULL_POINTER && !null.data && !null.size);
    assert(null.source_offset == UINT32_MAX);
    assert(l3::ebd_resource_span(directory, 1, 1, 4).data == directory.source_payload.data());
    assert(l3::ebd_resource_span(directory, 0, 0, 48).size == 48); // Not limited by next pointer.
    assert(l3::ebd_resource_span(directory, 1, 2, 1).data[0] == source.back());
    assert(l3::ebd_resource_span(directory, 1, 2, 0).status == l3::EbdResourceSpanStatus::AVAILABLE);
    rejects([&] { (void)l3::ebd_resource_span(directory, 2, 0, 1); });
    rejects([&] { (void)l3::ebd_resource_span(directory, 0, 3, 1); });
    rejects([&] { (void)l3::ebd_resource_span(directory, 0, 0, 49); });
    rejects([&] { (void)l3::ebd_resource_span(directory, 1, 2, 2); });
    rejects([&] { (void)l3::ebd_resource_span(directory, 0, 0, std::numeric_limits<size_t>::max()); });

    const auto cooked = l3::cook_l3ebd_directory(directory);
    const auto header = l3::inspect_l3ebd_directory(cooked);
    assert(header.version == 2 && header.header_bytes == 40 && header.flags == 7);
    assert(header.off_entries == 40 && header.off_source_payload == 72);
    assert(header.source_payload_bytes == source.size() && header.file_bytes == cooked.size());
    const auto loaded = l3::load_l3ebd_directory(cooked);
    assert(loaded.source_payload == source && loaded.load_base == base);
    assert(l3::cook_l3ebd_directory(loaded) == cooked);
    for (size_t row = 0; row < directory.entries.size(); ++row) {
        const auto& original = directory.entries[row]; const auto& restored = loaded.entries[row];
        assert(original.object_class == restored.object_class && original.subtype == restored.subtype);
        assert(original.variant == restored.variant && original.key_flags == restored.key_flags);
        for (size_t slot = 0; slot < 3; ++slot) assert(original.resource_offset[slot] == restored.resource_offset[slot]);
    }
    assert(l3::ebd_resource_span(loaded, 0, 0, 8).data == l3::ebd_resource_span(loaded, 0, 1, 8).data);
    auto legacy = directory; legacy.source_payload.clear();
    const auto v1 = l3::cook_l3ebd_directory(legacy);
    assert(l3::rd_u16(v1.data() + 4) == 1 && v1.size() == 64);
    const auto legacy_loaded = l3::load_l3ebd_directory(v1);
    assert(legacy_loaded.source_payload.empty() && legacy_loaded.entries[0].resource_offset[0] == 48);
    const auto absent = l3::ebd_resource_span(legacy_loaded, 0, 0, 8);
    assert(absent.status == l3::EbdResourceSpanStatus::PAYLOAD_UNAVAILABLE && absent.source_offset == 48);
    assert(!absent.data && !absent.size);
    assert(l3::ebd_resource_span(legacy_loaded, 0, 2, 8).status == l3::EbdResourceSpanStatus::NULL_POINTER);
    assert(l3::cook_l3ebd_directory(legacy_loaded) == v1);
    std::vector<uint8_t> empty_source(4, 0);
    const auto empty = l3::load_l3ebd_directory(l3::cook_l3ebd_directory(l3::parse_ebd_directory(empty_source, base)));
    assert(empty.entries.empty() && empty.source_payload == empty_source);
    assert(l3::inspect_l3ebd_directory(l3::cook_l3ebd_directory(l3::EbdDirectory{})).version == 1);

    rejects([&] { (void)l3::parse_ebd_directory(nullptr, 4, base); });
    rejects([&] { (void)l3::parse_ebd_directory(std::vector<uint8_t>{1, 0, 0}, base); });
    auto bad = source; put32(bad, 0, UINT32_MAX);
    rejects([&] { (void)l3::parse_ebd_directory(bad, base); });
    bad = source; put32(bad, 8, base - 1);
    rejects([&] { (void)l3::parse_ebd_directory(bad, base); });
    bad = source; put32(bad, 8, base + uint32_t(source.size()));
    rejects([&] { (void)l3::parse_ebd_directory(bad, base); });
    rejects([&] { (void)l3::parse_ebd_directory(source, UINT32_MAX - 2); });
    auto invalid_directory = directory; invalid_directory.entries[0].resource_offset[0] = 96;
    rejects([&] { (void)l3::cook_l3ebd_directory(invalid_directory); });
    rejects([&] { (void)l3::ebd_resource_span(invalid_directory, 0, 0, 0); });
    invalid_directory = directory; invalid_directory.source_payload.resize(2);
    rejects([&] { (void)l3::cook_l3ebd_directory(invalid_directory); });

    for (size_t length = 0; length < 40; ++length) {
        const std::vector<uint8_t> truncated(cooked.begin(), cooked.begin() + length);
        rejects([&] { (void)l3::load_l3ebd_directory(truncated); });
    }
    for (auto field : {size_t(8), size_t(12), size_t(20), size_t(24), size_t(32), size_t(36)}) {
        bad = cooked; put32(bad, field, UINT32_MAX);
        rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    }
    bad = cooked; bad[4] = 3;
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; bad[6] = 32;
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; bad[40] ^= 1; // Cooked key differs from original.
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; put32(bad, 44, 52); // Cooked offset differs from original.
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; put32(bad, 72, 1); // Original count differs.
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; put32(bad, 80, base + 96); // Original pointer outside payload.
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = cooked; bad.back() ^= 1; // Opaque resource-byte CRC protection.
    rejects([&] { (void)l3::load_l3ebd_directory(bad); });
    assert(l3::load_l3ebd_directory(bad, false).source_payload.back() != source.back());
    bad = v1; put32(bad, 8, 7);
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = v1; bad.push_back(0); put32(bad, 24, uint32_t(bad.size()));
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    bad = v1; put32(bad, 12, UINT32_MAX);
    rejects([&] { (void)l3::load_l3ebd_directory(bad, false); });
    std::cout << "{\"format\":\"ebd-resource-check/1\",\"synthetic_only\":true,\"v1_compatible\":true,\"v2_payload_exact\":true,\"aliases_preserved\":true,\"caller_sized_bounds\":true,\"malformed_rejected\":true,\"resource_semantics\":\"UNKNOWN\"}\n";
}
