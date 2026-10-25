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

// Bucket of 0x800261A4: otz >> 3 (OT base 0x1F80003C). A back face uses its
// max SZ (0x80025EBC), a drawn face the 0x80025900 otz.
constexpr size_t terrain_ot_bucket(int32_t otz) noexcept { return otz <= 0 ? 0u : size_t(otz) >> 3; }

// 0x8002602C: a face goes to the subdivider 0x8002466C instead of one packet
// when |MAC0| >= 2049, or when |MAC0| is 513..2048 and the quad is not a near
// parallelogram ((x0 + x3 - x1 - x2) / 2 or the same in y outside -3..3).
bool terrain_needs_subdivision_8002602c(int64_t mac0, const std::array<int16_t,4>& sx,
    const std::array<int16_t,4>& sy) noexcept;

} // namespace l3::psxgpu
