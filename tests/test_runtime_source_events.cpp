#include "legends3ds/runtime_session.h"
#include <cassert>
#include <stdexcept>
using namespace l3;
RuntimeSession make_session(){
 RuntimeMap map;RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
 TutorialRuntimeArea runtime;L3QolSettings settings{};l3_qol_settings_defaults(&settings);return RuntimeSession(std::move(map),std::move(runtime),{},settings);
}
int main(){
 auto session=make_session();SourceEventPollProfile profile;profile.decoder=1;profile.records.push_back({1540,1,0});
 session.bind_source_event_profile(profile,0);session.tick({});assert(session.source_event_result().status==SourceEventStepStatus::NeedsInitialWork);
 auto flags=initialize_source_story_flags_8001d8c0();SourceEventWork work;reset_source_event_work_8001d928(work);
 L3PlayerCompatWork player{};assert(apply_source_st00_area_event_prefix(0,0,flags,player).committed);
 session.bind_source_event_state(work,flags,80,0,std::nullopt);session.tick({});
 assert(session.source_event_result().status==SourceEventStepStatus::NeedsBusyHalfword&&session.source_event_work()->raw==work.raw);
 session.update_source_event_globals(80,0,0);session.tick({});assert(session.source_event_result().committed&&session.source_event_work()->phase04()==1);
 const auto phase1=*session.source_event_work();session.tick({});
 assert(session.source_event_result().status==SourceEventStepStatus::NeedsPlayerConsumer80042238&&session.source_event_work()->raw==phase1.raw);
 auto copy=session;copy.set_story_flag(1540,false);assert(session.story_flag(1540)&&!copy.story_flag(1540));
 assert(copy.source_event_flags().known[1540/8]&(1u<<(1540%8)));
 const auto snapshot=session.make_save_state(9);session.apply_save_state(snapshot);
 assert(!session.source_event_work());session.tick({});assert(session.source_event_result().status==SourceEventStepStatus::NeedsInitialWork);
 auto fresh=make_session();fresh.bind_source_event_profile(profile,0);
 assert(fresh.initialize_fresh_source_event_subset_st00());
 assert(fresh.source_event_player_known()[0]&&fresh.source_event_player_known()[0xbd]);
 assert(!fresh.source_event_player_known()[1]&&!fresh.source_event_player_known()[0x174]);
 fresh.tick({});assert(fresh.source_event_work()->phase04()==1);
 fresh.tick({});assert(fresh.source_event_work()->phase04()==2);
 assert(fresh.source_event_driver_intents().count==2&&fresh.source_event_result().driver_receipt_count==2);
 const auto phase2=*fresh.source_event_work();fresh.tick({});
 assert(fresh.source_event_result().status==SourceEventStepStatus::NeedsSourceConsumer&&fresh.source_event_work()->raw==phase2.raw);
 const auto saved=fresh.make_save_state(9);fresh.apply_save_state(saved);
 assert(!fresh.source_event_work()&&!fresh.source_event_player_known()[0xbd]&&fresh.source_event_driver_intents().count==0);
 bool rejected=false;try{copy.bind_source_event_profile(profile,1);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
 copy.tick({});assert(copy.source_event_work()->raw==phase1.raw);
 assert(!copy.transition_to_location(999,{},0));assert(copy.source_event_work()->raw==phase1.raw);
 assert(copy.transition_to_location(0,{},0));copy.tick({});assert(copy.source_event_work()->raw==phase1.raw);
 auto queued=make_session();queued.bind_source_event_profile(profile,0);
 SourceEventPhase2Package package;package.profile={0x20000003u,7,{{0x11223344u,0x55667788u}},true};
 queued.bind_source_event_phase2_package(package);
 assert(queued.initialize_fresh_source_event_subset_st00());
 queued.tick({});queued.tick({});queued.tick({});
 assert(queued.source_event_work()->phase04()==3&&queued.source_event_result().committed);
 assert(queued.source_event_result().command_receipt_count==1);
 assert(queued.source_event_command_queue().count==1);
 const auto request=queued.source_event_command_request();
 assert(request&&request->source_function==0x8001b89cu&&request->words[1]==7);
 const auto phase3=*queued.source_event_work();queued.tick({});
 assert(!queued.source_event_result().committed&&queued.source_event_work()->raw==phase3.raw);
 auto queue_copy=queued;queue_copy.apply_save_state(queue_copy.make_save_state(9));
 assert(queue_copy.source_event_command_queue().count==0&&!queue_copy.source_event_command_request());
 assert(queued.source_event_command_queue().count==1);
}
