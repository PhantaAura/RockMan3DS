#include "legends3ds/st00_source.h"
#include "legends3ds/endian_io.h"
#include <fstream>
#include <map>
#include <stdexcept>

namespace l3 {
std::vector<uint8_t> read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open: " + path.string());
    f.seekg(0, std::ios::end);
    auto n = f.tellg();
    if (n < 0) throw std::runtime_error("tellg failed");
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> b(static_cast<size_t>(n));
    if (!b.empty()) f.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!f && !b.empty()) throw std::runtime_error("read failed: " + path.string());
    return b;
}

static bool in_stg(uint32_t ptr, size_t bytes, size_t need = 1) {
    return ptr >= kSt00StgBase && uint64_t(ptr) + need <= uint64_t(kSt00StgBase) + bytes;
}

static SourceContactList parse_contact(const std::vector<uint8_t>& stg, uint32_t ptr) {
    SourceContactList out;
    out.source_ptr = ptr;
    if (ptr == 0) return out;

    uint32_t pos = ptr;
    for (size_t guard = 0; guard < 1024; ++guard, pos += 16) {
        if (!in_stg(pos, stg.size(), 16)) throw std::runtime_error("contact group out of STG bounds");
        size_t o = pos - kSt00StgBase;
        int16_t x0 = rd_s16(&stg[o]);
        int16_t x1 = rd_s16(&stg[o + 2]);
        int16_t z0 = rd_s16(&stg[o + 4]);
        int16_t z1 = rd_s16(&stg[o + 6]);
        uint32_t pp = rd_u32(&stg[o + 8]);
        uint32_t count = rd_u32(&stg[o + 12]);
        if (pp == 0) return out;
        if (count > 100000 || !in_stg(pp, stg.size(), size_t(count) * 8)) {
            throw std::runtime_error("contact primitive range invalid");
        }

        SourceContactGroup g;
        g.x0 = x0; g.x1 = x1; g.z0 = z0; g.z1 = z1;
        g.primitives.reserve(count);
        size_t po = pp - kSt00StgBase;
        for (uint32_t i = 0; i < count; ++i) {
            size_t q = po + size_t(i) * 8;
            g.primitives.push_back({rd_s16(&stg[q]), rd_s16(&stg[q + 2]), rd_u16(&stg[q + 4]), rd_u16(&stg[q + 6])});
        }
        out.groups.push_back(std::move(g));
    }
    throw std::runtime_error("unterminated contact list");
}

static SourceDescriptor read_descriptor(const std::vector<uint8_t>& stg, uint32_t ptr) {
    if (!in_stg(ptr, stg.size(), 48)) throw std::runtime_error("descriptor pointer invalid");
    size_t q = ptr - kSt00StgBase;
    SourceDescriptor d;
    d.source_ptr = ptr;
    d.word0 = rd_u32(&stg[q]);
    d.alt_contact_ptr = rd_u32(&stg[q + 4]);
    d.default_contact_ptr = rd_u32(&stg[q + 8]);
    for (int j = 0; j < 6; ++j) d.raw_words3_5[j] = rd_s16(&stg[q + 12 + j * 2]);
    for (int j = 0; j < 6; ++j) d.render_ptrs[j] = rd_u32(&stg[q + 24 + j * 4]);
    return d;
}

