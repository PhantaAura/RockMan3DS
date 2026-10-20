// The original 0x80037FB4 camera, facing -z, puts world -x on screen-right.
// A right-handed LookAt over (x, -y, z) vertices puts it on the left, which
// is why the 3DS LookAt path negates camera-space x. Trig values are authored.
#include "legends3ds/render/frontend_view.h"
#include "legends3ds/source_gameplay_camera.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
using namespace l3;
ReferenceTables refs(){
    ReferenceTables r;
    for(size_t i=0;i<r.trig.size();++i){const double a=double(i)*6.283185307179586/4096.0;
        r.trig[i]={int16_t(std::lround(std::sin(a)*4096)),int16_t(std::lround(std::cos(a)*4096))};}
    for(size_t i=0;i<r.scalar_quarter.size();++i)r.scalar_quarter[i]=int16_t(std::lround(std::sin(double(i)*1.5707963267948966/1024.0)*4096));
    r.scalar_quarter_known=true;return r;
}
// RTPS with sf=1 over the 0x1F800008 MATRIX layout, OFX 160, H 384.
double gte_sx(const std::array<uint32_t,8>& w,const std::array<int32_t,3>& p){
    auto lo=[](uint32_t x){return int64_t(int16_t(x&0xffffu));};
    auto hi=[](uint32_t x){return int64_t(int16_t(x>>16));};
    const int64_t m[3][3]={{lo(w[0]),hi(w[0]),lo(w[1])},{hi(w[1]),lo(w[2]),hi(w[2])},{lo(w[3]),hi(w[3]),lo(w[4])}};
    int64_t c[3];
    for(int r=0;r<3;++r)c[r]=(int64_t(int32_t(w[5+r]))*4096+m[r][0]*p[0]+m[r][1]*p[1]+m[r][2]*p[2])>>12;
    assert(c[2]>0);
    return 160.0+384.0*double(c[0])/double(c[2]);
}
}

int main(){
    const auto r=refs();
    const std::array<int32_t,3> target{{2822,-150,-4833}};
    // Tutorial start: yaw 2048, eye behind Mega Man (+z), looking down -z.
    const auto frame=source_gameplay_camera_80037fb4(r,target,512,2048,0x40,0,0);
    assert(frame.eye_800a39f0[2]>target[2]);
    const std::array<int32_t,3> minus_x{{target[0]-300,target[1],target[2]-800}};
    const std::array<int32_t,3> plus_x{{target[0]+300,target[1],target[2]-800}};
    assert(gte_sx(frame.camera_1f800008,minus_x)>160.0&&gte_sx(frame.camera_1f800008,plus_x)<160.0);
    // Frontend LookAt convention (same basis as Mtx_LookAt, right = f x up).
    FrontendViewSettings s;
    const FrontendCameraFrame cam{frontend_world_point(float(frame.eye_800a39f0[0]),float(frame.eye_800a39f0[1]),float(frame.eye_800a39f0[2])),
                                  frontend_world_point(float(target[0]),float(target[1]),float(target[2]))};
    const auto lx=project_frontend_point(frontend_world_point(float(minus_x[0]),float(minus_x[1]),float(minus_x[2])),cam,s);
    assert(lx.valid&&lx.x_pixels<200.0f); // unflipped LookAt: -x lands left, the mirror
    std::cout<<"PASS: original camera puts -x right; unflipped LookAt mirrors it\n";
}
