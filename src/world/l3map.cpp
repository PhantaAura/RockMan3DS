#include "legends3ds/l3map.h"
#include "legends3ds/endian_io.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace l3 {
uint32_t crc32(const uint8_t* data,size_t size){uint32_t c=0xFFFFFFFFu;for(size_t i=0;i<size;++i){c^=data[i];for(int b=0;b<8;++b)c=(c>>1)^(0xEDB88320u&uint32_t(-(int32_t)(c&1u)));}return c^0xFFFFFFFFu;}
static uint32_t stg_offset_or_ff(uint32_t p){if(!p)return 0xFFFFFFFFu;if(p<kSt00StgBase)return 0xFFFFFFFEu;return p-kSt00StgBase;}
std::vector<uint8_t> cook_st00_map(const St00Source& src,CookStats* stats_out){
    CookStats s; s.area_count=src.areas.size();s.idx_count=src.idx.size();s.descriptor_count=src.descriptors.size();s.contact_list_count=src.contact_lists.size();
    for(const auto&a:src.areas){s.cell_count+=a.cells.size();for(int16_t v:a.cells){if(v>0)++s.positive_cell_count;else if(v<0)++s.negative_cell_count;}}
    for(const auto&l:src.contact_lists){s.contact_group_count+=l.groups.size();for(const auto&g:l.groups)s.contact_primitive_count+=g.primitives.size();}
    std::map<uint32_t,uint16_t> desc_index; for(size_t i=0;i<src.descriptors.size();++i){if(i>=65535)throw std::runtime_error("too many descriptors");desc_index[src.descriptors[i].source_ptr]=uint16_t(i);}
    std::map<uint32_t,uint16_t> list_index; for(size_t i=0;i<src.contact_lists.size();++i){if(i>=65535)throw std::runtime_error("too many contact lists");list_index[src.contact_lists[i].source_ptr]=uint16_t(i);}
    std::vector<uint8_t> o; o.reserve(65536); o.insert(o.end(),{'L','3','M','P'}); wr_u16(o,kL3MapVersion);wr_u16(o,84);wr_u32(o,0);
    wr_u32(o,s.area_count);wr_u32(o,s.cell_count);wr_u32(o,s.positive_cell_count);wr_u32(o,s.negative_cell_count);wr_u32(o,s.idx_count);wr_u32(o,s.descriptor_count);wr_u32(o,s.contact_list_count);wr_u32(o,s.contact_group_count);wr_u32(o,s.contact_primitive_count);
    size_t poff=o.size(); for(int i=0;i<7;++i)wr_u32(o,0); size_t pfile=o.size();wr_u32(o,0);size_t pcrc=o.size();wr_u32(o,0);
    if(o.size()!=84)throw std::runtime_error("internal header size mismatch");
    uint32_t cell_cursor=0; uint32_t off_areas=o.size();
    for(const auto&a:src.areas){wr_s16(o,a.start_x);wr_s16(o,a.start_z);wr_u16(o,a.width);wr_u16(o,a.height);wr_u32(o,cell_cursor);wr_u32(o,a.cells.size());wr_u32(o,(uint32_t(a.start_x)<<9)-0x7F00u);wr_u32(o,(uint32_t(a.start_z)<<9)-0x7F00u);cell_cursor+=a.cells.size();}
    uint32_t off_cells=o.size(); for(const auto&a:src.areas)for(int16_t v:a.cells)wr_s16(o,v);
    uint32_t off_idx=o.size(); for(const auto&r:src.idx){wr_s16(o,r.x);wr_s16(o,r.y);wr_s16(o,r.z);wr_s16(o,r.raw6);auto it=desc_index.find(r.descriptor_ptr);wr_u16(o,it==desc_index.end()?0xFFFFu:it->second);wr_u16(o,0);}
    uint32_t off_desc=o.size(); for(const auto&d:src.descriptors){wr_u32(o,d.word0);auto ai=list_index.find(d.alt_contact_ptr),di=list_index.find(d.default_contact_ptr);wr_u16(o,ai==list_index.end()?0xFFFFu:ai->second);wr_u16(o,di==list_index.end()?0xFFFFu:di->second);for(int16_t v:d.raw_words3_5)wr_s16(o,v);for(uint32_t p:d.render_ptrs)wr_u32(o,stg_offset_or_ff(p));}
    uint32_t group_cursor=0; uint32_t off_lists=o.size(); for(const auto&l:src.contact_lists){wr_u32(o,group_cursor);wr_u32(o,l.groups.size());group_cursor+=l.groups.size();}
    uint32_t prim_cursor=0; uint32_t off_groups=o.size(); for(const auto&l:src.contact_lists)for(const auto&g:l.groups){wr_s16(o,g.x0);wr_s16(o,g.x1);wr_s16(o,g.z0);wr_s16(o,g.z1);wr_u32(o,prim_cursor);wr_u32(o,g.primitives.size());prim_cursor+=g.primitives.size();}
    uint32_t off_prims=o.size(); for(const auto&l:src.contact_lists)for(const auto&g:l.groups)for(const auto&p:g.primitives){wr_s16(o,p.y0);wr_s16(o,p.y1);wr_u16(o,p.code);wr_u16(o,p.flags);}
    const uint32_t offs[7]={off_areas,off_cells,off_idx,off_desc,off_lists,off_groups,off_prims};for(int i=0;i<7;++i)patch_u32(o,poff+i*4,offs[i]);patch_u32(o,pfile,o.size());uint32_t c=crc32(o.data()+84,o.size()-84);patch_u32(o,pcrc,c);if(stats_out)*stats_out=s;return o;
}
L3MapHeader inspect_l3map(const std::vector<uint8_t>& b, bool verify_crc) {
    if (b.size() < 84 || b[0] != 'L' || b[1] != '3' || b[2] != 'M' || b[3] != 'P') {
        throw std::runtime_error("not L3MP");
    }
    L3MapHeader h;
    h.version = rd_u16(&b[4]);
    h.header_bytes = rd_u16(&b[6]);
    h.flags = rd_u32(&b[8]);
    if (h.version != kL3MapVersion || h.header_bytes != 84) throw std::runtime_error("unsupported L3MP version/header");
    size_t q = 12;
    uint32_t* vals[] = {&h.stats.area_count, &h.stats.cell_count, &h.stats.positive_cell_count, &h.stats.negative_cell_count, &h.stats.idx_count, &h.stats.descriptor_count, &h.stats.contact_list_count, &h.stats.contact_group_count, &h.stats.contact_primitive_count};
    for (auto* p : vals) { *p = rd_u32(&b[q]); q += 4; }
    uint32_t* offs[] = {&h.off_areas, &h.off_cells, &h.off_idx, &h.off_descriptors, &h.off_contact_lists, &h.off_contact_groups, &h.off_contact_primitives};
    for (auto* p : offs) { *p = rd_u32(&b[q]); q += 4; }
    h.file_bytes = rd_u32(&b[q]); q += 4;
    h.payload_crc32 = rd_u32(&b[q]);
    if (h.file_bytes != b.size()) throw std::runtime_error("L3MP file size mismatch");
    if (verify_crc && crc32(b.data() + h.header_bytes, b.size() - h.header_bytes) != h.payload_crc32) throw std::runtime_error("L3MP CRC mismatch");
    return h;
}

