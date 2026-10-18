#include "legends3ds/runtime_session.h"
#include <cassert>
#include <iostream>
using namespace l3;
SourceEventRandomTapeDigest digest(const char* s){SourceEventRandomTapeDigest r{};const auto n=[](char c){return c<='9'?c-'0':c-'a'+10;};for(size_t i=0;i<32;++i)r[i]=uint8_t((n(s[i*2])<<4)|n(s[i*2+1]));return r;}
SourceEventRandomTapeProfile authored(){SourceEventRandomTapeProfile p;p.record_count=1;
 p.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");p.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");p.ebd_sha256=digest("ea7239d21db9d06c26a70825588ad7981a980ce798f7352588c5865a0e001d15");p.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");p.semantic_trace_sha256[0]=1;p.global_trace_sha256[0]=2;p.constructor={777,1,2,3,2,1,5};
 auto& r=p.records[0];r.source_actor=0x800c3580;r.actor_state08=1;r.count=6;r.global_ordinal_before=1;r.global_ordinal_after=7;
 const uint32_t ras[]={0x80100dd4,0x80100dec,0x80100e04,0x80100e40,0x80100e58,0x80100e70};for(size_t i=0;i<6;++i)r.calls[i]={ras[i],uint32_t(i+1),i+2};return p;
}
int main(){RuntimeMap map;RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=area.height=2;area.cell_count=4;map.areas.push_back(area);map.cells.assign(4,-32768);L3QolSettings q{};l3_qol_settings_defaults(&q);RuntimeSession s(map,TutorialRuntimeArea{}, {},q);
 const auto p=authored();assert(s.bind_source_scene_rng_replay(p));assert(s.source_scene_rng_replay().admitted());assert(!s.source_recorded_scene_admitted()&&!s.source_event_phase6_context().scratch_half_1f800002);
 auto bad=p;bad.ebd_sha256[0]^=1;assert(!s.bind_source_scene_rng_replay(bad)&&s.source_scene_rng_replay().profile().constructor.scratch_half==777);
 for(unsigned i=0;i<120;++i){RuntimeInputFrame in;in.held_psx_mask=in.pressed_psx_mask=uint16_t(1u<<(i%16));s.tick(in);assert(!s.source_recorded_scene_admitted()&&!s.source_event_phase6_context().scratch_half_1f800002&&s.source_scene_rng_replay().cursor()==0);}
 RuntimeSession copy=s;assert(copy.source_scene_rng_replay().admitted()&&!copy.source_recorded_scene_admitted());assert(!s.transition_to_area(999,{},0)&&s.source_scene_rng_replay().admitted());assert(s.transition_to_area(0,{},0)&&!s.source_scene_rng_replay().admitted());assert(copy.source_scene_rng_replay().admitted());copy.bind_source_event_phase5_profile({});assert(!copy.source_scene_rng_replay().admitted());assert(copy.bind_source_scene_rng_replay(p));copy.reset_player({},0,100,100);assert(!copy.source_scene_rng_replay().admitted()&&!copy.source_recorded_scene_admitted());
 SourceEventPlayerKnown known{};known[0]=known[6]=1;SourceEventDriverContext driver;SourceEventDriverIntentQueue intents;initialize_source_event_driver_fields(driver,intents);s.bind_source_event_consumers(known,driver,intents);const auto before=s.player_position();RuntimeInputFrame move;move.held_psx_mask=16;s.tick(move);assert(s.player_position().x==before.x&&s.player_position().y==before.y&&s.player_position().z==before.z);
 std::cout<<"Session recorded scene RNG: installation never seeds, strict missing owners/copy/transition/rebind/reset, source player gate holds movement PASS\n";
}
