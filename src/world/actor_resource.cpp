#include "legends3ds/actor_resource.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace l3 {
namespace {
struct Source {
    const EbdDirectory& ebd;
    const uint8_t* at(size_t offset, size_t bytes) const {
        if (ebd.source_payload.empty()) throw std::runtime_error("actor EBD source payload unavailable");
        if (offset > ebd.source_payload.size() || bytes > ebd.source_payload.size() - offset)
            throw std::runtime_error("actor resource window outside EBD payload");
        return ebd.source_payload.data() + offset;
    }
    uint32_t pointer(uint32_t address, size_t bytes) const {
        if (address == 0 || address < ebd.load_base)
            throw std::runtime_error("actor required source pointer null or below EBD base");
        const uint32_t off = address - ebd.load_base;
        if (off & 3u) throw std::runtime_error("actor source pointer is not word aligned");
        (void)at(off, bytes);
        return off;
    }
    uint32_t pointer_at(size_t offset, size_t bytes) const {
        return pointer(rd_u32(at(offset, 4)), bytes);
    }
};
int8_t signed_byte(uint8_t b) {
    return static_cast<int8_t>(b < 128 ? int(b) : int(b) - 256);
}
int16_t signed12(uint16_t n) {
    return static_cast<int16_t>(n < 2048 ? int(n) : int(n) - 4096);
}
const EbdDirectoryEntry& entry(const EbdDirectory& ebd, size_t row, uint8_t family) {
    if (row >= ebd.entries.size()) throw std::runtime_error("actor EBD row invalid");
    const auto& e = ebd.entries[row];
    if (e.object_class != family) throw std::runtime_error("actor resource class not supported");
    return e;
}
uint32_t slot(const EbdDirectory& ebd, size_t row, size_t resource, size_t bytes) {
    const auto span = ebd_resource_span(ebd, row, resource, bytes);
    if (span.status != EbdResourceSpanStatus::AVAILABLE)
        throw std::runtime_error("actor EBD required resource unavailable");
    if (span.source_offset & 3u) throw std::runtime_error("actor directory pointer not word aligned");
    return span.source_offset;
}
struct Part {
    uint8_t triangles = 0, quads = 0, vertices = 0;
    uint32_t triangle_offset = 0, quad_offset = 0, vertex_offset = 0;
};
}

