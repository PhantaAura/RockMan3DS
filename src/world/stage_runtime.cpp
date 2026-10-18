#include "legends3ds/stage_runtime.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/endian_io.h"
#include <array>
#include <stdexcept>

namespace l3 {
namespace {
void store_u16_at(std::vector<uint8_t>& b, size_t at, uint16_t v) {
    if (at + 2 > b.size()) throw std::runtime_error("stage work u16 offset outside slot");
    b[at + 0] = static_cast<uint8_t>(v);
    b[at + 1] = static_cast<uint8_t>(v >> 8);
}
void store_u32_at(std::vector<uint8_t>& b, size_t at, uint32_t v) {
    if (at + 4 > b.size()) throw std::runtime_error("stage work u32 offset outside slot");
    b[at + 0] = static_cast<uint8_t>(v);
    b[at + 1] = static_cast<uint8_t>(v >> 8);
    b[at + 2] = static_cast<uint8_t>(v >> 16);
    b[at + 3] = static_cast<uint8_t>(v >> 24);
}
void copy_raw_at(std::vector<uint8_t>& b, size_t at, const uint8_t* src, size_t n) {
    if (at + n > b.size()) throw std::runtime_error("stage work copy outside slot");
    for (size_t i = 0; i < n; ++i) b[at + i] = src[i];
}
}
StageObjectFamily classify_stage_object_family(uint8_t c) {
    switch (c) {
        case 0x00: return StageObjectFamily::Class00;
        case 0x20: return StageObjectFamily::Class20;
        case 0x40: return StageObjectFamily::Class40;
        case 0x60: return StageObjectFamily::Class60;
        case 0x80: return StageObjectFamily::Class80;
        case 0xC0: return StageObjectFamily::ClassC0;
        case 0xE0: return StageObjectFamily::ClassE0;
        default: return StageObjectFamily::Unknown;
    }
}

StageRuntimeObject instantiate_stage_runtime_object(const StageObjectRecord& s, size_t index) {
    StageRuntimeObject o;
    o.source_index = index;
    o.id = s.id;
    o.source_class = s.object_class;
    o.normalized_ebd_class = normalize_ebd_object_class(s.object_class);
    o.subtype = s.raw[2];
    o.source_param3 = s.raw[3];
    o.family = classify_stage_object_family(s.object_class);
    o.source_position = {s.x, s.y, s.z};
    // The object dispatcher visibly shifts source coordinates left by 16 for several
    // work families. Keep this canonical fixed representation for the native runtime
    // while retaining the source 16-bit values beside it.
    o.fixed_position_16_16 = {
        static_cast<int32_t>(s.x) * 65536,
        static_cast<int32_t>(s.y) * 65536,
        static_cast<int32_t>(s.z) * 65536,
    };
    o.raw = s.raw;
    return o;
}

std::vector<StageRuntimeObject> instantiate_stage_runtime_objects(const StageObjectList& list) {
    std::vector<StageRuntimeObject> out;
    out.reserve(list.records.size());
    for (size_t i = 0; i < list.records.size(); ++i) {
        out.push_back(instantiate_stage_runtime_object(list.records[i], i));
    }
    return out;
}

std::optional<StageObjectPoolSpec> stage_object_pool_spec(StageObjectFamily family) {
    // Allocator / capacity / stride:
    // 20: 0x80031418, 16 * 0x3C0
    // 40: 0x80031498,  4 * 0x5A4
    // 60: 0x80031518, 32 * 0x10C
    // 80: 0x80031618,  2 banks * 32 * 0x48
    // C0: 0x800316BC, 16 * 0x24
    // E0: 0x80031734, 32 * 0x50
    switch (family) {
        case StageObjectFamily::Class20: return StageObjectPoolSpec{family, 0x20, 16, 0x3C0, 1};
        case StageObjectFamily::Class40: return StageObjectPoolSpec{family, 0x40,  4, 0x5A4, 1};
        case StageObjectFamily::Class60: return StageObjectPoolSpec{family, 0x60, 32, 0x10C, 1};
        case StageObjectFamily::Class80: return StageObjectPoolSpec{family, 0x80, 64, 0x048, 2};
        case StageObjectFamily::ClassC0: return StageObjectPoolSpec{family, 0xC0, 16, 0x024, 1};
        case StageObjectFamily::ClassE0: return StageObjectPoolSpec{family, 0xE0, 32, 0x050, 1};
        case StageObjectFamily::Class00:
        case StageObjectFamily::Unknown:
            return std::nullopt;
    }
    return std::nullopt;
}

std::optional<StageObjectPoolSpec> stage_object_pool_spec_for_class(uint8_t source_class) {
    return stage_object_pool_spec(classify_stage_object_family(source_class));
}

std::vector<StageRuntimeAllocation> assign_fresh_stage_pool_slots(
    const std::vector<StageRuntimeObject>& objects) {
    // Index by the seven enum values before Unknown.
    std::array<size_t, 7> used{};
    std::vector<StageRuntimeAllocation> out;
    out.reserve(objects.size());
    for (const auto& object : objects) {
        StageRuntimeAllocation a;
        a.source_index = object.source_index;
        a.family = object.family;
        const auto spec = stage_object_pool_spec(object.family);
        if (spec) {
            const size_t fi = static_cast<size_t>(object.family);
            if (fi >= used.size()) throw std::runtime_error("invalid stage object family");
            if (used[fi] >= spec->capacity) {
                throw std::runtime_error("stage object source exceeds original pool capacity");
            }
            a.pool_slot = used[fi]++;
        }
        out.push_back(a);
    }
    return out;
}


std::optional<StageRuntimeWork> seed_fresh_stage_runtime_work(
    const StageRuntimeObject& object, const StageRuntimeAllocation& allocation) {
    if (object.source_index != allocation.source_index || object.family != allocation.family)
        throw std::runtime_error("stage allocation/object mismatch");
    const auto spec = stage_object_pool_spec(object.family);
    if (!spec || !allocation.pool_slot) return std::nullopt;
    if (*allocation.pool_slot >= spec->capacity) throw std::runtime_error("stage pool slot outside capacity");

    StageRuntimeWork w;
    w.source_index = object.source_index;
    w.family = object.family;
    w.pool_slot = *allocation.pool_slot;
    w.bytes.assign(spec->original_stride_bytes, 0);

    // Every generic allocator initializes routine byte +6 to 3 on allocation.
    if (w.bytes.size() <= 6) throw std::runtime_error("stage work slot too small");
    w.bytes[6] = 3;
    w.bytes[0] = object.raw[0];
    store_u16_at(w.bytes, 0x02, rd_u16(object.raw.data() + 0x02));
    store_u16_at(w.bytes, 0x04, rd_u16(object.raw.data() + 0x04));
    store_u32_at(w.bytes, 0x0C, rd_u32(object.raw.data() + 0x08));

    switch (object.family) {
        case StageObjectFamily::Class20:
        case StageObjectFamily::Class40:
        case StageObjectFamily::Class60: {
            w.layout = StageWorkSeedLayout::Common20_40_60;
            // The parser keeps both an eight-byte raw position/tail copy and duplicate
            // 16.16 XYZ positions at two work ranges.
            copy_raw_at(w.bytes, 0x14, object.raw.data() + 0x0C, 8);
            copy_raw_at(w.bytes, 0x2C, object.raw.data() + 0x0C, 8);
            const uint32_t fx = static_cast<uint32_t>(static_cast<int32_t>(object.source_position[0]) * 65536);
            const uint32_t fy = static_cast<uint32_t>(static_cast<int32_t>(object.source_position[1]) * 65536);
            const uint32_t fz = static_cast<uint32_t>(static_cast<int32_t>(object.source_position[2]) * 65536);
            store_u32_at(w.bytes, 0x1C, fx); store_u32_at(w.bytes, 0x20, fy); store_u32_at(w.bytes, 0x24, fz);
            store_u32_at(w.bytes, 0x34, fx); store_u32_at(w.bytes, 0x38, fy); store_u32_at(w.bytes, 0x3C, fz);
            const uint16_t scaled7 = static_cast<uint16_t>(uint16_t(object.raw[7]) << 4);
            store_u16_at(w.bytes, 0x56, scaled7);
            store_u16_at(w.bytes, 0x5E, scaled7);
            // Allocator/parser explicitly clear these observed fields.
            store_u16_at(w.bytes, 0x54, 0); store_u16_at(w.bytes, 0x58, 0);
            store_u16_at(w.bytes, 0x5C, 0); store_u16_at(w.bytes, 0x60, 0);
            break;
        }
        case StageObjectFamily::ClassC0:
            w.layout = StageWorkSeedLayout::CompactC0;
            copy_raw_at(w.bytes, 0x14, object.raw.data() + 0x0C, 8);
            break;
        case StageObjectFamily::Class80:
        case StageObjectFamily::ClassE0:
            w.layout = StageWorkSeedLayout::Compact80_E0;
            copy_raw_at(w.bytes, 0x38, object.raw.data() + 0x0C, 8);
            store_u16_at(w.bytes, 0x1C, static_cast<uint16_t>(object.source_position[0]));
            store_u16_at(w.bytes, 0x1E, static_cast<uint16_t>(object.source_position[1]));
            store_u16_at(w.bytes, 0x20, static_cast<uint16_t>(object.source_position[2]));
            break;
        case StageObjectFamily::Class00:
        case StageObjectFamily::Unknown:
            return std::nullopt;
    }
    return w;
}


StageRuntimeArea build_fresh_stage_runtime_area(const StageObjectList& source,
                                                const EbdDirectory& ebd) {
    StageRuntimeArea area;
    const auto objects = instantiate_stage_runtime_objects(source);
    const auto allocations = assign_fresh_stage_pool_slots(objects);
    if (objects.size() != allocations.size()) throw std::runtime_error("stage allocation count mismatch");
    area.instances.reserve(objects.size());
    for (size_t i = 0; i < objects.size(); ++i) {
        StageRuntimeInstance inst;
        inst.object = objects[i];
        inst.allocation = allocations[i];
        inst.work = seed_fresh_stage_runtime_work(inst.object, inst.allocation);
        inst.ebd.class_subtype_candidates = find_ebd_entries(
            ebd, inst.object.source_class, inst.object.subtype);
        inst.ebd.source_byte3_candidates = find_ebd_entries(
            ebd, inst.object.source_class, inst.object.subtype, inst.object.source_param3);
        if (inst.ebd.class_subtype_candidates.empty()) {
            inst.ebd.state = StageEbdBindingState::NoLocalCandidate;
        } else if (inst.ebd.class_subtype_candidates.size() == 1) {
            inst.ebd.state = StageEbdBindingState::UniqueClassSubtype;
        } else if (inst.ebd.source_byte3_candidates.size() == 1) {
            inst.ebd.state = StageEbdBindingState::AmbiguousNarrowedBySourceByte3;
        } else {
            inst.ebd.state = StageEbdBindingState::AmbiguousClassSubtype;
        }
        area.instances.push_back(std::move(inst));
    }
    return area;
}

} // namespace l3