RuntimeMap load_l3map(const std::vector<uint8_t>& b, bool verify_crc) {
    const auto h = inspect_l3map(b, verify_crc);
    RuntimeMap out;

    constexpr size_t kAreaBytes = 24;
    constexpr size_t kIdxBytes = 12;
    constexpr size_t kDescriptorBytes = 44;
    constexpr size_t kContactListBytes = 8;
    constexpr size_t kContactGroupBytes = 16;
    constexpr size_t kContactPrimitiveBytes = 8;

    if (uint64_t(h.off_areas) + uint64_t(h.stats.area_count) * kAreaBytes > h.off_cells)
        throw std::runtime_error("L3MP area section overflow");
    if (uint64_t(h.off_cells) + uint64_t(h.stats.cell_count) * 2u > h.off_idx)
        throw std::runtime_error("L3MP cell section overflow");
    if (uint64_t(h.off_idx) + uint64_t(h.stats.idx_count) * kIdxBytes > h.off_descriptors)
        throw std::runtime_error("L3MP IDX section overflow");
    if (uint64_t(h.off_descriptors) + uint64_t(h.stats.descriptor_count) * kDescriptorBytes > h.off_contact_lists)
        throw std::runtime_error("L3MP descriptor section overflow");
    if (uint64_t(h.off_contact_lists) + uint64_t(h.stats.contact_list_count) * kContactListBytes > h.off_contact_groups)
        throw std::runtime_error("L3MP contact-list section overflow");
    if (uint64_t(h.off_contact_groups) + uint64_t(h.stats.contact_group_count) * kContactGroupBytes > h.off_contact_primitives)
        throw std::runtime_error("L3MP contact-group section overflow");
    if (uint64_t(h.off_contact_primitives) + uint64_t(h.stats.contact_primitive_count) * kContactPrimitiveBytes > h.file_bytes)
        throw std::runtime_error("L3MP contact-primitive section overflow");

    out.areas.reserve(h.stats.area_count);
    for (uint32_t i = 0; i < h.stats.area_count; ++i) {
        const size_t p = size_t(h.off_areas) + size_t(i) * kAreaBytes;
        RuntimeMapArea a;
        a.start_x = rd_s16(&b[p + 0]);
        a.start_z = rd_s16(&b[p + 2]);
        a.width = rd_u16(&b[p + 4]);
        a.height = rd_u16(&b[p + 6]);
        a.cell_first = rd_u32(&b[p + 8]);
        a.cell_count = rd_u32(&b[p + 12]);
        a.world_origin_x = static_cast<int32_t>(rd_u32(&b[p + 16]));
        a.world_origin_z = static_cast<int32_t>(rd_u32(&b[p + 20]));
        if (uint64_t(a.cell_first) + a.cell_count > h.stats.cell_count)
            throw std::runtime_error("L3MP area cell range invalid");
        if (uint64_t(a.width) * a.height != a.cell_count)
            throw std::runtime_error("L3MP area dimensions/cell count mismatch");
        out.areas.push_back(a);
    }

    out.cells.reserve(h.stats.cell_count);
    for (uint32_t i = 0; i < h.stats.cell_count; ++i)
        out.cells.push_back(rd_s16(&b[size_t(h.off_cells) + size_t(i) * 2u]));

    out.idx.reserve(h.stats.idx_count);
    for (uint32_t i = 0; i < h.stats.idx_count; ++i) {
        const size_t p = size_t(h.off_idx) + size_t(i) * kIdxBytes;
        RuntimeMapIdx r;
        r.x = rd_s16(&b[p + 0]); r.y = rd_s16(&b[p + 2]); r.z = rd_s16(&b[p + 4]);
        r.raw6 = rd_s16(&b[p + 6]); r.descriptor_index = rd_u16(&b[p + 8]);
        if (rd_u16(&b[p + 10]) != 0u) throw std::runtime_error("L3MP IDX reserved field nonzero");
        if (r.descriptor_index != 0xFFFFu && r.descriptor_index >= h.stats.descriptor_count)
            throw std::runtime_error("L3MP IDX descriptor index invalid");
        out.idx.push_back(r);
    }

    out.descriptors.reserve(h.stats.descriptor_count);
    for (uint32_t i = 0; i < h.stats.descriptor_count; ++i) {
        const size_t p = size_t(h.off_descriptors) + size_t(i) * kDescriptorBytes;
        RuntimeMapDescriptor d;
        d.word0 = rd_u32(&b[p]);
        d.alt_contact_index = rd_u16(&b[p + 4]);
        d.default_contact_index = rd_u16(&b[p + 6]);
        if (d.alt_contact_index != 0xFFFFu && d.alt_contact_index >= h.stats.contact_list_count)
            throw std::runtime_error("L3MP alt contact index invalid");
        if (d.default_contact_index != 0xFFFFu && d.default_contact_index >= h.stats.contact_list_count)
            throw std::runtime_error("L3MP default contact index invalid");
        for (size_t w = 0; w < d.raw_words3_5.size(); ++w) d.raw_words3_5[w] = rd_s16(&b[p + 8 + w * 2]);
        for (size_t r = 0; r < d.render_offsets.size(); ++r) d.render_offsets[r] = rd_u32(&b[p + 20 + r * 4]);
        out.descriptors.push_back(d);
    }

    out.contact_lists.reserve(h.stats.contact_list_count);
    for (uint32_t i = 0; i < h.stats.contact_list_count; ++i) {
        const size_t p = size_t(h.off_contact_lists) + size_t(i) * kContactListBytes;
        RuntimeContactList l{rd_u32(&b[p]), rd_u32(&b[p + 4])};
        if (uint64_t(l.group_first) + l.group_count > h.stats.contact_group_count)
            throw std::runtime_error("L3MP contact list group range invalid");
        out.contact_lists.push_back(l);
    }

    out.contact_groups.reserve(h.stats.contact_group_count);
    for (uint32_t i = 0; i < h.stats.contact_group_count; ++i) {
        const size_t p = size_t(h.off_contact_groups) + size_t(i) * kContactGroupBytes;
        RuntimeContactGroup g;
        g.x0 = rd_s16(&b[p]); g.x1 = rd_s16(&b[p + 2]);
        g.z0 = rd_s16(&b[p + 4]); g.z1 = rd_s16(&b[p + 6]);
        g.primitive_first = rd_u32(&b[p + 8]); g.primitive_count = rd_u32(&b[p + 12]);
        if (uint64_t(g.primitive_first) + g.primitive_count > h.stats.contact_primitive_count)
            throw std::runtime_error("L3MP contact group primitive range invalid");
        out.contact_groups.push_back(g);
    }

    out.contact_primitives.reserve(h.stats.contact_primitive_count);
    for (uint32_t i = 0; i < h.stats.contact_primitive_count; ++i) {
        const size_t p = size_t(h.off_contact_primitives) + size_t(i) * kContactPrimitiveBytes;
        out.contact_primitives.push_back({rd_s16(&b[p]), rd_s16(&b[p + 2]), rd_u16(&b[p + 4]), rd_u16(&b[p + 6])});
    }
    return out;
}

