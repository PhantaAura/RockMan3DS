#include "legends3ds/mml_message.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <stdexcept>
#include <string>

namespace l3 {
namespace {
bool is_text_byte(uint8_t b) {
    return b < 0x84u || b == 0x86u;
}

MmlMessageToken make_token(MmlMessageTokenKind kind, uint8_t opcode,
                           const uint8_t* p = nullptr, size_t n = 0) {
    MmlMessageToken out;
    out.kind = kind;
    out.opcode = opcode;
    if (p && n) out.args.assign(p, p + n);
    return out;
}
} // namespace

int mml_message_fixed_arg_bytes(uint8_t op) {
    switch (op) {
        case 0x84: return 2; // CLOSE
        case 0x87: return 2; // CLEAR
        case 0x89: return 1; // COLOR
        case 0x8A: return 2;
        case 0x8B: return 2; // PAUSE
        case 0x8C: return 6; // WIN
        case 0x8D: return 1;
        case 0x8E: return 2;
        case 0x8F: return 4; // AUDIO
        case 0x93: return 2; // DECOR
        case 0x94: return 0; // MOVE_FREE
        case 0x96: return 2; // SEL
        case 0x97: return -1; // MULTI: one type byte + one byte per prior SEL in block
        case 0x99: return 2; // CALL
        case 0x9A: return 2; // GET
        case 0x9B: return 2; // GIVE
        case 0x9C: return 4; // MSG_IF
        case 0x9F: return 0; // NEXT
        case 0xA0: return 2; // PAD
        case 0xA1: return 0; // NO_CHAR_SFX
        case 0xA2: return 0; // CHAR_SFX
        case 0xA4: return 2; // WAIT
        case 0xA5: return 10; // CAM
        case 0xA6: return 2;
        case 0xA7: return 8;
        case 0xA9: return 0; // END
        case 0xAC: return 2; // EXE_JUMP
        case 0xAD: return 6;
        case 0xAE: return 2; // OPTION
        case 0xAF: return 1;
        case 0xB2: return 0;
        case 0xB3: return 0;
        case 0xB4: return 0;
        case 0xB9: return 3; // IF
        case 0xBA: return 0; // COST
        case 0xBF: return 2; // JUMP
        case 0xC8: return 0; // ZENNY_AMOUNT
        case 0xD0: return 1; // ITEM
        case 0xD1: return 2;
        case 0xD3: return 0; // PRICE_ZENNY
        case 0xD4: return 1;
        case 0xD6: return 2;
        case 0xD7: return 4;
        case 0xDA: return 2;
        case 0xDB: return 4;
        case 0xDD: return 4;
        case 0xE1: return 2;
        case 0xE3: return 2;
        default: return -1;
    }
}

MmlMessageFile parse_mml_message_payload(const std::vector<uint8_t>& b) {
    if (b.size() < 2) throw std::runtime_error("MSG payload too small");
    const uint16_t table_bytes = rd_u16(b.data());
    if (table_bytes < 2 || (table_bytes & 1u) != 0 || table_bytes > b.size()) {
        throw std::runtime_error("invalid MSG pointer-table size");
    }
    const size_t count = table_bytes / 2u;
    MmlMessageFile out;
    out.pointer_table_bytes = table_bytes;
    out.block_offsets.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const uint16_t off = rd_u16(b.data() + i * 2u);
        if (off < table_bytes || off > b.size()) throw std::runtime_error("MSG block offset outside payload");
        if (i && off < out.block_offsets.back()) throw std::runtime_error("MSG block offsets are not sorted");
        out.block_offsets.push_back(off);
    }
    if (out.block_offsets.empty() || out.block_offsets.front() != table_bytes) {
        throw std::runtime_error("MSG first block does not begin after pointer table");
    }

