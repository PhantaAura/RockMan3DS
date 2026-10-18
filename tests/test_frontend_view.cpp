#include "legends3ds/render/frontend_view.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
bool close(float a,float b){return std::abs(a-b)<0.0001f;}
void rejects(const std::string& s) {
    bool bad=false;try{(void)l3::parse_frontend_view_config(s);}catch(const std::runtime_error&){bad=true;}
    assert(bad);
}
}
int main() {
    const l3::FrontendViewSettings original{};
    assert(l3::valid_frontend_view(original));
    const auto p=l3::frontend_world_point(512,1024,-512);
    assert(close(p.x,1)&&close(p.y,-2)&&close(p.z,-1));
    const auto camera=l3::frontend_follow_camera(p,0);
    assert(close(camera.eye.z,p.z+511.0f/512.0f)&&close(camera.eye.y,p.y+187.0f/512.0f));
    assert(close(original.vertical_fov_degrees,2.0f*std::atan(120.0f/384.0f)*180.0f/3.14159265358979323846f));
    const auto legacy=l3::frontend_follow_camera(p,0,l3::legacy_frontend_view());
    assert(close(legacy.eye.z,4.2f)&&close(legacy.eye.y,1));
    assert(std::isnan(l3::frontend_vertical_fov_from_focal_pixels(0)));
    const auto center=l3::project_frontend_point(camera.target,camera);
    assert(center.visible&&close(center.x_pixels,200)&&close(center.y_pixels,120));
    auto point=camera.target;point.x+=0.1f;
    const auto right=l3::project_frontend_point(point,camera);
    assert(right.visible&&right.x_pixels>center.x_pixels);
    auto wide=original;wide.vertical_fov_degrees=90;
    const auto smaller=l3::project_frontend_point(point,camera,wide);
    assert(smaller.visible&&smaller.x_pixels<right.x_pixels);
    point=camera.target;point.x+=1;
    assert(!l3::project_frontend_point(point,camera).visible);
    assert(l3::project_frontend_point(point,camera,wide).visible);
    assert(!l3::project_frontend_point(camera.eye,camera).valid);
    point=camera.eye;point.z+=1;
    assert(!l3::project_frontend_point(point,camera).visible);
    const auto quarter=l3::frontend_follow_camera(p,1024);
    assert(close(quarter.eye.x,p.x+original.follow_distance));
    assert(l3::project_frontend_point(quarter.target,quarter).visible);
    assert(!l3::project_frontend_point({std::numeric_limits<float>::infinity(),0,0},camera).valid);
    assert(!l3::project_frontend_point({},{{},{}}).valid);
    const auto parsed=l3::parse_frontend_view_config("# local measurement\nvertical_fov_degrees = 50\nfollow_distance=4\neye_height=2\ntarget_height=1\n");
    assert(close(parsed.vertical_fov_degrees,50)&&close(parsed.follow_distance,4));
    assert(close(l3::parse_frontend_view_config("").follow_distance,original.follow_distance));
    rejects("vertical_fov_degrees=nan");rejects("vertical_fov_degrees=0");
    rejects("follow_distance=1e999");rejects("target_height=31");
    rejects("follow_distance=0");rejects("world_scale=1");
    rejects("eye_height=2\neye_height=3");rejects("target_height=1junk");
    rejects(std::string(1025,' '));rejects(std::string("eye_height=2\0",13));
    return 0;
}
