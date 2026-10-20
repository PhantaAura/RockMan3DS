#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace l3 {

constexpr uint16_t kL3MessageVersion = 1;

enum class MmlMessageTokenKind : uint8_t {
    Glyph,
    Newline,
    Control,
    UnknownControl,
};

struct MmlMessageToken {
    MmlMessageTokenKind kind = MmlMessageTokenKind::Glyph;
    uint8_t opcode = 0;
    std::vector<uint8_t> args;
};

struct MmlMessageBlock {
    uint16_t source_offset = 0;
    uint16_t source_end = 0;
    std::vector<MmlMessageToken> tokens;
};

struct MmlMessageFile {
    uint16_t pointer_table_bytes = 0;
    std::vector<uint16_t> block_offsets;
    std::vector<MmlMessageBlock> blocks;
};


struct L3MessageHeader {
    uint16_t version = 0;
    uint16_t header_bytes = 0;
    uint32_t flags = 0;
    uint32_t block_count = 0;
    uint32_t token_count = 0;
    uint32_t argument_bytes = 0;
    uint32_t off_blocks = 0;
    uint32_t off_tokens = 0;
    uint32_t off_arguments = 0;
    uint32_t file_bytes = 0;
    uint32_t payload_crc32 = 0;
};

struct MmlMessageStats {
    uint32_t blocks = 0;
    uint32_t empty_blocks = 0;
    uint32_t glyph_bytes = 0;
    uint32_t newline_bytes = 0;
    uint32_t control_commands = 0;
    uint32_t unknown_control_bytes = 0;
    uint32_t audio_commands = 0;
    uint32_t wait_commands = 0;
    uint32_t next_commands = 0;
    uint32_t selection_commands = 0;
    uint32_t multi_commands = 0;
    uint32_t camera_commands = 0;
};

// Parses the raw MSG payload after the MML DAT resource wrapper has been removed.
// The first u16 is both the pointer-table byte size and the first block offset.
// Block terminators are retained by the source offsets but are not emitted as tokens.
MmlMessageFile parse_mml_message_payload(const std::vector<uint8_t>& bytes);
MmlMessageStats inspect_mml_message(const MmlMessageFile& msg);

// Pointer-free runtime representation. It preserves token kinds/opcodes/arguments and source block ranges
// without copying any game data into this source package. Cooked files are generated locally from user data.
std::vector<uint8_t> cook_l3message(const MmlMessageFile& msg);
L3MessageHeader inspect_l3message(const std::vector<uint8_t>& bytes, bool verify_crc = true);

MmlMessageFile load_l3message(const std::vector<uint8_t>& bytes,
                              bool verify_crc = true);

enum class MmlRuntimeEventKind : uint8_t {
    Glyph,
    Newline,
    Audio,
    Wait,
    Next,
    End,
    Selection,
    Multi,
    Control,
    UnknownControl,
    BlockEnd,
};

struct MmlRuntimeEvent {
    MmlRuntimeEventKind kind = MmlRuntimeEventKind::BlockEnd;
    uint8_t opcode = 0;
    std::vector<uint8_t> args;
};

// Portable token cursor for the cooked message banks. It deliberately does not
// guess timing units or branch semantics for controls that are not yet translated;
// those controls are surfaced verbatim to the stage/event runtime.
class MmlMessageRuntimeCursor {
public:
    explicit MmlMessageRuntimeCursor(const MmlMessageFile& message);
    void start_block(size_t block_index);
    // Resume inside a block (a returning CALL). Throws when out of range.
    void seek(size_t block_index, size_t token_index);
    bool next(MmlRuntimeEvent& event);
    size_t block_index() const { return block_index_; }
    size_t token_index() const { return token_index_; }
    bool at_block_end() const;
private:
    const MmlMessageFile* message_ = nullptr;
    size_t block_index_ = 0;
    size_t token_index_ = 0;
    bool block_started_ = false;
    bool emitted_block_end_ = false;
};

// USA glyph byte to printable ASCII for frontend presentation, measured from
// st00_01 against the original screens and words: A..T 0x15..0x28, U..Z
// 0x2A..0x2F, a..n 0x30..0x3D, o..z 0x3F..0x4A, space 0x4F, '!' 0x0D, '?'
// 0x0E, '\'' 0x0C, ',' 0x5C, '-' 0x52, '.' 0x5E. Button icons (0x54, 0x57
// square, 0x5B R2) and other bytes return '\0'; see mml_glyph_icon.
char mml_glyph_ascii(uint8_t glyph);
// PS1 button icons as the port's 3DS buttons: 0x57 Square -> "Y" and 0x5B
// R2 -> "X" (icons seen on the original screen), 0x56 Cross (jump) -> "B",
// 0x54 Circle (investigate/doors) -> "A" (CANDIDATE icon identities).
const char* mml_glyph_icon(uint8_t glyph);

// Argument count for a known fixed-width command, or -1 for either text bytes,
// dynamic commands (MULTI), or currently unknown control bytes.
int mml_message_fixed_arg_bytes(uint8_t opcode);

} // namespace l3
