#include "legends3ds/st00_render.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include "legends3ds/tile_transform.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace l3 {
namespace {
uint32_t vaddr24(uint32_t w){return 0x80000000u|(w&0x00FFFFFFu);}
bool in_stg(uint32_t p,size_t n,size_t need=1){return p>=kSt00StgBase&&uint64_t(p)+need<=uint64_t(kSt00StgBase)+n;}
struct PairKey {uint32_t u=0,g=0;bool operator<(const PairKey&o)const{return std::tie(u,g)<std::tie(o.u,o.g);}};
}

St00RenderSource parse_st00_render(const St00Source& src,const std::vector<uint8_t>& stg){
    St00RenderSource out; out.descriptors.resize(src.descriptors.size());
    bool needs_default_negative_descriptor = false;
    for (const auto& area : src.areas) {
        for (int16_t cell : area.cells) {
            if (cell < 0 && (static_cast<uint16_t>(cell) & 0x80u) == 0u) {
                needs_default_negative_descriptor = true;
                break;
            }
        }
        if (needs_default_negative_descriptor) break;
    }
    if (needs_default_negative_descriptor) {
        for (size_t di = 0; di < src.descriptors.size(); ++di) {
            if (src.descriptors[di].source_ptr == kSt00StgBase) {
                if (di >= 0xFFFFu) throw std::runtime_error("default negative descriptor identity exceeds neutral range");
                out.default_negative_descriptor = static_cast<uint16_t>(di);
                break;
            }
        }
        if (out.default_negative_descriptor == 0xFFFFu)
            throw std::runtime_error("default negative descriptor absent from source collection");
    }
    for (size_t selector = 0; selector < src.special_negative_descriptor_ptrs.size(); ++selector) {
        const uint32_t ptr = src.special_negative_descriptor_ptrs[selector];
        if (!ptr) continue;
        for (size_t di = 0; di < src.descriptors.size(); ++di) {
            if (src.descriptors[di].source_ptr == ptr) {
                if (di >= 0xFFFFu) throw std::runtime_error("special negative descriptor identity exceeds neutral range");
                out.special_negative_descriptors[selector] = static_cast<uint16_t>(di);
                break;
            }
        }
        if (out.special_negative_descriptors[selector] == 0xFFFFu)
            throw std::runtime_error("special negative descriptor absent from source collection");
    }
    std::map<uint32_t,uint16_t> record_index; std::map<PairKey,uint32_t> stream_index;
    // Build records in first descriptor-reference order so the result is deterministic without preserving PS1 pointers.
    for(size_t di=0;di<src.descriptors.size();++di){
        for(size_t side=0;side<6;++side){uint32_t rp=src.descriptors[di].render_ptrs[side];if(!rp)continue;
            auto rit=record_index.find(rp);uint16_t ri;
            if(rit==record_index.end()){
                if(out.records.size()>=65535)throw std::runtime_error("too many ST00 render records");
                if(!in_stg(rp,stg.size(),4))throw std::runtime_error("render record header outside STG");
                const size_t ro=rp-kSt00StgBase;const uint8_t mc=stg[ro],pb=stg[ro+1],pc=pb&0x3Fu;
                const size_t bytes=4u+size_t(mc)*4u+size_t(pc)*8u;if(!in_stg(rp,stg.size(),bytes))throw std::runtime_error("render record outside STG");
                SourceRenderRecord rr;rr.source_ptr=rp;rr.flags_high2=pb>>6;rr.byte2=stg[ro+2];rr.byte3=stg[ro+3];rr.materials.reserve(mc);
                for(size_t m=0;m<mc;++m)rr.materials.push_back(rd_u32(stg.data()+ro+4+m*4));
                const size_t po=ro+4u+size_t(mc)*4u;rr.stream_indices.reserve(pc);
                for(size_t j=0;j<pc;++j){const uint32_t uw=rd_u32(stg.data()+po+j*8),gw=rd_u32(stg.data()+po+j*8+4);
                    // Some retail USA records reserve a stream slot with a zero/zero pair.  The pair
                    // carries no primitives and is ignored by the original pointer-driven path.
                    // Preserve strictness for half-null pairs: those are not the proved empty-slot form.
                    if(uw==0u&&gw==0u) continue;
                    if(uw==0u||gw==0u) throw std::runtime_error("render stream has half-null pointer pair");
                    PairKey key{uw,gw};auto sit=stream_index.find(key);uint32_t si;
                    if(sit==stream_index.end()){
                        const uint8_t code=uint8_t(uw>>24),count=uint8_t(gw>>24),slot=uint8_t(code&0x0F),shift=uint8_t(code>>4);
                        if(slot>=mc)throw std::runtime_error("stream material slot exceeds render-record material count");
                        const uint32_t up=vaddr24(uw),gp=vaddr24(gw);
                        if(!in_stg(up,stg.size(),size_t(count)*8u)||!in_stg(gp,stg.size(),size_t(count)*16u)){
                            std::ostringstream os;
                            os << "render stream primitive payload outside STG descriptor=" << di
                               << " side=" << side << " record=0x" << std::hex << rp
                               << " uw=0x" << uw << " gw=0x" << gw
                               << " up=0x" << up << " gp=0x" << gp << std::dec
                               << " count=" << unsigned(count) << " stg_bytes=" << stg.size();
                            throw std::runtime_error(os.str());
                        }
                        SourceRenderStream ss;ss.source_uv_word=uw;ss.source_geom_word=gw;ss.code=code;ss.base_shift=shift;ss.material_slot=slot;ss.primitives.reserve(count);
                        const size_t uo=up-kSt00StgBase,go=gp-kSt00StgBase;
                        for(size_t k=0;k<count;++k){
                            SourceRenderPrimitive p;const size_t q=go+k*16u;p.prefix=rd_u32(stg.data()+q);for(size_t z=0;z<8;++z)p.uv[z]=stg[uo+k*8u+z];
                            // Retain the inherited reversible gather used by the
                            // 0143 fallback. All twelve original bytes survive;
                            // enhanced positive/negative adapters invert this
                            // storage into their source-backed coordinate layout.
                            const int8_t b0=static_cast<int8_t>(stg[q+4]), b1=static_cast<int8_t>(stg[q+5]), b2=static_cast<int8_t>(stg[q+6]);
                            const int8_t b3=static_cast<int8_t>(stg[q+7]), b4=static_cast<int8_t>(stg[q+8]), b5=static_cast<int8_t>(stg[q+9]);
                            const int8_t b6=static_cast<int8_t>(stg[q+10]),b7=static_cast<int8_t>(stg[q+11]),b8=static_cast<int8_t>(stg[q+12]);
                            const int8_t b9=static_cast<int8_t>(stg[q+13]),b10=static_cast<int8_t>(stg[q+14]),b11=static_cast<int8_t>(stg[q+15]);
                            p.local_x={b0,b3,b5,b10}; p.local_z={b1,b6,b8,b11}; p.ignored_raw={b2,b4,b7,b9};
                            ss.primitives.push_back(p);
                        }
                        si=uint32_t(out.streams.size());out.streams.push_back(std::move(ss));stream_index.emplace(key,si);
                    }else si=sit->second;
                    rr.stream_indices.push_back(si);
                }
                ri=uint16_t(out.records.size());out.records.push_back(std::move(rr));record_index.emplace(rp,ri);
            }else ri=rit->second;
            out.descriptors[di].record_indices[side]=ri;
        }
    }
    return out;
}