static int32_t floor_div_0x200(int64_t value) {
    // C++ integer division truncates toward zero; map grid lookup needs floor for
    // negative world coordinates.
    if (value >= 0) return static_cast<int32_t>(value / 0x200);
    return static_cast<int32_t>(-(((-value) + 0x1FF) / 0x200));
}

std::optional<RuntimeCellRef> runtime_map_find_cell(const RuntimeMap& map,
                                                    size_t area_index,
                                                    int32_t world_x,
                                                    int32_t world_z) {
    if (area_index >= map.areas.size()) return std::nullopt;
    const auto& a = map.areas[area_index];
    // The retail containing-cell sample at 0x8004B1E0 uses +0x8000.
    // The renderer's (grid * 0x200) - 0x7F00 is the cell center, so using
    // 0x7F00 here incorrectly assigns a half cell on each axis to its neighbor.
    // Widen before adding so malformed/extreme query coordinates remain safe.
    const int32_t gx = floor_div_0x200(int64_t(world_x) + 0x8000);
    const int32_t gz = floor_div_0x200(int64_t(world_z) + 0x8000);
    const int32_t lx = gx - a.start_x;
    const int32_t lz = gz - a.start_z;
    if (lx < 0 || lz < 0 || lx >= a.width || lz >= a.height) return std::nullopt;
    const uint32_t local = uint32_t(lz) * a.width + uint32_t(lx);
    const uint32_t ci = a.cell_first + local;
    if (ci >= map.cells.size()) return std::nullopt;
    RuntimeCellRef out;
    out.area_index = area_index;
    out.grid_x = static_cast<uint16_t>(gx);
    out.grid_z = static_cast<uint16_t>(gz);
    out.cell_index = ci;
    out.packed_cell = map.cells[ci];
    out.world_origin_x = gx * 0x200 - 0x7F00;
    out.world_origin_z = gz * 0x200 - 0x7F00;
    return out;
}

