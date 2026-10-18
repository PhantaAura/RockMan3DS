#include "legends3ds/player_composition.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <array>
#include <stdexcept>

namespace l3 {
namespace {
constexpr std::array<uint8_t, kPlayerComponentSlots> kExpectedCounts{{6, 4, 3, 3, 2}};
constexpr std::array<uint8_t, kBaselinePlayerMeshEntries> kTransformLookup{{
    0, 8, 9, 10, 12, 13,  // body
    1, 1, 1, 1,           // four head mesh pieces all use transform 1
    2, 3, 4,               // right arm
    5, 6, 7,               // left arm
    11, 14                 // feet/legs
}};
}

PlayerComposition build_baseline_player_composition(const std::array<PbdModel, kPlayerComponentSlots>& components) {
    PlayerComposition out;
    out.attachments.reserve(kBaselinePlayerMeshEntries);
    size_t lookup = 0;
    for (size_t slot = 0; slot < components.size(); ++slot) {
        const auto& model = components[slot];
        if (model.parts.size() != kExpectedCounts[slot]) {
            throw std::runtime_error("baseline player PBD part count does not match executable renderer table");
        }
        for (size_t mesh = 0; mesh < model.parts.size(); ++mesh) {
            const uint8_t transform = kTransformLookup[lookup++];
            if (transform >= kPbdMainPosePartCount) {
                throw std::runtime_error("baseline player transform lookup exceeds skeleton part count");
            }
            PlayerMeshAttachment a;
            a.component_slot = static_cast<uint8_t>(slot);
            a.mesh_index = static_cast<uint8_t>(mesh);
            a.part_id = model.parts[mesh].part_id;
            a.transform_id = transform;
            out.attachments.push_back(a);
        }
    }
    if (lookup != kTransformLookup.size()) throw std::runtime_error("baseline player lookup length mismatch");
    return out;
}

std::vector<uint8_t> cook_l3player_composition(const PlayerComposition& c) {
    if (c.attachments.size() > 0xFFFFFFFFu) throw std::runtime_error("too many player composition attachments");
    std::vector<uint8_t> o;
    o.reserve(32u + c.attachments.size() * 8u);
    o.insert(o.end(), {'L','3','P','C'});
    wr_u16(o, kL3PlayerCompositionVersion);
    wr_u16(o, 32);
    wr_u32(o, 1u); // bit0: transform attachment mapping proven from executable renderer tables
    wr_u32(o, static_cast<uint32_t>(kPlayerComponentSlots));
    wr_u32(o, static_cast<uint32_t>(c.attachments.size()));
    const size_t p_off = o.size(); wr_u32(o, 0);
    const size_t p_file = o.size(); wr_u32(o, 0);
    const size_t p_crc = o.size(); wr_u32(o, 0);
    if (o.size() != 32) throw std::runtime_error("L3PC header size bug");
    const uint32_t off = static_cast<uint32_t>(o.size());
    for (const auto& a : c.attachments) {
        o.push_back(a.component_slot);
        o.push_back(a.mesh_index);
        o.push_back(a.part_id);
        o.push_back(a.transform_id);
        wr_u32(o, 0);
    }
    patch_u32(o, p_off, off);
    patch_u32(o, p_file, static_cast<uint32_t>(o.size()));
    patch_u32(o, p_crc, crc32(o.data() + 32, o.size() - 32));
    return o;
}

L3PlayerCompositionHeader inspect_l3player_composition(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < 32 || b[0] != 'L' || b[1] != '3' || b[2] != 'P' || b[3] != 'C') {
        throw std::runtime_error("not L3PC");
    }
    L3PlayerCompositionHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]); h.flags = rd_u32(&b[8]);
    h.component_slots = rd_u32(&b[12]); h.attachment_count = rd_u32(&b[16]);
    h.off_attachments = rd_u32(&b[20]); h.file_bytes = rd_u32(&b[24]); h.payload_crc32 = rd_u32(&b[28]);
    if (h.version != kL3PlayerCompositionVersion || h.header_bytes != 32) throw std::runtime_error("unsupported L3PC");
    if (h.component_slots != kPlayerComponentSlots) throw std::runtime_error("L3PC component slot count mismatch");
    if (h.file_bytes != b.size()) throw std::runtime_error("L3PC file size mismatch");
    if (h.off_attachments < 32 || uint64_t(h.off_attachments) + uint64_t(h.attachment_count) * 8u > b.size()) {
        throw std::runtime_error("L3PC attachment range invalid");
    }
    if (verify_crc && crc32(b.data() + 32, b.size() - 32) != h.payload_crc32) throw std::runtime_error("L3PC CRC mismatch");
    return h;
}


PlayerComposition load_l3player_composition(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3player_composition(b, verify_crc);
    PlayerComposition out;
    out.attachments.reserve(h.attachment_count);
    for (uint32_t i = 0; i < h.attachment_count; ++i) {
        const size_t o = size_t(h.off_attachments) + size_t(i) * 8u;
        if (rd_u32(&b[o + 4]) != 0) throw std::runtime_error("L3PC attachment reserved field nonzero");
        PlayerMeshAttachment a;
        a.component_slot = b[o]; a.mesh_index = b[o + 1]; a.part_id = b[o + 2]; a.transform_id = b[o + 3];
        if (a.component_slot >= kPlayerComponentSlots) throw std::runtime_error("L3PC component slot invalid");
        if (a.transform_id >= kPbdMainPosePartCount) throw std::runtime_error("L3PC transform id invalid");
        out.attachments.push_back(a);
    }
    return out;
}

} // namespace l3
