#include "legends3ds/source_gameplay_camera.h"
#include "legends3ds/player_contact_runtime.h"
#include "legends3ds/source_camera.h"

namespace l3 {
namespace {
int16_t rd16(const L3PlayerCompatWork& w,size_t o){
    return static_cast<int16_t>(uint16_t(w.bytes[o])|(uint16_t(w.bytes[o+1])<<8));
}
// rsin 0x8007A060 / rcos 0x8007A12C read the scalar quarter table. An L3RF v1
// pack has only the packed RotMatrix table; that substitution is counted.
int32_t rsin(const ReferenceTables& r,int32_t a){
    return r.scalar_quarter_known?reference_rsin_8007a060(r,a):r.trig[uint32_t(a)&0xfffu].sin_q12;
}
int32_t rcos(const ReferenceTables& r,int32_t a){
    return r.scalar_quarter_known?reference_rcos_8007a12c(r,a):r.trig[uint32_t(a)&0xfffu].cos_q12;
}
// mult + mflo + sra 12: low32 product, then an arithmetic shift.
int32_t mul_sra12(uint32_t a,int32_t b){return int32_t(a*uint32_t(b))>>12;}
int32_t wrap32(int64_t v){return static_cast<int32_t>(static_cast<uint32_t>(v));}
// Camera point bounds at 0x80097D60 (x0,x1,z0,z1,y0,y1), read from the EXE.
constexpr RetailPlayerBounds kCameraBounds{-32,32,-32,32,-32,32};

// 0x8003A218: shortest distance from the current yaw to `goal` (both & 0xFFF).
int32_t yaw_gap_8003a218(int32_t current,int32_t goal){
    const int32_t t=goal&0xfff,c=current&0xfff;
    int32_t direct,wrapped;
    if(t<c){direct=c-t;wrapped=t-(c-0x1000);}
    else {direct=t-c;wrapped=c-(t-0x1000);}
    return wrapped<direct?wrapped:direct;
}
// 0x8003A154: step the current yaw toward `goal` by `rate`, never lagging
// more than `max_lag`. Branch order and signed compares follow 0x8003A154.
int32_t yaw_step_8003a154(int32_t current,int32_t goal,int32_t rate,int32_t max_lag){
    const int32_t t=goal&0xfff,c=current&0xfff;
    if(t<c){
        const int32_t down=c-t,up=t-(c-0x1000);
        if(up<down){
            if(max_lag<up)return t-max_lag;
            const int32_t v=c+rate;
            return t+0x1000<v?t:v;
        }
        if(max_lag<down)return t+max_lag;
        const int32_t v=c-rate;
        return v<t?t:v;
    }
    const int32_t up=t-c,down=c-(t-0x1000);
    if(down<up){
        if(max_lag<down)return t+max_lag;
        const int32_t v=c-rate;
        return v<t-0x1000?t:v;
    }
    if(max_lag<up)return t-max_lag;
    const int32_t v=c+rate;
    return t<v?t:v;
}
} // namespace

std::array<int32_t,3> source_camera_eye_80037fb4(const ReferenceTables& refs,
    const std::array<int32_t,3>& target,int32_t d,int32_t yaw,int32_t pitch) noexcept {
    // Second half of 0x80037FB4: offset vector minus target, then the halfword
    // negation stored to 0x1F800028..2C and mirrored to 0x800A39F0..F8.
    const int32_t vx=wrap32(int64_t(wrap32(int64_t(wrap32(int64_t(d)*rsin(refs,yaw))>>12)*rcos(refs,pitch))>>12)-target[0]);
    const int32_t vy=wrap32(int64_t(wrap32(int64_t(d)*rsin(refs,pitch))>>12)-target[1]);
    const int32_t vz=wrap32(int64_t(wrap32(int64_t(wrap32(int64_t(d)*rcos(refs,yaw))>>12)*rcos(refs,pitch))>>12)-target[2]);
    const auto narrow=[](int32_t v){return int32_t(int16_t(uint16_t(-int16_t(uint16_t(v)))));};
    return {narrow(vx),narrow(vy),narrow(vz)};
}

SourceGameplayCameraFrame source_gameplay_camera_80037fb4(const ReferenceTables& refs,
    const std::array<int32_t,3>& t,int32_t d,int32_t yaw,int32_t pitch,
    uint8_t shift_byte,uint16_t pad) noexcept {
    SourceGameplayCameraFrame f;
    f.packed_trig_substituted=!refs.scalar_quarter_known;
    f.rotation_800a3a10={int16_t((0u-uint32_t(pitch))&0xfffu),int16_t(uint32_t(yaw)&0xfffu),0};
    const auto matrix=source_rot_matrix_yx_transposed_8007b838(refs,uint32_t(pitch),uint32_t(yaw));
    // sllv takes the low five bits of the shift byte (0x80038070..0x8003807C).
    const unsigned sh=shift_byte&31u;
    const uint32_t ds=uint32_t(d)<<sh;
    const auto base=[&](int32_t v){return (0u-uint32_t(v))<<sh;};
    const std::array<uint32_t,3> v{
        base(t[0])+uint32_t(mul_sra12(uint32_t(mul_sra12(ds,rsin(refs,yaw))),rcos(refs,pitch))),
        base(t[1])+uint32_t(mul_sra12(ds,rsin(refs,pitch))),
        base(t[2])+uint32_t(mul_sra12(uint32_t(mul_sra12(ds,rcos(refs,yaw))),rcos(refs,pitch)))};
    f.camera_1f800008=source_pack_camera_words(matrix,pad,source_apply_matrix_lv_8007a9b8(matrix,v));
    f.eye_800a39f0=source_camera_eye_80037fb4(refs,t,d,yaw,pitch);
    return f;
}

namespace {
void produce_80037fb4(SourceGameplayCamera& c,const ReferenceTables& refs) noexcept {
    c.previous_eye_800a3a00=c.eye_800a39f0;
    const auto f=source_gameplay_camera_80037fb4(refs,c.target_800a39a0,c.distance_800a3964,
        c.yaw_800a3968,c.pitch_800a396c,c.scratch_shift_1f800074,c.retained_pad_1f8000f2);
    c.camera_1f800008=f.camera_1f800008;c.rotation_800a3a10=f.rotation_800a3a10;
    c.eye_800a39f0=f.eye_800a39f0;
    if(f.packed_trig_substituted)++c.packed_trig_substitutions;
}
} // namespace

void source_gameplay_camera_init_80038648(SourceGameplayCamera& c,
    const ReferenceTables& refs,int16_t player_x,int16_t player_z) noexcept {
    // Scratch bytes are not camera state: the shift and the retained pad
    // survive a controller reset.
    const auto shift=c.scratch_shift_1f800074;const auto pad=c.retained_pad_1f8000f2;
    const auto shake=c.shake_800a395f;
    c=SourceGameplayCamera{};
    c.scratch_shift_1f800074=shift;c.retained_pad_1f8000f2=pad;c.shake_800a395f=shake;
    c.initialized=true;
    c.target_800a39a0={player_x,0,player_z};
    c.distance_800a3964=0x200;c.pitch_800a396c=0x30;c.yaw_800a3968=0;
    c.eye_800a39f0=c.target_800a39a0;
    c.pull_in_delay_80097d48=0x3c;c.yaw_rate_80097d28=0;c.follow_frames_80098668=0;
    c.distance_goal_80097d2c=0x200;
    // Tail: 0x80037FB4 with the initial angles, then 0x800397E8 restores
    // distance 0x200 / pitch 0x30 (already equal).
    produce_80037fb4(c,refs);
}

void source_gameplay_camera_update_80038894(SourceGameplayCamera& c,
    const ReferenceTables& refs,const L3PlayerCompatWork& player,
    const RuntimeMap& map,size_t area_index) noexcept {
    if(!c.initialized)return;
    ++c.follow_frames_80098668;
    const int32_t heading=rd16(player,0x56);
    c.target_800a39a0[0]=rd16(player,0x14);
    c.target_800a39a0[2]=rd16(player,0x18);
    c.target_800a39a0[1]=-0x96;   // -0x800A39B4, a fixed height in this mode
    const int32_t goal=heading+0x800;
    if(c.follow_frames_80098668==1){
        c.yaw_800a3968=goal;
    } else {
        const int32_t gap=yaw_gap_8003a218(c.yaw_800a3968,goal);
        if(gap<0x200){c.yaw_rate_80097d28=gap>>2;if(c.yaw_rate_80097d28<0x18)c.yaw_rate_80097d28=0x18;}
        else c.yaw_rate_80097d28=0x80;
        c.yaw_800a3968=yaw_step_8003a154(c.yaw_800a3968,goal,c.yaw_rate_80097d28,0x200);
    }
    // Distance: only while player +0x44D (0x800B55FD) is 1 does the original
    // test whether the previous eye sits inside collision (0x80049C54).
    bool hold=false;
    if(player.bytes[0x44D]==1){
        const auto test=runtime_point_embedded_80049c54(map,area_index,
            int16_t(c.eye_800a39f0[0]),int16_t(c.eye_800a39f0[1]),int16_t(c.eye_800a39f0[2]),
            kCameraBounds,true);
        if(test.status==RuntimePointEmbedding::Unsupported){++c.unsupported_eye_tests;hold=true;}
        else if(test.raw==0){c.pull_in_delay_80097d48=0x3c;c.distance_goal_80097d2c+=10;}
        else if(c.pull_in_delay_80097d48==0)c.distance_goal_80097d2c-=10;
        else --c.pull_in_delay_80097d48;
    } else if(c.pull_in_delay_80097d48!=0)--c.pull_in_delay_80097d48;
    else c.distance_goal_80097d2c-=10;
    if(!hold){
        if(c.distance_goal_80097d2c>0x300)c.distance_goal_80097d2c=0x300;
        if(c.distance_goal_80097d2c<0x200)c.distance_goal_80097d2c=0x200;
    }
    c.distance_800a3964=c.distance_goal_80097d2c;
    // 0x80038FE4 (0x800A395A == 0): target y -= shake, produce, shake -= 10.
    c.target_800a39a0[1]-=c.shake_800a395f;
    produce_80037fb4(c,refs);
    c.shake_800a395f=c.shake_800a395f<10?0:uint8_t(c.shake_800a395f-10);
}

} // namespace l3