const RuntimeMapIdx* runtime_map_cell_idx(const RuntimeMap& map, const RuntimeCellRef& cell) {
    if (cell.packed_cell <= 0) return nullptr;
    const size_t i = static_cast<uint16_t>(cell.packed_cell);
    return i < map.idx.size() ? &map.idx[i] : nullptr;
}

const RuntimeMapDescriptor* runtime_map_cell_descriptor(const RuntimeMap& map,
                                                        const RuntimeCellRef& cell) {
    const auto* idx = runtime_map_cell_idx(map, cell);
    if (!idx || idx->descriptor_index == 0xFFFFu || idx->descriptor_index >= map.descriptors.size()) return nullptr;
    return &map.descriptors[idx->descriptor_index];
}


std::optional<uint16_t> runtime_map_cell_contact_list_index(const RuntimeMap& map,
                                                             const RuntimeCellRef& cell) {
    const auto* idx = runtime_map_cell_idx(map, cell);
    const auto* desc = runtime_map_cell_descriptor(map, cell);
    if (!idx || !desc) return std::nullopt;
    const bool use_alt = (static_cast<uint16_t>(idx->raw6) & 0x0FFFu) != 0u;
    const uint16_t li = use_alt ? desc->alt_contact_index : desc->default_contact_index;
    if (li == 0xFFFFu || li >= map.contact_lists.size()) return std::nullopt;
    return li;
}