uint8_t st00_descriptor_slot(St00RenderBank bank, St00Lod lod) {
    return static_cast<uint8_t>(static_cast<uint8_t>(bank) * 3u + static_cast<uint8_t>(lod));
}

St00LodSelection select_st00_primary_lod(uint32_t depth) {
    St00LodSelection out;
    if (depth >= 0x1C21u) out.lod = St00Lod::Far;
    else if (depth >= 0x0AF1u) out.lod = St00Lod::Mid;
    else out.lod = St00Lod::Near;
    out.near_replay_path = depth < 0x0401u;
    out.descriptor_slot = st00_descriptor_slot(St00RenderBank::Primary, out.lod);
    return out;
}

St00CellOrigin st00_cell_origin(uint8_t grid_x, uint8_t grid_z, int16_t packed_cell, const St00Source& source) {
    St00CellOrigin out;
    out.x = (int32_t(grid_x) << 9) - 0x7F00;
    out.z = (int32_t(grid_z) << 9) - 0x7F00;
    if (packed_cell > 0) {
        const size_t index = static_cast<uint16_t>(packed_cell);
        if (index >= source.idx.size()) throw std::runtime_error("positive ST00 cell IDX index out of range");
        out.y = source.idx[index].y;
    } else {
        out.y = -int32_t((static_cast<uint16_t>(packed_cell) & 0x1F00u) >> 1);
    }
    return out;
}

