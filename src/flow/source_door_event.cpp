#include "legends3ds/source_door_event.h"
#include "legends3ds/player_runtime.h"
#include "legends3ds/l3map.h"

namespace l3 {
namespace {
int16_t rd16(const L3PlayerCompatWork& w,size_t o){return int16_t(uint16_t(w.bytes[o]|(uint16_t(w.bytes[o+1])<<8)));}
void wr16(L3PlayerCompatWork& w,size_t o,uint16_t v){w.bytes[o]=uint8_t(v);w.bytes[o+1]=uint8_t(v>>8);}
void wr32(L3PlayerCompatWork& w,size_t o,uint32_t v){for(size_t i=0;i<4;++i)w.bytes[o+i]=uint8_t(v>>(8*i));}
void copy(L3PlayerCompatWork& w,size_t to,size_t from,size_t n){for(size_t i=0;i<n;++i)w.bytes[to+i]=w.bytes[from+i];}
bool flag(const uint8_t* flags,size_t bytes,uint16_t id){
    return flags && size_t(id>>3)<bytes && (flags[id>>3]&(1u<<(id&7u)))!=0;
}
// 0x800519E4: 1 blocks the door. The message call needs the 0x7C0 locks clear.
bool locked_800519e4(const St00AuxRecord& r,const uint8_t* flags,size_t bytes,SourceDoorResult& out){
    const uint16_t id=uint16_t(r.halfwords[0])&0x1Fu;
    if(!flag(flags,bytes,0x7E0)&&!flag(flags,bytes,uint16_t(id|0x7E0u)))return false;
    if(!flag(flags,bytes,0x7C0)&&!flag(flags,bytes,uint16_t(id|0x7C0u)))out.locked_message=r.halfwords[2];
    return true;
}
} // namespace

const char* source_door_status_name(SourceDoorStatus s) noexcept {
    switch(s){
        case SourceDoorStatus::NoEvent:return "NO_EVENT";
        case SourceDoorStatus::PlayerBusy:return "PLAYER_BUSY";
        case SourceDoorStatus::UnsupportedType:return "UNSUPPORTED_TYPE";
        case SourceDoorStatus::GateClosed:return "GATE_CLOSED";
        case SourceDoorStatus::NoAuxMatch:return "NO_AUX_MATCH";
        case SourceDoorStatus::Locked:return "LOCKED";
        case SourceDoorStatus::LocalDoorOpened:return "LOCAL_DOOR_OPENED";
        case SourceDoorStatus::TransitionRequested:return "TRANSITION_REQUESTED";
    }
    return "UNKNOWN";
}

bool source_door_gate_80051970(const L3PlayerCompatWork& p) noexcept {
    uint32_t busy_184=0;for(size_t i=0;i<4;++i)busy_184|=uint32_t(p.bytes[0x184+i])<<(8*i);
    if(busy_184!=0)return false;
    const uint8_t action=p.bytes[9];
    if(!(action<2||action==4))return false;
    if((uint16_t(rd16(p,0x11E))&uint16_t(rd16(p,0x136)))==0)return false;
    return l3_pl00_state_busy_8004324c(&p)==0;
}

SourceDoorResult evaluate_source_door_event_80050c2c(const RuntimeEventPrimitiveQuery& query,
    const L3PlayerCompatWork& p,const St00AuxList& aux,const uint8_t* flags,size_t bytes) noexcept {
    SourceDoorResult out;out.query_status=query.status;
    if(query.status!=RuntimeEventQueryStatus::Hit||!query.sample)return out;
    const auto& s=*query.sample;out.sample=s;
    // 0x80050C60..84: a1 is the player only when +1 has no high nibble; that
    // branch also returns early while +0xBD is nonzero.
    const bool player_owner=(p.bytes[1]&0xF0u)==0;
    if(player_owner&&p.bytes[0xBD]!=0){out.status=SourceDoorStatus::PlayerBusy;return out;}
    if(s.type!=3){out.status=SourceDoorStatus::UnsupportedType;return out;}
    if(!player_owner||!source_door_gate_80051970(p)){out.status=SourceDoorStatus::GateClosed;return out;}
    out.status=SourceDoorStatus::NoAuxMatch;
    const int16_t px=rd16(p,0x14),pz=rd16(p,0x18);
    const int32_t facing=int16_t(s.heading);
    for(size_t i=0;i<aux.records.size();++i){
        const auto& r=aux.records[i];const auto& h=r.halfwords;
        if(h[0]==-1)break;
        if(h[4]!=s.x||h[5]!=s.y||h[6]!=s.z)continue;
        // 0x80050E74..8C: bit 0x400 only, so either side of the door heading.
        if(((uint32_t(facing-int32_t(h[7]))+0x200u)&0x400u)!=0)continue;
        // 0x80050E90..A4: signed divide by 0x400 truncating toward zero.
        const int32_t v=facing+0x200;
        const unsigned q=unsigned((v<0?(facing+0x5FF)>>10:v>>10)&3);
        if((q==0&&pz<s.z)||(q==1&&px<s.x)||(q==2&&s.z<pz)||(q==3&&s.x<px))continue;
        out.aux_record_index=i;
        if(locked_800519e4(r,flags,bytes,out)){out.status=SourceDoorStatus::Locked;return out;}
        if((uint16_t(h[0])&0x8000u)==0){out.flag_set=0x7B1;out.status=SourceDoorStatus::LocalDoorOpened;return out;}
        SourceDoorRequest q3;q3.aux_record_index=i;
        const auto put16=[&](size_t at,uint16_t value){q3.raw[at]=uint8_t(value);q3.raw[at+1]=uint8_t(value>>8);q3.known[at]=q3.known[at+1]=1;};
        const auto put8=[&](size_t at,uint8_t value){q3.raw[at]=value;q3.known[at]=1;};
        put16(0,uint16_t(s.x));put16(2,uint16_t(s.y));put16(4,uint16_t(s.z));put16(6,s.heading);
        put16(8,uint16_t(h[8]));put16(10,uint16_t(h[9]));put16(12,uint16_t(h[10]));put16(14,uint16_t(h[11]));
        put16(16,uint16_t(h[7]));put16(18,uint16_t(h[11]));
        put8(22,r.raw[6]);put8(25,10);put8(21,0);put8(24,2);put8(23,r.raw[7]);
        out.request=q3;out.flag_set=0x7B0;out.status=SourceDoorStatus::TransitionRequested;return out;
    }
    return out;
}

SourceEventHandoffTransitionIntent source_door_transition_intent(const SourceDoorRequest& q,uint64_t generation) noexcept {
    SourceEventHandoffTransitionIntent t;t.source_function=0x80050C2Cu;t.source_generation=generation;
    for(size_t i=0;i<t.raw.size();++i){t.raw[i]=q.raw[8+i];t.known[i]=q.known[8+i];}
    for(size_t a=0;a<3;++a)t.spawn[a]=int16_t(uint16_t(t.raw[a*2]|(uint16_t(t.raw[a*2+1])<<8)));
    t.source_stage=t.raw[14];t.destination_area=t.raw[15];t.argument=0;
    return t;
}

bool apply_source_door_arrival_80048898(const SourceEventHandoffTransitionIntent& t,
    const ReferenceTables& refs,L3PlayerCompatWork& p) noexcept {
    for(const size_t lane:{size_t(0),size_t(1),size_t(2),size_t(3),size_t(4),size_t(5),size_t(10),size_t(11)})
        if(!t.known[lane])return false;
    const auto lane16=[&](size_t at){return int16_t(uint16_t(t.raw[at]|(uint16_t(t.raw[at+1])<<8)));};
    const uint16_t heading=uint16_t(lane16(10));
    const auto& trig=refs.trig[heading&0x0FFFu];
    // sll 6 / sra 12 of rsin/rcos (0x8007A060/0x8007A12C); halfword stores.
    const int16_t x=int16_t(uint16_t(int32_t(lane16(0))+((int32_t(trig.sin_q12)*64)>>12)));
    const int16_t y=int16_t(uint16_t(int32_t(lane16(2))-1));
    const int16_t z=int16_t(uint16_t(int32_t(lane16(4))+((int32_t(trig.cos_q12)*64)>>12)));
    auto n=p;
    // Substate 8 targets in +0x428..+0x448.
    wr16(n,0x438,uint16_t(x));wr16(n,0x43A,uint16_t(y));wr16(n,0x43C,uint16_t(z));
    wr32(n,0x428,uint32_t(int32_t(x))<<16);wr32(n,0x42C,uint32_t(int32_t(y))<<16);wr32(n,0x430,uint32_t(int32_t(z))<<16);
    wr16(n,0x440,0);wr16(n,0x442,heading);wr16(n,0x444,0);n.bytes[0x448]=48;
    // State-3 substate-2 copy: previous pose, then current from the targets.
    // It runs every frame until substate 10; the second pass leaves the
    // previous pose equal to the arrival, as when control returns.
    for(int pass=0;pass<2;++pass){
        for(size_t i=0;i<8;++i)n.bytes[0x2C+i]=n.bytes[0x14+i];
        for(size_t i=0;i<16;++i)n.bytes[0x34+i]=n.bytes[0x1C+i];
        for(size_t i=0;i<8;++i)n.bytes[0x5C+i]=n.bytes[0x54+i];
        n.bytes[0xAD]=n.bytes[0xAC];
        for(size_t i=0;i<6;++i)n.bytes[0x14+i]=n.bytes[0x438+i];
        for(size_t i=0;i<12;++i)n.bytes[0x1C+i]=n.bytes[0x428+i];
        for(size_t i=0;i<6;++i)n.bytes[0x54+i]=n.bytes[0x440+i];
    }
    p=n;return true;
}

uint8_t source_event0_walk_limit_8008aa04(uint8_t stage) noexcept {
    // 0x8008AA04..0x8008AA23, indexed by the signed byte 0x800C1B60.
    static constexpr uint8_t table[32]={0x6C,0,0,0,0,0,0,0,0,0,0x6C,0,0,0,0x6C,0,
                                        0,0,0,0x70,0x6C,0,0,0,0,0x6C,0x6C,0,0,0,0,0};
    return stage<32?table[stage]:0;
}

std::optional<bool> source_player_event_ready_80042238(L3PlayerCompatWork& p,
    std::optional<uint8_t> byte_800c1b93) noexcept {
    p.bytes[0x449]=0xFF;p.bytes[0xBD]=uint8_t(p.bytes[0xBD]|0x40u);
    // 0x800422E8: clear +0xB4, stop sounds 22/23 (0x8001B2F0, not played), 1.
    const auto ready=[&]{wr16(p,0xB4,0);return std::optional<bool>(true);};
    if((p.bytes[0]&1u)==0)return ready();
    if(p.bytes[8]==1){
        const uint8_t action=p.bytes[9];
        bool check_busy=action==0;
        if(action==14){
            if(!byte_800c1b93)return std::nullopt;
            check_busy=*byte_800c1b93!=0;
        }
        if(check_busy&&l3_pl00_state_busy_8004324c(&p)==0)return ready();
        // 0x800422B4..D4: (+8 word & 0xFFFF00) == 0x020D00 writes +10 = 6.
        if(p.bytes[9]==0x0D&&p.bytes[10]==0x02)p.bytes[10]=6;
    }
    if(p.bytes[8]==3)return ready();
    return false;
}

void source_player_state3_80041b8c(L3PlayerCompatWork& p) noexcept {
    switch(p.bytes[9]){
        case 0:
            // 0x80041BE4..C48: current pose becomes the target, no animation.
            copy(p,0x438,0x14,8);copy(p,0x428,0x1C,16);copy(p,0x440,0x54,8);
            p.bytes[0x448]=0xFF;
            [[fallthrough]];
        case 1:
            // 0x800423A0.
            p.bytes[0x174]=0;p.bytes[0x177]=0;p.bytes[0x175]=uint8_t(p.bytes[0x175]&0xFEu);
            p.bytes[0x104]=p.bytes[0xAC];p.bytes[0x100]=p.bytes[0xA4];
            p.bytes[0xBD]=0x40;p.bytes[9]=2;
            [[fallthrough]];
        case 2:{
            // 0x80041C64..D2C: previous pose, then the targets.
            copy(p,0x2C,0x14,8);copy(p,0x34,0x1C,16);copy(p,0x5C,0x54,8);
            p.bytes[0xAD]=p.bytes[0xAC];
            copy(p,0x14,0x438,8);copy(p,0x1C,0x428,16);copy(p,0x54,0x440,8);
            p.bytes[0xBD]=uint8_t(p.bytes[0xBD]&0xF7u);
            const uint8_t request=p.bytes[0x448];
            if(request!=0xFF&&(p.bytes[0xAC]!=request||int8_t(p.bytes[0xA7])==-1)){
                l3_pl00_anim_request_80041ddc(&p,request,0,1);p.bytes[0x448]=0xFF;
            }
            return;
        }
        case 3:
            // 0x800423C8, then +8 word = 1 and +0xBD = 8.
            p.bytes[0x175]=uint8_t(p.bytes[0x175]|1u);
            p.bytes[8]=1;p.bytes[9]=0;p.bytes[10]=0;p.bytes[11]=0;p.bytes[0xBD]=8;
            return;
        default:return;
    }
}

bool source_player_input_select_8003c07c(L3PlayerCompatWork& p,uint16_t forced) noexcept {
    const uint8_t bd=p.bytes[0xBD];
    if(bd&0x40u){wr16(p,0x11C,0);wr16(p,0x11E,0);}
    else if(bd&0x20u){wr16(p,0x11C,forced);wr16(p,0x11E,0);}
    else return false;
    return true;
}

void source_event0_forced_walk_80048a9c(SourceDoorEvent0& e,L3PlayerCompatWork& p,uint8_t stage) noexcept {
    e.forced_pad=0;
    bool done=false;
    switch(e.walk_state){
        case 0:
            e.walk_limit=source_event0_walk_limit_8008aa04(stage);
            p.bytes[0xBD]=uint8_t(p.bytes[0xBD]|0x20u);++e.walk_state;
            [[fallthrough]];
        case 1:
            e.forced_pad=uint16_t(rd16(p,0x124));
            done=!(e.counter<37);
            break;
        case 2:
            p.bytes[0xBD]=uint8_t(p.bytes[0xBD]|0x20u);++e.walk_state;
            [[fallthrough]];
        case 3:
            e.forced_pad=uint16_t(rd16(p,0x124));
            done=int32_t(e.walk_limit)<e.counter;
            break;
        default:return;
    }
    if(!done)return;
    ++e.substate;++e.walk_state;
    p.bytes[0xBD]=uint8_t((p.bytes[0xBD]&0xDFu)|0x40u);e.forced_pad=0;
}

void source_player_event_release_80042314(L3PlayerCompatWork& p) noexcept {
    for(const size_t at:{size_t(0x449),size_t(0x111),size_t(0x71),size_t(0x73),size_t(0x168),size_t(0x169)})p.bytes[at]=0;
    for(size_t at=0x188;at<=0x197;++at)p.bytes[at]=0;
    if(p.bytes[0]&1u){
        p.bytes[0]=3;p.bytes[6]=3;
        if(p.bytes[8]==3){p.bytes[9]=3;return;}
    }
    p.bytes[0xBD]=8;
}

void source_player_state0_8003c7b0(L3PlayerCompatWork& p) noexcept {
    wr32(p,8,1);wr16(p,0x6C,60);
    for(const size_t at:{size_t(0x44),size_t(0x46),size_t(0x48),size_t(0x114),size_t(0x116),size_t(0x118),
                         size_t(0x108),size_t(0x10A)})wr16(p,at,0);
    for(const size_t at:{size_t(0x112),size_t(0x113),size_t(0x110),size_t(0x71),size_t(0x73),size_t(0x168),
                         size_t(0x169),size_t(0x104),size_t(0xAC),size_t(0x105),size_t(0xAD),size_t(0x174),
                         size_t(0x177),size_t(0x44B),size_t(0x44A)})p.bytes[at]=0;
    for(size_t at=0x188;at<=0x197;++at)p.bytes[at]=0;
    wr32(p,0x184,0);p.bytes[0x175]=7;p.bytes[6]=uint8_t(p.bytes[6]|3u);
    l3_pl00_anim_request_80041ddc(&p,0,0,1);
}

std::optional<uint16_t> source_descriptor_by_word0_8003ac24(const RuntimeMap& m,uint32_t key) noexcept {
    for(size_t i=0;i<m.descriptors.size()&&i<0xFFFFu;++i)if(m.descriptors[i].word0==key)return uint16_t(i);
    return std::nullopt;
}

std::optional<SourceDoorObject> source_door_object_init_801124b0(const RuntimeMap& m,uint16_t idx) noexcept {
    if(idx==0||idx>=m.idx.size())return std::nullopt;
    const uint16_t closed=m.idx[idx].descriptor_index;
    if(closed>=m.descriptors.size()||(m.descriptors[closed].word0&0x1000u)==0)return std::nullopt;
    const auto open=source_descriptor_by_word0_8003ac24(m,m.descriptors[closed].word0&~0x1000u);
    if(!open)return std::nullopt;
    SourceDoorObject o;o.idx_index=idx;o.closed_descriptor=closed;o.open_descriptor=*open;o.state=1;return o;
}

SourceDoorObjectStep source_door_object_step(SourceDoorObject& o,bool f7b0,bool f7b1,
    std::optional<int16_t> player_cell,const RuntimeMap& m) noexcept {
    SourceDoorObjectStep out;
    const bool in_cell=player_cell&&*player_cell==int16_t(o.idx_index);
    const auto start_closing=[&]{o.frames=16;++o.state;out.flag_7b2=true;out.sound=0x106;};
    switch(o.state){
        case 0:
            // 0x801124B0 re-reads the cell's current (restored) descriptor.
            if(const auto again=source_door_object_init_801124b0(m,o.idx_index))o=*again;
            else o.state=1;
            break;
        case 1:
            if(f7b1&&in_cell){o.state=2;o.frames=16;out.flag_7b2=true;out.sound=0x105;}
            break;
        case 2:
            if(--o.frames==0){++o.state;out.sound=0x185;out.flag_7b2=false;out.descriptor_write=o.open_descriptor;}
            break;
        case 3:
            if(f7b0){if(!f7b1)start_closing();}
            else if(!in_cell){start_closing();out.flag_7b1=false;}
            break;
        case 4:
            if(--o.frames==0){++o.state;out.sound=0x186;out.flag_7b2=false;}
            break;
        case 5:
            out.descriptor_write=o.closed_descriptor;o.state=0;
            break;
        default:break;
    }
    return out;
}
} // namespace l3
