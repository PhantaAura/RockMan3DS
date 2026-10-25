#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// PS1 GPU packet layer: the primitive buffer, the PsyQ ordering table and
// GP0 polygon decoding. The port's renderers write original-format packets
// here and a platform translator draws them in ordering-table order, the way
// StarHaven runs N64 display lists through a PICA200 translator.
namespace l3::psxgpu {

// Packets live in one word arena. A packet is addressed by the index of its
// tag word + 1 (0 terminates a chain, like the PsyQ 0xFFFFFF end marker).
// Tag word: (payload length in words << 24) | next packet. Words after the
// payload that the tag does not count are a port-side extra (per-vertex depth
// for the 3DS depth buffer); GP0 never sees them.
class PrimitiveBuffer {
public:
    void clear() noexcept { words_.clear(); }
    // Reserves tag + payload + extra words; returns the packet address.
    uint32_t allocate(uint8_t payload_words, uint8_t extra_words = 0);
    uint32_t& tag(uint32_t packet) { return words_[packet - 1]; }
    uint32_t tag(uint32_t packet) const { return words_[packet - 1]; }
    uint32_t* payload(uint32_t packet) { return words_.data() + packet; }
    const uint32_t* payload(uint32_t packet) const { return words_.data() + packet; }
    static uint8_t length(uint32_t tag) noexcept { return uint8_t(tag >> 24); }
    static uint32_t next(uint32_t tag) noexcept { return tag & 0xFFFFFFu; }
    size_t size_words() const noexcept { return words_.size(); }
private:
    std::vector<uint32_t> words_;
};

// ClearOTagR + AddPrim + DrawOTag(ot + n - 1): bucket n-1 is drawn first, so
// larger z draws farther back; inside a bucket the last packet added draws
// first.
class OrderingTable {
public:
    void clear_r(size_t buckets) { heads_.assign(buckets, 0); }
    size_t size() const noexcept { return heads_.size(); }
    // AddPrim(ot + bucket, packet); a bucket past the end is clamped.
    void add_prim(PrimitiveBuffer& buffer, size_t bucket, uint32_t packet);
    // Links packet right after prev (the 0x80026184 bit-28 chain).
    static void insert_after(PrimitiveBuffer& buffer, uint32_t prev, uint32_t packet);
    template <typename Fn> void draw(const PrimitiveBuffer& buffer, Fn&& fn) const {
        for (size_t b = heads_.size(); b-- > 0;)
            for (uint32_t p = heads_[b]; p; p = PrimitiveBuffer::next(buffer.tag(p)))
                fn(p);
    }
private:
    std::vector<uint32_t> heads_;
};

// GP0 0x20..0x3F polygons. Code bits: 0x10 Gouraud, 0x08 quad, 0x04 textured,
// 0x02 semi-transparent, 0x01 raw texture (no colour modulation).
struct Gp0Polygon {
    uint8_t code = 0;
    uint8_t vertices = 0;
    bool gouraud = false, textured = false, semi_transparent = false, raw_texture = false;
    std::array<int16_t,4> x{}, y{};
    std::array<uint8_t,4> u{}, v{};
    uint16_t clut = 0, tpage = 0;
    std::array<uint32_t,4> rgb{};   // 0xBBGGRR per vertex (flat: all equal)
};
constexpr uint8_t gp0_polygon_words(uint8_t code) noexcept {
    const unsigned n = (code & 0x08u) ? 4u : 3u;
    const unsigned per = 1u + ((code & 0x04u) ? 1u : 0u) + ((code & 0x10u) ? 1u : 0u);
    return uint8_t(n * per + ((code & 0x10u) ? 0u : 1u));
}
// nullopt for a non-polygon code or a short packet.
std::optional<Gp0Polygon> decode_gp0_polygon(const uint32_t* words, size_t count) noexcept;

// POLY_FT4 (code 0x2C..0x2F): 9 payload words.
struct PolyFt4 {
    uint32_t code_rgb = 0x2C808080u;
    std::array<int16_t,4> x{}, y{};
    std::array<uint8_t,4> u{}, v{};
    uint16_t clut = 0, tpage = 0;
};
void write_poly_ft4(uint32_t* payload, const PolyFt4& p) noexcept;
// POLY_FT3 (code 0x24..0x27): 7 payload words; the first three vertices of p.
void write_poly_ft3(uint32_t* payload, const PolyFt4& p) noexcept;

// Positive mode-2 terrain packet of 0x80026158..0x80026218:
//  word 1 = code word: (prefix & 0x02FFFFFF) | 0x2C000000 after the depth cue
//    (rgb), 0x2C010101 for a back face, | 0x1F80007C for a see-through face;
//  uv0 word = uv0 | CLUT << 16 | ramp << 16 (0x1F800148);
//  uv1 word = uv1 | TPAGE << 16 (0x1F80015C).
// material_word is CLUT << 16 | TPAGE.
struct TerrainPacketInput {
    uint32_t prefix = 0, rgb = 0x808080u, material_word = 0, see_through_code = 0;
    bool black = false;
    uint8_t ramp = 0;
    std::array<uint8_t,8> uv{};
    std::array<int16_t,4> sx{}, sy{};
};
PolyFt4 terrain_poly_ft4_80026158(const TerrainPacketInput& in) noexcept;

// Negative-cell packet of 0x80021B14 (0x80021EA0..0x80022100):
//  code word = (prefix & 0x03FFFFFF) | 0x2C000000, or | 0x24000000 (POLY_FT3,
//    vertices 0..2) for prefix bit 29; the depth cue (always applied) replaces
//    the rgb and keeps the code byte, so bit 24 (raw) and bit 25 (semi) stay;
//  uv0 word = uv0 | CLUT << 16 (0x1F8001C0) | ramp << 16 (0x1F800138);
//  uv1 word = uv1 | TPAGE << 16 (0x1F8001C4).
// The bucket is max SZ >> 3 (all four, or three for the triangle).
struct NegativePacket { PolyFt4 poly; bool triangle = false; };
NegativePacket terrain_negative_packet_80021b14(uint32_t prefix, uint32_t rgb, uint32_t material_word,
    uint8_t ramp, const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx,
    const std::array<int16_t,4>& sy) noexcept;
// 0x8002196C: the cell's ramp byte, 0x800C0DC0 (the area record copy made by
// 0x800371A0)[(cell depth / 512) & 31], the division truncating toward zero.
uint8_t terrain_negative_ramp_8002196c(int32_t cell_depth, const std::array<uint8_t,40>& area_record) noexcept;

// GTE AVSZ3 / AVSZ4 with the ZSF3 = 341 / ZSF4 = 256 that InitGeom
// (0x8007A520..0x8007A530) installs: OTZ = average SZ / 4, saturated to 16 bits.
constexpr uint16_t gte_avsz3(uint32_t sz1, uint32_t sz2, uint32_t sz3) noexcept {
    const uint64_t v = (uint64_t(341) * (sz1 + sz2 + sz3)) >> 12;
    return uint16_t(v > 0xFFFFu ? 0xFFFFu : v);
}
constexpr uint16_t gte_avsz4(uint32_t sz0, uint32_t sz1, uint32_t sz2, uint32_t sz3) noexcept {
    const uint64_t v = (uint64_t(256) * (sz0 + sz1 + sz2 + sz3)) >> 12;
    return uint16_t(v > 0xFFFFu ? 0xFFFFu : v);
}

// Mega Man's packets (0x80027BE0 triangles / 0x80027D94 quads, pre-built in the
// mesh and patched each frame):
//  word 1 = the player code word 0x1F8000E4 (0x80027F78: 0x24808080 after DPCS,
//    | 0x02000000 while 0x8005EBD4 is nonzero), | 0x08000000 for a quad;
//  uv0 word: CLUT bits 0..2 replaced by the actor ramp 0x1F8000E0;
//  bucket = AVSZ OTZ >> 1 (the 512-entry player OT 0x8009BF88 is spliced two
//    entries per main bucket by 0x80027F78).
PolyFt4 player_packet_80027be0(uint32_t code_word, bool quad, uint32_t material_word, uint8_t ramp,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept;
// 0x80027F78: 0x800C0DC0[(SZ3/4 >> 7) & 31] (the stored SZ is SZ3 >> 2,
// 0x80028018), dividing toward zero.
uint8_t actor_ramp_80027f78(int32_t sz_quarter, const std::array<uint8_t,40>& area_record) noexcept;

// E0 / effect mesh packets of 0x8002A6E8: code = modulation (work +0x2C &
// 0x02FFFFFF, after DPCS unless flag 0x08) | 0x24 / 0x2C; uv0 word = CLUT |
// uv0 | ramp (0x1F8000E0); uv1 word = TPAGE | uv1. Bucket AVSZ OTZ >> 1;
// NCLIP > 0 unless flag 0x40.
PolyFt4 mesh_packet_8002a6e8(uint32_t code_rgb, bool quad, uint32_t material_word, uint16_t ramp_bits,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept;
// 0x8002A3EC: the E0 ramp bits. Flag 0x08: none; flag 0x80: the CLUT-row
// form ((r >> 2) << 6) + (r & 3); otherwise r = 0x800C0DC0[(SZ/4 >> 7) & 31].
uint16_t e0_ramp_8002a3ec(uint8_t flags, int32_t sz_quarter, const std::array<uint8_t,40>& area_record) noexcept;
// Class-20 actor packets of 0x800289A0 / 0x80029608: code = 0x1F8000E4 |
// 0x24 / 0x2C; uv0 word = CLUT | uv0 | ramp; uv1 word = TPAGE | uv1 | 0x20
// (ABR 1). 0x800289A0 links only buckets below 2048.
PolyFt4 class20_packet_800289a0(uint32_t colour_code, bool quad, uint32_t material_word, uint8_t ramp,
    const std::array<uint8_t,8>& uv, const std::array<int16_t,4>& sx, const std::array<int16_t,4>& sy) noexcept;
// 0x80028540..0x8002857C: work +0x15E (15-bit BGR, bit 15 = semi) to the
// 0x1F8000E4 colour word (before DPCS).
constexpr uint32_t class20_colour_8002831c(uint16_t c) noexcept {
    return (uint32_t(c & 0x1Fu) << 3) | (uint32_t(c & 0x3E0u) << 6) | (uint32_t(c & 0x7C00u) << 9) |
           (uint32_t(c & 0x8000u) << 10);
}

// Bucket of 0x800261A4: otz >> 3 (OT base 0x1F80003C). A back face uses its
// max SZ (0x80025EBC), a drawn face the 0x80025900 otz.
constexpr size_t terrain_ot_bucket(int32_t otz) noexcept { return otz <= 0 ? 0u : size_t(otz) >> 3; }

// 0x8002602C: a face goes to the subdivider 0x8002466C instead of one packet
// when |MAC0| >= 2049, or when |MAC0| is 513..2048 and the quad is not a near
// parallelogram ((x0 + x3 - x1 - x2) / 2 or the same in y outside -3..3).
bool terrain_needs_subdivision_8002602c(int64_t mac0, const std::array<int16_t,4>& sx,
    const std::array<int16_t,4>& sy) noexcept;

} // namespace l3::psxgpu
