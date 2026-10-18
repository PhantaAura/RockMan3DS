#include "legends3ds/actor_pose_runtime.h"
#include "legends3ds/class20_03_proxy.h"
#include "legends3ds/class20_03_hit.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { bool count_allocations = false; size_t allocations = 0; }
void* operator new(std::size_t size) {
    if (count_allocations) ++allocations;
    if (void* result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer,std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer,std::size_t) noexcept { std::free(pointer); }

namespace {
using namespace l3;
struct Row {
    std::vector<int64_t> fields;
    size_t cursor = 0;
    int64_t next() { if (cursor >= fields.size()) throw std::runtime_error("truncated fixture"); return fields[cursor++]; }
    template<class T,size_t N> std::array<T,N> array() {
        std::array<T,N> result{}; for (auto& item : result) item=static_cast<T>(next()); return result;
    }
};
template<class Function> size_t rows(const char* file,Function function) {
    std::ifstream input(file); if (!input) throw std::runtime_error("missing pose/proxy fixture");
    size_t cases = 0; std::string line;
    while (std::getline(input,line)) {
        if (line.empty()) continue;
        Row row; std::istringstream tokens(line); int64_t value=0;
        while (tokens>>value) row.fields.push_back(value);
        function(row); assert(row.cursor==row.fields.size()); ++cases;
    }
    assert(cases); return cases;
}
std::array<uint32_t,2> words(const std::array<int16_t,4>& point) {
    return {{uint32_t(uint16_t(point[0]))|(uint32_t(uint16_t(point[1]))<<16),
             uint32_t(uint16_t(point[2]))|(uint32_t(uint16_t(point[3]))<<16)}};
}
void pose_fixtures(const char* file) {
    const auto cases=rows(file,[](Row& row) {
        ActorPoseCollisionInputs input;
        input.heading_56=static_cast<uint16_t>(row.next());
        const auto trig=row.array<int16_t,2>(),zero=row.array<int16_t,2>();
        input.scratch_shift_1f800074=static_cast<uint8_t>(row.next());input.flags_06=static_cast<uint8_t>(row.next());
        input.scale_158=row.array<int16_t,3>();input.raw_14=row.array<int16_t,3>();
        input.root_translation=row.array<int16_t,3>();input.root_scale_388=row.array<uint8_t,3>();
        input.preserved_186=static_cast<int16_t>(row.next());
        (void)row.next();(void)row.array<int16_t,3>(); // Source child count/rotation premises; root independent.
        const auto expected_point=row.array<int16_t,4>();
        const auto expected_matrix=row.array<int16_t,9>();
        ReferenceTables reference;
        const auto angle=input.heading_56<32768 ? int32_t(input.heading_56) : int32_t(input.heading_56)-65536;
        reference.trig[static_cast<uint32_t>(angle<0 ? -angle : angle)&4095U]={trig[0],trig[1]};
        reference.trig[0]={zero[0],zero[1]}; input.reference=&reference;
        count_allocations=true;
        const auto result=produce_actor_pose_collision_point_8003346c(input);
        count_allocations=false;
        if (result.point_180!=expected_point || result.scaled_heading_160!=expected_matrix) {
            std::cerr<<"pose mismatch heading="<<input.heading_56<<" shift="<<unsigned(*input.scratch_shift_1f800074)<<'\n';
            if(result.point_180)for(size_t i=0;i<4;++i)std::cerr<<(*result.point_180)[i]<<" vs "<<expected_point[i]<<'\n';
            throw std::runtime_error("original/native actor pose collision mismatch");
        }
        assert(result.status==ActorPoseCollisionStatus::SourceBackedPartial);
    });
    std::cout<<"original/native actor pose collision cases "<<cases<<'\n';
}
void tail_fixtures(const char* file) {
    const auto cases=rows(file,[](Row& row) {
        Class20_03CallbackState state;SourceHitProxyBank bank;Class20_03ProxyInputs input;
        SourceHitProxyTarget target;target.source_address=0x133c;
        state.work_00=static_cast<uint8_t>(row.next());state.behavior.flags_328=static_cast<uint16_t>(row.next());
        input.source_work_01=static_cast<uint8_t>(row.next());input.producer_330=static_cast<uint16_t>(row.next());
        state.packet_33c=target.packet_00=static_cast<uint32_t>(row.next());
        target.flags_04=static_cast<uint32_t>(row.next());input.mask_80098798=static_cast<uint32_t>(row.next());
        const auto old_count=bank.count=static_cast<size_t>(row.next());
        input.pose_point_180=row.array<int16_t,4>();input.target_33c=&target;
        state.previous_proxy_position_334={{91,92,93,94}};
        std::array<SourceHitProxyOwnerPoint,24> owners{};
        for(size_t i=0;i<owners.size();++i){owners[i].raw_bc_words={{0xa5a5a5a5,0x5a5a5a5a}};input.owners_800be478[i]=&owners[i];}
        const auto expected_point=row.array<int16_t,4>();const auto expected_packet=static_cast<uint32_t>(row.next());
        const auto expected_flags=static_cast<uint32_t>(row.next());const auto expected_count=static_cast<size_t>(row.next());
        const auto expected_record=row.array<uint32_t,3>();
        const auto expected_owner=row.array<uint32_t,2>();
        count_allocations=true;const auto result=class20_03_finalize_proxy_80102398(state,bank,input);count_allocations=false;
        assert(result.status==Class20_03ProxyStatus::SourceBackedPartial && result.packet_cleared);
        assert(state.previous_proxy_position_334==expected_point && state.packet_33c==expected_packet);
        assert(target.packet_00==expected_packet && target.flags_04==expected_flags && bank.count==expected_count);
        assert(owners[0].raw_bc_words==expected_owner);
        if(bank.count>old_count) {
            const auto& record=bank.records[old_count];
            const std::array<uint32_t,3> actual{{uint32_t(uint16_t(record.point[0]))|(uint32_t(uint16_t(record.point[1]))<<16),
                uint32_t(uint16_t(record.point[2]))|(uint32_t(record.encoded_extent)<<16),record.encoded_identity}};
            assert(actual==expected_record && record.parallel_word==(uint32_t(input.producer_330)|0x04880000));
            assert(result.registered && !result.bank_full && record.target==&target);
        } else assert(!result.registered);
    });
    std::cout<<"original/native class03 outer proxy cases "<<cases<<'\n';
}
void holds_and_chain() {
    ActorPoseCollisionInputs pose;
    assert(produce_actor_pose_collision_point_8003346c(pose).status==ActorPoseCollisionStatus::NeedsReference);
    ReferenceTables reference;reference.trig[0]={0,4096};pose.reference=&reference;
    assert(produce_actor_pose_collision_point_8003346c(pose).status==ActorPoseCollisionStatus::NeedsScratchShift);
    pose.scratch_shift_1f800074=0;
    assert(produce_actor_pose_collision_point_8003346c(pose).status==ActorPoseCollisionStatus::NeedsPreservedHalfword);
    pose.flags_06=64;
    assert(produce_actor_pose_collision_point_8003346c(pose).status==ActorPoseCollisionStatus::NeedsRootScale);
    pose.flags_06=0;pose.preserved_186=-1234;pose.scale_158={{512,512,512}};pose.root_translation={{16,-96,32}};
    const auto produced=produce_actor_pose_collision_point_8003346c(pose);
    assert((produced.point_180==std::array<int16_t,4>{{16,-96,32,-1234}}));
    Class20_03CallbackState state;state.work_00=1;state.behavior.flags_328=14;state.packet_33c=0x08000009;
    state.behavior.parameter_03=255;state.health_6e=100;state.variant_32e=0;
    SourceHitProxyBank bank;SourceHitProxyTarget actor_target;actor_target.source_address=0x133c;actor_target.packet_00=state.packet_33c;
    Class20_03ProxyInputs tail;tail.source_work_01=0x20;tail.target_33c=&actor_target;tail.mask_80098798=255;
    assert(class20_03_finalize_proxy_80102398(state,bank,tail).status==Class20_03ProxyStatus::NeedsPosePoint);
    assert(state.packet_33c==0x08000009 && bank.count==0);
    tail.pose_point_180=produced.point_180;tail.mask_80098798.reset();
    assert(class20_03_finalize_proxy_80102398(state,bank,tail).status==Class20_03ProxyStatus::NeedsMask);
    assert(state.packet_33c==0x08000009 && bank.count==0);
    tail.mask_80098798=0xffffffff;actor_target.flags_04=0x80000000;
    assert(class20_03_finalize_proxy_80102398(state,bank,tail).status==Class20_03ProxyStatus::NeedsOwnerPoint);
    assert(state.packet_33c==0x08000009 && bank.count==0);
    actor_target.flags_04=0;tail.mask_80098798=255;
    assert(class20_03_finalize_proxy_80102398(state,bank,tail).registered);
    assert(state.packet_33c==0 && bank.count==1);
    SourceHitProxyTarget other_target;other_target.source_address=0x2000;
    SourceHitProxyRegistration other;other.pointed_words=words(*produced.point_180);
    other.a1=32;other.a3=0x2000;other.stack10=0x60;other.stack18=0x08000003;
    other.target=&other_target;other.mask_80098798=255;
    assert(source_hit_proxy_register_800341bc(bank,other).status==SourceHitProxyStatus::SourceBackedPartial);
    SourceHitProxyRules rules;rules.unsigned_80097d00.fill(255);
    assert(source_hit_proxy_consume_pairs_8003450c(bank,rules).overlapping_pairs==1 && bank.count==0);
    assert(actor_target.packet_00==0x08000003 && actor_target.word_0e==1);
    state.packet_33c=actor_target.packet_00;
    Class20_03HitInputs hit;hit.packet_33c=state.packet_33c;hit.selected_record=Class20_03HitRecordByte09{0,9};hit.sound_consumer_ready=true;
    const auto consumed=class20_03_consume_hit_801041b0(state,hit);
    assert(consumed.status==Class20_03HitStatus::SourceBackedPartial && consumed.sound_8001a050==160);
    assert(state.health_6e==97 && state.work_332==3 && state.behavior.action_09==6 && state.packet_33c==0x08000003);
    assert(class20_03_finalize_proxy_80102398(state,bank,tail).registered && state.packet_33c==0 && actor_target.packet_00==0);
    const auto before=allocations;count_allocations=true;
    for(size_t i=0;i<4096;++i){bank.count=0;(void)produce_actor_pose_collision_point_8003346c(pose);(void)class20_03_finalize_proxy_80102398(state,bank,tail);}
    count_allocations=false;assert(allocations==before);
    std::cout<<"authored pose→proxy→pair→hit→tail chain; 4096 hot ticks allocate nothing\n";
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=3)throw std::runtime_error("expected pose and proxy fixture paths");
    pose_fixtures(argv[1]);tail_fixtures(argv[2]);assert(allocations==0);holds_and_chain();
}
