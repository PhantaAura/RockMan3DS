#include "legends3ds/runtime_session.h"
#include "legends3ds/child_runtime.h"
#include <cstdio>
#include <cstdlib>
using namespace l3;
static void require(bool value){if(!value){std::fprintf(stderr,"always-on integration check failed\n");std::abort();}}
RuntimeSession session() {
    RuntimeMap map;RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    return RuntimeSession(std::move(map),TutorialRuntimeArea{}, {},settings);
}
SourceEventCommandPackage authored_package() {
    SourceEventCommandPackage p;p.profile={17,900,true};auto& c=p.cold_context;
    c.byte_800988ec=0;c.byte_8009896c=0;c.byte_800988c0=0;c.byte_80098b42=0;c.byte_80098ab8=0;
    c.half_800ad142=0;c.half_800ad14e=0;c.word_800989b0=0;c.word_800989d8=0;
    c.callback_80091ffc=0;c.callback_80092000=0;c.mix_known.fill(1);c.response_known.fill(1);return p;
}
int main() {
    auto s=session();SourceEventCommandInputs input;input.receipt_count=1;
    require(!s.deliver_source_event_cd_receipts(input));
    require(!s.acknowledge_source_event_cd_mix(0));
    SourceEventPollProfile poll;poll.decoder=1;poll.records.push_back({1540,1,0});s.bind_source_event_profile(poll,0);
    SourceEventPhase2Package p;p.profile={0x20000003,17,{{0x03000000,0x3e0}},true};s.bind_source_event_phase2_package(p);
    s.bind_source_event_command_package(authored_package());require(s.initialize_fresh_source_event_subset_st00());
    require(s.acknowledge_source_event_cd_mix(0));require(!s.acknowledge_source_event_cd_mix(1));
    s.tick({});s.tick({});s.tick({});require(s.source_event_work()->phase04()==3);
    s.tick({});const auto& held=s.source_event_command_result();
    require(!held.committed&&held.pending_request&&held.pending_request->command==14);
    require(s.source_event_command_queue().phase_800989c4==0&&s.source_event_work()->phase04()==3);
    // Authored backend receipt, explicitly not a running native CD backend.
    input.receipts[0].request=*held.pending_request;input.receipts[0].poll_result=1;input.receipts[0].control_result=1;
    input.receipts[0].response_known.fill(1);
    auto independent=s;require(independent.deliver_source_event_cd_receipts(input));independent.tick({});
    require(independent.source_event_command_result().committed&&independent.source_event_cd_receipts_pending()==0);
    require(independent.source_event_command_queue().phase_800989c4==2);
    require(independent.source_event_command_queue().counter_80098828==1);
    require(s.source_event_command_queue().phase_800989c4==0&&s.source_event_cd_receipts_pending()==0);
    independent.tick({});require(!independent.source_event_command_result().committed);
    require(independent.source_event_command_result().pending_request->command==1);
    require(independent.source_event_command_queue().counter_80098828==1);
    require(independent.source_event_work()->phase04()==3); // No timer-generated Read status.
    input.receipt_count=7;require(independent.deliver_source_event_cd_receipts(input));
    input.receipt_count=1;require(!independent.deliver_source_event_cd_receipts(input));
    auto outgoing=independent;
    require(!outgoing.transition_to_location(999,{1,2,3},0));
    require(outgoing.source_event_cd_receipts_pending()==7);
    require(outgoing.source_event_command_queue().counter_80098828==1);
    outgoing.register_location(1,outgoing.active_map(),0,TutorialRuntimeArea{});
    require(outgoing.transition_to_location(1,{1,2,3},0));
    require(outgoing.source_event_cd_receipts_pending()==0);
    require(outgoing.source_event_command_queue().count==0);
    require(!outgoing.source_event_command_result().pending_request);
    require(!outgoing.source_event_command_context().byte_80098ab8);
    require(outgoing.source_event_cd_mix_intents().count==0);
    require(!outgoing.deliver_source_event_cd_receipts(input));
    require(!outgoing.acknowledge_source_event_cd_mix(0));
    outgoing.bind_source_event_profile(poll,0);outgoing.tick({});
    require(!outgoing.source_event_command_result().committed);
    require(!outgoing.source_event_command_result().pending_request);
    auto saved=independent.make_save_state(0);independent.apply_save_state(saved);
    require(independent.source_event_cd_receipts_pending()==0&&!independent.source_event_work());
    require(!independent.deliver_source_event_cd_receipts(input));

    ChildRuntime children;SourceHitProxyOwnerPoint point{{0x00020001,1000}};
    require(children.bind_proxy_owner(0,0x8009cf88,point));require(children.bind_proxy_owner(1,0x8009cf88,point));
    SourceGteRtpsState state;const auto held_lease=children.consume_player_leases(state,{},0xc0000000);
    require(held_lease.status==SourcePlayerProxyStatus::NeedsMatrixTranslation);
    require(children.proxy_owner_point(0)->raw_bc_words==point.raw_bc_words);
    state.projection_controls_bound=true;state.control[26]=256;
    const std::array<uint32_t,8> camera{{4096,0,4096,0,4096,0,0,1000}};
    const auto leased=children.consume_player_leases(state,camera,0xc0000000);
    require(leased.status==SourcePlayerProxyStatus::SourceBackedPartial&&leased.projected==2);
    const std::array<uint32_t,2> zero{};
    require(children.proxy_owner_point(0)->raw_bc_words==zero);
    require(children.proxy_owner_point(1)->raw_bc_words==zero);
    require(children.bind_proxy_owner(0,0x8009cf88,point));
    require(children.proxy_owner_point(1)->raw_bc_words==point.raw_bc_words);
    const auto selected=children.consume_player_leases(state,camera,0x80000000);
    require(selected.status==SourcePlayerProxyStatus::SourceBackedPartial&&selected.projected==1);
    require(children.proxy_owner_point(0)->raw_bc_words==zero);
    require(children.proxy_owner_point(1)->raw_bc_words==zero);
    const auto empty=children.consume_player_leases(state,camera,0xffu);
    require(empty.status==SourcePlayerProxyStatus::SourceBackedPartial&&empty.projected==0);
}