std::vector<ExpandedRenderPrimitive> expand_st00_descriptor_side(const St00RenderSource& source,
                                                                 size_t descriptor_index, size_t side,
                                                                 const St00CellOrigin& origin,
                                                                 uint16_t packed_cell,
                                                                 uint8_t global_scale_shift) {
    if (descriptor_index >= source.descriptors.size()) throw std::runtime_error("ST00 descriptor index out of range");
    if (side >= 6) throw std::runtime_error("ST00 descriptor render side out of range");
    const uint16_t ri = source.descriptors[descriptor_index].record_indices[side];
    if (ri == 0xFFFFu) return {};
    if (ri >= source.records.size()) throw std::runtime_error("ST00 render record index out of range");
    const auto& rec = source.records[ri];
    std::vector<ExpandedRenderPrimitive> out;
    for (uint32_t si : rec.stream_indices) {
        if (si >= source.streams.size()) throw std::runtime_error("ST00 stream index out of range");
        const auto& stream = source.streams[si];
        if (stream.material_slot >= rec.materials.size()) throw std::runtime_error("ST00 stream material slot out of range");
        const uint32_t material = rec.materials[stream.material_slot];
        const uint16_t shift = uint16_t(stream.base_shift) + global_scale_shift;
        if (shift > 15u) throw std::runtime_error("ST00 effective local-coordinate shift is unreasonable");
        for (const auto& prim : stream.primitives) {
            ExpandedRenderPrimitive e;
            e.prefix = prim.prefix; e.material_word = material; e.uv = prim.uv;
            for (size_t v = 0; v < 4; ++v) {
                const auto xz = legends_tile_transform_xz(static_cast<int16_t>(origin.x), static_cast<int16_t>(origin.z),
                                                          prim.local_x[v], prim.local_z[v], static_cast<uint8_t>(shift), packed_cell);
                e.world_x[v] = xz.x; e.world_y[v] = origin.y; e.world_z[v] = xz.z;
            }
            out.push_back(e);
        }
    }
    return out;
}

std::vector<ExpandedRenderPrimitive> expand_st00_descriptor_lod(const St00RenderSource& source,
                                                                size_t descriptor_index, St00RenderBank bank, St00Lod lod,
                                                                const St00CellOrigin& origin,
                                                                uint16_t packed_cell,
                                                                uint8_t global_scale_shift) {
    return expand_st00_descriptor_side(source, descriptor_index, st00_descriptor_slot(bank, lod),
                                       origin, packed_cell, global_scale_shift);
}