St00Source parse_st00(const std::vector<uint8_t>& mdt, const std::vector<uint8_t>& idx,
                      const std::vector<uint8_t>& stg, const std::vector<uint8_t>& hed,
                      const std::vector<uint8_t>& hd2) {
    St00Source out;
    if (mdt.size() < 4) throw std::runtime_error("MDT too small");
    uint32_t ac = rd_u32(mdt.data());
    if (ac > 256 || 4 + size_t(ac) * 8 > mdt.size()) throw std::runtime_error("bad MDT area table");

    for (uint32_t i = 0; i < ac; ++i) {
        size_t o = 4 + size_t(i) * 8;
        SourceArea a;
        a.start_x = mdt[o]; a.start_z = mdt[o + 1]; a.width = mdt[o + 2]; a.height = mdt[o + 3];
        uint32_t ptr = rd_u32(&mdt[o + 4]);
        if (ptr < kSt00MdtBase) throw std::runtime_error("MDT grid pointer below base");
        size_t go = ptr - kSt00MdtBase;
        size_t n = size_t(a.width) * a.height;
        if (go + n * 2 > mdt.size()) throw std::runtime_error("MDT grid out of range");
        a.cells.reserve(n);
        for (size_t c = 0; c < n; ++c) a.cells.push_back(rd_s16(&mdt[go + c * 2]));
        out.areas.push_back(std::move(a));
    }

    if (idx.size() % 12) throw std::runtime_error("IDX size not divisible by 12");
    out.idx.reserve(idx.size() / 12);
    for (size_t o = 0; o < idx.size(); o += 12) {
        out.idx.push_back({rd_s16(&idx[o]), rd_s16(&idx[o + 2]), rd_s16(&idx[o + 4]), rd_s16(&idx[o + 6]), rd_u32(&idx[o + 8])});
    }

    std::map<uint32_t, SourceDescriptor> by_ptr;
    if (hed.size() % 8) throw std::runtime_error("HED size not divisible by 8");
    for (size_t o = 0; o < hed.size(); o += 8) {
        uint32_t tag = rd_u32(&hed[o]);
        uint32_t ptr = rd_u32(&hed[o + 4]);
        if (tag == 0 && ptr == 0) continue;
        if (!in_stg(ptr, stg.size(), 48)) continue;
        by_ptr.emplace(ptr, read_descriptor(stg, ptr));
    }
    for (const auto& r : out.idx) {
        if (r.descriptor_ptr && !by_ptr.count(r.descriptor_ptr)) by_ptr.emplace(r.descriptor_ptr, read_descriptor(stg, r.descriptor_ptr));
    }
    // ROCK_NEO.EXE 0x800219C0..0x800219E4 and 0x80022274..0x80022298
    // select the loaded STG base for negative cells whose bit7 is clear. It is
    // not reached through a HED or IDX reference, so collect it explicitly.
    // Bit7-set pointer-table bindings remain unresolved and are not guessed.
    bool needs_default_negative_descriptor = false;
    for (const auto& area : out.areas) {
        for (int16_t cell : area.cells) {
            if (cell < 0 && (static_cast<uint16_t>(cell) & 0x80u) == 0u) {
                needs_default_negative_descriptor = true;
                break;
            }
        }
        if (needs_default_negative_descriptor) break;
    }
    if (needs_default_negative_descriptor && !by_ptr.count(kSt00StgBase))
        by_ptr.emplace(kSt00StgBase, read_descriptor(stg, kSt00StgBase));
    // Supplied type0 HD2 resources load at0x80139F00. The source draw reads
    // row+4 at0x80139F04+((packed<<1)&F8), i.e.8-byte row selector bits2..6.
    // Only actually referenced rows are admitted. Absent HD2/null pointers
    // remain unknown; a provided truncated or invalid pointer range is rejected.
    if (!hd2.empty()) {
        if (hd2.size() % 8) throw std::runtime_error("HD2 size not divisible by 8");
        std::array<bool, 32> used{};
        for (const auto& area : out.areas) {
            for (int16_t cell : area.cells) {
                const uint16_t packed = static_cast<uint16_t>(cell);
                if (cell < 0 && (packed & 0x80u) != 0u) used[(packed >> 2) & 31u] = true;
            }
        }
        for (size_t selector = 0; selector < used.size(); ++selector) {
            if (!used[selector]) continue;
            const size_t row = selector * 8u;
            if (row + 8u > hd2.size()) throw std::runtime_error("referenced HD2 row outside supplied table");
            const uint32_t ptr = rd_u32(&hd2[row + 4u]);
            if (!ptr) continue;
            if (!by_ptr.count(ptr)) by_ptr.emplace(ptr, read_descriptor(stg, ptr));
            out.special_negative_descriptor_ptrs[selector] = ptr;
        }
    }
    for (const auto& kv : by_ptr) out.descriptors.push_back(kv.second);

    std::map<uint32_t, bool> contact_ptrs;
    for (const auto& d : out.descriptors) {
        if (d.alt_contact_ptr) contact_ptrs[d.alt_contact_ptr] = true;
        if (d.default_contact_ptr) contact_ptrs[d.default_contact_ptr] = true;
    }
    for (const auto& kv : contact_ptrs) out.contact_lists.push_back(parse_contact(stg, kv.first));
    return out;
}
}
