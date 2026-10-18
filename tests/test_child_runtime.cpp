#include "legends3ds/child_runtime.h"
#include "legends3ds/class20_03_hit.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { size_t allocations=0; }
void* operator new(std::size_t size){
    ++allocations;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

namespace {
using namespace l3;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
uint16_t read16(const Class60_06Work& w,size_t off){return uint16_t(w[off]|(uint16_t(w[off+1])<<8));}
uint32_t read32(const Class60_06Work& w,size_t off){return uint32_t(read16(w,off))|(uint32_t(read16(w,off+2))<<16);}
void put16(Class60_06Work& w,size_t off,uint16_t n){w[off]=uint8_t(n);w[off+1]=uint8_t(n>>8);}
bool same_audio(const SourceAudioQueue& a,const SourceAudioQueue& b){
    if(a.count!=b.count || a.initialized!=b.initialized)return false;
    for(size_t i=0;i<a.records.size();++i)if(a.records[i].raw!=b.records[i].raw)return false;
    return true;
}
bool same_pool(const Class60SourcePool& a,const Class60SourcePool& b){
    return a.allocated_bitmap==b.allocated_bitmap && a.pending_release_bitmap==b.pending_release_bitmap && a.slots==b.slots;
}
SourceAudioQueue authored_audio(size_t count=0){
    SourceAudioQueue q;for(auto& r:q.records)r.raw.fill(0xa5);
    initialize_source_audio_queue_80019e14(q);q.count=count;return q;
}
ReferenceTables authored_refs(){ReferenceTables r;r.trig.fill({0,4096});return r;}
Class60_06Profile authored_profile(){
    Class60_06Profile p;for(size_t f=0;f<6;++f){p.normal_uv[f]={{uint16_t(7+f*31),uint16_t(19+f*23)}};p.alternate_uv[f]={{201,233}};}
    return p;
}
Class20_03CallbackState authored_parent(int16_t y=-1){
    Class20_03CallbackState p;p.behavior.action_09=4;p.behavior.phase_0a=2;
    p.behavior.timer_32c=1;p.behavior.animation_ac=21;p.behavior.heading_56=0;
    p.raw_14={{0,y,110,-21846}};p.fixed_1c={{0,int32_t(y)*65536,110*65536,0x13572468}};
    return p;
}
RuntimeMap authored_map(int16_t packed=-32768,uint16_t code=3,uint16_t flags=0){
    RuntimeMap map;RuntimeMapArea a;a.start_x=62;a.start_z=62;a.width=4;a.height=4;a.cell_count=16;
    map.areas.push_back(a);map.cells.assign(16,packed);
    if(packed>=0){
        map.idx.resize(2);map.idx[1].descriptor_index=0;
        RuntimeMapDescriptor d;d.default_contact_index=0;d.alt_contact_index=0;
        map.descriptors.push_back(d);map.contact_lists.push_back({0,1});
        map.contact_groups.push_back({-100,100,-100,100,0,1});
        map.contact_primitives.push_back({-128,128,code,flags});
    }
    return map;
}
uint32_t spawn(ChildRuntime& owner,const Class20_03CallbackState& parent,
    const ReferenceTables& refs,SourceAudioQueue& q){
    ParentChildDelivery d;require(owner.prepare_parent_delivery(parent,refs,q,d)==ChildRuntimeStatus::SourceBackedPartial,"producer did not prepare");
    require(owner.validate_parent_delivery(d,parent,q),"fresh delivery invalid");
    const auto identity=d.source_identity();q=d.staged_audio();
    require(owner.commit_parent_delivery(d),"producer commit failed");return identity;
}
ChildFrameContext context(const RuntimeMap& m,const ReferenceTables& r,const Class60_06Profile& p,SourceAudioQueue& q){
    ChildFrameContext c;c.map=&m;c.references=&r;c.profile=&p;c.audio=&q;c.proxy_mask_80098798=0;
    c.player_heading_800b5206=0;return c;
}
struct Snapshot {
    Class60SourcePool pool;
    SourceHitProxyBank proxies;
    SourceCollisionRegistry registry;
    SourceShadowQueue shadows;
    std::array<SourceHitProxyTarget,32> targets;
    SourceAudioQueue audio;
    SourceEffectPool effects;
};
Snapshot snapshot(const ChildRuntime& o,const SourceAudioQueue& q){
    return {o.pool(),o.hit_proxy_bank(),o.collision_registry(),o.shadow_queue(),o.proxy_targets(),q,o.effect_pool()};
}
bool same_target(const SourceHitProxyTarget& a,const SourceHitProxyTarget& b){
    return a.source_address==b.source_address && a.packet_00==b.packet_00 && a.flags_04==b.flags_04 && a.point_08==b.point_08 && a.word_0e==b.word_0e;
}
void unchanged(const ChildRuntime& o,const SourceAudioQueue& q,const Snapshot& s){
    require(same_pool(o.pool(),s.pool),"held transaction changed one of268 child bytes/pool masks");
    require(same_audio(q,s.audio),"held transaction changed retained audio bytes");
    require(o.effect_pool().slots==s.effects.slots && o.effect_pool().allocated_80098838==s.effects.allocated_80098838 && o.effect_pool().pending_release_800988c8==s.effects.pending_release_800988c8,"held child transaction changed effect pool");
    const auto& p=o.hit_proxy_bank();require(p.count==s.proxies.count,"held proxy count changed");
    for(size_t i=0;i<p.records.size();++i){const auto& a=p.records[i];const auto& b=s.proxies.records[i];
        require(a.point==b.point && a.encoded_extent==b.encoded_extent && a.encoded_identity==b.encoded_identity && a.parallel_word==b.parallel_word && a.target==b.target,"held proxy bytes/pointers changed");}
    const auto& r=o.collision_registry();require(r.count==s.registry.count && r.identities==s.registry.identities,"held collision registry changed");
    const auto& sh=o.shadow_queue();require(sh.count==s.shadows.count,"held shadow count changed");
    for(size_t i=0;i<sh.records.size();++i){const auto& a=sh.records[i];const auto& b=s.shadows.records[i];require(a.point==b.point && a.heading_scale==b.heading_scale && a.radius_type==b.radius_type && a.height_delta==b.height_delta,"held shadow record changed");}
    for(size_t i=0;i<32;++i)require(same_target(o.proxy_targets()[i],s.targets[i]),"held persistent proxy target changed");
}

void producer_transactions(){
    ChildRuntime o;auto q=authored_audio();const auto refs=authored_refs();auto p=authored_parent();
    for(size_t slot=0;slot<32;++slot)for(size_t byte=0;byte<268;++byte)
        require(o.pool().slots[slot][byte]==(byte==1?uint8_t(0x60u|slot):0),"boot did not restore source slot class");
    const auto initial=snapshot(o,q);ParentChildDelivery d;
    require(o.prepare_parent_delivery(p,refs,q,d)==ChildRuntimeStatus::SourceBackedPartial,"first delivery held");
    unchanged(o,q,initial);require(d.source_identity()==class60_source_pool_base && d.audio_queued()==1 && d.staged_audio().count==1,"producer identity/cue receipt wrong");
    auto changed=p;++changed.behavior.timer_32c;require(!o.validate_parent_delivery(d,changed,q),"stale phase/timer delivery accepted");
    changed=p;++changed.raw_14[3];require(!o.validate_parent_delivery(d,changed,q),"stale fourth sound lane accepted");
    changed=p;++changed.fixed_1c[2];require(!o.validate_parent_delivery(d,changed,q),"stale producer fixed position accepted");
    auto badq=q;++badq.records[23].raw[19];require(!o.validate_parent_delivery(d,p,badq),"changed reserved audio lane accepted");
    ChildRuntime another;require(!another.validate_parent_delivery(d,p,q),"wrong pool owner accepted receipt");
    auto replay=d;require(o.commit_parent_delivery(d) && !o.commit_parent_delivery(d) && !o.commit_parent_delivery(replay),"child delivery replay accepted");
    q=replay.staged_audio();
    for(size_t slot=1;slot<32;++slot)require(spawn(o,p,refs,q)==class60_source_pool_base+uint32_t(slot*268),"shared source allocation order wrong");
    const auto full=snapshot(o,q);require(o.prepare_parent_delivery(p,refs,q,d)==ChildRuntimeStatus::SourceBackedPartial && d.source_identity()==0,"independently full pool did not return zero");
    require(d.audio_queued()==0 && d.audio_dropped()==0 && o.commit_parent_delivery(d),"full allocator fabricated sound/mutation");unchanged(o,q,full);
    require(o.mark_release(class60_source_pool_base) && o.pool().pending_release_bitmap==0x80000000u && o.pool().allocated_bitmap==0xffffffffu,"pending release immediately freed slot");
    require(spawn(o,p,refs,q)==0,"pending child was reallocated before batch cleanup");
    o.consume_pending_releases();require(o.pool().pending_release_bitmap==0 && o.pool().allocated_bitmap==0x7fffffffu,"batch release mask not consumed");
    require(spawn(o,p,refs,q)==class60_source_pool_base,"first free source slot not reused");
    o.initialize();auto malformed=authored_audio(25);require(o.prepare_parent_delivery(p,refs,malformed,d)==ChildRuntimeStatus::NeedsSoundConsumer && !d.prepared(),"malformed sound queue authorized producer");
    auto no_audio=SourceAudioQueue{};require(o.prepare_parent_delivery(p,refs,no_audio,d)==ChildRuntimeStatus::NeedsSoundConsumer,"absent sound queue authorized producer");
    p.variant_32e=1;require(o.prepare_parent_delivery(p,refs,q,d)==ChildRuntimeStatus::UnsupportedSubtype && o.pool().allocated_bitmap==0,"unowned subtype9 advertised as live");
    p.variant_32e=0;p.behavior.phase_0a=1;p.behavior.timer_32c=13;require(o.prepare_parent_delivery(p,refs,q,d)==ChildRuntimeStatus::NotSpawnBranch,"nonspawn producer branch accepted");
}

void child_transactions(){
    ChildRuntime o;auto refs=authored_refs();auto profile=authored_profile();auto q=authored_audio();auto map=authored_map();
    const auto identity=spawn(o,authored_parent(),refs,q);auto c=context(map,refs,profile,q);
    auto before=snapshot(o,q);auto missing=c;missing.profile=nullptr;
    require(!o.tick(identity,missing).committed,"missing UV profile committed child");unchanged(o,q,before);
    missing=c;missing.proxy_mask_80098798.reset();require(o.tick(identity,missing).status==ChildRuntimeStatus::NeedsProxyMask,"unknown global mask guessed");unchanged(o,q,before);
    o.hit_proxy_bank().count=65;before=snapshot(o,q);require(o.tick(identity,c).proxy_status==SourceHitProxyStatus::InvalidBank,"invalid proxy count accepted");unchanged(o,q,before);
    o.hit_proxy_bank().count=64;before=snapshot(o,q);require(o.tick(identity,c).proxy_status==SourceHitProxyStatus::Full,"full proxy bank was silently dropped");unchanged(o,q,before);
    o.reset_frame_queues();o.collision_registry().count=128;before=snapshot(o,q);
    require(o.tick(identity,c).status==ChildRuntimeStatus::RegistryHeld,"staged proxy bypassed full physical registry");unchanged(o,q,before);
    o.reset_frame_queues();require(o.tick(identity,c).committed,"admitted child transaction held");
    const auto& w=o.pool().slots[0];require(w[0]==3 && w[8]==1 && w[15]==1 && read32(w,0xa0)==128,"real proxy receipt not applied to child");
    require(o.hit_proxy_bank().count==1 && o.collision_registry().count==1 && o.shadow_queue().count==1,"staged consumers not committed together");
    const auto& row=o.hit_proxy_bank().records[0];require(row.target==&o.proxy_targets()[0] && row.target->source_address==identity+0x9c,"proxy target escaped retained owner storage");
    require(row.encoded_identity==((0x60u<<24)|((identity+0x9c)&0x7fffffu)) && row.encoded_extent==0x8008 && row.parallel_word==0x08000000,"exact proxy argument mapping wrong");
    require(o.collision_registry().identities[0]==identity,"registry uses native pointer instead of source token");
    const auto primitive=class60_06_primitive(w);require(primitive && primitive->clut==0x3d48 && primitive->tpage==14 && primitive->uv==std::array<uint8_t,2>{{7,19}},"child packet CLUT/tpage/UV packing wrong");
    const auto texture=class60_06_texture_words(*primitive);require(texture[0]==0x3d481307 && texture[1]==0x000e1326,"packed UV carry or CLUT/tpage contract changed");
    auto edge=*primitive;edge.uv={{250,255}};
    const auto carried=class60_06_texture_words(edge);
    require(carried==std::array<uint32_t,4>{{0x3d48fffa,0x000f0019,0x00011efa,0x00011f19}},"source packed UV carry replaced by per-byte wrap");
    SourceHitProxyTarget actor;actor.source_address=0x800b109cu;
    SourceHitProxyRegistration external;external.target=&actor;external.mask_80098798=0;external.a1=32;external.a3=0x4000;
    external.stack10=0x20;external.stack18=0x08000000;external.pointed_words={{uint32_t(uint16_t(row.point[0]))|(uint32_t(uint16_t(row.point[1]))<<16),uint32_t(uint16_t(row.point[2]))}};
    require(source_hit_proxy_register_800341bc(o.hit_proxy_bank(),external).status==SourceHitProxyStatus::SourceBackedPartial,"shared actor/child proxy bank registration failed");
    before=snapshot(o,q);require(o.consume_proxy_pairs(std::nullopt).status==SourceHitProxyStatus::NeedsRules,"missing pair rules guessed");unchanged(o,q,before);
    SourceHitProxyRules rules;rules.unsigned_80097d00.fill(255);
    const auto pairs=o.consume_proxy_pairs(rules);require(pairs.overlapping_pairs==1 && o.hit_proxy_bank().count==0 && read32(o.pool().slots[0],0x9c)==0x08000000 && actor.packet_00==0x08000000,"actual kind0 proxy packet did not reach child/actor targets");
    o.reset_frame_queues();before=snapshot(o,q);const auto effect=o.tick(identity,c);
    require(effect.committed && effect.released && effect.effects_allocated==1 && effect.effects_exhausted==0,"real effect128 allocation did not commit child release");
    require(o.effect_pool().allocated_80098838[0]==0x80000000 && o.effect_pool().slots[0][0]==5 && o.effect_pool().slots[0][2]==128 && o.effect_pool().slots[0][3]==66,"child128 producer bytes or pool mask differ");
    const auto held128=o.tick_effects();require(held128.held==1 && held128.first_hold==SourceEffectStatus::NeedsFloorConsumer,"effect128 missing source floor guessed");
    o.consume_pending_releases();require(o.pool().allocated_bitmap==0 && o.effect_pool().allocated_80098838[0]==0x80000000,"child cleanup released held effect128");

    o.initialize();q=authored_audio();const auto ground_child=spawn(o,authored_parent(100),refs,q);before=snapshot(o,q);
    o.collision_registry().count=128;before=snapshot(o,q);
    require(o.tick(ground_child,c).status==ChildRuntimeStatus::RegistryHeld,"effect staging bypassed registry hold");unchanged(o,q,before);
    o.reset_frame_queues();auto ground_work=o.pool().slots[0];Class60_06Inputs ground_inputs;ground_inputs.references=&refs;ground_inputs.profile=&profile;
    const auto ground_request=class60_06_step_8010f354(ground_work,ground_inputs);
    require(ground_request.contact_request.has_value(),"ground child missing exact contact boundary");
    const auto ground=o.tick(ground_child,c);
    require(ground.committed && ground.effects_allocated==1 && o.effect_pool().slots[0][2]==131,"real contact131 allocation/initializer did not commit");
    for(size_t i=0;i<52;++i)require(o.effect_pool().slots[0][0x14+i]==(*ground_request.contact_request)[0x88+i],"copied contact131 original52-byte packet changed");
    const auto effects=o.tick_effects();require(effects.committed==1 && effects.held==0 && source_effect_primitive(o.effect_pool().slots[0]),"complete scalar131 callback not live");
    o.initialize();q=authored_audio();const auto high_child=spawn(o,authored_parent(-1000),refs,q);auto slope=authored_map(1,8);auto unknown=context(slope,refs,profile,q);before=snapshot(o,q);
    const auto held=o.tick(high_child,unknown);
    require(!held.committed && held.lifecycle_status==Class60_06Status::UnsupportedWall,"unproved slope geometry admitted child motion");unchanged(o,q,before);
    require(!o.tick(0x1234,c).committed && !o.mark_release(0x1234),"invalid source identity accepted");

    // Actual80033EBC startup installs mask255. These flags therefore differ
    // from the deliberately mask0 callback fixtures during the first batch.
    o.initialize();q=authored_audio();const auto fresh=spawn(o,authored_parent(),refs,q);
    c.proxy_mask_80098798=255;
    require(o.tick(fresh,c).committed && read32(o.pool().slots[0],0xa0)==144,"source mask255 first registration lost retained low flags");
    o.reset_frame_queues();require(o.tick(fresh,c).committed && read32(o.pool().slots[0],0xa0)==128,"source bit128 did not clear subsequent low flags");
}

void effect_exhaustion(){
    ChildRuntime owner;auto refs=authored_refs();auto profile=authored_profile();auto audio=authored_audio();auto map=authored_map();auto c=context(map,refs,profile,audio);
    for(size_t wave=0;wave<3;++wave){
        owner.reset_frame_queues();
        std::array<uint32_t,32> identities{};
        for(auto& id:identities)id=spawn(owner,authored_parent(100),refs,audio);
        for(auto id:identities){
            const auto result=owner.tick(id,c);require(result.committed,"real full effect pool blocked supported child contact");
            require((wave<2 && result.effects_allocated==1 && result.effects_exhausted==0) ||
                (wave==2 && result.effects_allocated==0 && result.effects_exhausted==1),"true64-slot source effect exhaustion wrong");
            require(owner.mark_release(id),"effect capacity setup child release failed");
        }
        owner.consume_pending_releases();
    }
    require(owner.effect_pool().allocated_80098838==std::array<uint32_t,2>{{0xffffffffu,0xffffffffu}},"full source effect masks differ");
    const auto allocation_count=allocations;
    for(size_t frame=0;frame<13;++frame)require(owner.tick_effects().committed==64,"full source131 batch held");
    require(allocations==allocation_count && owner.effect_pool().pending_release_800988c8==std::array<uint32_t,2>{{0xffffffffu,0xffffffffu}},"full effect lifecycle lost deferred release or allocated heap");
    owner.consume_pending_releases();require(owner.effect_pool().allocated_80098838==std::array<uint32_t,2>{},"full effect cleanup did not restore real capacity");
}

void capacity_and_allocations(){
    ChildRuntime o;auto refs=authored_refs();auto p=authored_profile();auto q=authored_audio();auto map=authored_map();auto parent=authored_parent(-2000);
    map.areas[0].start_x=56;map.areas[0].start_z=56;map.areas[0].width=16;
    map.areas[0].height=16;map.areas[0].cell_count=256;map.cells.assign(256,-32768);
    std::array<uint32_t,32> ids{};for(auto& id:ids)id=spawn(o,parent,refs,q);
    auto c=context(map,refs,p,q);const auto start=allocations;size_t committed=0;
    for(int frame=0;frame<32;++frame){
        o.reset_frame_queues();for(auto id:ids){const auto result=o.tick(id,c);require(result.committed,"bounded airborne child tick held");++committed;}
        require(o.hit_proxy_bank().count==32 && o.collision_registry().count==32 && o.shadow_queue().count==32,"shared32-slot batch count wrong");
    }
    require(allocations==start && committed==1024,"hot-loop heap allocation occurred");
    // Source shadow overflow returns before any floor query; all other staged
    // consumers still commit. Source proxy overflow instead holds everything.
    o.initialize();q=authored_audio();for(auto& id:ids)id=spawn(o,parent,refs,q);
    for(int pass=0;pass<2;++pass){o.hit_proxy_bank().count=0;for(auto id:ids)require(o.tick(id,c).committed,"capacity setup held");}
    require(o.shadow_queue().count==64 && o.collision_registry().count==64,"shadow boundary not reached");
    o.hit_proxy_bank().count=0;c.map=nullptr;
    // Active children need contact map even when the shadow queue is full.
    // A missing contact map must hold; no guessed floor may bypass contact.
    const auto before=snapshot(o,q);require(o.tick(ids[0],c).status==ChildRuntimeStatus::NeedsMap,"full shadows bypassed child contact");unchanged(o,q,before);
    c.map=&map;require(o.tick(ids[0],c).committed && o.shadow_queue().count==64,"full source shadow queue did not drop before floor");
    std::cout<<"1024 allocation-free shared child ticks; transaction rollback/producer/pair/capacity guards PASS\n";
}

void floor_fixtures(const char* path){
    std::ifstream file(path);require(bool(file),"shadow floor fixture missing");std::string line;std::getline(file,line);
    size_t admitted=0,held=0;
    while(std::getline(file,line)){
        std::istringstream row(line);int32_t packed,code,flags,cls,x,y,z,prior,expected,a0,a1,dy,a3;
        require(bool(row>>packed>>code>>flags>>cls>>x>>y>>z>>prior>>expected>>a0>>a1>>dy>>a3),"shadow floor fixture truncated");
        require(a0==0x1014 && a1==0 && dy==y-prior && a3==cls,"original wrapper argument mismatch");
        Class60_06Work w{};w[1]=uint8_t(cls);put16(w,0x14,uint16_t(x));put16(w,0x16,uint16_t(y));put16(w,0x18,uint16_t(z));put16(w,0x2e,uint16_t(prior));
        const auto before=w;const auto m=authored_map(int16_t(packed),uint16_t(code),uint16_t(flags));
        const auto floor=query_class60_shadow_floor_80048e6c(w,m,0);require(w==before,"floor wrapper changed work bytes");
        if(floor.status==RuntimeContactStatus::SourceBackedPartial){
            if(floor.result!=expected){std::cerr<<"floor case "<<admitted+held<<" native="<<floor.result<<" source="<<expected<<'\n';throw std::runtime_error("original wrapper floor result mismatch");}++admitted;
        }else{require(floor.status==RuntimeContactStatus::Unsupported,"unexpected floor hold status");++held;}
    }
    require(admitted>100 && held>0 && admitted+held==288,"floor wrapper coverage missing");
    std::cout<<"original80048E6C floor wrapper admitted="<<admitted<<" held="<<held<<'\n';
}
void proxy_lease_owner(){
    ChildRuntime owner;std::optional<uint32_t> mask=255;
    SourceHitProxyTarget target;target.source_address=0x8009f444;target.flags_04=128;
    SourceHitProxyOwnerPoint point;point.raw_bc_words={{0x12345678,0x87654321}};
    const auto start=allocations;
    for(size_t i=0;i<24;++i){
        const auto leased=owner.allocate_proxy_lease(mask,0x800a1000+uint32_t(i*4),point,target);
        require(leased.status==SourceHitProxyStatus::SourceBackedPartial && leased.index==int32_t(i),"source lease allocation order differs");
        require(owner.proxy_owner_point(i) && owner.proxy_owner_point(i)->raw_bc_words==point.raw_bc_words,"lease did not bind real BC/C0 storage");
    }
    require(*mask==0xffffffffu && target.flags_04==0xffffff80u,"source lease bits differ");
    require(owner.allocate_proxy_lease(mask,0x800a2000,point,target).status==SourceHitProxyStatus::Full,"full source lease bank did not return -1");
    require(owner.release_proxy_lease(mask,5)==SourceHitProxyStatus::SourceBackedPartial && *mask==0xfbffffffu,"source lease release cleared wrong bit");
    require(owner.proxy_owner_point(5) && target.flags_04==0xffffff80u,"source release improperly erased retained owner/flags");
    require(owner.allocate_proxy_lease(mask,0x800a3000,point,target).index==5,"source lease hole not reused");
    // The GTE consumer uses the owner installed by the producer.
    owner.initialize();mask=255;target.flags_04=0;
    require(owner.allocate_proxy_lease(mask,0x800a1000,point,target).index==0,"first source lease failed");
    SourceGteRtpsState gte;gte.projection_controls_bound=true;gte.control[26]=1;
    std::array<uint32_t,8> camera{};camera[0]=4096;camera[2]=4096;camera[4]=4096;camera[7]=128;
    const auto projected=owner.consume_player_leases(gte,camera,mask);
    require(projected.status==SourcePlayerProxyStatus::SourceBackedPartial && projected.projected==1,"new allocated lease not connected to source GTE consumer");
    require(owner.proxy_owner_point(0)->raw_bc_words==std::array<uint32_t,2>{},"source GTE consumption did not clear allocated BC/C0 words");
    require(allocations==start,"lease owner/consumer allocated heap");
    std::cout<<"24-slot lease producer -> owned BC/C0 -> original GTE consumer; release/reuse PASS\n";
}
SourceBusterPlayer authored_buster_player(int selection=0,int counter=1,int subtype=3){
    SourceBusterPlayer player;player.class_01=2;player.heading_56=4095;
    for(auto& lane:player.lanes){
        lane.selection_170_or_171=int8_t(selection);
        lane.muzzle_3c8_or_3e0=std::array<uint16_t,3>{{123,uint16_t(-234),345}};
        lane.subtype_80089a8b=uint8_t(subtype);lane.counter_4fa=int16_t(counter);
    }
    return player;
}
void buster_fixtures(const char* previous_fixture){
    std::string path(previous_fixture);const auto slash=path.find_last_of("/\\");
    path.replace(slash==std::string::npos?0:slash+1,std::string::npos,"source_buster_spawn_01436.tsv");
    std::ifstream in(path);require(in.good(),"Buster original fixture missing");
    std::array<int64_t,279> row{};size_t cases=0;
    while(in>>row[0]){
        for(size_t i=1;i<row.size();++i)require(bool(in>>row[i]),"truncated Buster original fixture");
        ChildRuntime owner;auto player=authored_buster_player(int(row[3]),int(row[4]),int(row[5]));
        std::optional<uint32_t> mask=uint32_t(row[6]);
        SourceHitProxyTarget target;target.source_address=0x80003000;target.flags_04=0x12345678;
        auto& bank=owner.hit_proxy_bank();bank.count=1;
        bank.records[0].encoded_identity=0x3000;bank.records[0].target=&target;
        const auto before=allocations;
        for(int64_t shot=0;shot<row[0];++shot){
            const auto result=owner.spawn_source_buster_80035724(player,uint32_t(row[1]),int32_t(row[2]),mask);
            require(result.status==SourceBusterStatus::SourceBackedPartial,"source Buster producer held authored valid fields");
            require(result.source_identity==(shot<16?source_buster_base+uint32_t(shot*268):0),"source Buster allocation identity differs");
        }
        require(allocations==before,"source Buster spawn allocated heap");
        const auto& pool=owner.buster_pool();const auto& last=pool.slots[size_t(std::min<int64_t>(row[0]-1,15))];
        require(pool.allocated_800989f0==uint32_t(row[7]) && mask==uint32_t(row[8]) && target.flags_04==uint32_t(row[9]),"source Buster allocation/lease masks differ");
        require(uint16_t(*player.lanes[row[1]?0:1].counter_4fa)==uint16_t(row[10]),"source Buster counter differs");
        for(size_t i=0;i<last.size();++i)if(last[i]!=row[11+i]){
            std::cerr<<"buster case="<<cases<<" byte="<<i<<" native="<<int(last[i])<<" original="<<row[11+i]<<'\n';
            throw std::runtime_error("source Buster exact scalar record differs");
        }
        ++cases;
    }
    require(in.eof() && cases==258,"source Buster differential coverage missing");
    std::cout<<cases<<" original80035724 allocation/lease/counter/whole-record differentials PASS\n";
}
void buster_guards_and_owner(){
    ChildRuntime owner;auto player=authored_buster_player();std::optional<uint32_t> mask;
    SourceHitProxyTarget target;target.source_address=0x80003000;target.flags_04=99;
    auto& bank=owner.hit_proxy_bank();bank.count=1;bank.records[0].encoded_identity=0x3000;bank.records[0].target=&target;
    const auto original=owner.buster_pool();
    const auto start=allocations;
    require(owner.spawn_source_buster_80035724(player,1,0,mask).status==SourceBusterStatus::NeedsProxyMask,"missing source lease mask accepted");
    require(owner.buster_pool().slots==original.slots && owner.buster_pool().allocated_800989f0==0 && target.flags_04==99,"held source Buster mutated owners");
    player.lanes[0].muzzle_3c8_or_3e0.reset();mask=0;
    require(owner.spawn_source_buster_80035724(player,1,0,mask).status==SourceBusterStatus::NeedsMuzzle,"missing source muzzle accepted");
    require(owner.buster_pool().slots==original.slots && target.flags_04==99 && *mask==0,"held muzzle changed owners");
    player=authored_buster_player();player.lanes[0].subtype_80089a8b.reset();
    require(owner.spawn_source_buster_80035724(player,1,0,mask).status==SourceBusterStatus::NeedsSubtype,"missing source table subtype accepted");
    player=authored_buster_player();player.lanes[0].counter_4fa.reset();
    require(owner.spawn_source_buster_80035724(player,1,0,mask).status==SourceBusterStatus::NeedsCounter,"missing source counter accepted");
    player=authored_buster_player();player.heading_56.reset();
    require(owner.spawn_source_buster_80035724(player,1,0,mask).status==SourceBusterStatus::NeedsPlayerFields,"missing source player field accepted");
    require(owner.buster_pool().slots==original.slots && target.flags_04==99 && *mask==0,"unknown source fields changed owners");
    player=authored_buster_player();
    const auto shot=owner.spawn_source_buster_80035724(player,1,0,mask);
    require(shot.source_identity==source_buster_base && shot.lease_index==0 && target.flags_04==(99u|0x80000000u),"real Buster producer did not install owned lease");
    SourceHitProxyOwnerPoint point{{0x12345678,0x87654321}};
    require(owner.bind_proxy_owner(0,shot.source_identity,point),"Buster lease point owner failed");
    require(read32(owner.buster_pool().slots[0],0xbc)==point.raw_bc_words[0],"Buster BC slot not connected to leased point owner");
    SourceGteRtpsState gte;gte.projection_controls_bound=true;gte.control[26]=1;
    std::array<uint32_t,8> camera{};camera[0]=4096;camera[2]=4096;camera[4]=4096;camera[7]=128;
    const auto projected=owner.consume_player_leases(gte,camera,mask);
    require(projected.status==SourcePlayerProxyStatus::SourceBackedPartial && projected.projected==1,"real Buster lease not consumed by original GTE path");
    require(read32(owner.buster_pool().slots[0],0xbc)==0 && read32(owner.buster_pool().slots[0],0xc0)==0,"GTE consumption did not clear actual Buster BC/C0 owner");
    require(allocations==start,"Buster owner/guards allocated heap");
    std::cout<<"real Buster producer -> lease -> owned Buster BC/C0 -> original GTE consumer PASS\n";
}
void buster_normal_hit_chain(){
    ChildRuntime owner;auto player=authored_buster_player(0,4,0x80);
    std::optional<uint32_t> mask=0;
    const auto shot=owner.spawn_source_buster_80035724(player,1,-1,mask);
    require(shot.status==SourceBusterStatus::SourceBackedPartial&&shot.source_identity==source_buster_base,
        "normal Buster source producer failed");
    auto audio=authored_audio();std::optional<uint32_t> busy=9;
    SourceBusterFrameContext frame;
    frame.owner_position_14=std::array<uint32_t,2>{{uint32_t(uint16_t(100))|(uint32_t(uint16_t(200))<<16),uint16_t(300)}};
    frame.owner_endpoint_400=std::array<uint32_t,2>{{uint32_t(uint16_t(104))|(uint32_t(uint16_t(200))<<16),uint16_t(300)}};
    frame.owner_endpoint_418=frame.owner_endpoint_400;
    frame.owner_busy_198=&busy;frame.proxy_mask_80098798=&mask;
    frame.map_collision_80049c24=false;frame.audio=&audio;
    const auto allocations_before=allocations;
    const auto first=owner.tick_source_busters_80017154(frame);
    require(first.committed==1&&first.registered==1&&first.held==0&&audio.count==1&&busy==0,
        "normal Buster state0 did not register segment/audio atomically");
    require(owner.hit_proxy_bank().count==2&&owner.buster_pool().slots[0][8]==1&&
        owner.buster_pool().slots[0][0x0b]==11,"normal Buster state0 bytes differ");
    SourceHitProxyTarget enemy;enemy.source_address=0x800b103cu;
    SourceHitProxyRegistration registration;registration.pointed_words=*frame.owner_endpoint_400;
    registration.a1=80;registration.a3=0x8000;registration.stack10=0x20;
    registration.stack18=0x04880000;registration.stack1c=0x00800000;
    registration.target=&enemy;registration.mask_80098798=0;
    require(source_hit_proxy_register_800341bc(owner.hit_proxy_bank(),registration).status==
        SourceHitProxyStatus::SourceBackedPartial,"tutorial enemy proxy registration failed");
    SourceHitProxyRules rules;rules.unsigned_80097d00.fill(255);
    const auto collision=owner.consume_proxy_pairs(rules);
    require(collision.overlapping_pairs==1&&enemy.packet_00==0x28000008u,
        "normal Buster did not deliver original 0x28000008 enemy packet");
    require(read32(owner.buster_pool().slots[0],0xc4)&0xff000000u,
        "enemy contact was not synchronized to Buster +C4 packet");
    Class20_03CallbackState enemy_state;enemy_state.packet_33c=enemy.packet_00;enemy_state.health_6e=20;
    Class20_03HitInputs hit;hit.packet_33c=enemy.packet_00;hit.sound_consumer_ready=true;
    const auto damage=class20_03_consume_hit_801041b0(enemy_state,hit);
    require(damage.status==Class20_03HitStatus::SourceBackedPartial&&enemy_state.health_6e==12,
        "tutorial enemy did not consume original eight-point Buster damage");
    const auto impact=owner.tick_source_busters_80017154(frame);
    require(impact.committed==1&&impact.impacts==1&&busy==1&&
        owner.buster_pool().slots[0][8]==2,"normal Buster actor impact state did not commit");
    require(owner.effect_pool().slots[0][0]==1&&owner.effect_pool().slots[0][2]==0x84,
        "normal Buster impact did not allocate original effect84");
    const auto terminal=owner.tick_source_busters_80017154(frame);
    require(terminal.released==1&&busy==0&&owner.buster_pool().pending_release_80098a60==0x80000000u,
        "normal Buster state2 did not mark deferred release");
    owner.consume_pending_releases();
    require(owner.buster_pool().allocated_800989f0==0,"normal Buster deferred release was not consumed");
    require(allocations==allocations_before,"normal Buster callback/hit chain allocated heap");
    std::cout<<"normal80 spawn -> callback -> segment -> enemy packet -> 8HP -> effect84/release PASS\n";
}
} // namespace

int main(int argc,char** argv){try{
    producer_transactions();child_transactions();effect_exhaustion();capacity_and_allocations();proxy_lease_owner();buster_guards_and_owner();buster_normal_hit_chain();
    if(argc>1){floor_fixtures(argv[1]);buster_fixtures(argv[1]);}std::cout<<"PASS bounded class60 child transaction owner\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
