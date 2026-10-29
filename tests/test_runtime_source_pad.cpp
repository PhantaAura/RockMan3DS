#include "legends3ds/runtime_session.h"
#include <cassert>
using namespace l3;
int main(){RuntimeMap map;RuntimeMapArea a;a.start_x=63;a.start_z=63;a.width=2;a.height=2;a.cell_count=4;map.areas.push_back(a);map.cells.assign(4,-32768);
 L3QolSettings q{};l3_qol_settings_defaults(&q);RuntimeSession s(map,TutorialRuntimeArea{}, {},q);
 assert(!s.service_source_main_loop_digital_pad_adapter(0x4000,0).committed);
 assert(!s.source_event_phase6_context().input_800c0c2a);
 s.initialize_source_main_loop_pads_from_executable_boot_800177cc();assert(s.service_source_main_loop_digital_pad_adapter(0,0).committed);
 assert(s.source_event_phase6_context().input_800c0c2a==0);
 RuntimeInputFrame cross;cross.held_psx_mask=0x4000;
 s.tick(cross);assert(s.source_event_phase6_context().input_800c0c2a==0x4000);
 s.tick(cross);assert(s.source_event_phase6_context().input_800c0c2a==0);
 s.tick({});assert(s.source_event_phase6_context().input_800c0c2a==0);
 RuntimeInputFrame pulse;pulse.pressed_psx_mask=8;s.tick(pulse);assert(s.source_event_phase6_context().input_800c0c2a==8);
 assert(!s.source_event_phase6_context().scratch_half_1f800002);
}
