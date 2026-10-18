#include "legends3ds/class20_03_hit.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <tuple>

namespace { bool measure = false; size_t allocations = 0; }
void* operator new(std::size_t size) {
    if (measure) ++allocations;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using State = l3::Class20_03CallbackState;
using Inputs = l3::Class20_03HitInputs;
using Status = l3::Class20_03HitStatus;
auto scalar(const State& s) {
    const auto& b = s.behavior;
    return std::make_tuple(s.health_6e,s.previous_health_b2,b.flags_328,s.work_332,
        b.parameter_03,b.action_09,b.phase_0a,s.aux_0b,b.heading_56,b.work_36c,
        s.work_15e,s.raw_14,s.fixed_1c,s.packet_33c,s.previous_raw_2c,s.previous_fixed_34,
        s.angle_54,s.previous_angle_5c,s.variant_32e,s.work_370,s.child_374);
}
State seed(const std::array<int64_t,24>& row) {
    State s; s.health_6e=static_cast<int16_t>(row[1]); s.previous_health_b2=-73;
    s.behavior.flags_328=static_cast<uint16_t>(row[2]); s.work_332=static_cast<int16_t>(row[3]);
    s.variant_32e=static_cast<int16_t>(row[4]); s.behavior.action_09=static_cast<uint8_t>(row[5]);
    s.behavior.phase_0a=static_cast<uint8_t>(row[6]); s.aux_0b=static_cast<uint8_t>(row[7]);
    s.behavior.heading_56=static_cast<uint16_t>(row[8]); s.behavior.work_36c=-117;
    s.behavior.parameter_03=255; s.work_15e=16912; s.packet_33c=static_cast<uint32_t>(row[0]);
    s.raw_14={{101,-7,-202,1234}}; s.fixed_1c={{123456,-76543,999,0x13579}};
    s.previous_raw_2c={{19,21,-91,711}}; s.previous_fixed_34={{333,-555,222,777}};
    s.angle_54={{13,19,29,39}}; s.previous_angle_5c={{11,21,31,41}};
    s.work_370=0xA5A5F00D; s.child_374=0x800A1111;
    return s;
}
Inputs input(const std::array<int64_t,24>& row) {
    Inputs in; in.packet_33c=static_cast<uint32_t>(row[0]);
    in.selected_record=l3::Class20_03HitRecordByte09{static_cast<int16_t>(row[4]),static_cast<uint8_t>(row[11])};
    in.hit_word_34a=static_cast<uint16_t>(row[10]); in.angle_80031198=static_cast<uint32_t>(row[9]);
    in.sound_consumer_ready=true; return in;
}
void differential(const char* path) {
    std::ifstream file(path); if(!file) throw std::runtime_error("original authored hit fixture unavailable");
    size_t count=0; std::array<int64_t,24> row{};
    while(file>>row[0]) {
        for(size_t i=1;i<row.size();++i) if(!(file>>row[i])) throw std::runtime_error("truncated hit fixture");
        auto state=seed(row); const auto before=state;
        const auto out=l3::class20_03_consume_hit_801041b0(state,input(row));
        assert(out.status==Status::SourceBackedPartial);
        const auto& b=state.behavior;
        const std::array<int64_t,11> actual{{*state.health_6e,*state.previous_health_b2,b.flags_328,
            state.work_332,b.parameter_03,b.action_09,b.phase_0a,state.aux_0b,b.heading_56,b.work_36c,state.work_15e}};
        for(size_t i=0;i<actual.size();++i) assert(actual[i]==row[12+i]);
        assert(state.packet_33c==before.packet_33c && state.raw_14==before.raw_14 &&
            state.fixed_1c==before.fixed_1c && state.previous_raw_2c==before.previous_raw_2c &&
            state.previous_fixed_34==before.previous_fixed_34 && state.angle_54==before.angle_54 &&
            state.previous_angle_5c==before.previous_angle_5c && state.work_370==before.work_370 &&
            state.child_374==before.child_374);
        if(row[23]<0) assert(!out.sound_8001a050 && !out.sound_pointed_words);
        else {
            assert(out.sound_8001a050 && *out.sound_8001a050==row[23] && out.sound_pointed_words);
            const std::array<uint32_t,2> expected{{uint32_t(uint16_t(101))|(uint32_t(uint16_t(-7))<<16),
                uint32_t(uint16_t(-202))|(uint32_t(uint16_t(1234))<<16)}};
            assert(*out.sound_pointed_words==expected);
        }
        ++count;
    }
    assert(file.eof() && count==787);
    std::cout<<count<<" original hit-consumer authored differentials PASS\n";
}
void holds() {
    const std::array<int64_t,24> row{{0x08000010,1,0,0,0,0,23,29,1024,4095,1,127}};
    for(unsigned missing=0;missing<7;++missing) {
        auto state=seed(row); auto in=input(row); Status expected=Status::NeedsPacket;
        switch(missing) {
        case 0: in.packet_33c.reset(); break;
        case 1: ++*in.packet_33c; break;
        case 2: state.health_6e.reset(); expected=Status::NeedsHealth; break;
        case 3: in.selected_record.reset(); expected=Status::NeedsCombatProfile; break;
        case 4: ++in.selected_record->actor_variant_32e; expected=Status::NeedsCombatProfile; break;
        case 5: in.hit_word_34a.reset(); expected=Status::NeedsHitContext; break;
        default: in.angle_80031198.reset(); expected=Status::NeedsAngle; break;
        }
        const auto before=scalar(state);
        assert(l3::class20_03_consume_hit_801041b0(state,in).status==expected && scalar(state)==before);
    }
    auto state=seed(row); auto in=input(row); in.sound_consumer_ready=false;
    const auto before=scalar(state);
    assert(l3::class20_03_consume_hit_801041b0(state,in).status==Status::NeedsSoundConsumer && scalar(state)==before);
    // No low24 field is a real no-op, including unknown HP/record/angle/audio.
    state.packet_33c=0x38000000; state.health_6e.reset(); in={}; in.packet_33c=state.packet_33c;
    assert(l3::class20_03_consume_hit_801041b0(state,in).status==Status::SourceBackedPartial);
    // Nonzero field without mask3800 emits163 and never guesses HP or stats.
    state.packet_33c=1; in.packet_33c=1; in.sound_consumer_ready=true;
    const auto shield=l3::class20_03_consume_hit_801041b0(state,in);
    assert(shield.status==Status::SourceBackedPartial && shield.sound_8001a050==163 && !state.health_6e);
    // A legitimate selected byte09=0 remains available, never falsey/unknown.
    state=seed(row); in=input(row); in.selected_record->unsigned_byte09=0;
    assert(l3::class20_03_consume_hit_801041b0(state,in).status==Status::SourceBackedPartial);
    std::cout<<"Hit missing/stale evidence and whole-state holds PASS\n";
}
void no_allocations() {
    const std::array<int64_t,24> row{{0x20000001,128,0,0,0,0,23,29,1024,0,0,0}};
    const auto initial=seed(row); const auto in=input(row);
    allocations=0; measure=true;
    for(unsigned i=0;i<4096;++i) { auto state=initial;
        assert(l3::class20_03_consume_hit_801041b0(state,in).status==Status::SourceBackedPartial); }
    measure=false; assert(allocations==0);
    std::cout<<"4096 hit updates allocate zero bytes PASS\n";
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=2) throw std::runtime_error("expected authored original hit fixture path");
    differential(argv[1]); holds(); no_allocations();
}
