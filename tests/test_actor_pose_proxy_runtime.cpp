#include "legends3ds/actor_runtime.h"
#include <cassert>
using namespace l3;
int main() {
    ImportedActorInstance actor;
    actor.source_class=0x20;actor.subtype=3;actor.initializer=0x80102410;
    actor.original_pool_slot=2;actor.raw_scale={{512,512,512}};
    actor.source_dispatch_flag6=3;actor.pose=ActorPoseResource{};
    actor.pose->root_translation={{16,0,-8}};
    actor.source_behavior=Class20_03CallbackState{};
    actor.source_behavior->work_00=3;actor.source_behavior->behavior.flags_328=4;
    actor.source_behavior->raw_14={{100,-128,300,0}};
    ReferenceTables references;references.trig[0]={0,4096};SourceHitProxyBank bank;
    // Resource binding and render state alone never establish source ownership.
    assert(!tick_imported_actor_pose_proxy(actor,references,bank,255).committed&&bank.count==0);
    assert(initialize_imported_actor_fresh_pose_proxy_subset(actor));
    assert(actor.source_proxy_target.source_address==0x8009f108u+2u*0x3c0u+0x33cu);
    assert(!initialize_imported_actor_fresh_pose_proxy_subset(actor));
    const auto before=*actor.source_behavior;
    assert(!tick_imported_actor_pose_proxy(actor,references,bank,{}).committed);
    assert(bank.count==0&&actor.source_behavior->proxy_position_180==before.proxy_position_180);
    const auto tick=tick_imported_actor_pose_proxy(actor,references,bank,255);
    assert(tick.committed&&tick.proxy.registered&&bank.count==1);
    assert(bank.records[0].target==&actor.source_proxy_target);
    assert(actor.source_behavior->proxy_position_180==actor.source_behavior->previous_proxy_position_334);
    actor.source_proxy_target.packet_00=0x80000003u;
    // A divergent packet owner holds rather than discarding a next-frame hit.
    assert(!tick_imported_actor_pose_proxy(actor,references,bank,255).committed&&bank.count==1);
    actor.source_pose_scratch_shift.reset();
    assert(tick_imported_actor_pose_proxy(actor,references,bank,255).pose_status==ActorPoseCollisionStatus::NeedsScratchShift);
    ImportedActorInstance recycled=actor;recycled.source_proxy_bound=false;recycled.original_pool_slot=16;
    assert(!initialize_imported_actor_fresh_pose_proxy_subset(recycled));
}
