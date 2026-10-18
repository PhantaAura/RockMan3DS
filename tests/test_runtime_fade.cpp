#include "legends3ds/runtime_session.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
static void require(bool value){if(!value){std::fprintf(stderr,"runtime fade integration failed\n");std::abort();}}
static RuntimeSession make_session(){
    RuntimeMap map;RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    return RuntimeSession(std::move(map),TutorialRuntimeArea{}, {},settings);
}
static SourceEventWork phase4(){SourceEventWork w;w.known.fill(1);w.raw[0]=1;w.raw[1]=1;w.raw[4]=4;w.raw[12]=1;return w;}
int main(){
    auto s=make_session();s.bind_source_event_state(phase4(),{},100,0,0);s.tick({});
    require(!s.source_event_result().committed&&s.source_event_work()->phase04()==4);
    require(s.source_fade_intents().count==0);
    // Synthetic profile scalars: original table values are never assumed here.
    s.bind_source_fade_profile({1,3,7});s.tick({});
    require(s.source_event_result().committed&&s.source_event_work()->phase04()==5);
    require(s.source_fade_context().work_80098a68.raw[10]==3);
    require(s.source_fade_intents().count==1&&s.source_event_result().pending_fade_scheduler_intent);
    require(!s.bind_source_fade_profile({1,8,9}));require(s.source_fade_intents().count==1);
    require(s.source_fade_context().work_80098a68.raw[10]==3);
    s.tick({});require(s.source_event_work()->phase04()==5&&s.source_fade_intents().count==1);
    require(!s.source_event_result().committed); // No invented callback completion.
    auto clone=s;clone.update_source_event_globals(100,0,0);clone.tick({});
    require(clone.source_event_result().request&&clone.source_event_result().request->source_function==0x80100efcu);
    require(s.source_fade_context().work_80098a68.raw[10]==3); // Separate owned bytes.
    require(!s.transition_to_location(999,{1,2,3},0));require(s.source_fade_intents().count==1);
    s.register_location(1,s.active_map(),0,TutorialRuntimeArea{});
    require(s.transition_to_location(1,{1,2,3},0));
    require(s.source_fade_intents().count==0&&!s.source_fade_intents().initialized);
    require(!s.source_fade_context().work_80098a68.known[10]);
    s.bind_source_event_state(phase4(),{},100,0,0);s.tick({});require(!s.source_event_result().committed);
    auto saved=clone.make_save_state(0);clone.apply_save_state(saved);
    require(!clone.source_event_work()&&clone.source_fade_intents().count==0);
    auto busy=make_session();busy.bind_source_event_state(phase4(),{},100,0,1);busy.tick({});
    require(busy.source_event_result().committed&&busy.source_event_work()->phase04()==5);
    require(busy.source_fade_intents().count==0); // Exact busy no-op needs no profile.
    auto unknown=make_session();unknown.bind_source_fade_profile({1,3,7});
    unknown.bind_source_event_state(phase4(),{},100,0,std::nullopt);unknown.tick({});
    require(!unknown.source_event_result().committed&&unknown.source_fade_intents().count==0);
}