static ActorMeshResource decode_ebd_actor_mesh_family(const EbdDirectory& ebd, size_t row,
                                      ActorMeshTier tier, size_t cap, uint8_t family, size_t part_limit,
                                      const std::array<uint8_t,96>* variant=nullptr) {
    (void)entry(ebd, row, family);
    const auto ti = static_cast<uint8_t>(tier);
    if (ti > 2) throw std::runtime_error("actor mesh tier invalid");
    const Source s{ebd};
    // Source +0x10..+0x6f is copied as 24 u32 words to work +0xf8.
    const uint32_t descriptor = slot(ebd, row, 0, 0x7c);
    const uint8_t count = s.at(size_t(descriptor) + 0x11, 1)[0];
    if(variant){const auto* original=s.at(size_t(descriptor)+0x10,96);
        for(size_t i=0;i<96;++i)if(i%4u&&(*variant)[i]!=original[i])
            throw std::runtime_error("actor mesh variant changes hierarchy/control lanes");}
    if (!count || count > part_limit)
        throw std::runtime_error("actor family part count unsupported");
    const uint32_t mesh = s.pointer_at(size_t(descriptor) + 0x70 + size_t(ti) * 4, 20);
    ActorMeshResource out;
    out.source_row = static_cast<uint32_t>(row);
    out.source_descriptor_offset = descriptor;
    out.tier = tier;
    out.footprint.descriptor_bytes = 0x7c;
    std::vector<Part> parts;
    parts.reserve(count);
    out.bindings.reserve(count);
    size_t owned = size_t(count) * (sizeof(PbdPart) + sizeof(ActorDrawBinding));
    for (uint8_t i = 0; i < count; ++i) {
        const auto* d = s.at(size_t(descriptor) + 0x10 + size_t(i) * 4, 4);
        ActorDrawBinding binding;
        binding.mesh_index = variant?(*variant)[size_t(i)*4]:d[0];
        binding.parent = i == 0 ? -1 : signed_byte(d[1]);
        binding.rotation_translation_index = d[2];
        binding.draw_control = signed_byte(d[3]);
        if (i && binding.parent >= static_cast<int>(i))
            throw std::runtime_error("actor parent does not precede child");
        if (binding.rotation_translation_index >= kActorRotationVectors)
            throw std::runtime_error("actor rotation vector index unsupported");
        const size_t po = size_t(mesh) + 20 + size_t(binding.mesh_index) * 20;
        const auto* p = s.at(po, 20);
        Part part;
        part.triangles = p[0]; part.quads = p[1]; part.vertices = p[2];
        binding.source_part_byte3 = p[3];
        binding.material_word = rd_u32(p + 12);
        binding.source_part_offset = static_cast<uint32_t>(po);
        if (!part.vertices && (part.triangles || part.quads))
            throw std::runtime_error("actor faces without vertices");
        const size_t vertex_bytes = size_t(part.vertices) * 8;
        const size_t consumer_bytes = ((size_t(part.vertices) + 2) / 3) * 24;
        if (part.vertices) part.vertex_offset = s.pointer(rd_u32(p + 16), consumer_bytes);
        if (part.triangles) part.triangle_offset = s.pointer(rd_u32(p + 4), size_t(part.triangles) * 12);
        if (part.quads) part.quad_offset = s.pointer(rd_u32(p + 8), size_t(part.quads) * 12);
        out.footprint.part_bytes += 20;
        out.footprint.vertex_bytes += vertex_bytes;
        out.footprint.vertex_consumer_bytes += consumer_bytes;
        out.footprint.packet_bytes += size_t(part.triangles + part.quads) * 12;
        owned += size_t(part.vertices) * sizeof(PbdVertex) + size_t(part.triangles) * sizeof(PbdTri)
               + size_t(part.quads) * sizeof(PbdQuad);
        if (owned > cap) throw std::runtime_error("actor decoded memory budget exceeded");
        parts.push_back(part);
        out.bindings.push_back(binding);
    }
    out.footprint.decoded_bytes = owned;
    out.model.parts.reserve(count);
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& part = parts[i];
        const uint32_t material = out.bindings[i].material_word;
        const uint16_t clut = static_cast<uint16_t>(material >> 16);
        const uint16_t tpage = static_cast<uint16_t>(material | 0x20u);
        PbdPart p;
        p.part_id = static_cast<uint8_t>(i); // draw slot, not an inferred source identity.
        p.vertices.reserve(part.vertices);
        p.triangles.reserve(part.triangles);
        p.quads.reserve(part.quads);
        for (size_t j = 0; j < part.vertices; ++j) {
            const auto* v = s.at(size_t(part.vertex_offset) + j * 8, 8);
            p.vertices.push_back({rd_s16(v), rd_s16(v + 2), rd_s16(v + 4), rd_u16(v + 6)});
        }
        for (size_t j = 0; j < part.triangles; ++j) {
            const auto* q = s.at(size_t(part.triangle_offset) + j * 12, 12);
            PbdTri f;
            for (size_t k = 0; k < 6; ++k) f.uv[k] = q[k];
            for (size_t k = 0; k < 3; ++k) {
                f.index[k] = q[8 + k];
                if (f.index[k] >= part.vertices) throw std::runtime_error("actor triangle index out of range");
            }
            f.clut = clut; f.tpage = tpage; f.r = f.g = f.b = 128; f.code = 0x24;
            p.triangles.push_back(f);
        }
        for (size_t j = 0; j < part.quads; ++j) {
            const auto* q = s.at(size_t(part.quad_offset) + j * 12, 12);
            PbdQuad f;
            for (size_t k = 0; k < 8; ++k) f.uv[k] = q[k];
            for (size_t k = 0; k < 4; ++k) {
                f.index[k] = q[8 + k];
                if (f.index[k] >= part.vertices) throw std::runtime_error("actor quad index out of range");
            }
            f.clut = clut; f.tpage = tpage; f.r = f.g = f.b = 128; f.code = 0x2c;
            p.quads.push_back(f);
        }
        out.model.parts.push_back(std::move(p));
    }
    return out;
}

