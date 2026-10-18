#include "legends3ds/source_event_pipeline.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}
void operator delete[](void* p,size_t)noexcept{std::free(p);}
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint8_t digit(char c){if(c>='0'&&c<='9')return uint8_t(c-'0');if(c>='a'&&c<='f')return uint8_t(c-'a'+10);throw std::runtime_error("hex");}
template<size_t N>std::array<uint8_t,N> hex(const std::string& text){require(text.size()==2*N,"hex size");std::array<uint8_t,N> a{};for(size_t i=0;i<N;++i)a[i]=uint8_t(digit(text[2*i])*16+digit(text[2*i+1]));return a;}
void put(uint8_t* p,uint32_t v){for(size_t i=0;i<4;++i)p[i]=uint8_t(v>>(8*i));}
l3::SourceFadeContext fade(const std::array<uint8_t,72>& bytes){l3::SourceFadeContext f;size_t at=0;for(size_t i=0;i<16;++i)f.work_80098a68.raw[i]=bytes[at++];f.work_80098a68.known.fill(1);for(auto& p:f.tiles_80098a28){for(size_t i=0;i<16;++i)p.raw[i]=bytes[at++];p.known.fill(1);}for(auto& p:f.draw_modes_80098a10){for(size_t i=0;i<12;++i)p.raw[i]=bytes[at++];p.known.fill(1);}return f;}
std::array<uint8_t,72> flatten(const l3::SourceFadeContext& f){std::array<uint8_t,72> a{};size_t at=0;for(auto v:f.work_80098a68.raw)a[at++]=v;for(auto& p:f.tiles_80098a28)for(auto v:p.raw)a[at++]=v;for(auto& p:f.draw_modes_80098a10)for(auto v:p.raw)a[at++]=v;return a;}
l3::SourceFadeStartupProfile scalars(unsigned duration,unsigned step){l3::SourceFadeStartupProfile p;p.decoder=2;p.duration_80097a44=2345;p.duration_80097a46=uint16_t(duration);p.step_80097a4c=uint16_t(step);return p;}
bool same_commands(const l3::SourceEventCommandQueue& a,const l3::SourceEventCommandQueue& b){if(a.count!=b.count||a.current_index!=b.current_index)return false;for(size_t i=0;i<a.records.size();++i)if(a.records[i].raw!=b.records[i].raw||a.records[i].known!=b.records[i].known)return false;return true;}
void spawn_leaf(const char* fixture){
 std::ifstream in(fixture);require(bool(in),"spawn fixture open");std::string line;size_t cases=0,guards=0;
 while(std::getline(in,line)){
  std::istringstream row(line);char kind=0;unsigned id,argument6,state,seed;std::string transition,after,extra;
  require(bool(row>>kind>>id>>argument6>>state>>seed>>transition>>after)&&kind=='S'&&!(row>>extra),"spawn fixture row");
  L3PlayerCompatWork original{};for(size_t i=0;i<sizeof(original.bytes);++i)original.bytes[i]=uint8_t(seed*37+i*17+11);original.bytes[8]=uint8_t(state);
  l3::SourceEventHandoffTransitionIntent intent;intent.raw=hex<18>(transition);intent.source_generation=31;
  // The leaf does not read retained transition lanes6..9/12..17.
  for(size_t i=0;i<6;++i)intent.known[i]=1;intent.known[10]=intent.known[11]=1;
  L3PlayerCompatWork player=original;l3::SourceEventPlayerKnown known;known.fill(1);const auto allocation_count=allocations;
  require(l3::apply_source_handoff_spawn_80015ee8(intent,31,player,known,bool(argument6))&&allocations==allocation_count,"spawn leaf publication/allocation");
  const auto expected=hex<1184>(after);for(size_t i=0;i<expected.size();++i)if(player.bytes[i]!=expected[i])throw std::runtime_error("spawn differential case "+std::to_string(id)+" offset "+std::to_string(i));
  require(player.bytes[8]==state,"spawn forced retained player state");
  for(unsigned bad=0;bad<11;++bad){
   auto supplied=intent;auto generation=uint64_t(31);player=original;known.fill(1);
   if(bad<6)supplied.known[bad]=0;else if(bad<8)supplied.known[bad+4]=0;
   else if(bad==8)known[8]=0;else if(bad==9)generation=0;else supplied.source_generation=30;
   const auto saved_known=known;const auto count=allocations;
   require(!l3::apply_source_handoff_spawn_80015ee8(supplied,generation,player,known,bool(argument6))&&allocations==count&&known==saved_known,"spawn atomic known/generation guard");
   for(size_t i=0;i<sizeof(player.bytes);++i)require(player.bytes[i]==original.bytes[i],"held spawn changed player");++guards;
  }
  // State3 mirrors include fourth XYZ/fixed lanes and retained angle bytes.
  // Copy their validity without requiring or inventing the retained values.
  if(state==3){
   player=original;known.fill(1);for(size_t i:{size_t(0x1a),size_t(0x1b),size_t(0x28),size_t(0x29),size_t(0x2a),size_t(0x2b),size_t(0x54),size_t(0x55),size_t(0x58),size_t(0x59),size_t(0x5a),size_t(0x5b)})known[i]=0;
   require(l3::apply_source_handoff_spawn_80015ee8(intent,31,player,known,bool(argument6)),"spawn unknown retained mirror lanes held");
   for(const auto range:std::array<std::array<size_t,3>,3>{{{{0x438,0x14,8}},{{0x428,0x1c,16}},{{0x440,0x54,8}}}})
    for(size_t i=0;i<range[2];++i)require(known[range[0]+i]==known[range[1]+i]&&player.bytes[range[0]+i]==player.bytes[range[1]+i],"spawn mirror validity/value differential");
   for(size_t i=0;i<expected.size();++i)require(player.bytes[i]==expected[i],"spawn partial validity changed scalar result");
  }
  ++cases;
 }
 require(cases==64&&guards==704,"spawn fixture counts");
 std::cout<<cases<<" unchanged-original spawn cases; "<<guards<<" atomic spawn guards; retained-state and unknown-mirror checks; zero leaf allocations\n";
}
void load_completion_receipt(){
 l3::SourceEventHandoffTransitionIntent intent;intent.source_generation=31;intent.destination_area=1;
 intent.known[12]=1;intent.raw[12]=2;
 int owner=0;std::optional<int8_t> status=2;
 l3::SourceAreaLoadCompletionReceipt receipt{&owner,31,1,0x80015840u,0x80038648u,true,true,true};
 require(l3::acknowledge_source_area_load_80015840(intent,receipt,&owner,status)&&status&&*status==0,
  "source area load completion receipt");
 for(unsigned bad=0;bad<11;++bad){
  auto supplied=receipt;auto candidate=intent;status=2;const void* current=&owner;
  switch(bad){case 0:supplied.native_owner=nullptr;break;case 1:supplied.source_generation=30;break;
   case 2:supplied.destination_area=2;break;case 3:supplied.source_callback=0x80015844u;break;
   case 4:supplied.camera_initializer=0x8003864cu;break;case 5:supplied.area_replaced=false;break;
   case 6:supplied.spawn_applied=false;break;case 7:supplied.gameplay_camera_initialized=false;break;
   case 8:candidate.known[12]=0;break;case 9:status=1;break;default:current=nullptr;break;}
  const auto before=status;
  require(!l3::acknowledge_source_area_load_80015840(candidate,supplied,current,status)&&status==before,
   "invalid source area load completion accepted");
 }
 status.reset();require(!l3::acknowledge_source_area_load_80015840(intent,receipt,&owner,status)&&!status,
  "unknown transition status fabricated completion");
}
void head_uv_reset(){
 std::array<l3::PbdModel,l3::kPlayerComponentSlots> models;auto& head=models[1];head.parts.resize(4);
 for(size_t i=0;i<4;++i)head.parts[i].part_id=uint8_t(i?14+i:1);
 l3::PlayerComposition composition;
 for(size_t i=2;i<4;++i){auto& part=head.parts[i];part.vertices.resize(1);
  l3::PbdTri tri;tri.code=0x25;tri.clut=uint16_t(0x1200+i);tri.tpage=uint16_t(0x3400+i);
  for(size_t k=0;k<tri.uv.size();++k)tri.uv[k]=uint8_t(k*17+i);part.triangles.push_back(tri);
  l3::PbdQuad quad;quad.code=0x2d;quad.clut=uint16_t(0x5600+i);quad.tpage=uint16_t(0x7800+i);
  for(size_t k=0;k<quad.uv.size();++k)quad.uv[k]=uint8_t(k*31+i);part.quads.push_back(quad);
  composition.attachments.push_back({1,uint8_t(i),part.part_id,1});
 }
 auto live=l3::build_player_visuals(models,composition);l3::PlayerVisualPrimitive body;body.vertex_count=3;body.part_id=16;body.uv[0]={{71,93}};
 live.insert(live.begin(),body);const auto original=live;
 const auto baseline=l3::capture_source_event_head_uv_baseline(head,live);require(baseline&&baseline->record_count()==4,"small head baseline");
 const auto unchanged=[&](const auto& before){for(size_t i=0;i<live.size();++i)require(live[i].uv==before[i].uv&&live[i].clut==before[i].clut&&live[i].tpage==before[i].tpage,"held UV owner changed");};
 for(size_t i=1;i<live.size();++i){for(auto& uv:live[i].uv)uv={{255,254}};live[i].clut=1;live[i].tpage=2;}
 // A mismatch at the last record must not partially reset preceding records.
 live.back().transform_id=0;const auto held=live;const auto count=allocations;
 require(!l3::restore_source_event_head_uv_8002d004(*baseline,live,&live,31)&&allocations==count,"head stale topology accepted");unchanged(held);
 live.back().transform_id=1;
 require(!l3::restore_source_event_head_uv_8002d004(*baseline,live,nullptr,31)&&!l3::restore_source_event_head_uv_8002d004(*baseline,live,&live,0),"head missing owner accepted");
 const auto allocation_count=allocations;const auto receipt=l3::restore_source_event_head_uv_8002d004(*baseline,live,&live,31);
 require(receipt&&receipt->admitted&&receipt->native_owner==&live&&receipt->source_generation==31&&
  receipt->completed_function==0x8002d004u&&receipt->first_header==0x800f7820u&&receipt->second_header==0x800f7830u&&
  !receipt->argument5&&!receipt->argument6&&allocations==allocation_count,"head actual reset receipt");
 require(live[0].uv==body.uv&&live[0].clut==body.clut&&live[0].tpage==body.tpage,"unrelated body modified");
 for(size_t i=1;i<live.size();++i){require(live[i].clut==original[i].clut&&live[i].tpage==original[i].tpage,"head material reset");
  for(size_t v=0;v<live[i].vertex_count;++v)require(live[i].uv[v]==original[i].uv[v],"head UV reset");
  if(live[i].vertex_count==3)require(live[i].uv[3]==std::array<uint8_t,2>{{255,254}},"unused triangle vertex overwritten");}
 l3::PlayerPoseMatrices pose;const auto posed=l3::pose_player_visuals(live,pose);
 for(size_t i=0;i<live.size();++i)require(posed[i].uv==live[i].uv&&posed[i].clut==live[i].clut&&posed[i].tpage==live[i].tpage,"reset not visible to pose bridge");
 // The receipt comes from the real live reset above, then enables the original
 // event handoff. Merely having loaded a model is not a completion receipt.
 l3::SourceEventWork work;work.known.fill(1);work.raw[0]=work.raw[1]=1;work.raw[4]=7;
 L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known;known.fill(1);
 auto flags=l3::initialize_source_story_flags_8001d8c0();l3::SourceEventCommandQueue queue;queue.initialized=true;
 l3::SourceEventPhase6Context context;context.awaiting_caller_801011e8=true;
 l3::SourceFadeContext fade;fade.work_80098a68.known[10]=fade.work_80098a68.known[11]=1;
 l3::SourceEventHandoffInputs input;input.player=&player;input.player_known=&known;input.flags=&flags;input.commands=&queue;input.display=&*receipt;input.source_generation=31;
 const auto handoff=l3::consume_source_event1_handoff_80101260(work,context,fade,input);
 require(handoff.committed&&context.transition_pending&&context.transition_pending->source_generation==31&&
  context.transition_pending->known[10]&&context.transition_pending->known[11]&&
  !context.transition_pending->raw[10]&&!context.transition_pending->raw[11],"actual reset receipt did not enable source handoff");
 auto wrong=original;wrong.back().uv[0][0]^=1;require(!l3::capture_source_event_head_uv_baseline(head,wrong),"shifted UV accepted as baseline");
 wrong=original;wrong.pop_back();require(!l3::capture_source_event_head_uv_baseline(head,wrong),"partial head baseline accepted");
 auto malformed=head;malformed.parts[2].part_id=15;require(!l3::capture_source_event_head_uv_baseline(malformed,original),"wrong head header identity accepted");
}
void pipeline_route(){
 // Authored already-completed callback/camera/timeline continuation, using the
 // inherited positive EBD witness. This asserts routing, not live chronology.
 std::vector<uint8_t> bytes(232);for(size_t i=0;i<bytes.size();++i)bytes[i]=uint8_t(i*7+3);put(bytes.data(),14);
 for(size_t i=0;i<14;++i){auto* row=bytes.data()+4+i*16;row[0]=0xe0;row[1]=3;row[2]=uint8_t(i%4);row[3]=0;put(row+4,0x8016c000u+228u+uint32_t(i%4));put(row+8,0);put(row+12,0);}
 const auto directory=l3::parse_ebd_directory(bytes,0x8016c000);
 l3::SourceEventPhase5Profile p5;p5.initialized=true;p5.directory_token=0x8011af4c;p5.callback_table_token=0x8011b2e8;
 p5.ebd_source_digest=hex<32>("a5cc2dc41d7325d8e23aca0856bf6f486a81c19316e55bc37424bb49c3078207");
 for(size_t i=0;i<16;++i){auto& r=p5.records[i];r.raw[0]=uint8_t(i);r.raw[1]=i?3:5;r.raw[3]=i?0xe0:0x40;if(i){r.resource_variant=uint8_t((i-1)%4);r.resource_directory_index=r.resource_variant;r.resource_token=0x8016c000u+228u+r.resource_variant;}}
 const auto witness=l3::bind_source_event_phase5_resources(p5,directory);require(witness.admitted(),"pipeline EBD witness");
 l3::SourceEventWork authored;authored.known.fill(1);authored.raw[0]=authored.raw[1]=1;authored.raw[4]=6;authored.raw[9]=19;put(authored.raw.data()+16,44);
 std::optional<l3::SourceEventWork> event=authored;l3::SourceEventPhase5Context scene;scene.byte_80122428=0;
 l3::SourceEventPhase6Context state;state.awaiting_caller_801011e8=true;
 l3::SourceFadeContext f;f.work_80098a68.known[10]=f.work_80098a68.known[11]=1;auto profile=scalars(3,31);
 l3::SourceFadeSchedulerIntentQueue intents;intents.initialized=true;auto flags=l3::initialize_source_story_flags_8001d8c0();
 l3::SourceEventInputs input;input.flags=&flags;input.phase5_context=&scene;input.phase5_profile=&p5;input.phase5_resources=&witness;input.phase6_context=&state;
 input.busy_80098a72=0;input.fade_context=&f;input.fade_profile=&profile;input.fade_intents=&intents;
 auto result=l3::step_source_event_pipeline(event,nullptr,input);
 require(result.committed&&event->phase04()==7&&intents.count==1&&!state.awaiting_caller_801011e8,"pipeline type10 phase7 routing");
 // An independently supplied later source fade-completion state and completed
 // scene continuation may approach the handoff; missing display must hold.
 f.work_80098a68.raw[10]=f.work_80098a68.raw[11]=0;state.awaiting_caller_801011e8=true;
 L3PlayerCompatWork player{};l3::SourceEventPlayerKnown known;known.fill(1);l3::SourceEventCommandQueue commands;commands.initialized=true;
 l3::SourceEventHandoffInputs handoff;handoff.player=&player;handoff.player_known=&known;handoff.flags=&flags;handoff.commands=&commands;handoff.source_generation=31;input.handoff=&handoff;
 const auto before=*event;result=l3::step_source_event_pipeline(event,nullptr,input);
 require(!result.committed&&result.phase6_consumer_status==l3::SourceEventPhase6Status::NeedsHandoffDisplayOwner&&event->raw==before.raw&&event->known==before.known&&!state.transition_pending&&state.awaiting_caller_801011e8&&commands.count==0,"pipeline missing display fabricated completion");
}
}
int main(int argc,char** argv){try{
 head_uv_reset();
 load_completion_receipt();
 require(argc==3,"fixture arguments");spawn_leaf(argv[2]);std::ifstream in(argv[1]);require(bool(in),"fixture open");std::string line;size_t fades=0,handoffs=0,guards=0;
 while(std::getline(in,line)){
  std::istringstream row(line);char kind=0;unsigned id=0;row>>kind>>id;
  if(kind=='F'){
   unsigned duration,step,busy;std::string before,after,extra;require(bool(row>>duration>>step>>busy>>before>>after)&&!(row>>extra),"fade row");
   const auto initial=hex<72>(before);auto f=fade(initial);auto p=scalars(duration,step);l3::SourceFadeSchedulerIntentQueue q;q.initialized=true;
   const auto count=allocations;auto result=l3::consume_source_fade_startup_80017c30_type10(f,&p,&q);
   require(result.committed&&flatten(f)==hex<72>(after)&&q.count==(busy?0u:1u)&&allocations==count,"type10 source differential");++fades;
   if(!busy){for(unsigned bad=0;bad<5;++bad){auto held=fade(initial);auto supplied=p;auto queue=q;queue.count=0;const auto initial_queue=queue;
     switch(bad){case 0:supplied.duration_80097a46.reset();break;case 1:supplied.step_80097a4c.reset();break;case 2:supplied.decoder=1;break;case 3:queue.initialized=false;break;default:queue.count=queue.entries.size();break;}
     const auto before_queue=queue;result=l3::consume_source_fade_startup_80017c30_type10(held,&supplied,&queue);
     require(!result.committed&&flatten(held)==initial&&queue.count==before_queue.count&&queue.initialized==before_queue.initialized,"type10 atomic guard");(void)initial_queue;++guards;}
   }
  }else if(kind=='H'){
   unsigned count,after_count;std::string wb,wa,pb,pa,tb,ta,fb,fa,qb,qa,extra;
   require(bool(row>>count>>after_count>>wb>>wa>>pb>>pa>>tb>>ta>>fb>>fa>>qb>>qa)&&!(row>>extra),"handoff row");
   auto work=l3::source_event_work_from_original(hex<168>(wb));L3PlayerCompatWork player{};
   const auto player_before=hex<1184>(pb);for(size_t i=0;i<1184;++i)player.bytes[i]=player_before[i];
   l3::SourceEventPlayerKnown known;known.fill(1);auto flags=l3::source_event_flags_from_original(hex<256>(fb));
   l3::SourceEventCommandQueue commands;commands.initialized=true;commands.count=count;const auto queue_before=hex<256>(qb);
   for(size_t i=0;i<16;++i){for(size_t k=0;k<16;++k)commands.records[i].raw[k]=queue_before[16*i+k];commands.records[i].known.fill(1);}
   l3::SourceEventPhase6Context ctx;ctx.awaiting_caller_801011e8=true;l3::SourceFadeContext f;f.work_80098a68.known[10]=f.work_80098a68.known[11]=1;
   l3::SourceEventHandoffDisplayReceipt receipt;receipt.native_owner=&receipt;receipt.source_generation=31;receipt.completed_function=0x8002d004;receipt.first_header=0x800f7820;receipt.second_header=0x800f7830;receipt.admitted=true;
   l3::SourceEventHandoffInputs input;input.player=&player;input.player_known=&known;input.flags=&flags;input.commands=&commands;input.display=&receipt;input.source_generation=31;
   const auto original_work=work;const auto original_flags=flags;const auto original_player=player;const auto original_known=known;const auto original_commands=commands;
   const auto allocation_count=allocations;const auto result=l3::consume_source_event1_handoff_80101260(work,ctx,f,input);
   require(result.committed&&result.player_resumed&&result.transition_produced&&ctx.transition_pending&&allocations==allocation_count,"handoff publication");
   require(work.raw==hex<168>(wa)&&flags.values==l3::source_event_flags_from_original(hex<256>(fa)).values&&commands.count==after_count,"handoff scalar differential");
   const auto expected_player=hex<1184>(pa);const auto expected_queue=hex<256>(qa);for(size_t i=0;i<1184;++i)if(player.bytes[i]!=expected_player[i])throw std::runtime_error("player resume case "+std::to_string(id)+" offset "+std::to_string(i)+" got "+std::to_string(player.bytes[i])+" expected "+std::to_string(expected_player[i]));
   for(size_t i=0;i<256;++i)require(commands.records[i/16].raw[i%16]==expected_queue[i],"source command compaction differential");
   auto transition=hex<18>(tb);const auto& intent=*ctx.transition_pending;for(size_t i=0;i<18;++i)if(intent.known[i])transition[i]=intent.raw[i];
   require(transition==hex<18>(ta)&&intent.spawn==std::array<int16_t,3>{{2822,-1,-4833}}&&intent.destination_area==1&&intent.source_generation==31,"masked transition differential");
   for(unsigned bad=0;bad<10;++bad){
    work=original_work;player=original_player;known=original_known;flags=original_flags;commands=original_commands;ctx={};ctx.awaiting_caller_801011e8=true;
    auto supplied=input;auto bad_receipt=receipt;supplied.display=&bad_receipt;auto fade_owner=f;
    switch(bad){case 0:supplied.player=nullptr;break;case 1:known[8]=0;break;case 2:supplied.flags=nullptr;break;case 3:supplied.commands=nullptr;break;case 4:supplied.display=nullptr;break;case 5:bad_receipt.source_generation=30;break;case 6:bad_receipt.first_header=0x800f7824;break;case 7:bad_receipt.argument5=1;break;case 8:fade_owner.work_80098a68.known[10]=0;break;default:commands.initialized=false;break;}
    const auto saved_known=known;const auto saved_commands=commands;const auto held=l3::consume_source_event1_handoff_80101260(work,ctx,fade_owner,supplied);
    require(!held.committed&&!ctx.transition_pending&&work.raw==original_work.raw&&known==saved_known&&flags.values==original_flags.values&&same_commands(commands,saved_commands),"handoff atomic owner guard");
    for(size_t i=0;i<1184;++i)require(player.bytes[i]==original_player.bytes[i],"held player changed");++guards;
   }
   ++handoffs;
  }else throw std::runtime_error("fixture kind");
 }
 require(fades==18&&handoffs==48,"fixture counts");
 // Type10 caller condition and phase publication use actual owned fade busy.
 for(unsigned condition=0;condition<4;++condition){l3::SourceEventWork w;w.known.fill(1);w.raw[0]=w.raw[1]=1;w.raw[4]=6;w.raw[9]=condition==0?19:0;put(w.raw.data()+16,44);
  l3::SourceEventPhase6Context c;c.awaiting_caller_801011e8=true;c.byte_800c1b6a=condition==1?1:0;c.input_800c0c2a=condition==1?8:0;
  auto flags=l3::initialize_source_story_flags_8001d8c0();l3::SourceFadeContext f;f.work_80098a68.known[10]=f.work_80098a68.known[11]=1;if(condition==3)f.work_80098a68.raw[10]=1;
  auto p=scalars(3,31);l3::SourceFadeSchedulerIntentQueue q;q.initialized=true;const auto result=l3::consume_source_event1_type10_fade_8010122c(w,c,f,&p,&q,&flags);
  require(result.committed==(condition<2)&&w.raw[4]==(condition<2?7:6)&&q.count==(condition<2?1u:0u),"caller condition/phase7 gate");++guards;
 }
 pipeline_route();
 std::cout<<fades<<" type10 + "<<handoffs<<" unchanged-original handoff cases; "<<guards<<" atomic/condition guards; pipeline routing/held display; zero leaf allocations\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