    out.blocks.reserve(count);
    for (size_t block_index = 0; block_index < count; ++block_index) {
        const size_t begin = out.block_offsets[block_index];
        const size_t end = block_index + 1u < count ? out.block_offsets[block_index + 1u] : b.size();
        if (end < begin || end > b.size()) throw std::runtime_error("MSG block range invalid");
        if (end - begin < 2u) throw std::runtime_error("MSG block missing two-byte terminator");
        // All tutorial blocks observed so far end in 00 00. Treat it as framing, not text.
        if (b[end - 2u] != 0 || b[end - 1u] != 0) {
            throw std::runtime_error("MSG block terminator is not 00 00");
        }

        MmlMessageBlock block;
        block.source_offset = static_cast<uint16_t>(begin);
        block.source_end = static_cast<uint16_t>(end);
        size_t i = begin;
        unsigned selection_count = 0;
        const size_t token_end = end - 2u;
        while (i < token_end) {
            const uint8_t op = b[i];
            if (is_text_byte(op)) {
                block.tokens.push_back(make_token(op == 0x86u ? MmlMessageTokenKind::Newline : MmlMessageTokenKind::Glyph, op));
                ++i;
                continue;
            }

            if (op == 0x97u) {
                const size_t args = 1u + selection_count;
                if (args > token_end - (i + 1u)) throw std::runtime_error("truncated MSG MULTI command");
                block.tokens.push_back(make_token(MmlMessageTokenKind::Control, op, b.data() + i + 1u, args));
                i += 1u + args;
                continue;
            }

            const int fixed = mml_message_fixed_arg_bytes(op);
            if (fixed >= 0) {
                const size_t args = static_cast<size_t>(fixed);
                if (args > token_end - (i + 1u)) throw std::runtime_error("truncated MSG control command");
                block.tokens.push_back(make_token(MmlMessageTokenKind::Control, op, b.data() + i + 1u, args));
                if (op == 0x96u) ++selection_count;
                i += 1u + args;
                continue;
            }

            // Preserve unknown high bytes exactly instead of guessing an argument width. This is important
            // for commands/glyph extensions not yet named by community tools.
            block.tokens.push_back(make_token(MmlMessageTokenKind::UnknownControl, op));
            ++i;
        }
        out.blocks.push_back(std::move(block));
    }
    return out;
}

MmlMessageStats inspect_mml_message(const MmlMessageFile& msg) {
    MmlMessageStats s;
    s.blocks = static_cast<uint32_t>(msg.blocks.size());
    for (const auto& b : msg.blocks) {
        if (b.tokens.empty()) ++s.empty_blocks;
        for (const auto& t : b.tokens) {
            switch (t.kind) {
                case MmlMessageTokenKind::Glyph: ++s.glyph_bytes; break;
                case MmlMessageTokenKind::Newline: ++s.newline_bytes; break;
                case MmlMessageTokenKind::UnknownControl: ++s.unknown_control_bytes; break;
                case MmlMessageTokenKind::Control:
                    ++s.control_commands;
                    if (t.opcode == 0x8Fu) ++s.audio_commands;
                    else if (t.opcode == 0xA4u) ++s.wait_commands;
                    else if (t.opcode == 0x9Fu) ++s.next_commands;
                    else if (t.opcode == 0x96u) ++s.selection_commands;
                    else if (t.opcode == 0x97u) ++s.multi_commands;
                    else if (t.opcode == 0xA5u) ++s.camera_commands;
                    break;
            }
        }
    }
    return s;
}


std::vector<uint8_t> cook_l3message(const MmlMessageFile& msg) {
    uint32_t token_count = 0, arg_bytes = 0;
    for (const auto& block : msg.blocks) {
        token_count += static_cast<uint32_t>(block.tokens.size());
        for (const auto& token : block.tokens) arg_bytes += static_cast<uint32_t>(token.args.size());
    }
    std::vector<uint8_t> o;
    o.reserve(48u + msg.blocks.size()*12u + size_t(token_count)*8u + arg_bytes);
    o.insert(o.end(), {'L','3','M','S'}); wr_u16(o,kL3MessageVersion); wr_u16(o,48); wr_u32(o,1u);
    wr_u32(o,static_cast<uint32_t>(msg.blocks.size())); wr_u32(o,token_count); wr_u32(o,arg_bytes);
    const size_t p_blocks=o.size(); wr_u32(o,0); const size_t p_tokens=o.size(); wr_u32(o,0);
    const size_t p_args=o.size(); wr_u32(o,0); const size_t p_file=o.size(); wr_u32(o,0);
    const size_t p_crc=o.size(); wr_u32(o,0); wr_u32(o,0);
    if(o.size()!=48) throw std::runtime_error("L3MS header size bug");

    const uint32_t off_blocks=static_cast<uint32_t>(o.size());
    uint32_t token_cursor=0;
    for(const auto& block:msg.blocks){
        wr_u16(o,block.source_offset); wr_u16(o,block.source_end);
        wr_u32(o,token_cursor); wr_u32(o,static_cast<uint32_t>(block.tokens.size()));
        token_cursor += static_cast<uint32_t>(block.tokens.size());
    }
    const uint32_t off_tokens=static_cast<uint32_t>(o.size());
    uint32_t arg_cursor=0;
    for(const auto& block:msg.blocks) for(const auto& token:block.tokens){
        o.push_back(static_cast<uint8_t>(token.kind)); o.push_back(token.opcode);
        if(token.args.size()>0xFFFFu) throw std::runtime_error("L3MS token argument list too large");
        wr_u16(o,static_cast<uint16_t>(token.args.size())); wr_u32(o,arg_cursor);
        arg_cursor += static_cast<uint32_t>(token.args.size());
    }
    const uint32_t off_args=static_cast<uint32_t>(o.size());
    for(const auto& block:msg.blocks) for(const auto& token:block.tokens) o.insert(o.end(),token.args.begin(),token.args.end());
    patch_u32(o,p_blocks,off_blocks); patch_u32(o,p_tokens,off_tokens); patch_u32(o,p_args,off_args);
    patch_u32(o,p_file,static_cast<uint32_t>(o.size())); patch_u32(o,p_crc,crc32(o.data()+48,o.size()-48));
    return o;
}

