// Real Area 1 map + retail trig: a woken trap-room 0x19 for 1200 frames.
#include "legends3ds/class20_19.h"
#include "legends3ds/l3map.h"
#include "legends3ds/source_angle.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
static std::vector<uint8_t> rd(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int,char**v){std::string root=v[1];
 auto map=l3::load_l3map(rd(root+"/st00.l3map"));map.idx[119].descriptor_index=19;auto refs=l3::load_l3reference(rd(root+"/reference.l3ref"));
 auto ab=rd(root+"/actor_angle.l3ref");auto steps=l3::load_l3actor_angle(ab.data(),ab.size());
 l3::Class20_03MotionWorld w;w.map=&map;w.area_index=1;w.references=&refs;w.bounds={-32,32,-32,32,-32,0};w.source_a6=0x30;
 for(int which=0;which<2;++which){
 l3::Class20_19State s;s.work_00=1;s.parameter_03=1;s.heading_56=0x200;
 s.raw_14={int16_t(which?-2814:-3922),-200,-4860,0x607};for(int i=0;i<3;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
 s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;
 l3::Class20_19Inputs in;in.world=&w;in.record=l3::Class20_19Record{32,16,16,6,0x103};in.angle_steps=&steps;
 in.player_raw={-3328,-1,-4870};in.player_fixed={-3328*65536,-65536,-4870*65536};in.player_heading=2048;in.sound_consumer_ready=true;
 in.wake_flag_set=true;in.detected_8003648c=true;
 uint32_t rng=12345;std::map<int,int> statuses;int actions[8]{};int y_min=0,y_max=-999;
 for(int f=0;f<1200;++f){rng=rng*1103515245u+12345u;in.random_sample=uint16_t((rng>>16)&0x7fff);
  const int dx=s.raw_14[0]+3328,dz=s.raw_14[2]+4870;in.work_80=uint32_t(dx*dx+dz*dz);
  in.wake_flag_set=f>0;auto r=l3::step_class20_19_80104864(s,in);statuses[int(r.status)]++;
  if(r.status==l3::Class20_19Status::SourceBackedPartial){actions[s.action_09&7]++;if(s.raw_14[1]<y_min)y_min=s.raw_14[1];if(f>200&&s.raw_14[1]>y_max)y_max=s.raw_14[1];}
  w.state74=r.state74_after;}
 std::printf("reaverbot %d end pos=%d:%d:%d action=%u hidden=%d statuses:",which,s.raw_14[0],s.raw_14[1],s.raw_14[2],s.action_09,!(s.work_00&2));
 for(auto&[k,c]:statuses)std::printf(" %d:%d",k,c);std::printf(" | frames per action:");for(int i=0;i<6;++i)std::printf(" %d",actions[i]);std::printf(" | y_min=%d y_late_max=%d\n",y_min,y_max);}
}
