// Mega Buster firing chain: 0x80040B68 -> 0x80040E9C -> 0x80042F78 lane and
// the 0x800321C8/0x800323FC overlay track, with authored banks and rows (no
// retail data). Frame order follows 0x8003C4EC (lane at 0x8003CA00, then the
// action handler's fire input) and then 0x8003C418 (overlay).
#include "legends3ds/source_player_fire.h"
#include <cassert>
#include <iostream>

using namespace l3;
namespace {
void wr16(L3PlayerCompatWork& w,size_t o,uint16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(v>>8);}
PbdAnimationAction keyed(uint16_t index,size_t keys,bool frames){
    PbdAnimationAction a;a.action_index=index;a.has_s_records=true;a.has_m_pointer=frames;a.m_is_frame_table=frames;
    for(size_t k=0;k<keys;++k){PbdSRecord r;r.word0=uint16_t(0x0100u|k);r.word1=k+1==keys?0xff00u:0;a.s_records.push_back(r);}
    if(frames)for(size_t k=0;k<keys;++k){std::array<uint16_t,kPbdPackedAngleChannels> f{};f[3]=uint16_t(100+k);f[15]=uint16_t(200+k);f[21]=uint16_t(300+k);a.angle_frames.push_back(f);}
    return a;
}
}
int main(){
    PbdAnimation left;left.action_slots=32;
    for(uint16_t i=0;i<4;++i)left.actions.push_back(keyed(i,20,true));
    PbdAnimation main_bank;main_bank.action_slots=32;
    main_bank.actions.push_back(keyed(0,8,false));           // S action 0 (idle keys)
    main_bank.actions.back().has_m_pointer=false;
    { auto m=keyed(1,8,true);m.has_s_records=false;m.s_records.clear();main_bank.actions.push_back(m); } // M action 1
    SourcePlayerAnimationBanks banks;banks.main=&main_bank;banks.left_arm_04=&left;

    SourceBusterProfile profile;profile.has_fire_rows=true;
    profile.fire_rows_80089a84[1].bytes={{1,4,13,1,1}};      // authored timing row
    profile.levels[1][0].words[3]=0;

    L3PlayerCompatWork w{};
    w.bytes[0x171]=1;w.bytes[0x175]=7;wr16(w,0x13a,0x8000);
    w.bytes[0x09]=0;w.bytes[0xac]=0;w.bytes[0xad]=0;
    SourceOverlayTrack overlay;

    // Not firing: the overlay mirrors the main request.
    assert(source_overlay_update_8003c418(w,overlay,banks)&&w.bytes[0x104]==0&&overlay.request==0);
    assert(source_left_arm_request_800423dc(w)==0x60);
    w.bytes[0x09]=2;w.bytes[0x112]=1;assert(source_left_arm_request_800423dc(w)==0x63);
    w.bytes[0x112]=0;assert(source_left_arm_request_800423dc(w)==0x61);w.bytes[0x09]=0;
    assert(source_fire_context_for_action(2)==1&&source_fire_context_for_action(8)==2&&!source_fire_context_for_action(3));

    // Gate: +0x175 bit 2 clear (the port's old init) never fires.
    { auto g=w;g.bytes[0x175]=1;assert(!source_fire_input_80040b68(g,0,0x8000,32767,profile)&&g.bytes[0x174]==0); }
    // Gate: an empty counter never fires.
    { auto g=w;assert(!source_fire_input_80040b68(g,0,0x8000,0,profile)); }

    // Press: context 0 -> action 5: +174 = 0x82, +104 = 0x60, main asks 0x60.
    assert(source_fire_input_80040b68(w,0,0x8000,32767,profile));
    assert(w.bytes[0x174]==0x82&&w.bytes[0x104]==0x60&&w.bytes[0x105]==0&&w.bytes[0x178]==0&&w.bytes[0xac]==0x60);
    assert(source_overlay_update_8003c418(w,overlay,banks)&&overlay.request==0x60&&w.bytes[0x100]==0);

    int shot_frames=0,first_shot=-1,end_frame=-1;
    for(int f=1;f<40;++f){
        if((w.bytes[0x174]&0xc0)==0x80)assert(source_buster_lane_80042f78(w,overlay,banks,0,32767,profile));
        if(w.bytes[0x174]&0x20){++shot_frames;if(first_shot<0)first_shot=f;}
        assert(source_overlay_update_8003c418(w,overlay,banks));
        if(w.bytes[0x174]&0x80)assert(w.bytes[0x106]==2&&w.bytes[0x104]==0x60);
        if(w.bytes[0x174]==0&&end_frame<0)end_frame=f;
    }
    // Key 1 is reached on frame 1; the lane raises bit 0x20 on frame 2 only.
    assert(first_shot==2&&shot_frames==1);
    // Rapid frame 4, end frame 13, then the hold key (control -1) at 19 ends it.
    assert(end_frame>19&&end_frame<=21&&w.bytes[0x177]==0);
    const auto* frame=source_overlay_frame(w,overlay,banks);
    assert(frame==nullptr||(*frame)[3]>=100);

    // Held button during state 3 queues a re-fire (+177 = 1), which restarts
    // through action 1 and lane state 1.
    {
        L3PlayerCompatWork g=w;SourceOverlayTrack t=overlay;
        g.bytes[0x174]=0x82;wr16(g,0x178,3);g.bytes[0x100]=5;
        assert(source_buster_lane_80042f78(g,t,banks,0x8000,32767,profile)&&g.bytes[0x174]==0&&g.bytes[0x177]==1);
        assert(source_fire_input_80040b68(g,0,0x8000,32767,profile)&&g.bytes[0x174]==0x82&&g.bytes[0x178]==1);
        assert(source_buster_lane_80042f78(g,t,banks,0,32767,profile)&&(g.bytes[0x174]&0x20)&&g.bytes[0x178]==2);
        assert(t.request==0x60&&t.record==1&&g.bytes[0x100]==1);
    }

    // 0x8005C8A4 refill: weapon 1 level 0 full count, every 17 frames.
    {
        SourceBusterProfile r=profile;r.levels[1][0].words[1]=3;
        L3PlayerCompatWork g{};g.bytes[0x171]=1;
        int16_t counter=0;uint8_t timer=0;int refills=0,first=-1;
        for(int f=0;f<40;++f)if(source_buster_refill_8005c8a4(g,counter,timer,r)){++refills;if(first<0)first=f;}
        assert(first==16&&refills==2&&counter==3);
        g.bytes[0x171]=0;counter=0;assert(!source_buster_refill_8005c8a4(g,counter,timer,r)&&counter==0);
        g.bytes[0x171]=1;r.levels[1][0].words[1]=1;counter=0;timer=0;
        for(int f=0;f<40;++f)assert(!source_buster_refill_8005c8a4(g,counter,timer,r));
    }

    // 0x800325D4 selector 2: torso parts 0/1 and left arm 5..7 from the overlay.
    PbdMainPoseBank a,b;
    for(size_t p=0;p<kPbdMainPosePartCount;++p){a.part_rotation[p]={1,1,1};b.part_rotation[p]={2,2,2};}
    a.root_translation={5,6,7};b.root_translation={9,9,9};
    const auto m2=source_merge_overlay_pose_800325d4(a,b,2);
    for(size_t p=0;p<kPbdMainPosePartCount;++p){
        const bool over=p==0||p==1||(p>=5&&p<=7);
        assert(m2.part_rotation[p][0]==(over?2:1));
    }
    assert(m2.root_translation[0]==5);
    const auto m1=source_merge_overlay_pose_800325d4(a,b,1);assert(m1.part_rotation[3][0]==2&&m1.part_rotation[6][0]==1);
    const auto m0=source_merge_overlay_pose_800325d4(a,b,0);assert(m0.part_rotation[0][0]==1);
    std::cout<<"PASS Buster fire input -> lane 0x80042F78 -> overlay track -> selector-2 pose\n";
}
