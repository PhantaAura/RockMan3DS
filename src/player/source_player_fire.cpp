#include "legends3ds/source_player_fire.h"

namespace l3 {
namespace {
uint16_t rd16(const L3PlayerCompatWork& w,size_t o){return uint16_t(w.bytes[o]|(uint16_t(w.bytes[o+1])<<8));}
void wr16(L3PlayerCompatWork& w,size_t o,uint16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(v>>8);}
bool busy(const L3PlayerCompatWork& w){return l3_pl00_state_busy_8004324c(&w)!=0;}
const SourceBusterFireRow* fire_row(const SourceBusterProfile& p,const L3PlayerCompatWork& w){
    const auto weapon=size_t(w.bytes[0x171]&15u);
    return p.has_fire_rows?&p.fire_rows_80089a84[weapon]:nullptr;
}
// 0x80040E00(work, base, context, flag) -> 0x80040E9C.
bool fire_action_80040e00(L3PlayerCompatWork& w,uint16_t base,uint8_t context,uint8_t flag){
    uint16_t action=base;
    if(context==1){
        if(flag==0){w.bytes[0x09]=0;wr16(w,0x0a,0);return false;}
    }else if(context==0){
        action=uint16_t(base+1u);
    }else if(context==2){
        if(flag==0)return false;
    }else return true;
    // Actions 4..7 store the 0x800423DC / 0x80042458 request. Only the left
    // arm (4/5) is reached from the Buster branches ported here.
    const uint8_t request=action>=4u?source_left_arm_request_800423dc(w):0u;
    (void)l3_pl00_fire_state_80040e9c(&w,action,1,request);
    return true;
}
const PbdAnimationAction* action_of(const PbdAnimation* a,uint16_t i){return a?find_pbd_animation_action(*a,i):nullptr;}
void load_key(L3PlayerCompatWork& w,const PbdSRecord& r){
    w.bytes[0x100]=uint8_t(r.word0);w.bytes[0x101]=uint8_t(r.word0>>8);
    w.bytes[0x102]=uint8_t(r.word1);w.bytes[0x103]=uint8_t(r.word1>>8);
}
// 0x800321C8(work, request, record).
bool start_overlay(L3PlayerCompatWork& w,SourceOverlayTrack& t,const SourcePlayerAnimationBanks& b,
                   uint8_t request,uint8_t record){
    const auto acts=source_player_animation_actions(b,request,w.bytes[0x171]);
    if(!acts.keys||record>=acts.keys->s_records.size())return false;
    t.request=request;t.record=record;t.valid=true;
    w.bytes[0x104]=request;load_key(w,acts.keys->s_records[record]);
    return true;
}
} // namespace

uint8_t source_left_arm_request_800423dc(const L3PlayerCompatWork& w) noexcept {
    const uint8_t action=w.bytes[0x09];
    if(action!=2&&action!=8)return 0x60;          // jump table 0x80010C74
    const int8_t stance=int8_t(w.bytes[0x112]);
    if(stance==1)return 0x63;
    if(stance==0)return 0x61;
    if(stance==3)return 0x62;
    return 0x60;
}

std::optional<uint8_t> source_fire_context_for_action(uint8_t action) noexcept {
    switch(action){
        case 0:case 1:case 4:case 6:case 14:return 0;
        case 2:return 1;
        case 7:case 8:return 2;
        default:return std::nullopt;
    }
}

bool source_fire_input_80040b68(L3PlayerCompatWork& w,uint8_t context,uint16_t pad,
                                int16_t counter_171,const SourceBusterProfile& profile) noexcept {
    const auto* row=fire_row(profile,w);
    if(!row)return false;
    const uint8_t flag=row->bytes[4];                    // DAT_80089a88 = row +4
    const bool button=(rd16(w,0x13a)&pad)!=0||(w.bytes[0x176]&3u)==2u;
    const bool ready=(w.bytes[0x175]&5u)==5u;
    if(int8_t(w.bytes[0x174])&0xc0){w.bytes[0x176]=0;return false;}
    uint8_t queued=w.bytes[0x177];
    if(queued==1&&button&&ready&&fire_action_80040e00(w,0,context,flag)){
        wr16(w,0x178,1);w.bytes[0x177]=0;w.bytes[0x176]=0;return true;
    }
    queued=w.bytes[0x177];
    // queued == 2 is the +0x170 lane re-fire (0x80037440): not ported, so it
    // falls through as the original does when that test fails.
    (void)queued;
    if(button&&counter_171>0&&ready){
        w.bytes[0x176]=0;
        return fire_action_80040e00(w,4,context,flag);
    }
    w.bytes[0x176]=0;return false;
}

SourceAnimationActions source_player_animation_actions(const SourcePlayerAnimationBanks& b,
                                                       uint8_t request,uint8_t sel171) noexcept {
    SourceAnimationActions out;
    switch(request>>4){
        case 0:case 1:case 2:case 3:
            out.keys=action_of(b.main,request);
            out.poses=b.main?pbd_main_body_pose_action_for_request(*b.main,request):nullptr;
            break;
        case 6:case 7:{
            const PbdAnimation* bank=sel171?b.left_arm_04:b.left_arm_00;
            out.keys=out.poses=action_of(bank,uint16_t(request-0x60u));
            break;
        }
        default:break;   // 0x40..0x5F right arm, 0x80+ second main family: not cooked
    }
    if(out.keys&&(!out.keys->has_s_records||out.keys->s_records.empty()))out.keys=nullptr;
    if(out.poses&&(!out.poses->m_is_frame_table||out.poses->angle_frames.empty()))out.poses=nullptr;
    return out;
}

bool source_overlay_update_8003c418(L3PlayerCompatWork& w,SourceOverlayTrack& t,
                                    const SourcePlayerAnimationBanks& b) noexcept {
    if(!busy(w)){w.bytes[0x104]=w.bytes[0xac];w.bytes[0x100]=w.bytes[0xa4];}
    if(w.bytes[0x104]!=w.bytes[0x105]||!t.valid||t.request!=w.bytes[0x104]){
        // 0x800321C8 with record +0x100 (the key index), then +105 = +104.
        if(!start_overlay(w,t,b,w.bytes[0x104],w.bytes[0x100]))return false;
        w.bytes[0x105]=w.bytes[0x104];
        return true;
    }
    // 0x800323FC(work, +174 & 3).
    const auto acts=source_player_animation_actions(b,t.request,w.bytes[0x171]);
    if(!acts.keys||t.record>=acts.keys->s_records.size())return false;
    wr16(w,0x106,uint16_t(w.bytes[0x174]&3u));
    const uint8_t count=w.bytes[0x101];
    w.bytes[0x101]=uint8_t(count-1u);
    if(count==1){
        const int8_t control=int8_t(acts.keys->s_records[t.record].word1>>8);
        if(control<0){
            if(control==-1)w.bytes[0x101]=uint8_t(acts.keys->s_records[t.record].word0>>8);
            else {
                const uint8_t next=uint8_t(control&0x7f);
                if(next>=acts.keys->s_records.size())return false;
                t.record=next;load_key(w,acts.keys->s_records[next]);
            }
        }else{
            if(size_t(t.record)+1u>=acts.keys->s_records.size())return false;
            ++t.record;load_key(w,acts.keys->s_records[t.record]);
        }
    }
    return true;
}

const std::array<uint16_t,kPbdPackedAngleChannels>* source_overlay_frame(
    const L3PlayerCompatWork& w,const SourceOverlayTrack& t,const SourcePlayerAnimationBanks& b) noexcept {
    if(!t.valid)return nullptr;
    const auto acts=source_player_animation_actions(b,t.request,w.bytes[0x171]);
    const uint8_t frame=w.bytes[0x100];
    if(!acts.poses||frame>=acts.poses->angle_frames.size())return nullptr;
    return &acts.poses->angle_frames[frame];
}

bool source_buster_lane_80042f78(L3PlayerCompatWork& w,SourceOverlayTrack& overlay,
                                 const SourcePlayerAnimationBanks& banks,uint16_t pad,
                                 int16_t counter_171,const SourceBusterProfile& profile) noexcept {
    const auto* row=fire_row(profile,w);
    if(!row)return false;
    const auto& r=row->bytes;
    const int8_t state=int8_t(w.bytes[0x178]);
    const auto set_state=[&](int v){wr16(w,0x178,uint16_t(int16_t(v)));};
    const auto state3=[&]{
        if(w.bytes[0x100]==r[2]){set_state(int8_t(w.bytes[0x178])+1);return;}
        if((rd16(w,0x13a)&pad)&&(w.bytes[0x175]&5u)==5u){w.bytes[0x174]=0;w.bytes[0x177]=1;}
    };
    switch(state){
        case 0:
            if(w.bytes[0x100]==r[0]){set_state(state+2);w.bytes[0x174]|=0x20;}
            return true;
        case 1:{
            // 0x800320B8(work, +AC, row[0]) restarts the main track at that
            // key; the port's main track restarts through a forced 0x80041DDC.
            if(!(w.bytes[0x174]&4u))l3_pl00_anim_request_80041ddc(&w,w.bytes[0xac],r[0],1);
            const uint8_t request=source_left_arm_request_800423dc(w);
            w.bytes[0x104]=request;
            if(!start_overlay(w,overlay,banks,request,r[0]))return false;
            set_state(state+1);w.bytes[0x174]|=0x20;
            return true;
        }
        case 2:{
            w.bytes[0x174]&=0xcf;
            if(r[3]&&counter_171>0){
                // 0x80037664: halfword +6 of the weapon's level row at +0x4F7 + w*8.
                const auto weapon=size_t(w.bytes[0x171]&15u);
                const int8_t level=int8_t(w.bytes[0x4f7+weapon*8]);
                if(level<0||size_t(level)>=kSourceBusterLevelCount)return false;
                const int extra=profile.levels[weapon][size_t(level)].words[3];
                if(int(w.bytes[0x100])!=int(r[1])+extra)return true;   // lbu + lh, full compare
                set_state(state+1);state3();
            }else set_state(state+2);
            return true;
        }
        case 3:state3();return true;
        case 4:
            if(int8_t(w.bytes[0x103])==-1){
                w.bytes[0x174]=0;w.bytes[0x177]=0;
                if((w.bytes[0x171]&15u)==0)wr16(w,0x44,0);
            }
            return true;
        default:return true;
    }
}

bool source_buster_refill_8005c8a4(const L3PlayerCompatWork& w,int16_t& counter,uint8_t& timer,
                                   const SourceBusterProfile& profile) noexcept {
    if(w.bytes[0x171]!=1)return false;
    const int8_t level=int8_t(w.bytes[0x4f5+1*8]);
    if(level<0||size_t(level)>=kSourceBusterLevelCount)return false;
    const int16_t full=profile.levels[1][size_t(level)].words[1];
    const uint8_t limit=level==7?0:17;
    if(counter!=0){if(full<=0)return false;}
    else if(full<2)return false;
    timer=uint8_t(timer+1u);
    if(timer<limit)return false;
    timer=0;counter=full;return true;
}

PbdMainPoseBank source_merge_overlay_pose_800325d4(const PbdMainPoseBank& main,
                                                   const PbdMainPoseBank& overlay,uint16_t sel) noexcept {
    PbdMainPoseBank out=main;
    if(sel<1||sel>3)return out;
    out.part_rotation[0]=overlay.part_rotation[0];out.part_rotation[1]=overlay.part_rotation[1];
    if(sel==1||sel==3)for(size_t p=2;p<=4;++p)out.part_rotation[p]=overlay.part_rotation[p];
    if(sel==2||sel==3)for(size_t p=5;p<=7;++p)out.part_rotation[p]=overlay.part_rotation[p];
    return out;
}

} // namespace l3
