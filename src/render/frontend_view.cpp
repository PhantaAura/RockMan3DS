#include "legends3ds/render/frontend_view.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <array>

namespace l3 {
namespace {
constexpr float kTau=6.2831853071795864769f;
bool finite(FrontendPoint p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
std::string trim(const std::string& s) {
    const auto a=s.find_first_not_of(" \t\r");
    if(a==std::string::npos)return {};
    return s.substr(a,s.find_last_not_of(" \t\r")-a+1);
}
}
float frontend_vertical_fov_from_focal_pixels(float focal) noexcept {
    if(!std::isfinite(focal)||focal<=0)return std::numeric_limits<float>::quiet_NaN();
    return 2.0f*std::atan(120.0f/focal)*360.0f/kTau;
}
FrontendViewSettings legacy_frontend_view() noexcept {
    FrontendViewSettings s;s.vertical_fov_degrees=62;s.follow_distance=5.2f;
    s.eye_height=3;s.target_height=1.1f;return s;
}
bool valid_frontend_view(const FrontendViewSettings& s) noexcept {
    return std::isfinite(s.vertical_fov_degrees)&&s.vertical_fov_degrees>=10&&s.vertical_fov_degrees<=120&&
        std::isfinite(s.follow_distance)&&s.follow_distance>=0.1f&&s.follow_distance<=60&&
        std::isfinite(s.eye_height)&&s.eye_height>=-30&&s.eye_height<=30&&
        std::isfinite(s.target_height)&&s.target_height>=-30&&s.target_height<=30;
}
FrontendViewSettings parse_frontend_view_config(const std::string& text) {
    if(text.size()>1024||text.find('\0')!=std::string::npos)throw std::runtime_error("view config size/encoding");
    FrontendViewSettings s;std::array<bool,4> seen{};
    size_t cursor=0;
    while(cursor<text.size()) {
        const auto end_line=text.find('\n',cursor);
        const size_t stop=end_line==std::string::npos?text.size():end_line;
        auto line=trim(text.substr(cursor,stop-cursor));cursor=stop+1;
        if(line.empty()||line.front()=='#')continue;
        const auto eq=line.find('=');if(eq==std::string::npos)throw std::runtime_error("view config requires key=value");
        const auto key=trim(line.substr(0,eq)), value=trim(line.substr(eq+1));
        size_t slot;float* out=nullptr;
        if(key=="vertical_fov_degrees"){slot=0;out=&s.vertical_fov_degrees;}
        else if(key=="follow_distance"){slot=1;out=&s.follow_distance;}
        else if(key=="eye_height"){slot=2;out=&s.eye_height;}
        else if(key=="target_height"){slot=3;out=&s.target_height;}
        else throw std::runtime_error("unknown view config key");
        if(seen[slot])throw std::runtime_error("duplicate view config key");
        seen[slot]=true;char* end=nullptr;*out=std::strtof(value.c_str(),&end);
        if(value.empty()||end!=value.c_str()+value.size())throw std::runtime_error("invalid view config number");
    }
    if(!valid_frontend_view(s))throw std::runtime_error("view config out of diagnostic range");
    return s;
}
FrontendPoint frontend_world_point(float x,float y,float z) noexcept {
    return {x*kFrontendWorldScale,-y*kFrontendWorldScale,z*kFrontendWorldScale};
}
FrontendCameraFrame frontend_follow_camera(FrontendPoint p,unsigned yaw,const FrontendViewSettings& s) noexcept {
    const float a=float(yaw&4095u)*kTau/4096.0f;
    return {{p.x+std::sin(a)*s.follow_distance,p.y+s.eye_height,p.z+std::cos(a)*s.follow_distance},
        {p.x,p.y+s.target_height,p.z}};
}
FrontendProjection project_frontend_point(FrontendPoint p,const FrontendCameraFrame& c,const FrontendViewSettings& s) noexcept {
    FrontendProjection r;if(!valid_frontend_view(s)||!finite(p)||!finite(c.eye)||!finite(c.target))return r;
    float fx=c.target.x-c.eye.x,fy=c.target.y-c.eye.y,fz=c.target.z-c.eye.z;
    const float norm=std::sqrt(fx*fx+fy*fy+fz*fz);if(norm<=0)return r;
    fx/=norm;fy/=norm;fz/=norm;
    const float horizontal=std::sqrt(fx*fx+fz*fz);if(horizontal<=0)return r;
    const float rx=-fz/horizontal,rz=fx/horizontal;
    const float ux=-rz*fy,uy=rz*fx-rx*fz,uz=rx*fy;
    const float x=p.x-c.eye.x,y=p.y-c.eye.y,z=p.z-c.eye.z;
    r.depth=x*fx+y*fy+z*fz;if(r.depth<=0)return r;
    const float extent=r.depth*std::tan(s.vertical_fov_degrees*kTau/720.0f);
    const float right=x*rx+z*rz,up=x*ux+y*uy+z*uz;
    r.x_pixels=200.0f+120.0f*right/extent;
    r.y_pixels=120.0f-120.0f*up/extent;
    r.valid=std::isfinite(r.depth)&&std::isfinite(r.x_pixels)&&std::isfinite(r.y_pixels);
    r.visible=r.valid&&r.depth>=kFrontendNear&&r.depth<=kFrontendFar&&std::abs(up)<=extent&&std::abs(right)<=extent*kFrontendAspect;
    return r;
}
}
