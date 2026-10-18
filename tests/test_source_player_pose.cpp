#include "legends3ds/source_player_pose.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>

namespace { bool count_allocations=false;std::size_t allocations=0; }
void* operator new(std::size_t n){if(count_allocations)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);} void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);} void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

namespace {
l3::ReferenceTables reference(){
    l3::ReferenceTables r;
    for(std::size_t i=0;i<r.trig.size();++i){
        const double a=double(i)*6.2831853071795864769/4096.0;
        r.trig[i].sin_q12=static_cast<std::int16_t>(std::lround(std::sin(a)*4096.0));
        r.trig[i].cos_q12=static_cast<std::int16_t>(std::lround(std::cos(a)*4096.0));
    }
    return r;
}
void ordinary_pose(){
    const auto refs=reference();l3::PbdMainSkeleton skeleton;l3::PbdMainPoseBank bank;
    bank.root_translation={{8,-16,24}};
    skeleton.base_translation[2]={{8,0,0}};skeleton.base_translation[3]={{0,16,0}};
    skeleton.base_translation[4]={{0,0,-24}};skeleton.base_translation[5]={{-8,0,0}};
    skeleton.base_translation[6]={{0,-16,0}};skeleton.base_translation[7]={{0,0,24}};
    l3::SourcePlayerPoseInputs input;input.skeleton=&skeleton;input.primary_pose=&bank;input.reference=&refs;
    input.scratch_shift_1f800074=0;input.limb_pose_selector_106=0;
    input.player_position_14={{100,200,-300}};
    const auto before=allocations;count_allocations=true;
    const auto pose=l3::produce_source_player_integer_pose_800325d4(input);
    const auto receipt=l3::source_player_muzzles_800331a0(pose);l3::SourceBusterPlayer player;
    assert(l3::bind_source_player_muzzles_80035724(receipt,player));count_allocations=false;
    assert(allocations==before&&pose.status==l3::SourcePlayerPoseStatus::SourceBackedPartial&&pose.part_count==15);
    assert((pose.parts[0].position_3a8==std::array<std::int16_t,3>{{101,198,-297}}));
    assert((pose.parts[4].position_3a8==std::array<std::int16_t,3>{{102,200,-300}}));
    assert((pose.parts[7].position_3a8==std::array<std::int16_t,3>{{100,196,-294}}));
    assert(player.lanes[0].muzzle_3c8_or_3e0&&player.lanes[1].muzzle_3c8_or_3e0);
    assert(*player.lanes[0].muzzle_3c8_or_3e0==receipt.muzzle_3c8_part4);
    assert(*player.lanes[1].muzzle_3c8_or_3e0==receipt.muzzle_3e0_part7);
}
void rotation_and_guards(){
    const auto refs=reference();l3::PbdMainSkeleton skeleton;l3::PbdMainPoseBank bank;
    bank.root_translation={{0,0,0}};skeleton.base_translation[2]={{0,0,8}};
    l3::SourcePlayerPoseInputs input;input.skeleton=&skeleton;input.primary_pose=&bank;input.reference=&refs;
    input.scratch_shift_1f800074=0;input.limb_pose_selector_106=0;input.heading_56=1024;
    input.player_position_14={{0,0,0}};
    auto pose=l3::produce_source_player_integer_pose_800325d4(input);
    assert(pose.status==l3::SourcePlayerPoseStatus::SourceBackedPartial);
    assert(pose.parts[2].position_3a8[0]==1&&pose.parts[2].position_3a8[2]==0);
    input.limb_pose_selector_106.reset();assert(l3::produce_source_player_integer_pose_800325d4(input).status==l3::SourcePlayerPoseStatus::NeedsLimbPoseSelector);
    input.limb_pose_selector_106=1;assert(l3::produce_source_player_integer_pose_800325d4(input).status==l3::SourcePlayerPoseStatus::NeedsAlternateLimbPose);
    input.limb_pose_selector_106=0;input.scratch_shift_1f800074=3;assert(l3::produce_source_player_integer_pose_800325d4(input).status==l3::SourcePlayerPoseStatus::UnsupportedScratchShift);
    input.scratch_shift_1f800074=0;skeleton.parent[4]=5;assert(l3::produce_source_player_integer_pose_800325d4(input).status==l3::SourcePlayerPoseStatus::InvalidHierarchy);
    l3::SourceBusterPlayer player;const auto before=player;
    l3::SourcePlayerMuzzleReceipt held;held.status=l3::SourcePlayerPoseStatus::NeedsPose;
    assert(!l3::bind_source_player_muzzles_80035724(held,player));
    assert(player.lanes[0].muzzle_3c8_or_3e0==before.lanes[0].muzzle_3c8_or_3e0);
}
}
int main(){ordinary_pose();rotation_and_guards();std::cout<<"PASS source800325D4 integer pose -> part4/part7 Buster muzzles\n";}