std::array<uint16_t, kActorPoseChannels> unpack_actor_pose_channels(const uint8_t* bytes, size_t size) {
    if (!bytes || size < kActorPackedPoseBytes) throw std::runtime_error("actor packed pose window truncated");
    std::array<uint16_t, kActorPoseChannels> out{};
    for (size_t i = 0; i < out.size(); ++i) {
        const size_t bit = i * 12, byte = bit / 8;
        const unsigned shift = static_cast<unsigned>(bit % 8);
        const uint32_t value = uint32_t(bytes[byte]) | uint32_t(bytes[byte + 1]) << 8;
        out[i] = static_cast<uint16_t>((value >> shift) & 0xfffu);
    }
    return out;
}

static ActorPoseResource decode_ebd_actor_pose_family(const EbdDirectory& ebd, size_t row,
                                      const ActorMeshResource& mesh,
                                      uint8_t request, uint8_t record_index, uint8_t family, size_t part_limit) {
    (void)entry(ebd, row, family);
    if (mesh.source_row != row || mesh.bindings.empty() || mesh.bindings.size() > part_limit)
        throw std::runtime_error("actor pose mesh/row mismatch");
    const Source s{ebd};
    const uint32_t r1 = slot(ebd, row, 1, 8);
    const uint32_t r2 = slot(ebd, row, 2, 4);
    const uint32_t records = s.pointer_at(size_t(r2) + size_t(request) * 4, 8);
    const size_t sequence_offset = size_t(records) + size_t(record_index) * 8;
    const auto* record = s.at(sequence_offset, 8);
    const uint32_t frames = s.pointer_at(size_t(r1) + 4 + size_t(request) * 4, 4);
    const uint32_t pose = s.pointer_at(size_t(frames) + size_t(record[0]) * 4, kActorPackedPoseBytes);
    const auto channels = unpack_actor_pose_channels(s.at(pose, kActorPackedPoseBytes), kActorPackedPoseBytes);
    const uint32_t translations = s.pointer_at(r1, 8);
    ActorPoseResource out;
    out.request = request; out.record_index = record_index; out.frame_index = record[0];
    out.countdown = record[1]; out.next_control = signed_byte(record[3]);
    for (size_t i = 0; i < 8; ++i) out.sequencing_record[i] = record[i];
    for (size_t i = 0; i < 3; ++i) out.root_translation[i] = signed12(channels[i]);
    for (size_t i = 0; i < out.rotations.size(); ++i)
        for (size_t j = 0; j < 3; ++j) out.rotations[i][j] = channels[3 + i * 3 + j];
    out.base_translation.reserve(mesh.bindings.size());
    for (size_t i = 0; i < mesh.bindings.size(); ++i) {
        const auto index = mesh.bindings[i].rotation_translation_index;
        std::array<int16_t, 3> t{};
        // Index 0 explicitly copies root orientation/position; no child vector read.
        if (i && index && mesh.bindings[i].parent >= 0) {
            const auto* v = s.at(size_t(translations) + size_t(index) * 8, 8);
            for (size_t j = 0; j < 3; ++j) t[j] = rd_s16(v + j * 2);
        }
        out.base_translation.push_back(t);
    }
    out.sequence_source_offset = static_cast<uint32_t>(sequence_offset);
    out.pose_source_offset = pose;
    out.translations_source_offset = translations;
    return out;
}
ActorMeshResource decode_ebd_actor_mesh(const EbdDirectory& ebd, size_t row,
                                      ActorMeshTier tier, size_t cap) {
    return decode_ebd_actor_mesh_family(ebd,row,tier,cap,0x20,kActorClass20PartLimit);
}
ActorPoseResource decode_ebd_actor_pose(const EbdDirectory& ebd, size_t row,
                                      const ActorMeshResource& mesh,
                                      uint8_t request, uint8_t record_index) {
    return decode_ebd_actor_pose_family(ebd,row,mesh,request,record_index,0x20,kActorClass20PartLimit);
}
ActorMeshResource decode_ebd_scene_actor40_mesh(const EbdDirectory& ebd, size_t row,
                                      ActorMeshTier tier, size_t cap) {
    return decode_ebd_actor_mesh_family(ebd,row,tier,cap,0x40,kActorDrawDescriptorSlots);
}
ActorMeshResource decode_ebd_scene_actor40_mesh_variant(const EbdDirectory& ebd,size_t row,
    const std::array<uint8_t,96>& descriptor,ActorMeshTier tier,size_t cap) {
    return decode_ebd_actor_mesh_family(ebd,row,tier,cap,0x40,kActorDrawDescriptorSlots,&descriptor);
}
ActorPoseResource decode_ebd_scene_actor40_pose(const EbdDirectory& ebd, size_t row,
                                      const ActorMeshResource& mesh,
                                      uint8_t request, uint8_t record_index) {
    return decode_ebd_actor_pose_family(ebd,row,mesh,request,record_index,0x40,kActorDrawDescriptorSlots);
}
static ActorPoseSequenceCache decode_ebd_actor_pose_sequence_family(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    uint8_t request, size_t cap, uint8_t family, size_t part_limit) {
    ActorPoseSequenceCache out;
    const size_t translation_bytes = mesh.bindings.size() * sizeof(std::array<int16_t, 3>);
    const auto footprint = [&](size_t capacity, size_t count) {
        return sizeof(ActorPoseSequenceCache) +
            capacity * (sizeof(ActorPoseResource) + sizeof(ActorSequenceRecord)) +
            count * translation_bytes;
    };
    for (size_t index = 0; index < kActorSequenceRecordLimit; ++index) {
        const size_t capacity = out.poses.capacity();
        const size_t wanted_capacity = index == capacity
            ? std::min(kActorSequenceRecordLimit, capacity ? capacity * 2 : size_t(8)) : capacity;
        if (footprint(wanted_capacity, index + 1) > cap)
            throw std::runtime_error("actor pose cache decoded-element budget exceeded");
        if (wanted_capacity != capacity) {
            out.poses.reserve(wanted_capacity);
            out.records.reserve(wanted_capacity);
        }
        auto pose = decode_ebd_actor_pose_family(
            ebd, row, mesh, request, static_cast<uint8_t>(index), family, part_limit);
        if (pose.base_translation.size() != mesh.bindings.size() ||
            pose.base_translation.capacity() != mesh.bindings.size())
            throw std::runtime_error("actor pose cache part storage inconsistent");
        const auto next = actor_sequence_next_index(static_cast<uint16_t>(index),
                                                    pose.sequencing_record[3]);
        out.records.push_back(pose.sequencing_record);
        out.poses.push_back(std::move(pose));
        out.decoded_bytes = sizeof(ActorPoseSequenceCache) +
            out.poses.capacity() * sizeof(ActorPoseResource) +
            out.records.capacity() * sizeof(ActorSequenceRecord) +
            (index + 1) * translation_bytes;
        if (out.decoded_bytes > cap)
            throw std::runtime_error("actor pose cache decoded-element budget exceeded");
        if (next <= index) return out;
        if (next != index + 1)
            throw std::runtime_error("actor pose cache noncontiguous record edge unsupported");
    }
    throw std::runtime_error("actor pose cache reachable record limit exceeded");
}
ActorPoseSequenceCache decode_ebd_actor_pose_sequence(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    uint8_t request, size_t cap) {
    return decode_ebd_actor_pose_sequence_family(
        ebd, row, mesh, request, cap, 0x20, kActorClass20PartLimit);
}
ActorPoseSequenceCache decode_ebd_scene_actor40_pose_sequence(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    uint8_t request, size_t cap) {
    return decode_ebd_actor_pose_sequence_family(
        ebd, row, mesh, request, cap, 0x40, kActorDrawDescriptorSlots);
}
} // namespace l3