std::vector<uint8_t> cook_l3render(const St00RenderSource&s,L3RenderStats*stats_out){
    if (s.default_negative_descriptor != 0xFFFFu && s.default_negative_descriptor >= s.descriptors.size())
        throw std::runtime_error("default negative descriptor index invalid");
    bool has_special_negative_descriptors = false;
    for (uint16_t index : s.special_negative_descriptors) {
        if (index == 0xFFFFu) continue;
        if (index >= s.descriptors.size()) throw std::runtime_error("special negative descriptor index invalid");
        has_special_negative_descriptors = true;
    }
    L3RenderStats st;st.descriptor_count=s.descriptors.size();st.record_count=s.records.size();st.stream_count=s.streams.size();for(const auto&r:s.records){st.stream_references+=r.stream_indices.size();st.material_words+=r.materials.size();}for(const auto&x:s.streams)st.primitive_count+=x.primitives.size();
    std::vector<uint8_t>o;o.reserve(96+st.descriptor_count*12+st.record_count*24+st.material_words*4+st.stream_references*4+st.stream_count*20+st.primitive_count*24);
    o.insert(o.end(),{'L','3','R','G'});wr_u16(o,kL3RenderVersion);wr_u16(o,80);wr_u32(o,7u | (s.default_negative_descriptor != 0xFFFFu ? 8u : 0u) | (has_special_negative_descriptors ? 16u : 0u)); // optional neutral negative bindings
    wr_u32(o,st.descriptor_count);wr_u32(o,st.record_count);wr_u32(o,st.stream_count);wr_u32(o,st.stream_references);wr_u32(o,st.material_words);wr_u32(o,st.primitive_count);
    const size_t po=o.size();for(int i=0;i<6;++i)wr_u32(o,0);const size_t pf=o.size();wr_u32(o,0);const size_t pc=o.size();wr_u32(o,0);wr_u32(o,s.default_negative_descriptor != 0xFFFFu ? s.default_negative_descriptor : 0u);wr_u32(o,0);wr_u32(o,0);if(o.size()!=80)throw std::runtime_error("L3RG header size bug");
    const uint32_t od=o.size();for(const auto&d:s.descriptors)for(uint16_t x:d.record_indices)wr_u16(o,x);
    if (has_special_negative_descriptors) {
        patch_u32(o, 72, static_cast<uint32_t>(o.size()));
        patch_u32(o, 76, 32u);
        for (uint16_t index : s.special_negative_descriptors) wr_u16(o, index);
    }
    uint32_t matcur=0,refcur=0;const uint32_t orc=o.size();for(const auto&r:s.records){wr_u32(o,matcur);wr_u16(o,uint16_t(r.materials.size()));o.push_back(r.flags_high2);o.push_back(r.byte2);o.push_back(r.byte3);o.push_back(0);wr_u16(o,0);wr_u32(o,refcur);wr_u16(o,uint16_t(r.stream_indices.size()));wr_u16(o,0);wr_u32(o,0);matcur+=r.materials.size();refcur+=r.stream_indices.size();}
    const uint32_t om=o.size();for(const auto&r:s.records)for(uint32_t w:r.materials)wr_u32(o,w);
    const uint32_t orf=o.size();for(const auto&r:s.records)for(uint32_t x:r.stream_indices)wr_u32(o,x);
    uint32_t primcur=0;const uint32_t os=o.size();for(const auto&x:s.streams){o.push_back(x.code);o.push_back(x.base_shift);o.push_back(x.material_slot);o.push_back(0);wr_u32(o,primcur);wr_u32(o,x.primitives.size());wr_u32(o,0);wr_u32(o,0);primcur+=x.primitives.size();}
    const uint32_t op=o.size();for(const auto&x:s.streams)for(const auto&p:x.primitives){wr_u32(o,p.prefix);for(uint8_t v:p.uv)o.push_back(v);for(int8_t v:p.local_x)o.push_back(uint8_t(v));for(int8_t v:p.local_z)o.push_back(uint8_t(v));for(int8_t v:p.ignored_raw)o.push_back(uint8_t(v));}
    const uint32_t offs[6]={od,orc,om,orf,os,op};
    for(int i=0;i<6;++i) patch_u32(o,po+i*4,offs[i]);
    patch_u32(o,pf,uint32_t(o.size()));
    patch_u32(o,pc,crc32(o.data()+80,o.size()-80));
    if(stats_out) *stats_out=st;
    return o;
}

