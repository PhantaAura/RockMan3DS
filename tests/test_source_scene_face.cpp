// Opening face channels (0x8010C820 / 0x8010C6B8 / 0x8002D404 / 0x8002D320) on authored data.
#include "legends3ds/source_scene_face.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
namespace {
void require(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", why); std::exit(1); } }
void put32(std::vector<uint8_t>& b, size_t at, uint32_t v) { for (int i = 0; i < 4; ++i) b[at + i] = uint8_t(v >> (8 * i)); }
}
int main() {
    // 0x8002D404(frame, 42, 51, 6): frame 8 -> column 2, row 1 -> u 84, v 51.
    require(source_face_offset_8002d404(8, 42, 51, 6) == ((51u << 24) | (84u << 16) | (51u << 8) | 84u), "offset packing");
    require(source_face_offset_8002d404(0, 42, 51, 6) == 0, "frame 0 offset");

    SourceSceneFaceProfile prof; prof.loaded = true;
    // Mouth channel (1): cue at timeline record 1 (event+8 halfword 0x0100), frame 5 -> script 2.
    prof.cues[1].count = 1; prof.cues[1].cues[0] = {0x0100, 5, 2};
    // Script 2: frame 11 for 3, frame 12 for 2, then -2 (loop back to entry 0): a mouth flap.
    auto& s = prof.scripts[1][2]; s.count = 3;
    s.entries[0] = {{11, 0, 3, 0}}; s.entries[1] = {{12, 0, 2, 0}}; s.entries[2] = {{13, 0, 0xFE, 0xFF}};
    SourceSceneFaceState st; std::array<SourceFaceUvCopy,4> out{};
    require(step_source_scene_face_8010c820(st, prof, 0x0100, 4, out) == 0 && !st.running[1], "no cue before its frame");
    size_t n = step_source_scene_face_8010c820(st, prof, 0x0100, 5, out);
    require(n == 1 && out[0].destination_part == 18 && out[0].source_part == 19 &&
            out[0].offset == source_face_offset_8002d404(11, 42, 51, 6) && st.counter[1] == 1, "cue starts the mouth on frame 11");
    std::vector<uint8_t> frames;
    for (int t = 0; t < 12; ++t) {
        n = step_source_scene_face_8010c820(st, prof, 0x0100, uint32_t(6 + t), out);
        if (n) frames.push_back(uint8_t(out[0].offset == source_face_offset_8002d404(11, 42, 51, 6) ? 11 :
                                        out[0].offset == source_face_offset_8002d404(12, 42, 51, 6) ? 12 : 13));
    }
    // 11 held 3 frames -> 12 held 2 -> 13 (duration -2 jumps back to entry 0 next frame) -> 11 ...
    require(frames.size() >= 4 && frames[0] == 12 && frames[1] == 13 && frames[2] == 12, "script steps and loops");

    // 0x8002D320: copy part 19's UV words + offset into a decoded part (32-bit add, carries).
    std::vector<uint8_t> ebd(0x400, 0); const uint32_t base = 0x80180000u;
    put32(ebd, 0x70, base + 0x100);                        // descriptor+0x70 -> Near mesh
    const size_t part19 = 0x100 + 20 + 20 * 19;
    ebd[part19] = 1; ebd[part19 + 1] = 1;                  // 1 triangle, 1 quad
    put32(ebd, part19 + 4, base + 0x300); put32(ebd, part19 + 8, base + 0x320);
    put32(ebd, 0x300, 0x10FF2001u); put32(ebd, 0x304, 0x00003040u);
    put32(ebd, 0x320, 0x01020304u); put32(ebd, 0x324, 0x05060708u);
    PbdPart dst; dst.triangles.resize(1); dst.quads.resize(1);
    const uint32_t off = 0x00010002u;
    require(apply_source_face_uv_8002d320(dst, ebd.data(), ebd.size(), base, base, 19, off), "uv copy");
    const uint32_t w0 = 0x10FF2001u + off;   // byte 1 0x20 + 0x00, byte 2 0xFF + 0x01 carries into byte 3
    require(dst.triangles[0].uv[0] == uint8_t(w0) && dst.triangles[0].uv[2] == uint8_t(w0 >> 16) && dst.triangles[0].uv[3] == uint8_t(w0 >> 24),
            "triangle word add with carry");
    require(dst.quads[0].uv[0] == 0x06 && dst.quads[0].uv[2] == 0x03 && dst.quads[0].uv[4] == 0x0A && dst.quads[0].uv[6] == 0x07, "quad words");
    PbdPart big; big.triangles.resize(2);
    require(!apply_source_face_uv_8002d320(big, ebd.data(), ebd.size(), base, base, 19, off), "destination larger than source holds");
    std::puts("source_scene_face: offset, cue timing, script loop, UV copy with carry OK");
}