const RuntimeContactList* runtime_map_cell_contact_list(const RuntimeMap& map,
                                                        const RuntimeCellRef& cell) {
    const auto li = runtime_map_cell_contact_list_index(map, cell);
    return li ? &map.contact_lists[*li] : nullptr;
}

std::vector<RuntimeContactGroupRef> runtime_map_cell_contact_groups(const RuntimeMap& map,
                                                                    const RuntimeCellRef& cell) {
    std::vector<RuntimeContactGroupRef> out;
    const auto li = runtime_map_cell_contact_list_index(map, cell);
    if (!li) return out;
    const auto& list = map.contact_lists[*li];
    out.reserve(list.group_count);
    for (uint32_t i = 0; i < list.group_count; ++i) {
        const uint32_t gi = list.group_first + i;
        if (gi >= map.contact_groups.size()) throw std::runtime_error("runtime contact group range escaped validated list");
        const auto& g = map.contact_groups[gi];
        RuntimeContactGroupRef r;
        r.contact_list_index = *li;
        r.group_index = gi;
        r.world_x0 = cell.world_origin_x + g.x0;
        r.world_x1 = cell.world_origin_x + g.x1;
        r.world_z0 = cell.world_origin_z + g.z0;
        r.world_z1 = cell.world_origin_z + g.z1;
        r.primitive_first = g.primitive_first;
        r.primitive_count = g.primitive_count;
        r.primitives.reserve(g.primitive_count);
        for (uint32_t pi = 0; pi < g.primitive_count; ++pi) {
            const uint32_t primitive_index = g.primitive_first + pi;
            if (primitive_index >= map.contact_primitives.size()) {
                throw std::runtime_error("runtime contact primitive range escaped validated group");
            }
            RuntimeContactPrimitiveRef pr;
            pr.primitive_index = primitive_index;
            pr.primitive = map.contact_primitives[primitive_index];
            pr.dispatch_family = runtime_contact_dispatch_family(pr.primitive.code);
            r.primitives.push_back(pr);
        }
        out.push_back(std::move(r));
    }
    return out;
}
std::vector<RuntimeCellRef> runtime_map_collect_contact_candidates_8004b1e0(
    const RuntimeMap& map,
    size_t area_index,
    int16_t query_x,
    int16_t query_z,
    bool include_zero) {
    std::vector<RuntimeCellRef> out;
    if (area_index >= map.areas.size()) return out;
    const auto& a = map.areas[area_index];
    if (a.width == 0 || a.height == 0) return out;

    // Retail builds four signed-halfword offset pairs on the stack. 0x8000
    // samples the containing cell. Near either 0x200-cell edge it chooses
    // 0x7E00 (previous cell) or 0x8200 (next cell) for the neighbor sample.
    const uint16_t qx_u = static_cast<uint16_t>(query_x);
    const uint16_t qz_u = static_cast<uint16_t>(query_z);
    const uint16_t nx = ((qx_u & 0x01FFu) < 0x0101u) ? 0x7E00u : 0x8200u;
    const uint16_t nz = ((qz_u & 0x01FFu) < 0x0101u) ? 0x7E00u : 0x8200u;
    const std::array<std::array<uint16_t,2>,4> offsets{{
        {{0x8000u,0x8000u}}, {{nx,0x8000u}}, {{0x8000u,nz}}, {{nx,nz}}
    }};

    std::vector<int16_t> positive_values;
    positive_values.reserve(4);
    out.reserve(4);
    for (const auto& off : offsets) {
        // Floor division matches the retail arithmetic shift, including a
        // previous-cell sample below grid zero, without signed-shift behavior.
        const int32_t gx = floor_div_0x200(int64_t(query_x) + off[0]);
        const int32_t gz = floor_div_0x200(int64_t(query_z) + off[1]);
        const int32_t lx = gx - static_cast<int32_t>(a.start_x);
        const int32_t lz = gz - static_cast<int32_t>(a.start_z);
        // sltu after subtraction in retail is equivalent to this signed bounds
        // check because negative values become large unsigned values.
        if (lx < 0 || lz < 0 || lx >= static_cast<int32_t>(a.width) || lz >= static_cast<int32_t>(a.height)) continue;
        const uint32_t local = static_cast<uint32_t>(lz) * a.width + static_cast<uint32_t>(lx);
        const uint32_t ci = a.cell_first + local;
        if (ci >= map.cells.size()) throw std::runtime_error("runtime map candidate escaped validated area cell range");
        const int16_t packed = map.cells[ci];
        if (packed == 0 && !include_zero) continue;
        if (packed > 0 && std::find(positive_values.begin(), positive_values.end(), packed) != positive_values.end()) continue;
        if (packed > 0) positive_values.push_back(packed);

        RuntimeCellRef r;
        r.area_index = area_index;
        r.grid_x = static_cast<uint16_t>(gx);
        r.grid_z = static_cast<uint16_t>(gz);
        r.cell_index = ci;
        r.packed_cell = packed;
        r.world_origin_x = gx * 0x200 - 0x7F00;
        r.world_origin_z = gz * 0x200 - 0x7F00;
        out.push_back(r);
    }
    return out;
}

