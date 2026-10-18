#include "legends3ds/runtime_session.h"
#include <cassert>
using namespace l3;
namespace {
RuntimeSession make_session() {
    RuntimeMap map;RuntimeMapArea area;
    area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    return RuntimeSession(std::move(map),TutorialRuntimeArea{}, {},settings);
}
void prepare_phase4(RuntimeSession& session) {
    SourceEventPollProfile profile;profile.decoder=1;profile.records.push_back({1540,1,0});
    session.bind_source_event_profile(profile,0);
    assert(session.bind_source_fade_profile({1,1,64})); // authored, not retail constants
    SourceEventWork work;work.known.fill(1);work.raw[0]=1;work.raw[1]=1;
    work.raw[4]=4;work.raw[12]=1;
    session.bind_source_event_state(work,initialize_source_story_flags_8001d8c0(),80,0,0);
    session.tick({});
    assert(session.source_event_work()->phase04()==5);
    assert(session.source_fade_intents().count==1);
}
}
int main() {
    auto session=make_session();prepare_phase4(session);
    // Startup alone is no completion. The native owner consumes the queued
    // callback exactly once and presents the source grayscale/subtract mode.
    const auto first=session.service_source_fade_scheduler();
    assert(first.committed&&first.native_scheduler_acknowledged);
    assert(session.source_fade_intents().count==0);
    assert(session.source_fade_callback_state().running);
    assert(!session.bind_source_fade_profile({1,1,64}));
    const auto image=session.source_fade_presentation();
    assert(image.visible&&image.subtract_from_scene&&image.width==320&&image.height==240);
    // The missing source endpoint byte holds atomically instead of guessing.
    SourceFadeCallbackResult result;
    for(unsigned i=0;i<8;++i) {
        result=session.service_source_fade_scheduler();
        if(!result.committed)break;
    }
    assert(result.status==SourceFadeCallbackStatus::NeedsEndpointHoldByte);
    const auto held=session.source_fade_context().work_80098a68.raw;
    const auto iterations=session.source_fade_callback_state().committed_iterations;
    assert(!session.service_source_fade_scheduler().committed);
    assert(session.source_fade_context().work_80098a68.raw==held);
    assert(session.source_fade_callback_state().committed_iterations==iterations);
    session.bind_source_fade_scheduler_budget(2); // explicit authored owner input
    for(unsigned i=0;i<16&&session.source_fade_callback_state().running;++i)
        result=session.service_source_fade_scheduler();
    assert(result.source_callback_terminated&&!session.source_fade_callback_state().running);
    assert(!session.source_fade_presentation().visible);
    const auto finished=session.source_fade_context().work_80098a68.raw;
    assert(!session.service_source_fade_scheduler().committed);
    assert(session.source_fade_context().work_80098a68.raw==finished);
    auto takeover=make_session();prepare_phase4(takeover);
    takeover.bind_source_fade_scheduler_budget(2);
    takeover.begin_cutscene(1); // unported takeover cannot retain fresh owner
    for(unsigned i=0;i<16;++i) {
        result=takeover.service_source_fade_scheduler();
        if(!result.committed)break;
    }
    assert(result.status==SourceFadeCallbackStatus::NeedsEndpointHoldByte);
    // Rejected transition preserves owners; accepted area replacement and save
    // restore clear them and cannot manufacture a fresh original budget.
    auto pending=make_session();prepare_phase4(pending);
    assert(!pending.transition_to_area(999,{},0));assert(pending.source_fade_intents().count==1);
    pending.apply_save_state(pending.make_save_state(9));
    assert(pending.source_fade_intents().count==0&&!pending.source_fade_callback_state().running);
    assert(!pending.service_source_fade_scheduler().committed);
    auto cleared=make_session();prepare_phase4(cleared);
    assert(cleared.transition_to_area(0,{},0));
    assert(cleared.source_fade_intents().count==0&&!cleared.source_fade_callback_state().running);
}
