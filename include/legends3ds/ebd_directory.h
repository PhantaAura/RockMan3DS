#pragma once
#include "legends3ds/stage_spawn.h"
#include "legends3ds/shared_byte_payload.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr size_t kEbdDirectoryEntryBytes = 16;
constexpr uint16_t kL3EbdDirectoryVersion = 2;

struct L3EbdDirectoryHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t entry_count = 0;
    uint32_t source_load_base = 0;
    uint32_t off_entries = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
    uint32_t off_source_payload = 0; // v2 only; zero for v1.
    uint32_t source_payload_bytes = 0;
};

// The common EBD lookup routine (USA ROCK_NEO.EXE 0x8002FA38) reads a u32 entry
// count at the EBD load base, then walks count 16-byte directory records at base+4.
// Bytes 0..2 are compared against normalized object class, object subtype and an
// optional variant byte. The remaining 12 bytes are three resource pointers whose
// use depends on object class. Their offsets are retained without assigning
// resource types or guessing resource sizes from adjacent pointers.
struct EbdDirectoryEntry {
    uint8_t object_class = 0;
    uint8_t subtype = 0;
    uint8_t variant = 0;
    uint8_t key_flags = 0;
    uint32_t resource_offset[3]{}; // UINT32_MAX represents a source null pointer.
};

struct EbdDirectory {
    uint32_t load_base = 0;
    std::vector<EbdDirectoryEntry> entries;
    // Complete owner-local EBD bytes, unchanged, including the source directory.
    // Empty for legacy v1 offset-only cooked files. No resource semantics implied.
    SharedBytePayload source_payload;
};

enum class EbdResourceSpanStatus { AVAILABLE, NULL_POINTER, PAYLOAD_UNAVAILABLE };
struct EbdResourceSpan {
    EbdResourceSpanStatus status = EbdResourceSpanStatus::PAYLOAD_UNAVAILABLE;
    const uint8_t* data = nullptr;
    size_t size = 0;
    uint32_t source_offset = UINT32_MAX;
};
// Borrowed caller-sized window at an exact source pointer. No next-pointer size
// inference. Throws on invalid row/slot or available-payload bounds; null and
// legacy payload absence are distinct results. Lifetime ends on payload mutation,
// move or directory destruction. The caller must establish the requested size.
EbdResourceSpan ebd_resource_span(const EbdDirectory& dir, size_t entry_index,
                                  size_t resource_slot, size_t requested_bytes);

// Reproduces the class normalization at 0x8002FE74.
uint8_t normalize_ebd_object_class(uint8_t object_class);

EbdDirectory parse_ebd_directory(const uint8_t* data, size_t bytes, uint32_t load_base);
EbdDirectory parse_ebd_directory(const std::vector<uint8_t>& data, uint32_t load_base);

// Pointer-free cooked directory used by the native runtime. This intentionally
// v2 preserves complete source bytes when present; v1 remains offset-only.
// Owner-derived cooked files belong in owner-local installs, not source ZIPs.
std::vector<uint8_t> cook_l3ebd_directory(const EbdDirectory& dir);
L3EbdDirectoryHeader inspect_l3ebd_directory(const std::vector<uint8_t>& bytes,
                                             bool verify_crc = true);
EbdDirectory load_l3ebd_directory(const std::vector<uint8_t>& bytes,
                                  bool verify_crc = true);

std::vector<size_t> find_ebd_entries(const EbdDirectory& dir,
                                     uint8_t object_class,
                                     uint8_t subtype);
std::vector<size_t> find_ebd_entries(const EbdDirectory& dir,
                                     uint8_t object_class,
                                     uint8_t subtype,
                                     uint8_t variant);

// Stage setup stores the low byte of the +0x02 halfword as the object subtype.
// The high byte is not blindly treated as the EBD variant because original code
// can derive/pass a separate variant byte. This helper therefore returns the exact
// class+subtype candidate set; a single result is safe to call an unambiguous link.
std::vector<size_t> stage_object_ebd_candidates(const StageObjectRecord& object,
                                                const EbdDirectory& dir);

// Variant-aware form using raw stage byte +3. The common object parser copies bytes +2/+3
// together into runtime work +2/+3, and multiple original initializers explicitly pass
// work+3 as the optional variant selector to the EBD lookup. Not every initializer does so,
// therefore this helper is opt-in rather than the default candidate policy.
std::vector<size_t> stage_object_ebd_candidates_record_variant(const StageObjectRecord& object,
                                                               const EbdDirectory& dir);

} // namespace l3
