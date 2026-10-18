#include "legends3ds/source_hit_proxy.h"
#include "legends3ds/l3map.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

namespace { std::size_t allocations = 0; }
void* operator new(std::size_t n) {
    ++allocations;
    if (auto* p = std::malloc(n)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

using namespace l3;
namespace {
using Row = std::array<std::int64_t, 48>;
SourceHitProxyRegistration registration(const Row& row, std::size_t offset,
                                        SourceHitProxyTarget& target,
                                        std::array<SourceHitProxyOwnerPoint, 24>& owners) {
    SourceHitProxyRegistration input;
    input.pointed_words = {{static_cast<std::uint32_t>(row[offset]),
                            static_cast<std::uint32_t>(row[offset + 1])}};
    input.a1 = static_cast<std::uint32_t>(row[offset + 2]);
    input.a2 = static_cast<std::uint32_t>(row[offset + 3]);
    input.a3 = static_cast<std::uint32_t>(row[offset + 4]);
    input.stack10 = static_cast<std::uint32_t>(row[offset + 5]);
    target.source_address = static_cast<std::uint32_t>(row[offset + 6]);
    input.stack18 = static_cast<std::uint32_t>(row[offset + 7]);
    input.stack1c = static_cast<std::uint32_t>(row[offset + 8]);
    input.stack20 = static_cast<std::uint32_t>(row[offset + 9]);
    input.mask_80098798 = static_cast<std::uint32_t>(row[offset + 10]);
    target.flags_04 = static_cast<std::uint32_t>(row[offset + 11]);
    target.packet_00 = static_cast<std::uint32_t>(row[offset + 12]);
    input.target = &target;
    for (std::size_t i = 0; i < owners.size(); ++i) input.owners_800be478[i] = &owners[i];
    return input;
}
void compare_target(const SourceHitProxyTarget& t, const Row& row, std::size_t offset) {
    if (t.packet_00 != static_cast<std::uint32_t>(row[offset])) {
        std::cerr << "packet mismatch " << t.packet_00 << " != " << row[offset] << " inputs:";
        for (std::size_t i=0;i<27;++i) std::cerr << " " << row[i];
        std::cerr << "\n";
    }
    assert(t.packet_00 == static_cast<std::uint32_t>(row[offset]));
    assert(t.flags_04 == static_cast<std::uint32_t>(row[offset + 1]));
    for (std::size_t i = 0; i < 3; ++i) assert(t.point_08[i] == row[offset + 2 + i]);
    assert(t.word_0e == row[offset + 5]);
}
void put16(std::vector<std::uint8_t>& b, std::size_t off, std::uint16_t v) {
    b[off] = static_cast<std::uint8_t>(v); b[off + 1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t off, std::uint32_t v) {
    put16(b, off, static_cast<std::uint16_t>(v)); put16(b, off + 2, static_cast<std::uint16_t>(v >> 16U));
}
std::vector<std::uint8_t> rules_bytes() {
    std::vector<std::uint8_t> b(32);
    b[0]='L';b[1]='3';b[2]='P';b[3]='R';put16(b,4,1);put16(b,6,24);
    put32(b,8,32);put32(b,12,8);
    for (std::size_t i=0;i<8;++i)b[24+i]=static_cast<std::uint8_t>(i*31);
    put32(b,16,crc32(b.data()+24,8)); return b;
}
void lease_differentials(const char* paired_fixture) {
    auto path=std::string(paired_fixture);
    const auto separator=path.find_last_of("/\\");
    path.replace(separator==std::string::npos?0:separator+1,std::string::npos,
                 "source_hit_proxy_leases_01436.tsv");
    std::ifstream fixture(path);assert(fixture.good());
    std::array<std::int64_t,32> row{};std::size_t cases=0;
    while(fixture>>row[0]) {
        for(std::size_t i=1;i<row.size();++i)assert(static_cast<bool>(fixture>>row[i]));
        std::optional<std::uint32_t> mask=static_cast<std::uint32_t>(row[1]);
        SourceHitProxyTarget target;target.flags_04=static_cast<std::uint32_t>(row[2]);
        target.packet_00=0x12345678;target.point_08={{13,-14,15}};target.word_0e=17;
        std::array<std::uint32_t,24> owners{};
        for(std::size_t i=0;i<owners.size();++i)owners[i]=0x5000+static_cast<std::uint32_t>(i*4);
        const auto before=allocations;
        if(row[0]==0) {
            const auto result=source_hit_proxy_allocate_lease_8003545c(mask,owners,
                static_cast<std::uint32_t>(row[3]),target);
            assert(result.index==row[5]);
            assert(result.status==(row[5]<0?SourceHitProxyStatus::Full:
                SourceHitProxyStatus::SourceBackedPartial));
        } else {
            assert(row[0]==1);
            assert(source_hit_proxy_release_lease_800354e0(mask,static_cast<std::uint32_t>(row[4]))
                ==SourceHitProxyStatus::SourceBackedPartial);
        }
        assert(allocations==before && mask==static_cast<std::uint32_t>(row[6]) &&
            target.flags_04==static_cast<std::uint32_t>(row[7]));
        for(std::size_t i=0;i<owners.size();++i)assert(owners[i]==static_cast<std::uint32_t>(row[8+i]));
        assert((target.packet_00==0x12345678 && target.point_08==std::array<std::int16_t,3>({{13,-14,15}})
            && target.word_0e==17));
        ++cases;
    }
    assert(fixture.eof() && cases==758);
    std::optional<std::uint32_t> unknown;
    std::array<std::uint32_t,24> owners{};SourceHitProxyTarget target;target.flags_04=99;
    assert(source_hit_proxy_allocate_lease_8003545c(unknown,owners,0x3000,target).status
        ==SourceHitProxyStatus::NeedsMask);
    assert((!unknown && target.flags_04==99 && owners==std::array<std::uint32_t,24>{}));
    assert(source_hit_proxy_release_lease_800354e0(unknown,0)==SourceHitProxyStatus::NeedsMask && !unknown);
    std::cout<<cases<<" original lease allocation/release differentials PASS; unknown mask holds; zero allocations\n";
}
void composite_differentials(const char* paired_fixture) {
    auto path=std::string(paired_fixture);const auto separator=path.find_last_of("/\\");
    path.replace(separator==std::string::npos?0:separator+1,std::string::npos,"source_hit_proxy_composite_01440.tsv");
    std::ifstream fixture(path);assert(fixture.good());std::string line;size_t registrations=0,pairs=0;
    const auto packed=[](const std::array<int16_t,3>& p){return std::array<uint32_t,2>{{uint32_t(uint16_t(p[0]))|(uint32_t(uint16_t(p[1]))<<16),uint32_t(uint16_t(p[2]))|(1234u<<16)}};};
    while(std::getline(fixture,line)){
        std::istringstream input(line);char type;input>>type;std::vector<int64_t> r;int64_t value;while(input>>value)r.push_back(value);
        SourceHitProxyBank bank;std::array<SourceHitProxyTarget,2> targets{};std::array<SourceHitProxyOwnerPoint,24> owners{};
        for(size_t i=0;i<24;++i)owners[i].raw_bc_words={{0x11112222u,0x33334444u}};
        if(type=='R'){
            assert(r.size()==26);bank.count=size_t(r[0]);targets[0].source_address=0x1000;targets[0].flags_04=uint32_t(r[2]);
            SourceHitProxyCompositeRegistration in;in.target=&targets[0];in.mask_80098798=uint32_t(r[1]);in.stack10=uint32_t(r[3])<<9;in.stack14=3u<<13;
            in.first_words=packed({{int16_t(r[4]),int16_t(r[5]),int16_t(r[6])}});in.second_words=packed({{int16_t(r[7]),int16_t(r[8]),int16_t(r[9])}});
            in.first_extent=32;in.second_extent=64;in.stack18=16;in.stack20=0x28000008;in.stack28=128;in.midpoint_padding_06=uint16_t(r[10]);
            for(size_t i=0;i<24;++i)in.owners_800be478[i]=&owners[i];const auto before=allocations;
            const auto result=source_hit_proxy_register_composite_8003430c(bank,in);assert(allocations==before);
            assert((result.status==SourceHitProxyStatus::SourceBackedPartial)==bool(r[11]));assert(bank.count==size_t(r[12])&&targets[0].flags_04==uint32_t(r[13]));
            if(r[11])for(size_t i=0;i<2;++i){const auto& row=bank.records[size_t(r[0])+i];
                assert((uint32_t(uint16_t(row.point[0]))|(uint32_t(uint16_t(row.point[1]))<<16))==uint32_t(r[14+i*3]));
                assert((uint32_t(uint16_t(row.point[2]))|(uint32_t(row.encoded_extent)<<16))==uint32_t(r[15+i*3]));
                assert(row.encoded_identity==uint32_t(r[16+i*3])&&row.parallel_word==uint32_t(r[20+i]));}
            assert(owners[0].raw_bc_words[0]==uint32_t(r[22])&&owners[0].raw_bc_words[1]==uint32_t(r[23])&&owners[23].raw_bc_words[0]==uint32_t(r[24])&&owners[23].raw_bc_words[1]==uint32_t(r[25]));
            if(r[11]&&(uint32_t(r[1])&uint32_t(r[2])&0xffffff00u)){
                SourceHitProxyBank denied;denied.count=size_t(r[0]);targets[0].flags_04=uint32_t(r[2]);const auto saved_owners=owners;auto missing=in;missing.midpoint_padding_06.reset();
                assert(source_hit_proxy_register_composite_8003430c(denied,missing).status==SourceHitProxyStatus::NeedsOwnerPointPadding&&denied.count==size_t(r[0])&&targets[0].flags_04==uint32_t(r[2]));
                for(size_t i=0;i<24;++i)assert(owners[i].raw_bc_words==saved_owners[i].raw_bc_words);
            }
            ++registrations;
        }else{
            assert(type=='P'&&r.size()==36);
            for(size_t i=0;i<2;++i){auto& target=targets[i];target.source_address=0x1000+uint32_t(i*0x100);target.point_08={{0x5678,0x1234,0x1234}};target.word_0e=0x4321;
                const size_t offset=i?8:2;const auto first=packed({{int16_t(r[offset]),int16_t(r[offset+1]),int16_t(r[offset+2])}});
                const auto second=packed({{int16_t(r[offset+3]),int16_t(r[offset+4]),int16_t(r[offset+5])}});
                if(r[i]){SourceHitProxyCompositeRegistration in;in.target=&target;in.mask_80098798=0;in.first_words=first;in.second_words=second;in.first_extent=in.second_extent=uint32_t(r[14+i]);in.stack10=uint32_t(r[i])<<9;in.stack14=i?(3u<<13):0;in.stack18=i?16:32;in.stack20=i?0x28000008u:0x08000001u;in.stack24=uint32_t(r[16+i]);in.stack28=128;assert(source_hit_proxy_register_composite_8003430c(bank,in).status==SourceHitProxyStatus::SourceBackedPartial);}
                else {SourceHitProxyRegistration in;in.target=&target;in.mask_80098798=0;in.pointed_words=first;in.a1=uint32_t(r[14+i]);in.a3=i?(3u<<13):0;in.stack10=i?16:32;in.stack18=i?0x28000008u:0x08000001u;in.stack1c=uint32_t(r[16+i]);in.stack20=128;assert(source_hit_proxy_register_800341bc(bank,in).status==SourceHitProxyStatus::SourceBackedPartial);}}
            SourceHitProxyRules rules;rules.unsigned_80097d00.fill(uint8_t(r[18]));SourceHitProxyCompositeScratch scratch;scratch.first_record=0;scratch.second_record=r[0]?2:1;scratch.first_unowned_second_point=std::array<int16_t,3>{{int16_t(r[19]),int16_t(r[20]),int16_t(r[21])}};
            if(r[0]==0&&r[1]==1&&(uint32_t(r[17])&0x00800000u)){
                const auto count_before=bank.count;const auto saved_targets=targets;
                assert(source_hit_proxy_consume_pairs_8003450c(bank,rules).status==SourceHitProxyStatus::NeedsCompositePointScratch&&bank.count==count_before);
                for(size_t i=0;i<2;++i)assert(targets[i].packet_00==saved_targets[i].packet_00&&targets[i].flags_04==saved_targets[i].flags_04&&targets[i].point_08==saved_targets[i].point_08&&targets[i].word_0e==saved_targets[i].word_0e);
            }
            const auto before=allocations;const auto result=source_hit_proxy_consume_pairs_8003450c(bank,rules,&scratch);assert(allocations==before);
            if(result.status!=SourceHitProxyStatus::SourceBackedPartial)std::cerr<<"composite case "<<pairs<<" held "<<unsigned(result.status)<<'\n';assert(result.status==SourceHitProxyStatus::SourceBackedPartial);
            for(size_t i=0;i<2;++i){const size_t o=22+i*6;const auto& t=targets[i];if(t.packet_00!=uint32_t(r[o]))std::cerr<<"composite packet "<<pairs<<" target "<<i<<" actual="<<t.packet_00<<" expected="<<r[o]<<'\n';assert(t.packet_00==uint32_t(r[o])&&t.flags_04==uint32_t(r[o+1]));for(size_t k=0;k<3;++k)assert(t.point_08[k]==r[o+2+k]);assert(t.word_0e==uint16_t(r[o+5]));}
            assert(bank.count==size_t(r[34])&&result.admitted_pairs==size_t(r[35]));++pairs;
        }
    }
    assert(registrations==6&&pairs==15);std::cout<<registrations<<" original composite registrations + "<<pairs<<" sampled pair differentials PASS; zero heap\n";
}
}

int main(int argc, char** argv) {
    assert(argc == 2);
    lease_differentials(argv[1]);
    composite_differentials(argv[1]);
    std::ifstream fixture(argv[1]); assert(fixture.good());
    std::string line; std::size_t cases = 0;
    Row first_row{};
    while (std::getline(fixture,line)) {
        std::istringstream stream(line); Row row{};
        for (auto& x : row) assert(static_cast<bool>(stream >> x));
        std::int64_t extra=0;assert(!(stream>>extra));
        if (!cases) first_row = row;
        SourceHitProxyBank bank;
        std::array<SourceHitProxyTarget,2> targets{};
        std::array<SourceHitProxyOwnerPoint,24> owners{};
        for (std::size_t i=0;i<2;++i) {
            auto input=registration(row,i*13,targets[i],owners);
            assert(source_hit_proxy_register_800341bc(bank,input).status == SourceHitProxyStatus::SourceBackedPartial);
            const auto& r=bank.records[i];
            assert((static_cast<std::uint32_t>(static_cast<std::uint16_t>(r.point[0])) |
                   (static_cast<std::uint32_t>(static_cast<std::uint16_t>(r.point[1]))<<16U)) == static_cast<std::uint32_t>(row[27+i*3]));
            assert((static_cast<std::uint32_t>(static_cast<std::uint16_t>(r.point[2])) |
                   (static_cast<std::uint32_t>(r.encoded_extent)<<16U)) == static_cast<std::uint32_t>(row[28+i*3]));
            assert(r.encoded_identity == static_cast<std::uint32_t>(row[29+i*3]));
        }
        SourceHitProxyRules rules;rules.unsigned_80097d00.fill(static_cast<std::uint8_t>(row[26]));
        assert(source_hit_proxy_consume_pairs_8003450c(bank,rules).status==SourceHitProxyStatus::SourceBackedPartial);
        compare_target(targets[0],row,33);compare_target(targets[1],row,39);
        assert(bank.count==static_cast<std::size_t>(row[45]));
        assert(owners[0].raw_bc_words[0]==static_cast<std::uint32_t>(row[46]));
        assert(owners[0].raw_bc_words[1]==static_cast<std::uint32_t>(row[47]));
        ++cases;
    }
    assert(cases==872);
    auto bytes=rules_bytes(); SourceHitProxyRules decoded;
    assert(decode_source_hit_proxy_rules(bytes,decoded));
    assert(decoded.unsigned_80097d00[7]==217);
    for (const auto off : {0U,4U,6U,8U,12U,16U,20U,24U}) {
        auto bad=bytes;bad[off]^=1;SourceHitProxyRules sentinel;sentinel.unsigned_80097d00.fill(99);
        assert(!decode_source_hit_proxy_rules(bad,sentinel));
        assert(sentinel.unsigned_80097d00[0]==99);
    }
    auto truncated=bytes;truncated.pop_back();assert(!decode_source_hit_proxy_rules(truncated,decoded));
    auto oversized=bytes;oversized.push_back(0);assert(!decode_source_hit_proxy_rules(oversized,decoded));

    SourceHitProxyBank bank;
    SourceHitProxyTarget target;
    std::array<SourceHitProxyOwnerPoint,24> owners{};
    auto input=registration(first_row,0,target,owners);
    input.mask_80098798.reset();
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::NeedsMask);
    assert(bank.count==0 && target.flags_04==static_cast<std::uint32_t>(first_row[11]));
    input.mask_80098798=0xffffffffU;target.flags_04=0x800000ffU;input.owners_800be478[0]=nullptr;
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::NeedsOwnerPoint);
    assert(bank.count==0 && target.flags_04==0x800000ffU);
    input.owners_800be478[0]=&owners[0];
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
    assert(target.flags_04==0x80000080U && owners[0].raw_bc_words==input.pointed_words);
    const auto saved=target.packet_00;
    assert(source_hit_proxy_consume_pairs_8003450c(bank,std::nullopt).status==SourceHitProxyStatus::NeedsRules);
    assert(bank.count==1 && target.packet_00==saved);
    SourceHitProxyRules all;all.unsigned_80097d00.fill(255);
    bank.records[0].encoded_extent|=0x200U;
    assert(source_hit_proxy_consume_pairs_8003450c(bank,all).status==SourceHitProxyStatus::NeedsCompositeShape);
    assert(bank.count==1 && target.packet_00==saved);
    bank.records[0].encoded_extent&=static_cast<std::uint16_t>(~0x200U);
    bank.records[0].encoded_identity^=1;
    assert(source_hit_proxy_consume_pairs_8003450c(bank,all).status==SourceHitProxyStatus::NeedsTarget);
    assert(bank.count==1 && target.packet_00==saved);
    bank.count=64;input.target=nullptr;input.mask_80098798.reset();
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::Full);
    bank.count=65;assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::InvalidBank);
    assert(source_hit_proxy_consume_pairs_8003450c(bank,all).status==SourceHitProxyStatus::InvalidBank);

    // Preserve source pair ordering, including one shared packet target.
    bank={};target={};target.source_address=0x1000;
    input=registration(first_row,0,target,owners);input.stack18=0x08000001;input.a3=0;
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
    input.stack10=96;input.stack18=0x10000002;
    assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
    const auto result=source_hit_proxy_consume_pairs_8003450c(bank,all);
    assert(result.admitted_pairs==1 && result.overlapping_pairs==1);
    assert(target.packet_00==0x18000003U);

    // Three original bank entries prove ordered accumulation beyond a pair.
    bank={};std::array<SourceHitProxyTarget,3> three{};
    const std::array<std::uint32_t,3> words{{0x08000001,0x10000002,0x20000003}};
    const std::array<std::uint32_t,3> classes{{32,96,64}};
    for (std::size_t i=0;i<3;++i) {
        three[i].source_address=0x1000U+static_cast<std::uint32_t>(i)*0x100U;
        input.target=&three[i];input.stack10=classes[i];input.stack18=words[i];
        input.pointed_words={{static_cast<std::uint32_t>(i)*32U,0}};
        assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
    }
    assert(source_hit_proxy_consume_pairs_8003450c(bank,all).overlapping_pairs==3);
    assert(three[0].packet_00==0x30000005 && three[1].packet_00==0x28000004 && three[2].packet_00==0x18000003);
    input.pointed_words={{0,0}};
    const auto before=allocations;
    for (int i=0;i<4096;++i) {
        bank={};target={};target.source_address=0x1000;
        input.target=&target;input.stack10=32;input.stack18=0x08000001;
        assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
        input.stack10=96;input.stack18=0x10000002;
        assert(source_hit_proxy_register_800341bc(bank,input).status==SourceHitProxyStatus::SourceBackedPartial);
        assert(source_hit_proxy_consume_pairs_8003450c(bank,all).overlapping_pairs==1);
    }
    assert(allocations==before);
    std::cout << cases << " original-instruction kind0 proxy differentials PASS; malformed/unknown/atomic/alias/capacity guards PASS;4096 allocation-free ticks\n";
}