std::optional<RuntimeContactDispatchFamily> runtime_contact_dispatch_family(uint16_t code) {
    const uint16_t family = static_cast<uint16_t>((code & 0x00F0u) >> 4);
    if (family > 7u) return std::nullopt;
    return static_cast<RuntimeContactDispatchFamily>(family);
}

static int16_t wrap_s16(int32_t v) { return static_cast<int16_t>(static_cast<uint16_t>(v)); }

uint8_t runtime_contact_family01_classify_8004e304(const RuntimeContactGroupRef& group,
                                                    const RuntimeContactFamily01Query& query) {
    // The retail routine first mutates its four signed-16 group coordinates by
    // subtracting the four motion-query offsets.  Preserve PS1 halfword wrap.
    const int16_t x0 = wrap_s16(group.world_x0 - query.subtract_x0);
    const int16_t x1 = wrap_s16(group.world_x1 - query.subtract_x1);
    const int16_t z0 = wrap_s16(group.world_z0 - query.subtract_z0);
    const int16_t z1 = wrap_s16(group.world_z1 - query.subtract_z1);
    if (!(x0 < query.query_x && query.query_x < x1 &&
          z0 < query.query_z && query.query_z < z1)) return 0;

    const int32_t dx = static_cast<int32_t>(x1) - x0;
    const int32_t dz = static_cast<int32_t>(z1) - z0;
    if (dz == 0) return 0; // unreachable for a retail-valid group; avoids host UB/trap.
    const int32_t first = x0 + ((static_cast<int32_t>(query.query_z) - z0) * dx) / dz;
    const int32_t second = x0 + ((static_cast<int32_t>(z1) - query.query_z) * dx) / dz;
    const uint8_t b0 = query.query_x < first ? 1u : 0u;
    const uint8_t b1 = query.query_x < second ? 1u : 0u;
    return static_cast<uint8_t>(1u + (b0 << 1u) + b1);
}

}
