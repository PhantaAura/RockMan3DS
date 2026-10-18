#include "legends3ds/pbd_model.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <algorithm>
#include <stdexcept>

namespace l3 {
namespace {
struct RawPart { uint8_t nt=0,nq=0,nv=0,id=0; uint32_t tp=0,qp=0,vp=0; };
static bool ptr_range(uint32_t p,uint32_t base,size_t file_bytes,size_t need){
    return p>=base && uint64_t(p)+need<=uint64_t(base)+file_bytes;
}
static bool tri_code(uint8_t c){ return (c&0xFCu)==0x24u; }
static bool quad_code(uint8_t c){ return (c&0xFCu)==0x2Cu; }
// L3MD v1 stores one RGB triplet per face. Flat untextured/textured polygons
// fit that contract; Gouraud packets require additional per-vertex colors.
// Reject them rather than silently flattening a structurally valid package.
static void require_flat_code(uint8_t code, bool quad){
    if ((code & 0xF8u) != (quad ? 0x28u : 0x20u))
        throw std::runtime_error(quad ? "L3MD unsupported quad color opcode" :
                                      "L3MD unsupported triangle color opcode");
}
static bool validate_candidate(const std::vector<uint8_t>& b,size_t n,uint32_t& base_out){
    if(n==0||n*16>b.size())return false;
    std::vector<RawPart> r(n); uint32_t minp=0xFFFFFFFFu;
    for(size_t i=0;i<n;++i){size_t o=i*16;auto&x=r[i];x.nt=b[o];x.nq=b[o+1];x.nv=b[o+2];x.id=b[o+3];x.tp=rd_u32(&b[o+4]);x.qp=rd_u32(&b[o+8]);x.vp=rd_u32(&b[o+12]);
        const uint8_t cnt[3]={x.nt,x.nq,x.nv};const uint32_t pp[3]={x.tp,x.qp,x.vp};
        for(int k=0;k<3;++k){if((cnt[k]==0)!=(pp[k]==0))return false;if(pp[k])minp=std::min(minp,pp[k]);}
    }
    if(minp==0xFFFFFFFFu||minp<n*16)return false;
    uint32_t base=minp-uint32_t(n*16);
    for(const auto&x:r){
        if(x.nt&&!ptr_range(x.tp,base,b.size(),size_t(x.nt)*68))return false;
        if(x.nq&&!ptr_range(x.qp,base,b.size(),size_t(x.nq)*84))return false;
        if(x.nv&&!ptr_range(x.vp,base,b.size(),size_t(x.nv)*8))return false;
        for(uint32_t j=0;j<x.nt;++j){size_t o=size_t(x.tp-base)+j*68;if(!std::equal(b.begin()+o,b.begin()+o+32,b.begin()+o+32)||!tri_code(b[o+7]))return false;for(int k=0;k<3;++k)if(b[o+64+k]>=x.nv)return false;if(b[o+67]!=0)return false;}
        for(uint32_t j=0;j<x.nq;++j){size_t o=size_t(x.qp-base)+j*84;if(!std::equal(b.begin()+o,b.begin()+o+40,b.begin()+o+40)||!quad_code(b[o+7]))return false;for(int k=0;k<4;++k)if(b[o+80+k]>=x.nv)return false;}
    }
    base_out=base;return true;
}
}
PbdModel parse_player_pbd_mesh(const std::vector<uint8_t>& b){
    size_t chosen=0;uint32_t base=0;for(size_t n=1;n<=64&&n*16<=b.size();++n){uint32_t cb=0;if(validate_candidate(b,n,cb)){if(chosen)throw std::runtime_error("ambiguous PBD mesh header");chosen=n;base=cb;}}
    if(!chosen)throw std::runtime_error("unsupported/unrecognized player P-type PBD");
    PbdModel m;m.inferred_load_base=base;m.parts.reserve(chosen);
    for(size_t i=0;i<chosen;++i){size_t h=i*16;uint8_t nt=b[h],nq=b[h+1],nv=b[h+2],id=b[h+3];uint32_t tp=rd_u32(&b[h+4]),qp=rd_u32(&b[h+8]),vp=rd_u32(&b[h+12]);PbdPart p;p.part_id=id;
        p.vertices.reserve(nv);for(uint32_t j=0;j<nv;++j){size_t o=size_t(vp-base)+j*8;p.vertices.push_back({rd_s16(&b[o]),rd_s16(&b[o+2]),rd_s16(&b[o+4]),rd_u16(&b[o+6])});}
        p.triangles.reserve(nt);for(uint32_t j=0;j<nt;++j){size_t o=size_t(tp-base)+j*68;PbdTri f;f.r=b[o+4];f.g=b[o+5];f.b=b[o+6];f.code=b[o+7];f.uv={b[o+12],b[o+13],b[o+20],b[o+21],b[o+28],b[o+29]};f.clut=rd_u16(&b[o+14]);f.tpage=rd_u16(&b[o+22]);f.index={b[o+64],b[o+65],b[o+66]};p.triangles.push_back(f);}
        p.quads.reserve(nq);for(uint32_t j=0;j<nq;++j){size_t o=size_t(qp-base)+j*84;PbdQuad f;f.r=b[o+4];f.g=b[o+5];f.b=b[o+6];f.code=b[o+7];f.uv={b[o+12],b[o+13],b[o+20],b[o+21],b[o+28],b[o+29],b[o+36],b[o+37]};f.clut=rd_u16(&b[o+14]);f.tpage=rd_u16(&b[o+22]);f.index={b[o+80],b[o+81],b[o+82],b[o+83]};p.quads.push_back(f);}
        m.parts.push_back(std::move(p));
    }return m;
}
std::vector<uint8_t> cook_l3model(const PbdModel&m,L3ModelStats*out){
    for(const auto& p:m.parts){
        for(const auto& f:p.triangles) require_flat_code(f.code,false);
        for(const auto& f:p.quads) require_flat_code(f.code,true);
    }
    L3ModelStats s;s.part_count=m.parts.size();for(const auto&p:m.parts){s.vertex_count+=p.vertices.size();s.triangle_count+=p.triangles.size();s.quad_count+=p.quads.size();}
    std::vector<uint8_t> o;o.reserve(64+s.part_count*32+s.vertex_count*8+s.triangle_count*20+s.quad_count*24);o.insert(o.end(),{'L','3','M','D'});wr_u16(o,kL3ModelVersion);wr_u16(o,64);wr_u32(o,1);wr_u32(o,s.part_count);wr_u32(o,s.vertex_count);wr_u32(o,s.triangle_count);wr_u32(o,s.quad_count);size_t po=o.size();for(int i=0;i<4;++i)wr_u32(o,0);size_t pf=o.size();wr_u32(o,0);size_t pc=o.size();wr_u32(o,0);wr_u32(o,0);wr_u32(o,0);wr_u32(o,0);if(o.size()!=64)throw std::runtime_error("L3MD header size bug");
    uint32_t vs=0,ts=0,qs=0;uint32_t offp=o.size();for(const auto&p:m.parts){o.push_back(p.part_id);o.push_back(0);wr_u16(o,0);wr_u32(o,vs);wr_u32(o,p.vertices.size());wr_u32(o,ts);wr_u32(o,p.triangles.size());wr_u32(o,qs);wr_u32(o,p.quads.size());wr_u32(o,0);vs+=p.vertices.size();ts+=p.triangles.size();qs+=p.quads.size();}
    uint32_t offv=o.size();for(const auto&p:m.parts)for(const auto&v:p.vertices){wr_s16(o,v.x);wr_s16(o,v.y);wr_s16(o,v.z);wr_u16(o,v.pad);}
    uint32_t offt=o.size();for(const auto&p:m.parts)for(const auto&f:p.triangles){for(auto i:f.index)wr_u16(o,i);for(auto x:f.uv)o.push_back(x);wr_u16(o,f.clut);wr_u16(o,f.tpage);o.push_back(f.r);o.push_back(f.g);o.push_back(f.b);o.push_back(f.code);}
    uint32_t offq=o.size();for(const auto&p:m.parts)for(const auto&f:p.quads){for(auto i:f.index)wr_u16(o,i);for(auto x:f.uv)o.push_back(x);wr_u16(o,f.clut);wr_u16(o,f.tpage);o.push_back(f.r);o.push_back(f.g);o.push_back(f.b);o.push_back(f.code);}
    const uint32_t offs[4]={offp,offv,offt,offq};for(int i=0;i<4;++i)patch_u32(o,po+i*4,offs[i]);patch_u32(o,pf,o.size());patch_u32(o,pc,crc32(o.data()+64,o.size()-64));if(out)*out=s;return o;
}
L3ModelHeader inspect_l3model(const std::vector<uint8_t>&b,bool verify){if(b.size()<64||b[0]!='L'||b[1]!='3'||b[2]!='M'||b[3]!='D')throw std::runtime_error("not L3MD");L3ModelHeader h;h.version=rd_u16(&b[4]);h.header_bytes=rd_u16(&b[6]);h.flags=rd_u32(&b[8]);h.stats.part_count=rd_u32(&b[12]);h.stats.vertex_count=rd_u32(&b[16]);h.stats.triangle_count=rd_u32(&b[20]);h.stats.quad_count=rd_u32(&b[24]);h.off_parts=rd_u32(&b[28]);h.off_vertices=rd_u32(&b[32]);h.off_triangles=rd_u32(&b[36]);h.off_quads=rd_u32(&b[40]);h.file_bytes=rd_u32(&b[44]);h.payload_crc32=rd_u32(&b[48]);if(h.version!=1||h.header_bytes!=64)throw std::runtime_error("unsupported L3MD");if(h.file_bytes!=b.size())throw std::runtime_error("L3MD file size mismatch");if(verify&&crc32(b.data()+64,b.size()-64)!=h.payload_crc32)throw std::runtime_error("L3MD CRC mismatch");if(!(h.off_parts>=64&&h.off_parts<=h.off_vertices&&h.off_vertices<=h.off_triangles&&h.off_triangles<=h.off_quads&&h.off_quads<=h.file_bytes))throw std::runtime_error("L3MD bad section offsets");return h;}

PbdModel load_l3model(const std::vector<uint8_t>& b, bool verify) {
    const auto h = inspect_l3model(b, verify);
    constexpr size_t kPartBytes = 32;
    constexpr size_t kVertexBytes = 8;
    constexpr size_t kTriBytes = 20;
    constexpr size_t kQuadBytes = 24;
    if (uint64_t(h.off_parts) + uint64_t(h.stats.part_count) * kPartBytes > h.off_vertices)
        throw std::runtime_error("L3MD part table size mismatch");
    if (uint64_t(h.off_vertices) + uint64_t(h.stats.vertex_count) * kVertexBytes > h.off_triangles)
        throw std::runtime_error("L3MD vertex table size mismatch");
    if (uint64_t(h.off_triangles) + uint64_t(h.stats.triangle_count) * kTriBytes > h.off_quads)
        throw std::runtime_error("L3MD triangle table size mismatch");
    if (uint64_t(h.off_quads) + uint64_t(h.stats.quad_count) * kQuadBytes > h.file_bytes)
        throw std::runtime_error("L3MD quad table size mismatch");

    struct PartMeta {
        uint8_t id = 0;
        uint32_t vf = 0, vc = 0, tf = 0, tc = 0, qf = 0, qc = 0;
    };
    std::vector<PartMeta> metas;
    metas.reserve(h.stats.part_count);
    for (uint32_t i = 0; i < h.stats.part_count; ++i) {
        const size_t o = size_t(h.off_parts) + size_t(i) * kPartBytes;
        if (b[o + 1] != 0 || rd_u16(&b[o + 2]) != 0 || rd_u32(&b[o + 28]) != 0)
            throw std::runtime_error("L3MD part reserved field nonzero");
        PartMeta m;
        m.id = b[o];
        m.vf = rd_u32(&b[o + 4]); m.vc = rd_u32(&b[o + 8]);
        m.tf = rd_u32(&b[o + 12]); m.tc = rd_u32(&b[o + 16]);
        m.qf = rd_u32(&b[o + 20]); m.qc = rd_u32(&b[o + 24]);
        if (uint64_t(m.vf) + m.vc > h.stats.vertex_count ||
            uint64_t(m.tf) + m.tc > h.stats.triangle_count ||
            uint64_t(m.qf) + m.qc > h.stats.quad_count)
            throw std::runtime_error("L3MD part range invalid");
        metas.push_back(m);
    }

    std::vector<PbdVertex> vertices;
    vertices.reserve(h.stats.vertex_count);
    for (uint32_t i = 0; i < h.stats.vertex_count; ++i) {
        const size_t o = size_t(h.off_vertices) + size_t(i) * kVertexBytes;
        vertices.push_back({rd_s16(&b[o]), rd_s16(&b[o + 2]), rd_s16(&b[o + 4]), rd_u16(&b[o + 6])});
    }
    std::vector<PbdTri> tris;
    tris.reserve(h.stats.triangle_count);
    for (uint32_t i = 0; i < h.stats.triangle_count; ++i) {
        const size_t o = size_t(h.off_triangles) + size_t(i) * kTriBytes;
        PbdTri f;
        for (size_t k = 0; k < 3; ++k) f.index[k] = static_cast<uint8_t>(rd_u16(&b[o + k * 2]));
        for (size_t k = 0; k < 6; ++k) f.uv[k] = b[o + 6 + k];
        f.clut = rd_u16(&b[o + 12]); f.tpage = rd_u16(&b[o + 14]);
        f.r = b[o + 16]; f.g = b[o + 17]; f.b = b[o + 18]; f.code = b[o + 19];
        require_flat_code(f.code, false);
        tris.push_back(f);
    }
    std::vector<PbdQuad> quads;
    quads.reserve(h.stats.quad_count);
    for (uint32_t i = 0; i < h.stats.quad_count; ++i) {
        const size_t o = size_t(h.off_quads) + size_t(i) * kQuadBytes;
        PbdQuad f;
        for (size_t k = 0; k < 4; ++k) f.index[k] = static_cast<uint8_t>(rd_u16(&b[o + k * 2]));
        for (size_t k = 0; k < 8; ++k) f.uv[k] = b[o + 8 + k];
        f.clut = rd_u16(&b[o + 16]); f.tpage = rd_u16(&b[o + 18]);
        f.r = b[o + 20]; f.g = b[o + 21]; f.b = b[o + 22]; f.code = b[o + 23];
        require_flat_code(f.code, true);
        quads.push_back(f);
    }

    PbdModel out;
    out.parts.reserve(metas.size());
    for (const auto& m : metas) {
        PbdPart p;
        p.part_id = m.id;
        p.vertices.assign(vertices.begin() + m.vf, vertices.begin() + m.vf + m.vc);
        p.triangles.assign(tris.begin() + m.tf, tris.begin() + m.tf + m.tc);
        p.quads.assign(quads.begin() + m.qf, quads.begin() + m.qf + m.qc);
        for (const auto& f : p.triangles) for (uint8_t idx : f.index)
            if (idx >= p.vertices.size()) throw std::runtime_error("L3MD triangle vertex index invalid");
        for (const auto& f : p.quads) for (uint8_t idx : f.index)
            if (idx >= p.vertices.size()) throw std::runtime_error("L3MD quad vertex index invalid");
        out.parts.push_back(std::move(p));
    }
    return out;
}
}