L3RenderHeader inspect_l3render(const std::vector<uint8_t>& b, bool verify) {
    if (b.size() < 80 || b[0] != 'L' || b[1] != '3' || b[2] != 'R' || b[3] != 'G')
        throw std::runtime_error("not L3RG");
    L3RenderHeader h;
    h.version = rd_u16(&b[4]); h.header_bytes = rd_u16(&b[6]); h.flags = rd_u32(&b[8]);
    size_t q = 12;
    uint32_t* counts[] = {&h.stats.descriptor_count, &h.stats.record_count, &h.stats.stream_count,
                          &h.stats.stream_references, &h.stats.material_words, &h.stats.primitive_count};
    for (auto* count : counts) { *count = rd_u32(&b[q]); q += 4; }
    uint32_t* offsets[] = {&h.off_descriptors, &h.off_records, &h.off_materials,
                           &h.off_stream_refs, &h.off_streams, &h.off_primitives};
    for (auto* offset : offsets) { *offset = rd_u32(&b[q]); q += 4; }
    h.file_bytes = rd_u32(&b[q]); q += 4; h.payload_crc32 = rd_u32(&b[q]);
    if (h.version != kL3RenderVersion || h.header_bytes != 80)
        throw std::runtime_error("unsupported L3RG");
    if ((h.flags & ~31u) != 0u) throw std::runtime_error("L3RG unknown flags");
    const uint32_t default_index = rd_u32(&b[68]);
    if (h.flags & 8u) {
        if (default_index >= 0xFFFFu || default_index >= h.stats.descriptor_count)
            throw std::runtime_error("L3RG default negative descriptor index invalid");
        h.default_negative_descriptor = static_cast<uint16_t>(default_index);
    } else if (default_index != 0u) {
        throw std::runtime_error("L3RG unbound default descriptor field nonzero");
    }
    h.off_special_negative_descriptors = rd_u32(&b[72]);
    h.special_negative_descriptor_count = rd_u32(&b[76]);
    if (h.flags & 16u) {
        if (h.special_negative_descriptor_count != 32u
            || h.off_special_negative_descriptors != uint64_t(h.off_descriptors) + uint64_t(h.stats.descriptor_count) * 12u
            || uint64_t(h.off_special_negative_descriptors) + 64u != h.off_records
            || uint64_t(h.off_special_negative_descriptors) + 64u > b.size())
            throw std::runtime_error("L3RG special negative descriptor table range invalid");
        for (size_t selector = 0; selector < 32; ++selector) {
            const uint16_t index = rd_u16(&b[size_t(h.off_special_negative_descriptors) + selector * 2u]);
            if (index != 0xFFFFu && index >= h.stats.descriptor_count)
                throw std::runtime_error("L3RG special negative descriptor index invalid");
        }
    } else if (h.off_special_negative_descriptors != 0u || h.special_negative_descriptor_count != 0u) {
        throw std::runtime_error("L3RG unbound special descriptor fields nonzero");
    }
    if (h.file_bytes != b.size()) throw std::runtime_error("L3RG file size mismatch");
    if (!(h.off_descriptors >= 80 && h.off_descriptors <= h.off_records
          && h.off_records <= h.off_materials && h.off_materials <= h.off_stream_refs
          && h.off_stream_refs <= h.off_streams && h.off_streams <= h.off_primitives
          && h.off_primitives <= h.file_bytes)) throw std::runtime_error("L3RG bad offsets");
    if (uint64_t(h.off_descriptors) + uint64_t(h.stats.descriptor_count) * 12u > h.off_records)
        throw std::runtime_error("L3RG descriptor section mismatch");
    if (uint64_t(h.off_records) + uint64_t(h.stats.record_count) * 24u > h.off_materials)
        throw std::runtime_error("L3RG record section mismatch");
    if (uint64_t(h.off_materials) + uint64_t(h.stats.material_words) * 4u > h.off_stream_refs)
        throw std::runtime_error("L3RG material section mismatch");
    if (uint64_t(h.off_stream_refs) + uint64_t(h.stats.stream_references) * 4u > h.off_streams)
        throw std::runtime_error("L3RG ref section mismatch");
    if (uint64_t(h.off_streams) + uint64_t(h.stats.stream_count) * 20u > h.off_primitives)
        throw std::runtime_error("L3RG stream section mismatch");
    if (uint64_t(h.off_primitives) + uint64_t(h.stats.primitive_count) * 24u > h.file_bytes)
        throw std::runtime_error("L3RG primitive section mismatch");
    if (verify && crc32(b.data() + 80, b.size() - 80) != h.payload_crc32)
        throw std::runtime_error("L3RG CRC mismatch");
    return h;
}

