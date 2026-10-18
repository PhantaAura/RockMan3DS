#include "legends3ds/runtime_session.h"
#include <cassert>
#include <iostream>
using namespace l3;
SourceLoopSnapshotDigest digest(const char* text){SourceLoopSnapshotDigest d{};const auto n=[](char c){return c<='9'?c-'0':c-'a'+10;};for(size_t i=0;i<32;++i)d[i]=uint8_t((n(text[i*2])<<4)|n(text[i*2+1]));return d;}
int main(){RuntimeMap map;RuntimeMapArea a;a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;map.areas.push_back(a);map.cells.assign(4,-32768);
 L3QolSettings q{};l3_qol_settings_defaults(&q);RuntimeSession s(map,TutorialRuntimeArea{}, {},q);
 SourceLoopSnapshot snapshot;snapshot.pc=kSourceLoopSnapshotConstructorPc;snapshot.loops=37;snapshot.scratch_half=36;snapshot.cadence=2;snapshot.event_id=1;snapshot.event_phase=5;snapshot.main_phase=3;snapshot.subphase=2;
 snapshot.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");snapshot.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");snapshot.ebd_sha256=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");snapshot.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
 assert(!s.source_loop_snapshot_replay()&&!s.source_loop_snapshot_admitted());assert(s.bind_source_loop_snapshot_replay(snapshot));
 assert(!s.source_event_phase6_context().scratch_half_1f800002);
 auto bad=snapshot;bad.ebd_sha256[0]^=1;assert(!s.bind_source_loop_snapshot_replay(bad)&&s.source_loop_snapshot_replay()->ebd_sha256==snapshot.ebd_sha256);
 s.initialize_source_main_loop_pads_from_executable_boot_800177cc();
 for(unsigned i=0;i<120;++i){RuntimeInputFrame in;in.held_psx_mask=uint16_t(1u<<(i%16));in.pressed_psx_mask=in.held_psx_mask;s.tick(in);assert(!s.source_loop_snapshot_admitted()&&!s.source_event_phase6_context().scratch_half_1f800002&&!s.service_source_main_loop_tail_80011ecc());}
 assert(s.source_main_loop_recorded_phase_gate().status==SourceMainLoopPhaseStatus::NeedsMainPhase);
 RuntimeSession copy=s;assert(copy.source_loop_snapshot_replay()&&!copy.source_loop_snapshot_admitted());
 assert(!s.transition_to_area(999,{},0)&&s.source_loop_snapshot_replay());
 assert(s.transition_to_area(0,{},0)&&!s.source_loop_snapshot_replay()&&!s.source_loop_snapshot_admitted());
 assert(copy.source_loop_snapshot_replay());copy.bind_source_event_phase5_profile({});assert(!copy.source_loop_snapshot_replay());
 assert(copy.bind_source_loop_snapshot_replay(snapshot));copy.reset_player({},0,100,100);assert(!copy.source_loop_snapshot_admitted()&&!copy.source_event_phase6_context().scratch_half_1f800002);
 std::cout<<"runtime recorded loop profile: install never seeds; missing constructor/owner, frame/input isolation, copy/transition/rebind guards PASS\n";
}
