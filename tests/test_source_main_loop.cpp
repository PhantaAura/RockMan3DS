#include "legends3ds/source_main_loop.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
unsigned digit(char c){if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);throw std::runtime_error("fixture hex rejected");}
std::array<uint8_t,72> hex(const std::string& text){
    require(text.size()==144,"fixture extent rejected");std::array<uint8_t,72> out{};
    for(size_t i=0;i<out.size();++i)out[i]=uint8_t(digit(text[2*i])*16+digit(text[2*i+1]));return out;
}
bool same(const l3::SourceMainLoopPads& a,const l3::SourceMainLoopPads& b){
    for(size_t port=0;port<2;++port)if(a[port].raw!=b[port].raw||a[port].known!=b[port].known)return false;
    return true;
}
void differential(const std::string& folder){
    std::ifstream file(folder+"source-main-loop-pads.tsv");require(bool(file),"pad fixture missing");
    std::string line;std::getline(file,line);size_t count=0;
    while(std::getline(file,line)){
        unsigned seed;std::string before0,before1,after0,after1,extra;std::istringstream row(line);
        require(bool(row>>seed>>before0>>before1>>after0>>after1)&&!(row>>extra),"pad row rejected");
        l3::SourceMainLoopPads pads;pads[0].raw=hex(before0);pads[1].raw=hex(before1);
        for(auto& pad:pads)pad.known.fill(1);
        const auto allocated=allocations;const auto result=l3::consume_source_main_loop_pads_80012298(pads);
        require(result.committed&&result.status==l3::SourceMainLoopPadStatus::Consumed&&allocations==allocated,
            "source pad consumer failed or allocated");
        require(pads[0].raw==hex(after0)&&pads[1].raw==hex(after1),"unchanged full pad owner differential failed");++count;
    }
    require(count==256,"pad case coverage changed");
    std::ifstream gates(folder+"source-main-loop-gates.tsv");require(bool(gates),"gate fixture missing");std::getline(gates,line);count=0;
    while(std::getline(gates,line)){
        unsigned cadence,ready;uint32_t elapsed;std::istringstream row(line);
        require(bool(row>>cadence>>elapsed>>ready),"gate row rejected");
        const auto status=l3::evaluate_source_main_loop_gate_80011ce8(uint8_t(cadence),elapsed);
        require(status==(ready?l3::SourceMainLoopGateStatus::Ready:l3::SourceMainLoopGateStatus::WaitingForVblank),"unsigned gate differential failed");++count;
    }
    require(count==50,"gate coverage changed");
    std::ifstream phases(folder+"source-main-loop-phase.tsv");require(bool(phases),"phase fixture missing");std::getline(phases,line);count=0;
    while(std::getline(phases,line)){
        int phase,subphase;unsigned ready;std::istringstream row(line);
        require(bool(row>>phase>>subphase>>ready),"phase row rejected");
        const auto result=l3::evaluate_source_main_loop_phase_800155d4(int8_t(phase),int8_t(subphase));
        require((result.status==l3::SourceMainLoopPhaseStatus::ReadyForUpdate)==bool(ready),"phase call boundary differential failed");
        if(ready)require(result.update_pass==0x80015db4&&result.e0_sweep==0x80017528,"phase sweep identity changed");++count;
    }
    require(count==11,"phase coverage changed");
    std::ifstream active_phases(folder+"source-main-loop-phase-active.tsv");require(bool(active_phases),"active-phase fixture missing");std::getline(active_phases,line);count=0;
    while(std::getline(active_phases,line)){
        int phase,subphase,control;unsigned active,ready;std::istringstream row(line);
        require(bool(row>>phase>>subphase>>control>>active>>ready),"active-phase row rejected");
        const auto result=l3::evaluate_source_main_loop_phase_800155d4(int8_t(phase),int8_t(subphase),int8_t(control),uint8_t(active));
        require((result.status==l3::SourceMainLoopPhaseStatus::ReadyForUpdate)==bool(ready)&&result.e0_sweep==0x80017528,"active-event phase gate differential failed");++count;
    }
    require(count==5,"active-event gate coverage changed");
    std::ifstream vblanks(folder+"source-main-loop-vblank.tsv");require(bool(vblanks),"vblank fixture missing");std::getline(vblanks,line);count=0;
    while(std::getline(vblanks,line)){
        uint32_t before,after;std::istringstream row(line);require(bool(row>>before>>after),"vblank row rejected");
        std::optional<uint32_t> word=before;require(l3::consume_source_main_loop_vblank_800123fc(word)&&word==after,"vblank differential failed");++count;
    }
    require(count==9,"vblank coverage changed");
}
void unknown_and_native_packet(){
    l3::SourceMainLoopPads unknown;const auto before=unknown;
    for(unsigned i=0;i<4096;++i){const auto result=l3::consume_source_main_loop_pads_80012298(unknown);
        require(!result.committed&&same(unknown,before),"unknown pad state invented cold initialization");}
    l3::SourceMainLoopPads pads;l3::initialize_source_main_loop_pads_from_executable_boot(pads);
    for(const auto& pad:pads)for(size_t i=0;i<pad.raw.size();++i)require(pad.raw[i]==0&&pad.known[i],"explicit cold clear failed");
    // This is an authored native digital-packet translation, not BIOS input.
    std::array<uint8_t,8> packet{{0,0x41,0xff,0xff,0,0,0,0}};
    l3::bind_source_main_loop_pad_packet(pads[0],packet);l3::bind_source_main_loop_pad_packet(pads[1],packet);
    require(l3::consume_source_main_loop_pads_80012298(pads).committed&&l3::source_main_loop_pad_pressed_800c0c2a(pads[0])==0,"neutral packet failed");
    packet[2]=0xf7;l3::bind_source_main_loop_pad_packet(pads[0],packet);
    require(l3::consume_source_main_loop_pads_80012298(pads).committed&&l3::source_main_loop_pad_pressed_800c0c2a(pads[0])==8,"packet did not produce source Start press predicate");
    require(l3::consume_source_main_loop_pads_80012298(pads).committed&&l3::source_main_loop_pad_pressed_800c0c2a(pads[0])==0,"held Start fabricated another press");
    packet[2]=255;l3::bind_source_main_loop_pad_packet(pads[0],packet);
    require(l3::consume_source_main_loop_pads_80012298(pads).committed&&l3::source_main_loop_pad_pressed_800c0c2a(pads[0])==0&&pads[0].raw[44]==8,"Start release producer failed");
    pads[1].known[37]=0;const auto retained=pads;const auto result=l3::consume_source_main_loop_pads_80012298(pads);
    require(!result.committed&&result.missing_port==1&&result.missing_offset==37&&same(pads,retained),"missing second-port retained owner partially advanced first port");
    for(size_t offset:{size_t(0),size_t(1),size_t(2),size_t(7),size_t(38),size_t(46),size_t(56),size_t(64)}){
        auto missing=retained;missing[1].known[37]=1;missing[0].known[offset]=0;
        if(offset==64)missing[0].raw[56]=1;
        const auto snapshot=missing;const auto held=l3::consume_source_main_loop_pads_80012298(missing);
        require(!held.committed&&same(missing,snapshot),"missing owner failed transactional hold");
    }
    l3::SourceMainLoopScratchOwners scratch;l3::initialize_source_main_loop_scratch_prefix_80012004(scratch);
    require(scratch.byte_1f800000==0&&scratch.byte_1f800001==2&&!scratch.scratch_half_1f800002,"scratch prefix invented half2");
    scratch.scratch_half_1f800002=65535;l3::initialize_source_main_loop_scratch_prefix_80012004(scratch);
    require(scratch.scratch_half_1f800002==65535,"scratch prefix overwrote half2");
    std::optional<uint32_t> missing_vblank;
    require(!l3::consume_source_main_loop_vblank_800123fc(missing_vblank)&&!missing_vblank,"vblank owner invented initial counter");
    require(l3::evaluate_source_main_loop_gate_80011ce8({},0)==l3::SourceMainLoopGateStatus::NeedsCadenceByte&&
        l3::evaluate_source_main_loop_gate_80011ce8(2,{})==l3::SourceMainLoopGateStatus::NeedsVblankWord,"missing cadence owners admitted");
    using Phase=l3::SourceMainLoopPhaseStatus;
    require(l3::evaluate_source_main_loop_phase_800155d4({},3).status==Phase::NeedsMainPhase&&
        l3::evaluate_source_main_loop_phase_800155d4(3,{}).status==Phase::NeedsSubphase&&
        l3::evaluate_source_main_loop_phase_800155d4(2,3).status==Phase::NeedsPhaseConsumer,"unknown or different phase admitted E0 sweep");
    for(int8_t subphase:{int8_t(0),int8_t(2),int8_t(6)})require(l3::evaluate_source_main_loop_phase_800155d4(3,subphase).status==Phase::NeedsBranchOwners,"conditional phase admitted without branch owners");
    require(l3::evaluate_source_main_loop_phase_800155d4(3,2,{},1).status==Phase::NeedsBranchOwners&&
        l3::evaluate_source_main_loop_phase_800155d4(3,2,0,{}).status==Phase::NeedsBranchOwners&&
        l3::evaluate_source_main_loop_phase_800155d4(3,2,1,1).status==Phase::NeedsBranchOwners&&
        l3::evaluate_source_main_loop_phase_800155d4(3,2,0,0).status==Phase::NeedsBranchOwners,"missing/outside active-event fast-path owners admitted update sweep");
}
} // namespace
int main(int argc,char** argv){try{
    require(argc==2,"fixture folder required");std::string folder=argv[1];if(!folder.empty()&&folder.back()!='/')folder+='/';
    differential(folder);unknown_and_native_packet();std::cout<<"source_main_loop passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
