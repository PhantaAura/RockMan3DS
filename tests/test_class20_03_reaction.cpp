#include "legends3ds/class20_03_reaction.h"
#include "legends3ds/actor_reaction_contact.h"
#include "legends3ds/actor_runtime.h"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <tuple>
namespace {bool measured=false;size_t allocations=0;}
void* operator new(std::size_t n){if(measured)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using State=l3::Class20_03CallbackState;using Status=l3::Class20_03ReactionStatus;
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
l3::SourceReactionProfile profile(){
    l3::SourceReactionProfile p;
    for(size_t i=0;i<3;++i)for(size_t j=0;j<12;++j)p.timer_pairs[i][j]=uint8_t(i*53+j*17+3);
    return p;
}
auto snapshot(const State& s){const auto& b=s.behavior;return std::make_tuple(
    b.parameter_03,b.action_09,b.phase_0a,s.aux_0b,b.animation_ac,b.flags_328,b.timer_32c,b.work_330,
    s.random_32a,s.health_6e,s.velocity_44,s.reaction_table_364,s.work_370,b.heading_56,
    b.work_36c,s.raw_14,s.fixed_1c,s.packet_33c,s.hit_flags_340,s.hit_point_344,s.hit_word_34a,
    s.previous_raw_2c,s.previous_fixed_34);}
State seed(const std::array<int64_t,48>& r){
    State s;auto& b=s.behavior;b.action_09=uint8_t(r[0]);b.phase_0a=uint8_t(r[1]);s.aux_0b=uint8_t(r[2]);
    b.timer_32c=int16_t(r[3]);b.flags_328=uint16_t(r[4]);b.parameter_03=uint8_t(r[5]);s.health_6e=int16_t(r[6]);
    s.random_32a=uint16_t(r[7]);s.reaction_table_364=uint32_t(r[8]);
    for(size_t i=0;i<3;++i)s.velocity_44[i]=int16_t(r[9+i]);b.heading_56=uint16_t(r[12]);
    b.animation_ac=251;b.work_330=731;b.work_36c=-117;s.work_370=0xa5a5f00d;
    s.raw_14={{101,-7,-202,1234}};s.fixed_1c={{123456,-76543,999,0x13579}};
    return s;
}
l3::Class20_03ReactionInputs inputs(const State& s,const l3::SourceReactionProfile& p,const std::array<int64_t,48>& r){
    l3::Class20_03ReactionInputs in;in.profile=&p;in.combat_byte_b=231;in.combat_record_variant=0;
    in.target_heading_80031198=uint16_t(r[13]);in.sound_consumer_ready=true;
    if(r[15]){
        l3::Class20_03PreparedReaction m;m.entry_raw_14=s.raw_14;m.entry_fixed_1c=s.fixed_1c;
        m.raw_14={{111,-17,-212,5678}};m.fixed_1c={{456789,-1234567,7654321,987654}};
        m.velocity_after_wall={{321,-654,987}};m.heading=s.behavior.heading_56;
        m.forward=int16_t(r[16]);m.vertical=int16_t(r[17]);
        for(size_t i=0;i<3;++i)m.velocity_before_wall[i]=int16_t(r[18+i]);
        m.contact_word=uint32_t(r[14]);in.motion=m;
    }
    return in;
}
void differential(const char* path){
    std::ifstream f(path);require(bool(f),"private reaction oracle missing");const auto p=profile();
    std::array<int64_t,48> row{};size_t cases=0;
    while(f>>row[0]){
        for(size_t i=1;i<row.size();++i)require(bool(f>>row[i]),"reaction oracle truncated");
        auto s=seed(row);const auto before=s;const auto in=inputs(s,p,row);
        const auto plan=l3::class20_03_plan_reaction(s,in);
        require(plan.status==Status::SourceBackedPartial&&plan.moves==bool(row[15]),"reaction source motion dispatch differs");
        if(plan.moves)require(plan.forward==row[16]&&plan.vertical==row[17]&&plan.velocity_before_wall==in.motion->velocity_before_wall,"reaction source motion arguments differ");
        const auto result=l3::class20_03_step_reaction(s,in);
        require(result.status==Status::SourceBackedPartial,"reaction held proved case");
        const auto& b=s.behavior;const std::array<int64_t,24> actual{{b.action_09,b.phase_0a,s.aux_0b,b.animation_ac,b.flags_328,
            b.timer_32c,b.work_330,s.random_32a,*s.health_6e,s.velocity_44[0],s.velocity_44[1],s.velocity_44[2],
            s.reaction_table_364.value_or(0),s.work_370,b.heading_56,b.work_36c,s.raw_14[0],s.raw_14[1],s.raw_14[2],s.raw_14[3],
            s.fixed_1c[0],s.fixed_1c[1],s.fixed_1c[2],s.fixed_1c[3]}};
        for(size_t i=0;i<actual.size();++i)if(actual[i]!=row[21+i]){
            std::cerr<<"case "<<cases<<" action "<<row[0]<<" phase "<<row[1]<<" timer "<<row[3]<<" field "<<i<<" got "<<actual[i]<<" expected "<<row[21+i]<<'\n';
            throw std::runtime_error("original reaction fields differ");
        }
        require(s.packet_33c==before.packet_33c&&s.previous_raw_2c==before.previous_raw_2c&&s.previous_fixed_34==before.previous_fixed_34,"reaction cleared packet or changed snapshot");
        if(row[45]<0)require(!result.sound_8001a050&&!result.sound_pointed_words,"reaction invented sound");
        else require(result.sound_8001a050==row[45]&&result.sound_pointed_words&&(*result.sound_pointed_words)[0]==uint32_t(row[46])&&(*result.sound_pointed_words)[1]==uint32_t(row[47]),"reaction sound words differ");
        ++cases;
    }
    require(f.eof()&&cases>6000,"reaction oracle coverage insufficient");std::cout<<cases<<" original reaction differentials PASS\n";
}
void guards(){
    const auto p=profile();std::array<int64_t,48> row{{7,0,0,19,0x200e,0,128,0x1234,0,-32768,17,19,4095,1024,0}};
    auto s=seed(row);auto in=inputs(s,p,row);const auto plan=l3::class20_03_plan_reaction(s,in);
    l3::Class20_03PreparedReaction m;m.entry_raw_14=s.raw_14;m.entry_fixed_1c=s.fixed_1c;m.raw_14=s.raw_14;m.fixed_1c=s.fixed_1c;
    m.forward=plan.forward;m.vertical=plan.vertical;m.heading=s.behavior.heading_56;
    m.velocity_before_wall=plan.velocity_before_wall;m.velocity_after_wall=plan.velocity_before_wall;in.motion=m;
    for(unsigned missing=0;missing<6;++missing){
        auto next=s;auto bad=in;Status expected=Status::NeedsMotion;
        switch(missing){case 0:bad.profile=nullptr;expected=Status::NeedsProfile;break;
        case 1:next.health_6e.reset();expected=Status::NeedsHealth;break;
        case 2:bad.motion.reset();break;
        case 3:++bad.motion->entry_raw_14[3];expected=Status::StaleMotion;break;
        case 4:++bad.motion->velocity_before_wall[1];expected=Status::StaleMotion;break;
        default:bad.motion->contact_word=0x1000;bad.sound_consumer_ready=false;expected=Status::NeedsSoundConsumer;break;}
        const auto before=snapshot(next);const auto result=l3::class20_03_step_reaction(next,bad);
        require(result.status==expected&&snapshot(next)==before&&!result.sound_8001a050,"reaction prerequisite partially committed");
    }
    s.behavior.action_09=8;s.behavior.parameter_03=64;in.motion.reset();in.combat_byte_b.reset();
    const auto before=snapshot(s);require(l3::class20_03_step_reaction(s,in).status==Status::NeedsCombatByteB&&snapshot(s)==before,"reaction borrowed missing combat byteB");
    in.combat_byte_b=0;in.combat_record_variant=1;require(l3::class20_03_step_reaction(s,in).status==Status::NeedsCombatByteB,"reaction borrowed other variant record");
    s.behavior.phase_0a=1;s.reaction_table_364.reset();require(l3::class20_03_step_reaction(s,in).status==Status::NeedsTableOwner,"reaction invented table owner");
    s.reaction_table_364=0x8011beec;s.aux_0b=10;require(l3::class20_03_step_reaction(s,in).status==Status::UnsupportedTableIndex,"reaction overread bounded timer table");
    s.behavior.phase_0a=3;s.behavior.timer_32c=1;in.target_heading_80031198.reset();require(l3::class20_03_step_reaction(s,in).status==Status::NeedsTargetHeading,"reaction invented recovery heading");
    in.target_heading_80031198=0;measured=true;
    for(size_t i=0;i<1024;++i){auto next=s;require(l3::class20_03_step_reaction(next,in).status==Status::SourceBackedPartial,"recovery hot loop held");}
    measured=false;require(allocations==0,"reaction hot loop allocated");
    // The state1 reducer now reaches the proved recovery action and tail.
    s.behavior.parameter_03=0x20;s.behavior.flags_328=0;s.packet_33c=0;
    l3::Class20_03CallbackInputs c;c.action.packet_33c=0;c.action.target_heading_80031198=0;
    const auto out=l3::class20_03_step_state1(s,c);
    require(out.status==l3::Class20_03CallbackStatus::SourceBackedPartial&&s.behavior.heading_56==0,"reaction disconnected from state1 reducer");
}
void profile_format(){
    const auto p=profile();auto data=l3::encode_source_reaction_profile(p);
    require(l3::load_source_reaction_profile(data.data(),data.size()).timer_pairs==p.timer_pairs,"reaction metadata roundtrip differs");
    for(size_t offset:{size_t(4),size_t(32),size_t(64)}){
        auto bad=data;bad[offset]^=1;bool rejected=false;
        try{(void)l3::load_source_reaction_profile(bad.data(),bad.size());}catch(const std::runtime_error&){rejected=true;}
        require(rejected,"reaction metadata corruption accepted");
    }
}
void contact(){
    l3::RuntimeMap map;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    map.areas.push_back(a);map.cells.assign(16,1);map.idx.resize(2);map.idx[1].descriptor_index=0;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;map.descriptors.push_back(d);
    map.contact_lists.push_back({0,1});map.contact_groups.push_back({-100,100,-100,100,0,1});map.contact_primitives.push_back({-128,128,3,0});
    State s;s.behavior.action_09=7;s.health_6e=128;s.raw_14={{0,-128,0,1234}};
    for(size_t i=0;i<4;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;s.velocity_44={{321,-654,987}};
    l3::ActorContactWork w;w.raw=s.raw_14;w.fixed=s.fixed_1c;w.previous_raw=s.previous_raw_2c;w.previous_fixed=s.previous_fixed_34;w.velocity=s.velocity_44;w.state74=1;
    const auto p=profile();l3::Class20_03ReactionInputs in;in.profile=&p;l3::ReferenceTables refs;refs.trig.fill({0,4096});
    const l3::RetailPlayerBounds bounds{-60,60,-60,60,-128,0};
    const auto out=l3::prepare_actor_reaction_contact(s,in,w,map,0,refs,bounds);
    require(out.status==l3::ActorReactionContactStatus::SourceBackedPartial&&out.inputs,"reaction motion/contact not admitted");
    require(out.plan.forward==-256&&out.plan.vertical==2304&&out.inputs->raw_14[2]==16,
        "reaction source motion delta differs");
    auto stale=w;++stale.previous_raw[3];require(l3::prepare_actor_reaction_contact(s,in,stale,map,0,refs,bounds).status==l3::ActorReactionContactStatus::StaleWork,"reaction contact accepted stale fourth lane");
    map.cells.clear();require(l3::prepare_actor_reaction_contact(s,in,w,map,0,refs,bounds).status==l3::ActorReactionContactStatus::UnsupportedBoundary,"reaction contact invented missing map");
}
void contact_differential(const char* path){
    std::ifstream f(path);require(bool(f),"private reaction contact oracle missing");
    const auto p=profile();std::array<int64_t,35> r{};size_t admitted=0,held=0,corrected=0;
    while(f>>r[0]){
        for(size_t i=1;i<r.size();++i)require(bool(f>>r[i]),"contact oracle truncated");
        State s;s.behavior.action_09=uint8_t(r[0]);s.behavior.phase_0a=uint8_t(r[1]);s.aux_0b=uint8_t(r[2]);
        s.behavior.parameter_03=uint8_t(r[3]);s.behavior.timer_32c=int16_t(r[4]);s.behavior.heading_56=4095;
        s.health_6e=128;s.reaction_table_364=(r[3]&64)?0x8011bef8u:0x8011beecu;s.velocity_44={{321,-654,987}};
        for(size_t i=0;i<4;++i){s.raw_14[i]=int16_t(r[8+i]);s.fixed_1c[i]=int32_t(r[12+i]);}
        s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;
        l3::ActorContactWork w;w.raw=s.raw_14;w.fixed=s.fixed_1c;w.previous_raw=s.previous_raw_2c;
        w.previous_fixed=s.previous_fixed_34;w.velocity=s.velocity_44;w.state74=1;
        l3::RuntimeMap map;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
        map.areas.push_back(a);map.cells.assign(16,1);map.idx.resize(2);map.idx[1].descriptor_index=0;
        l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;map.descriptors.push_back(d);
        map.contact_lists.push_back({0,1});map.contact_groups.push_back({-100,100,-100,100,0,1});
        map.contact_primitives.push_back({-128,128,uint16_t(r[7]),0});
        l3::ReferenceTables refs;refs.trig.fill({int16_t(r[5]),int16_t(r[6])});
        l3::Class20_03ReactionInputs in;in.profile=&p;in.combat_byte_b=231;in.combat_record_variant=0;
        const l3::RetailPlayerBounds bounds{-60,60,-60,60,-128,0};
        const auto out=l3::prepare_actor_reaction_contact(s,in,w,map,0,refs,bounds);
        if(out.status==l3::ActorReactionContactStatus::SourceBackedPartial){
            require(out.inputs&&out.plan.forward==r[20]&&out.plan.vertical==r[21],"reaction contact source arguments differ");
            const auto& x=out.after_wall;const std::array<int64_t,12> actual{{x.raw[0],x.raw[1],x.raw[2],x.raw[3],
                x.fixed[0],x.fixed[1],x.fixed[2],x.fixed[3],x.velocity[0],x.velocity[1],x.velocity[2],x.state74}};
            require(uint32_t(out.wall.result)==uint32_t(r[22]),"reaction contact source return differs");
            for(size_t i=0;i<actual.size();++i)if(actual[i]!=r[23+i]){
                std::cerr<<"contact "<<admitted+held<<" action "<<r[0]<<" phase "<<r[1]<<" code "<<r[7]<<" field "<<i<<" got "<<actual[i]<<" expected "<<r[23+i]<<'\n';
                throw std::runtime_error("original reaction contact fields differ");
            }
            ++admitted;corrected+=out.wall.result!=0;
        }else {require(!out.inputs,"held reaction contact published receipt");++held;}
    }
    require(f.eof()&&admitted>100&&held>0&&corrected>0,"reaction contact oracle coverage insufficient");
    std::cout<<"original reaction contact admitted="<<admitted<<" held="<<held<<" corrected="<<corrected<<" PASS\n";
}
l3::ImportedActorInstance actor_fixture(){
    l3::ImportedActorInstance actor;actor.source_class=0x20;actor.subtype=3;actor.initializer=0x80102410;
    actor.source_dispatch_flag0=1;actor.source_dispatch_flag6=3;actor.source_behavior.emplace();
    actor.source_behavior_initialized=true;actor.source_contact_velocity={{321,-654,987}};actor.source_contact_state74=1;
    actor.floor_bounds=l3::RetailPlayerBounds{-60,60,-60,60,-128,0};actor.source_combat_byte09=127;
    auto& s=*actor.source_behavior;s.behavior.parameter_03=0x20;s.behavior.flags_328=14;s.work_00=3;s.health_6e=128;
    s.raw_14={{0,-128,0,1234}};for(size_t i=0;i<4;++i)s.fixed_1c[i]=int32_t(s.raw_14[i])*65536;
    s.previous_raw_2c=s.raw_14;s.previous_fixed_34=s.fixed_1c;
    l3::ActorAnimationBank bank;bank.source_row=0;
    for(uint8_t id=0;id<19;++id){
        l3::ActorAnimationRequestCache cache;cache.request=id;
        for(uint8_t index=0;index<2;++index){
            l3::ActorSequenceRecord record{{index,2,1,uint8_t(index?0x80:0),0,0,0,0}};
            l3::ActorPoseResource pose;pose.request=id;pose.record_index=index;pose.frame_index=index;
            pose.countdown=2;pose.next_control=int8_t(record[3]);pose.sequencing_record=record;
            pose.base_translation.push_back({{int16_t(id),int16_t(index),0}});
            cache.sequence.records.push_back(record);cache.sequence.poses.push_back(pose);
        }
        bank.requests.push_back(cache);
    }
    actor.animation_bank=std::make_shared<const l3::ActorAnimationBank>(std::move(bank));
    actor.pose=actor.animation_bank->requests[0].sequence.poses[0];
    actor.animation_cursor=l3::actor_sequence_begin(0,actor.pose->sequencing_record);return actor;
}
l3::RuntimeMap actor_map(){
    l3::RuntimeMap map;l3::RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    map.areas.push_back(a);map.cells.assign(16,1);map.idx.resize(2);map.idx[1].descriptor_index=0;
    l3::RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;map.descriptors.push_back(d);
    map.contact_lists.push_back({0,1});map.contact_groups.push_back({-100,100,-100,100,0,1});
    map.contact_primitives.push_back({-128,128,3,0});return map;
}
void actor_bridge(){
    const auto p=profile();auto map=actor_map();l3::ReferenceTables refs;refs.trig.fill({0,4096});refs.trig[1024]={4096,0};
    l3::SourceAudioQueue audio;l3::initialize_source_audio_queue_80019e14(audio);
    auto actor=actor_fixture();require(l3::initialize_imported_actor_fresh_pose_proxy_subset(actor),"fresh actor proxy owner unavailable");
    actor.source_proxy_target.packet_00=0x08000001;actor.source_proxy_target.flags_04=0x12345678;
    actor.source_proxy_target.point_08={{64,-17,0}};actor.source_proxy_target.word_0e=1;
    require(l3::stage_imported_actor_proxy_packet(actor)==l3::ImportedActorPacketStatus::SourceBackedPartial,"actor source alias not staged");
    auto& s=*actor.source_behavior;
    require(s.packet_33c==actor.source_proxy_target.packet_00&&s.hit_flags_340==0x12345678&&
        s.hit_point_344==actor.source_proxy_target.point_08&&s.hit_word_34a==1&&s.health_6e==128,"actor alias changed HP or lost hit context");
    s.work_332=127;const auto original=snapshot(s);const auto audio_before=audio.records;
    const auto reaction=l3::prepare_imported_actor_reaction_contact(actor,map,0,refs,&p,nullptr,0,&audio);
    require(reaction.status==l3::ActorReactionContactStatus::SourceBackedPartial&&reaction.inputs&&
        reaction.staged_hit_status==l3::Class20_03HitStatus::SourceBackedPartial&&reaction.initial_action==8&&
        reaction.initial_heading==1024&&reaction.incoming_packet==0x08000001,"actor reaction prepared before source hit/direction selection");
    bool same_audio=true;for(size_t i=0;i<audio.records.size();++i)same_audio&=audio.records[i].raw==audio_before[i].raw;
    require(snapshot(s)==original&&audio.count==0&&same_audio,"read-only post-hit preparation published work/audio");
    l3::Class20_03CallbackInputs in;in.action.packet_33c=0;in.action.target_heading_80031198=0;in.reaction_profile=&p;
    const auto held=l3::tick_imported_actor_state1(actor,in,nullptr,nullptr,&audio,nullptr,nullptr,nullptr,nullptr,nullptr,&reaction);
    require(!held.committed&&held.status==l3::Class20_03CallbackStatus::NeedsPacket&&snapshot(s)==original,"actor bridge consumed pending packet without clear/effect owner");
    l3::SourceHitProxyBank bank;
    const auto tail=l3::tick_imported_actor_pose_proxy(actor,refs,bank,0);
    require(!tail.committed&&!tail.proxy.packet_cleared&&tail.proxy.status==l3::Class20_03ProxyStatus::NeedsBodyTransaction&&
        s.packet_33c==0x08000001&&actor.source_proxy_target.packet_00==0x08000001&&bank.count==0,"separate proxy tail cleared held packet");
    actor.source_proxy_target.packet_00=0;
    require(l3::stage_imported_actor_proxy_packet(actor)==l3::ImportedActorPacketStatus::PendingPacketConflict&&snapshot(s)==original,"alias staging lost pending packet");
    ++actor.source_proxy_target.source_address;
    require(l3::stage_imported_actor_proxy_packet(actor)==l3::ImportedActorPacketStatus::InvalidIdentity&&snapshot(s)==original,"alias staging accepted wrong source pool identity");
    actor=actor_fixture();require(l3::initialize_imported_actor_fresh_pose_proxy_subset(actor),"automatic alias owner unavailable");
    actor.source_proxy_target.packet_00=0x08000001;actor.source_proxy_target.point_08={{64,-17,0}};actor.source_proxy_target.word_0e=1;
    const auto automatic=l3::tick_imported_actor_state1(actor,in,nullptr,nullptr,&audio);
    require(!automatic.committed&&automatic.status==l3::Class20_03CallbackStatus::NeedsPacket&&
        actor.source_behavior->packet_33c==0x08000001&&actor.source_behavior->hit_point_344==actor.source_proxy_target.point_08&&
        actor.source_behavior->health_6e==128,"live tick did not stage held source packet automatically");
    actor=actor_fixture();actor.source_behavior->behavior.action_09=7;
    const auto prepared=l3::prepare_imported_actor_reaction_contact(actor,map,0,refs,&p,nullptr,0,&audio);
    require(prepared.status==l3::ActorReactionContactStatus::SourceBackedPartial&&prepared.inputs,"zero-packet native reaction not prepared");
    const auto before=snapshot(*actor.source_behavior);auto stale=prepared;++stale.initial_aux;
    const auto rejected=l3::tick_imported_actor_state1(actor,in,nullptr,nullptr,&audio,nullptr,nullptr,nullptr,nullptr,nullptr,&stale);
    require(!rejected.committed&&rejected.status==l3::Class20_03CallbackStatus::NeedsReactionMotion&&snapshot(*actor.source_behavior)==before,"stale native reaction receipt committed");
    const auto committed=l3::tick_imported_actor_state1(actor,in,nullptr,nullptr,&audio,nullptr,nullptr,nullptr,nullptr,nullptr,&prepared);
    require(committed.committed&&committed.status==l3::Class20_03CallbackStatus::SourceBackedPartial&&
        actor.source_contact_velocity==actor.source_behavior->velocity_44&&
        actor.source_contact_state74==prepared.after_wall.state74,"native reaction velocity/wall state not committed with body/pose");
    // Recovery needs no contact receipt; the retained velocity remains owned.
    actor.source_behavior->behavior.action_09=7;actor.source_behavior->behavior.phase_0a=3;
    actor.source_behavior->behavior.timer_32c=30000;actor.source_behavior->behavior.flags_328=0;
    measured=true;bool ok=true;
    for(size_t i=0;i<1024;++i)ok&=l3::tick_imported_actor_state1(actor,in,nullptr,nullptr,&audio).committed;
    measured=false;require(ok&&allocations==0,"native reaction hot loop held or allocated");
}
}
int main(int argc,char** argv){try{profile_format();guards();contact();actor_bridge();if(argc>1)differential(argv[1]);if(argc>2)contact_differential(argv[2]);std::cout<<"PASS source reaction actions7/8\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
