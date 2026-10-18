#include "legends3ds/source_scene_view.h"
#include "legends3ds/render/frontend_view.h"
#include <stdexcept>

namespace l3 {
namespace {
std::uint16_t u16(const std::uint8_t* p) noexcept {return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8u);}
std::uint32_t u32(const std::uint8_t* p) noexcept {return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8u)|(std::uint32_t(p[2])<<16u)|(std::uint32_t(p[3])<<24u);}
std::int32_t s32(std::uint32_t value) noexcept {return value<0x80000000u?static_cast<std::int32_t>(value):static_cast<std::int32_t>(std::int64_t(value)-4294967296LL);}
std::int16_t s16(std::uint32_t value) noexcept {value&=65535u;return static_cast<std::int16_t>(value<32768u?std::int32_t(value):std::int32_t(value)-65536);}
std::int32_t asr(std::uint32_t value,unsigned bits) noexcept {return s32((value>>bits)|(value&0x80000000u?(0xffffffffu<<(32u-bits)):0u));}
std::int64_t asr12(std::int64_t value) noexcept {return value>=0?value/4096:-((-value+4095)/4096);}
std::int32_t mul12(std::int32_t a,std::int32_t b) noexcept {return asr(std::uint32_t(a)*std::uint32_t(b),12);}
std::uint32_t crc(const std::uint8_t* p,std::size_t n) noexcept {std::uint32_t c=0xffffffffu;for(std::size_t i=0;i<n;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1u)^(0xedb88320u&(0u-(c&1u)));}return c^0xffffffffu;}
void require(bool condition,const char* why){if(!condition)throw std::runtime_error(why);}
std::int32_t scalar_sin(const SourceSceneViewProfile& profile,std::int32_t angle) noexcept {
    const bool negative=angle<0;const auto magnitude=(negative?0u-std::uint32_t(angle):std::uint32_t(angle))&4095u;
    std::int32_t value=0;
    if(magnitude<=1024u)value=profile.scalar_sine[magnitude];
    else if(magnitude<=2048u)value=profile.scalar_sine[2048u-magnitude];
    else if(magnitude<=3072u)value=-std::int32_t(profile.scalar_sine[magnitude-2048u]);
    else value=-std::int32_t(profile.scalar_sine[4096u-magnitude]);
    return negative?-value:value;
}
std::int32_t scalar_cos(const SourceSceneViewProfile& profile,std::int32_t angle) noexcept {
    const auto magnitude=(angle<0?0u-std::uint32_t(angle):std::uint32_t(angle))&4095u;
    if(magnitude<=1024u)return profile.scalar_sine[1024u-magnitude];
    if(magnitude<=2048u)return -std::int32_t(profile.scalar_sine[magnitude-1024u]);
    if(magnitude<=3072u)return -std::int32_t(profile.scalar_sine[3072u-magnitude]);
    return profile.scalar_sine[magnitude-3072u];
}
std::array<std::int16_t,9> rotation_yx(const ReferenceTables& reference,std::int32_t pitch,std::int32_t yaw) noexcept {
    const auto x=reference.trig[(0u-std::uint32_t(pitch))&4095u];
    const auto y=reference.trig[std::uint32_t(yaw)&4095u];const auto z=reference.trig[0];
    const std::int32_t sx=x.sin_q12,cx=x.cos_q12,sy=y.sin_q12,cy=y.cos_q12,sz=z.sin_q12,cz=z.cos_q12;
    const auto a=mul12(sy,sx),b=mul12(cy,sx);
    const std::array<std::uint32_t,9> full{{
        std::uint32_t(mul12(cy,cz))+std::uint32_t(mul12(a,sz)),0u-std::uint32_t(mul12(cy,sz))+std::uint32_t(mul12(a,cz)),std::uint32_t(mul12(sy,cx)),
        std::uint32_t(mul12(sz,cx)),std::uint32_t(mul12(cz,cx)),0u-std::uint32_t(sx),
        0u-std::uint32_t(mul12(sy,cz))+std::uint32_t(mul12(b,sz)),std::uint32_t(mul12(sy,sz))+std::uint32_t(mul12(b,cz)),std::uint32_t(mul12(cy,cx))}};
    std::array<std::int16_t,9> result{};
    for(std::size_t row=0;row<3;++row)for(std::size_t col=0;col<3;++col)result[row*3+col]=s16(full[col*3+row]);
    return result;
}
std::array<std::int32_t,3> apply_matrix_lv(const std::array<std::int16_t,9>& matrix,const std::array<std::uint32_t,3>& vector) noexcept {
    std::array<std::int16_t,3> high{},low{};
    for(std::size_t col=0;col<3;++col){const auto value=vector[col];const bool negative=s32(value)<0;const auto magnitude=negative?0u-value:value;
        const auto h=std::uint32_t(asr(magnitude,15));high[col]=s16(negative?0u-h:h);low[col]=s16(negative?0u-(magnitude&32767u):magnitude&32767u);}
    std::array<std::int32_t,3> result{};
    for(std::size_t row=0;row<3;++row){std::int64_t h=0,l=0;for(std::size_t col=0;col<3;++col){h+=std::int64_t(matrix[row*3+col])*high[col];l+=std::int64_t(matrix[row*3+col])*low[col];}
        result[row]=s32((std::uint32_t(h)<<3u)+std::uint32_t(asr12(l)));}
    return result;
}
} // namespace
SourceSceneViewProfile load_source_scene_view_profile(const std::vector<std::uint8_t>& bytes) {
    require(bytes.size()==kSourceSceneViewProfileBytes,"scene view profile size rejected");const auto* p=bytes.data();
    require(p[0]=='L'&&p[1]=='3'&&p[2]=='S'&&p[3]=='V'&&u16(p+4)==1&&u16(p+6)==64,"scene view profile magic/version rejected");
    require(u32(p+8)==1&&u32(p+12)==0x80092348u&&u32(p+16)==1025&&u32(p+20)==bytes.size()&&u32(p+28)==0,"scene view profile coverage/reserved rejected");
    constexpr std::array<std::uint8_t,32> digest{{0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0}};
    for(std::size_t i=0;i<digest.size();++i)require(p[32+i]==digest[i],"scene view EXE identity rejected");
    require(u32(p+24)==crc(p+64,bytes.size()-64),"scene view profile CRC rejected");
    SourceSceneViewProfile result;result.boot_ofx=u32(p+64);result.boot_ofy=u32(p+68);result.boot_h=u32(p+72);
    require(result.boot_ofx==(160u<<16u)&&result.boot_ofy==(120u<<16u)&&result.boot_h==384,"scene view boot projection domain rejected");
    for(std::size_t i=0;i<result.scalar_sine.size();++i){result.scalar_sine[i]=s16(u16(p+76+i*2));require(result.scalar_sine[i]>=0&&result.scalar_sine[i]<=4096,"scene view scalar trig domain rejected");}
    require(result.scalar_sine[0]==0&&result.scalar_sine[1024]==4096,"scene view scalar trig endpoints rejected");result.initialized=true;return result;
}
bool initialize_source_scene_projection_80011f48(SourceProjectionControls& owner,const SourceSceneViewProfile& profile) noexcept {
    if(!profile.initialized)return false;
    owner.ofx=profile.boot_ofx;owner.ofy=profile.boot_ofy;owner.h=profile.boot_h;return true;
}
SourceSceneViewResult produce_source_scene_view_8001e550(const SourceSceneViewInputs& input) noexcept {
    using Status=SourceSceneViewStatus;SourceSceneViewResult out;
    const auto hold=[&](Status status)noexcept{out.status=status;return out;};
    if(!input.active_resources_admitted)return hold(Status::NeedsActiveResources);
    if(!input.profile||!input.profile->initialized)return hold(Status::NeedsProfile);
    if(!input.reference)return hold(Status::NeedsReference);
    if(!input.scene||!input.scene->mode_800a395c)return hold(Status::NeedsCameraMode);
    if(*input.scene->mode_800a395c==1)return hold(Status::GameplayCameraOwned);
    if(!input.script||!input.script->byte_800bc733)return hold(Status::NeedsScriptControl);
    if(*input.script->byte_800bc733!=0)return hold(Status::NeedsTrackedActor);
    if(!input.script->byte_1f800074)return hold(Status::NeedsSourceShift);
    if(*input.script->byte_1f800074>2)return hold(Status::UnsupportedSourceShift);
    const auto& camera=input.scene->camera_800bc738;
    const auto known=[&](std::size_t offset,std::size_t n)noexcept{for(std::size_t i=0;i<n;++i)if(!camera.known[offset+i]){out.missing_camera_offset=std::uint16_t(offset+i);return false;}return true;};
    if(!known(0,1))return hold(Status::NeedsCameraBytes);
    if(camera.raw[0]!=0)return hold(Status::NeedsTargetPointCamera);
    for(std::size_t axis=0;axis<3;++axis)if(!known(8+axis*4,4)||!known(152+axis*4,4))return hold(Status::NeedsCameraBytes);
    for(const std::size_t offset:{std::size_t(0x3a),std::size_t(0x3e),std::size_t(0x42)})if(!known(offset,2))return hold(Status::NeedsCameraBytes);
    const auto& projection=input.script->projection;
    if(!projection.ofx||!projection.ofy||!projection.h)return hold(Status::NeedsProjectionControls);
    const auto h=*projection.h&65535u;
    // The native optical adapter supports positive H and original integer pixel
    // offsets. Source fixed/negative offsets remain explicit unsupported optics.
    if(h==0||h>=32768u||(*projection.ofx&65535u)||(*projection.ofy&65535u))return hold(Status::UnsupportedOptics);
    SourceSceneViewFrame frame;frame.source_shift=*input.script->byte_1f800074;
    frame.yaw=s16(u16(camera.raw.data()+0x3a));frame.pitch=s16(u16(camera.raw.data()+0x3e));frame.distance=s16(u16(camera.raw.data()+0x42));
    for(std::size_t axis=0;axis<3;++axis)frame.target[axis]=asr(u32(camera.raw.data()+8+axis*4)+u32(camera.raw.data()+152+axis*4),16);
    frame.rotation_q12=rotation_yx(*input.reference,frame.pitch,frame.yaw);
    const auto sy=scalar_sin(*input.profile,frame.yaw),cy=scalar_cos(*input.profile,frame.yaw);
    const auto sp=scalar_sin(*input.profile,frame.pitch),cp=scalar_cos(*input.profile,frame.pitch);
    const auto distance=s32(std::uint32_t(std::int32_t(frame.distance))<<frame.source_shift);
    std::array<std::uint32_t,3> inverse_eye{{
        (0u-std::uint32_t(frame.target[0]))<<frame.source_shift,
        (0u-std::uint32_t(frame.target[1]))<<frame.source_shift,
        (0u-std::uint32_t(frame.target[2]))<<frame.source_shift}};
    inverse_eye[0]+=std::uint32_t(mul12(mul12(distance,sy),cp));inverse_eye[1]+=std::uint32_t(mul12(distance,sp));inverse_eye[2]+=std::uint32_t(mul12(mul12(distance,cy),cp));
    frame.translation=apply_matrix_lv(frame.rotation_q12,inverse_eye);
    frame.eye={{s16(std::uint32_t(frame.target[0])-std::uint32_t(mul12(mul12(frame.distance,sy),cp))),
        s16(std::uint32_t(frame.target[1])-std::uint32_t(mul12(frame.distance,sp))),
        s16(std::uint32_t(frame.target[2])-std::uint32_t(mul12(mul12(frame.distance,cy),cp)))}};
    const float scale=512.0f*float(1u<<frame.source_shift);
    constexpr std::array<float,3> output_sign{{1.0f,-1.0f,-1.0f}},input_sign{{1.0f,-1.0f,1.0f}};
    for(std::size_t row=0;row<3;++row){for(std::size_t col=0;col<3;++col)frame.native_view_row_major[row*4+col]=output_sign[row]*input_sign[col]*float(frame.rotation_q12[row*3+col])/4096.0f;
        frame.native_view_row_major[row*4+3]=output_sign[row]*float(frame.translation[row])/scale;}
    frame.native_view_row_major[15]=1.0f;frame.focal_pixels=float(h);
    frame.center_x_pixels=float(s32(*projection.ofx))/65536.0f+40.0f;
    frame.center_y_pixels=float(s32(*projection.ofy))/65536.0f;
    frame.vertical_fov_degrees=frontend_vertical_fov_from_focal_pixels(frame.focal_pixels);
    out.status=Status::SourceBackedPartial;out.frame=frame;return out;
}
} // namespace l3