L3MessageHeader inspect_l3message(const std::vector<uint8_t>& b, bool verify_crc) {
    if(b.size()<48||b[0]!='L'||b[1]!='3'||b[2]!='M'||b[3]!='S') throw std::runtime_error("not L3MS");
    L3MessageHeader h; h.version=rd_u16(b.data()+4); h.header_bytes=rd_u16(b.data()+6); h.flags=rd_u32(b.data()+8);
    h.block_count=rd_u32(b.data()+12); h.token_count=rd_u32(b.data()+16); h.argument_bytes=rd_u32(b.data()+20);
    h.off_blocks=rd_u32(b.data()+24); h.off_tokens=rd_u32(b.data()+28); h.off_arguments=rd_u32(b.data()+32);
    h.file_bytes=rd_u32(b.data()+36); h.payload_crc32=rd_u32(b.data()+40);
    if(h.version!=kL3MessageVersion||h.header_bytes!=48) throw std::runtime_error("unsupported L3MS");
    if(h.file_bytes!=b.size()) throw std::runtime_error("L3MS file size mismatch");
    const uint64_t block_end=uint64_t(h.off_blocks)+uint64_t(h.block_count)*12u;
    const uint64_t token_end=uint64_t(h.off_tokens)+uint64_t(h.token_count)*8u;
    const uint64_t arg_end=uint64_t(h.off_arguments)+h.argument_bytes;
    if(h.off_blocks<48||block_end>h.off_tokens||h.off_tokens<block_end||token_end>h.off_arguments||h.off_arguments<token_end||arg_end!=h.file_bytes)
        throw std::runtime_error("L3MS section range mismatch");
    if(verify_crc&&crc32(b.data()+48,b.size()-48)!=h.payload_crc32) throw std::runtime_error("L3MS CRC mismatch");
    for(uint32_t i=0;i<h.block_count;++i){
        const size_t q=size_t(h.off_blocks)+size_t(i)*12u;
        const uint32_t ts=rd_u32(b.data()+q+4), tc=rd_u32(b.data()+q+8);
        if(uint64_t(ts)+tc>h.token_count) throw std::runtime_error("L3MS block token range mismatch");
    }
    for(uint32_t i=0;i<h.token_count;++i){
        const size_t q=size_t(h.off_tokens)+size_t(i)*8u;
        const uint16_t argc=rd_u16(b.data()+q+2); const uint32_t ao=rd_u32(b.data()+q+4);
        if(uint64_t(ao)+argc>h.argument_bytes) throw std::runtime_error("L3MS token argument range mismatch");
    }
    return h;
}


MmlMessageFile load_l3message(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3message(b, verify_crc);
    MmlMessageFile out;
    out.blocks.reserve(h.block_count);
    out.block_offsets.reserve(h.block_count);
    if (h.block_count) out.pointer_table_bytes = rd_u16(b.data() + h.off_blocks);

    for (uint32_t bi = 0; bi < h.block_count; ++bi) {
        const size_t bp = size_t(h.off_blocks) + size_t(bi) * 12u;
        MmlMessageBlock block;
        block.source_offset = rd_u16(b.data() + bp + 0);
        block.source_end = rd_u16(b.data() + bp + 2);
        const uint32_t token_first = rd_u32(b.data() + bp + 4);
        const uint32_t token_count = rd_u32(b.data() + bp + 8);
        out.block_offsets.push_back(block.source_offset);
        block.tokens.reserve(token_count);
        for (uint32_t ti = 0; ti < token_count; ++ti) {
            const size_t tp = size_t(h.off_tokens) + size_t(token_first + ti) * 8u;
            const uint8_t raw_kind = b[tp + 0];
            if (raw_kind > static_cast<uint8_t>(MmlMessageTokenKind::UnknownControl)) {
                throw std::runtime_error("L3MS token kind invalid");
            }
            MmlMessageToken t;
            t.kind = static_cast<MmlMessageTokenKind>(raw_kind);
            t.opcode = b[tp + 1];
            const uint16_t argc = rd_u16(b.data() + tp + 2);
            const uint32_t arg_off = rd_u32(b.data() + tp + 4);
            const size_t ap = size_t(h.off_arguments) + size_t(arg_off);
            t.args.assign(b.begin() + static_cast<std::ptrdiff_t>(ap),
                          b.begin() + static_cast<std::ptrdiff_t>(ap + argc));
            block.tokens.push_back(std::move(t));
        }
        out.blocks.push_back(std::move(block));
    }
    return out;
}

