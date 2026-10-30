// Real Area 1 map: a morphed corridor pop-out (subtype 3, parameter 0x40) through action 8.
#include "legends3ds/actor_reaction_contact.h"
#include "legends3ds/class20_03_behavior.h"
#include "legends3ds/source_combat_profile.h"
#include "legends3ds/source_reaction_profile.h"
#include "legends3ds/l3map.h"
#include <cstdio>
#include <fstream>
#include <iterator>
static std::vector<uint8_t> rd(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int,char**v){std::string root=v[1];
 auto map=l3::load_l3map(rd(root+"/st00.l3map"));auto refs=l3::load_l3reference(rd(root+"/reference.l3ref"));
 auto cb=rd(root+"/actor_combat.l3cb");auto profiles=l3::load_l3source_combat_profiles(cb.data(),cb.size());
 auto rb=rd(root+"/actor_reactions.l3r7");auto reaction=l3::load_source_reaction_profile(rb.data(),rb.size());
 const auto* p=l3::find_source_combat_profile(profiles,1,0x20,3,0,0);
 if(!p||!p->unsigned_byte0b){std::puts("no profile/byte0b");return 1;}
 std::printf("hp=%d byte0b=%u\n",p->initial_hp(),unsigned(*p->unsigned_byte0b));
 l3::Class20_03CallbackState s;s.behavior.action_09=8;s.behavior.parameter_03=0x40;s.health_6e=p->initial_hp();
 s.behavior.heading_56=1024;s.behavior.flags_328=14;
 s.raw_14={{2624,-129,-7040,0}};for(int i=0;i<4;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
 std::array<int16_t,3> vel{};uint8_t st74=0;
 const l3::RetailPlayerBounds bounds{-60,60,-60,60,-128,0};
 for(int f=0;f<80;++f){
  s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;s.velocity_44=vel;
  l3::Class20_03ReactionInputs in;in.profile=&reaction;in.combat_byte_b=*p->unsigned_byte0b;in.combat_record_variant=0;
  in.target_heading_80031198=0;in.sound_consumer_ready=true;
  l3::ActorContactWork w;w.raw=s.raw_14;w.fixed=s.fixed_1c;w.previous_raw=s.previous_raw_2c;w.previous_fixed=s.previous_fixed_34;
  w.velocity=vel;w.state74=st74;w.object_class=0x20;
  auto prep=l3::prepare_actor_reaction_contact(s,in,w,map,1,refs,bounds);
  if(prep.inputs)in.motion=prep.inputs;
  auto r=l3::class20_03_step_reaction(s,in);
  if(r.status!=l3::Class20_03ReactionStatus::SourceBackedPartial){std::printf("f%d prep=%u held=%u\n",f,unsigned(prep.status),unsigned(r.status));break;}
  vel=s.velocity_44;st74=prep.after_wall.state74;
  std::printf("f%02d phase=%u aux=%u anim=%u timer=%d pos=%d:%d:%d contact=%x%s\n",f,s.behavior.phase_0a,s.aux_0b,s.behavior.animation_ac,
    s.behavior.timer_32c,s.raw_14[0],s.raw_14[1],s.raw_14[2],s.work_370,r.sound_8001a050?" LAND":"");
  if(s.behavior.phase_0a==3&&s.behavior.timer_32c<0)break;
  if(s.behavior.flags_328&1)break;
 }}