St00RenderSource load_l3render(const std::vector<uint8_t>& b,bool verify){
    const auto h=inspect_l3render(b,verify); St00RenderSource out;
    out.default_negative_descriptor = h.default_negative_descriptor;
    if (h.special_negative_descriptor_count) {
        for (size_t selector = 0; selector < out.special_negative_descriptors.size(); ++selector)
            out.special_negative_descriptors[selector] = rd_u16(&b[size_t(h.off_special_negative_descriptors) + selector * 2u]);
    }
    out.descriptors.resize(h.stats.descriptor_count);
    for(uint32_t i=0;i<h.stats.descriptor_count;++i){const size_t p=size_t(h.off_descriptors)+size_t(i)*12u;for(size_t j=0;j<6;++j){const uint16_t ri=rd_u16(&b[p+j*2]);if(ri!=0xFFFFu&&ri>=h.stats.record_count)throw std::runtime_error("L3RG descriptor record index invalid");out.descriptors[i].record_indices[j]=ri;}}

    struct RecordMeta{uint32_t mat_first=0;uint16_t mat_count=0;uint8_t flags=0,b2=0,b3=0;uint32_t ref_first=0;uint16_t ref_count=0;};
    std::vector<RecordMeta> metas; metas.reserve(h.stats.record_count);
    for(uint32_t i=0;i<h.stats.record_count;++i){const size_t p=size_t(h.off_records)+size_t(i)*24u;RecordMeta m;m.mat_first=rd_u32(&b[p]);m.mat_count=rd_u16(&b[p+4]);m.flags=b[p+6];m.b2=b[p+7];m.b3=b[p+8];if(b[p+9]!=0||rd_u16(&b[p+10])!=0||rd_u16(&b[p+18])!=0||rd_u32(&b[p+20])!=0)throw std::runtime_error("L3RG record reserved field nonzero");m.ref_first=rd_u32(&b[p+12]);m.ref_count=rd_u16(&b[p+16]);if(uint64_t(m.mat_first)+m.mat_count>h.stats.material_words||uint64_t(m.ref_first)+m.ref_count>h.stats.stream_references)throw std::runtime_error("L3RG record range invalid");metas.push_back(m);}

    std::vector<uint32_t> all_mats(h.stats.material_words);for(uint32_t i=0;i<h.stats.material_words;++i)all_mats[i]=rd_u32(&b[size_t(h.off_materials)+size_t(i)*4u]);
    std::vector<uint32_t> all_refs(h.stats.stream_references);for(uint32_t i=0;i<h.stats.stream_references;++i){all_refs[i]=rd_u32(&b[size_t(h.off_stream_refs)+size_t(i)*4u]);if(all_refs[i]>=h.stats.stream_count)throw std::runtime_error("L3RG stream reference invalid");}
    out.records.reserve(h.stats.record_count);
    for(const auto&m:metas){SourceRenderRecord r;r.flags_high2=m.flags;r.byte2=m.b2;r.byte3=m.b3;r.materials.assign(all_mats.begin()+m.mat_first,all_mats.begin()+m.mat_first+m.mat_count);r.stream_indices.assign(all_refs.begin()+m.ref_first,all_refs.begin()+m.ref_first+m.ref_count);out.records.push_back(std::move(r));}

    struct StreamMeta{uint8_t code=0,shift=0,mat=0;uint32_t prim_first=0,prim_count=0;};std::vector<StreamMeta> sm;sm.reserve(h.stats.stream_count);
    for(uint32_t i=0;i<h.stats.stream_count;++i){const size_t p=size_t(h.off_streams)+size_t(i)*20u;StreamMeta m;m.code=b[p];m.shift=b[p+1];m.mat=b[p+2];if(b[p+3]!=0||rd_u32(&b[p+12])!=0||rd_u32(&b[p+16])!=0)throw std::runtime_error("L3RG stream reserved field nonzero");m.prim_first=rd_u32(&b[p+4]);m.prim_count=rd_u32(&b[p+8]);if(uint64_t(m.prim_first)+m.prim_count>h.stats.primitive_count)throw std::runtime_error("L3RG primitive range invalid");sm.push_back(m);}
    std::vector<SourceRenderPrimitive> prims;prims.reserve(h.stats.primitive_count);
    for(uint32_t i=0;i<h.stats.primitive_count;++i){const size_t p=size_t(h.off_primitives)+size_t(i)*24u;SourceRenderPrimitive v;v.prefix=rd_u32(&b[p]);for(size_t j=0;j<8;++j)v.uv[j]=b[p+4+j];for(size_t j=0;j<4;++j)v.local_x[j]=static_cast<int8_t>(b[p+12+j]);for(size_t j=0;j<4;++j)v.local_z[j]=static_cast<int8_t>(b[p+16+j]);for(size_t j=0;j<4;++j)v.ignored_raw[j]=static_cast<int8_t>(b[p+20+j]);prims.push_back(v);}
    out.streams.reserve(h.stats.stream_count);for(const auto&m:sm){SourceRenderStream s;s.code=m.code;s.base_shift=m.shift;s.material_slot=m.mat;s.primitives.assign(prims.begin()+m.prim_first,prims.begin()+m.prim_first+m.prim_count);out.streams.push_back(std::move(s));}
    return out;
}
}
