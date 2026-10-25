// PS1 GPU packet layer: PsyQ ordering-table order, GP0 polygon decoding and
// the positive mode-2 terrain packet (0x80026158) and subdivision test
// (0x8002602C). Values are authored.
#include "legends3ds/render/psx_gpu.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace l3::psxgpu;

int main() {
    // Packet sizes from the GP0 code.
    assert(gp0_polygon_words(0x20) == 4 && gp0_polygon_words(0x24) == 7 && gp0_polygon_words(0x28) == 5);
    assert(gp0_polygon_words(0x2C) == 9 && gp0_polygon_words(0x30) == 6 && gp0_polygon_words(0x3C) == 12);

    // DrawOTag(ot + n - 1): far buckets first; inside a bucket, last added first.
    PrimitiveBuffer buf;
    OrderingTable ot;
    ot.clear_r(8);
    const uint32_t a = buf.allocate(9), b = buf.allocate(9), c = buf.allocate(9), d = buf.allocate(9, 4);
    ot.add_prim(buf, 2, a);
    ot.add_prim(buf, 6, b);
    ot.add_prim(buf, 2, c);
    ot.add_prim(buf, 100, d);                 // clamped to the last bucket
    const uint32_t e = buf.allocate(9);
    OrderingTable::insert_after(buf, b, e);   // bit-28 chain: right after b
    std::vector<uint32_t> order;
    ot.draw(buf, [&](uint32_t p) { order.push_back(p); });
    assert((order == std::vector<uint32_t>{d, b, e, c, a}));
    assert(PrimitiveBuffer::length(buf.tag(d)) == 9 && buf.size_words() == 5 * 10 + 4);

    // POLY_FT4 round trip.
    PolyFt4 q;
    q.code_rgb = 0x2E405060u;
    q.x = {{-40, 300, -40, 300}}; q.y = {{10, 10, 200, 200}};
    q.u = {{0, 63, 0, 63}}; q.v = {{0, 0, 31, 31}};
    q.clut = 0x7A12; q.tpage = 0x002D;
    uint32_t w[9];
    write_poly_ft4(w, q);
    const auto p = decode_gp0_polygon(w, 9);
    assert(p && p->vertices == 4 && p->textured && p->semi_transparent && !p->gouraud && !p->raw_texture);
    assert(p->x == q.x && p->y == q.y && p->u == q.u && p->v == q.v && p->clut == 0x7A12 && p->tpage == 0x2D);
    assert(p->rgb[3] == 0x405060u);
    assert(!decode_gp0_polygon(w, 8) && !decode_gp0_polygon(w, 0));
    uint32_t fill[3] = {0x02000000u, 0, 0};
    assert(!decode_gp0_polygon(fill, 3));

    // POLY_FT3 (the subdivider's edge triangles, code & ~0x08).
    PolyFt4 tri = q; tri.code_rgb = 0x24405060u;
    uint32_t w3[7];
    write_poly_ft3(w3, tri);
    const auto t3 = decode_gp0_polygon(w3, 7);
    assert(t3 && t3->vertices == 3 && t3->textured && !t3->semi_transparent && t3->clut == 0x7A12 && t3->tpage == 0x2D);
    assert(t3->x[2] == -40 && t3->y[2] == 200 && t3->u[1] == 63 && t3->v[2] == 31);

    // Gouraud triangle: colours per vertex.
    const uint32_t g3[6] = {0x30111111u, 0x00010002u, 0x00222222u, 0x00030004u, 0x00333333u, 0x00050006u};
    const auto g = decode_gp0_polygon(g3, 6);
    assert(g && g->vertices == 3 && g->gouraud && g->rgb[0] == 0x111111u && g->rgb[2] == 0x333333u && g->x[2] == 6 && g->y[2] == 5);

    // Terrain packet 0x80026158: code from the prefix (semi bit kept, other
    // high bits dropped), DPCS rgb, CLUT + ramp, TPAGE; back face 0x2C010101.
    TerrainPacketInput in;
    in.prefix = 0x5E7E7E7Eu;   // bits 30/28/27/26/25 set
    in.rgb = 0x203040u;
    in.material_word = 0x7A00002Du;
    in.ramp = 3;
    in.uv = {{1, 2, 3, 4, 5, 6, 7, 8}};
    in.sx = {{1, 2, 3, 4}}; in.sy = {{5, 6, 7, 8}};
    auto t = terrain_poly_ft4_80026158(in);
    assert(t.code_rgb == 0x2E203040u && t.clut == 0x7A03 && t.tpage == 0x2D && t.u[3] == 7 && t.v[3] == 8);
    in.black = true; in.prefix = 0x007E7E7Eu; in.see_through_code = 0x02000000u;
    t = terrain_poly_ft4_80026158(in);
    assert(t.code_rgb == 0x2E010101u);
    assert(terrain_ot_bucket(5391) == 673 && terrain_ot_bucket(-5) == 0);

    // Subdivision: small faces never, big faces always, mid faces unless
    // they are near parallelograms.
    const std::array<int16_t,4> sx{{0, 40, 0, 40}}, sy{{0, 0, 30, 30}};
    assert(!terrain_needs_subdivision_8002602c(512, sx, sy));
    assert(terrain_needs_subdivision_8002602c(-2049, sx, sy));
    assert(!terrain_needs_subdivision_8002602c(1200, sx, sy));
    const std::array<int16_t,4> skew{{0, 40, 0, 60}};
    assert(terrain_needs_subdivision_8002602c(1200, skew, sy));
    std::cout << "psx gpu ok\n";
    return 0;
}