namespace {
MmlRuntimeEventKind runtime_kind_for_token(const MmlMessageToken& t) {
    if (t.kind == MmlMessageTokenKind::Glyph) return MmlRuntimeEventKind::Glyph;
    if (t.kind == MmlMessageTokenKind::Newline) return MmlRuntimeEventKind::Newline;
    if (t.kind == MmlMessageTokenKind::UnknownControl) return MmlRuntimeEventKind::UnknownControl;
    switch (t.opcode) {
        case 0x8F: return MmlRuntimeEventKind::Audio;
        case 0xA4: return MmlRuntimeEventKind::Wait;
        case 0x9F: return MmlRuntimeEventKind::Next;
        case 0xA9: return MmlRuntimeEventKind::End;
        case 0x96: return MmlRuntimeEventKind::Selection;
        case 0x97: return MmlRuntimeEventKind::Multi;
        default: return MmlRuntimeEventKind::Control;
    }
}
}

MmlMessageRuntimeCursor::MmlMessageRuntimeCursor(const MmlMessageFile& message)
    : message_(&message) {}

void MmlMessageRuntimeCursor::start_block(size_t block_index) {
    if (!message_ || block_index >= message_->blocks.size()) {
        throw std::runtime_error("message runtime block index out of range");
    }
    block_index_ = block_index;
    token_index_ = 0;
    block_started_ = true;
    emitted_block_end_ = false;
}

void MmlMessageRuntimeCursor::seek(size_t block_index, size_t token_index) {
    start_block(block_index);
    if (token_index > message_->blocks[block_index].tokens.size())
        throw std::runtime_error("message runtime token index out of range");
    token_index_ = token_index;
}

char mml_glyph_ascii(uint8_t g) {
    if (g >= 0x15 && g <= 0x28) return char('A' + (g - 0x15));
    if (g >= 0x2A && g <= 0x2F) return char('U' + (g - 0x2A));
    if (g >= 0x30 && g <= 0x3D) return char('a' + (g - 0x30));
    if (g >= 0x3F && g <= 0x4A) return char('o' + (g - 0x3F));
    switch (g) {
    case 0x4F: return ' '; case 0x0D: return '!'; case 0x0E: return '?';
    case 0x52: return '-'; case 0x5E: return '.';
    case 0x0C: return '\''; case 0x5C: return ',';
    default: return '\0';
    }
}

const char* mml_glyph_icon(uint8_t g) {
    // The port's 3DS buttons for the PS1 icon: Square=Y, R2=X, Circle=A, Cross=B.
    switch (g) { case 0x57: return "Y"; case 0x5B: return "X"; case 0x54: return "A"; case 0x56: return "B"; default: return nullptr; }
}

bool MmlMessageRuntimeCursor::at_block_end() const {
    if (!block_started_ || !message_ || block_index_ >= message_->blocks.size()) return false;
    return token_index_ >= message_->blocks[block_index_].tokens.size();
}

bool MmlMessageRuntimeCursor::next(MmlRuntimeEvent& event) {
    if (!block_started_ || !message_) throw std::runtime_error("message runtime block not started");
    const auto& block = message_->blocks[block_index_];
    if (token_index_ < block.tokens.size()) {
        const auto& t = block.tokens[token_index_++];
        event.kind = runtime_kind_for_token(t);
        event.opcode = t.opcode;
        event.args = t.args;
        return true;
    }
    if (!emitted_block_end_) {
        emitted_block_end_ = true;
        event.kind = MmlRuntimeEventKind::BlockEnd;
        event.opcode = 0;
        event.args.clear();
        return true;
    }
    return false;
}

} // namespace l3
