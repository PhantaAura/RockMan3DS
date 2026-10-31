#include <cstdio>
#include "legends3ds/scene_e0.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/runtime_session.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <sstream>
#include <string>
#include <stdexcept>

static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void put(std::vector<uint8_t>& bytes,size_t at,uint32_t word,size_t count=4){
    for(size_t i=0;i<count;++i)bytes[at+i]=uint8_t(word>>(i*8));
}
std::vector<uint8_t> payload(){
    std::vector<uint8_t> out(4096);put(out,0,14);
    for(size_t i=0;i<14;++i){const size_t at=4+i*16;out[at]=0xe0;out[at+1]=3;out[at+2]=uint8_t(i);put(out,at+4,0x8016c100);}
    const std::array<size_t,3> meshes{{512,800,1040}};
    for(size_t i=0;i<3;++i){const size_t mesh=meshes[i];put(out,256+0x70+i*4,0x8016c000+uint32_t(mesh));
        out[mesh+20]=1;out[mesh+21]=1;out[mesh+22]=4;out[mesh+23]=uint8_t(9+i);
        put(out,mesh+24,0x8016c000+700);put(out,mesh+28,0x8016c000+712);
        put(out,mesh+32,0x12340002+uint32_t(i));put(out,mesh+36,0x8016c000+600);}
    for(size_t i=0;i<4;++i){put(out,600+i*8,uint16_t(int(i)*3-4),2);put(out,602+i*8,uint16_t(int(i)*5-8),2);
        put(out,604+i*8,uint16_t(int(i)*7-16),2);put(out,606+i*8,uint16_t(i+10),2);}
    const std::array<uint8_t,12> triangle{{11,12,21,22,31,32,99,98,0,1,2,97}},quad{{11,12,21,22,31,32,41,42,0,1,3,2}};
    for(size_t i=0;i<12;++i){out[700+i]=triangle[i];out[712+i]=quad[i];}return out;
}
l3::SourceEventPhase5Profile profile(){
    l3::SourceEventPhase5Profile out;out.initialized=true;out.directory_token=0x8011af4c;out.callback_table_token=0x8011b2e8;
    // Independently computed by Python hashlib over this authored payload.
    const char* sha="49541723b8946e0c5e95bdcfd8feac8567afc9433e293bf145b2f1428d7b2e34";
    const auto digit=[](char c){return uint8_t(c<='9'?c-'0':c-'a'+10);};
    for(size_t i=0;i<32;++i)out.ebd_source_digest[i]=uint8_t(digit(sha[i*2])*16+digit(sha[i*2+1]));
    for(size_t i=0;i<16;++i){auto& r=out.records[i];r.raw[0]=uint8_t(i);r.raw[1]=i?3:5;r.raw[3]=i?0xe0:0x40;
        r.raw[4]=uint8_t(i%8);r.raw[12]=uint8_t(i*3);r.raw[14]=uint8_t(i*5);r.raw[16]=uint8_t(i*7);
        if(i){r.resource_variant=uint8_t((i-1)%14);r.resource_directory_index=r.resource_variant;r.resource_token=0x8016c100;}}
    return out;
}
l3::SourceEventPhase5Context constructor(const l3::SourceEventPhase5Profile& p,const l3::SourceEventPhase5ResourceBinding& witness){
    l3::SourceEventPhase5Context state;l3::initialize_source_event_phase5_pools_8002e88c(state);
    state.initial_eye_800a39a0=std::array<uint32_t,3>{};state.initial_rotation_800a3964=std::array<uint32_t,3>{};
    l3::SourceEventWork work;work.known.fill(1);work.raw[0]=1;work.raw[1]=1;work.raw[4]=5;
    require(l3::consume_source_event1_phase5_80100efc(work,state,p,&witness).committed,"authored constructor held");return state;
}
void authored(){
    using Status=l3::SceneE0Status;
    const auto bytes=payload();const auto directory=l3::parse_ebd_directory(bytes,0x8016c000);const auto p=profile();
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);require(witness.admitted(),"authored resource SHA join rejected");
    const auto cache=l3::load_scene_e0_resources(directory,p,witness);require(cache.resource_count()==3&&cache.decoded_bytes()<128*1024,"LOD dedup or budget failed");
    for(size_t i=0;i<3;++i){const auto mesh=l3::decode_scene_e0_mesh(directory,0,static_cast<l3::SceneE0MeshTier>(i));
        require(mesh.part.vertices.size()==4&&mesh.part.triangles.size()==1&&mesh.part.quads.size()==1&&
            mesh.source_part_byte3==9+i&&mesh.material_word==0x12340002+i,"typed E0 mesh decode differs");
        require(mesh.part.vertices[0].x==-4&&mesh.part.vertices[3].z==5&&mesh.part.triangles[0].uv[4]==31&&
            mesh.part.quads[0].index[2]==3&&mesh.part.triangles[0].clut==0x1234,"source vertex/packet/material fields lost");
    }
    auto state=constructor(p,witness);const auto before_allocations=allocations;
    for(size_t i=0;i<15;++i){auto& work=state.pool_e0.slots[i];const auto before=work;
        const auto result=l3::initialize_scene_e0_first_update_80112bdc(work,cache,directory);
        require(result.committed&&result.status==Status::Initialized&&work.raw[8]==1&&
            work.raw[0]==(work.raw[3]?7:15),"first callback stores incorrect");
        for(size_t j=0;j<80;++j)if(j!=0&&j!=8&&(j<24||j>=56))require(work.raw[j]==before.raw[j]&&work.known[j]==before.known[j],"initializer altered retained fields");
        const auto draw=l3::scene_e0_draw_view(work,cache,directory,l3::SceneE0MeshTier::Near);
        require(draw.status==Status::Ready&&draw.view.mesh&&draw.view.rotation==std::array<int16_t,9>{{4096,0,0,0,4096,0,0,0,4096}}&&
            draw.view.position[0]==int16_t((i+1)*3),"scene draw adapter differs");
        const auto after=work;require(l3::initialize_scene_e0_first_update_80112bdc(work,cache,directory).status==Status::NeedsVariantBehavior&&
            work.raw==after.raw&&work.known==after.known,"later variant silently approximated");
    }
    require(allocations==before_allocations,"E0 callback/draw allocated per frame");
    const auto baseline=state.pool_e0.slots[0];
    for(size_t i=0;i<8;++i){auto work=baseline;work.raw[8]=0;Status expected;
        switch(i){case 0:work.known[0]=0;expected=Status::NeedsFields;break;
            case 1:work.raw[6]=0;expected=Status::NotScheduled;break;
            case 2:work.raw[2]=4;expected=Status::UnsupportedSubtype;break;
            case 3:work.raw[3]=8;expected=Status::UnsupportedVariant;break;
            case 4:work.raw[8]=2;expected=Status::NeedsVariantBehavior;break;
            case 5:work.known[21]=0;expected=Status::NeedsFields;break;
            case 6:work.raw[20]^=4;expected=Status::NeedsResourceOwnership;break;
            default:work.raw[3]=7;work.raw[20]^=8;expected=Status::NeedsResourceOwnership;break;}
        const auto before=work;const auto result=l3::initialize_scene_e0_first_update_80112bdc(work,cache,directory);
        require(!result.committed&&result.status==expected&&work.raw==before.raw&&work.known==before.known,"atomic E0 admission hold incorrect");}
    for(size_t i=0;i<5;++i){auto work=baseline;Status expected;
        switch(i){case 0:work.known[40]=0;expected=Status::NeedsFields;break;
            case 1:work.raw[0]|=0x20;expected=Status::UnsupportedDrawMode;break;
            case 2:work.raw[6]&=0xfd;expected=Status::NotScheduled;break;
            case 3:work.raw[0]=0x13;expected=Status::UnsupportedDrawMode;break;
            default:work.raw[20]^=4;expected=Status::NeedsResourceOwnership;break;}
        require(l3::scene_e0_draw_view(work,cache,directory,l3::SceneE0MeshTier::Near).status==expected,"draw admission hold incorrect");}
    for(size_t i=0;i<9;++i){auto bad=bytes;
        switch(i){case 0:put(bad,256+0x70,0);break;case 1:put(bad,256+0x70,0x8016c001);break;
            case 2:put(bad,256+0x70,0x8016d000-20);break;case 3:bad[534]=0;break;
            case 4:put(bad,548,0x8016d000-4);break;case 5:put(bad,536,0);break;
            case 6:put(bad,540,0x8016d000-8);break;case 7:bad[708]=4;break;default:bad[722]=4;break;}
        bool rejected=false;try{(void)l3::decode_scene_e0_mesh(l3::parse_ebd_directory(bad,0x8016c000),0,l3::SceneE0MeshTier::Near);}
        catch(const std::exception&){rejected=true;}require(rejected,"malformed typed E0 resource admitted");}
    bool budget=false;try{(void)l3::load_scene_e0_resources(directory,p,witness,cache.decoded_bytes()-1);}catch(const std::exception&){budget=true;}
    require(budget,"cumulative resource budget not enforced");
    auto changed=p;changed.records[5].raw[12]^=1;bool changed_rejected=false;
    try{(void)l3::load_scene_e0_resources(directory,changed,witness);}catch(const std::exception&){changed_rejected=true;}
    require(changed_rejected,"mutated profile retained draw admission");
    auto replacement=l3::parse_ebd_directory(bytes,0x8016c000);require(!cache.resident_in(replacement),"same byte replacement silently rebound lease");
    l3::SceneE0Resources expired;{auto transient=l3::parse_ebd_directory(bytes,0x8016c000);
        const auto current=l3::bind_source_event_phase5_resources(p,transient);expired=l3::load_scene_e0_resources(transient,p,current);
        require(expired.resident_in(transient),"transient owner admission failed");transient.source_payload.clear();
        require(!expired.resident_in(transient),"cache prevented owner unload");}
    require(l3::scene_e0_draw_view(baseline,expired,replacement,l3::SceneE0MeshTier::Near).status==Status::NeedsResourceOwnership,
        "expired cache drew retained decoded data");
    const auto before_alloc=allocations;for(size_t i=0;i<4096;++i){auto work=baseline;work.raw[8]=0;
        require(l3::initialize_scene_e0_first_update_80112bdc(work,cache,replacement).status==Status::NeedsResourceOwnership,"replacement initialized stale scene");
        require(l3::scene_e0_draw_view(work,cache,replacement,l3::SceneE0MeshTier::Near).status==Status::NeedsResourceOwnership,"replacement drew stale scene");}
    require(allocations==before_alloc,"4096 lifecycle holds allocated");
    require(l3::scene_e0_tier_for_source_depth(700)==l3::SceneE0MeshTier::Near&&
        l3::scene_e0_tier_for_source_depth(701)==l3::SceneE0MeshTier::Middle&&
        l3::scene_e0_tier_for_source_depth(1800)==l3::SceneE0MeshTier::Middle&&
        l3::scene_e0_tier_for_source_depth(1801)==l3::SceneE0MeshTier::Far,"source LOD boundary differs");
    std::cout<<"typed E0 geometry/LOD/dedup; 15 callbacks; 8 atomic guards; 5 draw guards; 9 malformed resources; memory/profile/replacement; 4096 allocation-free holds PASS\n";
}
void session_integration(){
    using Status=l3::SceneE0Status;
    l3::RuntimeMap map;l3::RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
    l3::TutorialRuntimeArea runtime_area;runtime_area.ebd=l3::parse_ebd_directory(payload(),0x8016c000);
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    l3::RuntimeSession session(std::move(map),std::move(runtime_area),{},settings);const auto p=profile();
    session.bind_source_event_phase5_profile(p);
    require(session.bind_source_event_phase5_resources()==l3::SourceEventPhase5ResourceStatus::Admitted,"session owner binding rejected");
    const auto cache=l3::load_scene_e0_resources(session.active_area().ebd,p,session.source_event_phase5_resources());
    // Authored test chronology: this mutable test instance owns the snapshot.
    // This does not assert complete retail main-phase/fade gating.
    auto& context=const_cast<l3::SourceEventPhase5Context&>(session.source_event_phase5_context());
    context=constructor(p,session.source_event_phase5_resources());context.pool_e0.used[0]=0x80000000u;
    const auto inactive=context.pool_e0.slots[1];const auto allocated=allocations;
    const auto first=session.service_diagnostic_scene_e0_first_updates(cache);
    require(first.initialized_count==1&&first.held_count==0&&first.slots[0].committed&&first.diagnostic_chronology&&
        context.pool_e0.slots[0].raw[8]==1&&context.pool_e0.slots[1].raw==inactive.raw&&context.pool_e0.slots[1].known==inactive.known,
        "session first diagnostic callback or inactive owner differs");
    const auto initialized=context.pool_e0.slots[0];const auto repeat=session.service_diagnostic_scene_e0_first_updates(cache);
    require(repeat.initialized_count==0&&repeat.held_count==1&&repeat.slots[0].status==Status::NeedsVariantBehavior&&
        context.pool_e0.slots[0].raw==initialized.raw&&allocations==allocated,"session repeated callback mutated or allocated");
    auto& active=const_cast<l3::TutorialRuntimeArea&>(session.active_area());active.ebd.entries[0].key_flags^=1;
    const auto stale=session.service_diagnostic_scene_e0_first_updates(cache);
    require(stale.status==Status::NeedsResourceOwnership&&stale.initialized_count==0&&context.pool_e0.slots[0].raw==initialized.raw,
        "mutated active EBD retained session admission");
    active.ebd.entries[0].key_flags^=1;
    require(!session.transition_to_location(999,{1,2,3},0),"unregistered transition unexpectedly admitted");
    require(session.service_diagnostic_scene_e0_first_updates(cache).held_count==1,"rejected transition discarded current owners");
    session.bind_source_event_phase5_profile(p);const auto rebound=session.service_diagnostic_scene_e0_first_updates(cache);
    require(rebound.status==Status::NeedsResourceOwnership&&rebound.initialized_count==0,"profile rebind reused old owner cache");
    require(session.bind_source_event_phase5_resources()==l3::SourceEventPhase5ResourceStatus::Admitted,"profile ownership rebound failed");
    context=constructor(p,session.source_event_phase5_resources());context.pool_e0.used[0]=0x80000000u;
    session.register_location(1,session.active_map(),0,l3::TutorialRuntimeArea{});
    require(session.transition_to_location(1,{1,2,3},0),"registered area transition held");
    require(session.service_diagnostic_scene_e0_first_updates(cache).status==Status::NeedsResourceOwnership,
        "accepted transition retained E0 constructor admission");
    std::cout<<"authored RuntimeSession E0 diagnostic chronology: one callback, inactive/repeated holds, EBD mutation, profile reset, rejected/accepted transition PASS\n";
}
void attachment_session_integration(){
    using Status=l3::SceneE0Status;
    l3::RuntimeMap map;l3::RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;
    map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
    l3::TutorialRuntimeArea runtime_area;runtime_area.ebd=l3::parse_ebd_directory(payload(),0x8016c000);
    L3QolSettings settings{};l3_qol_settings_defaults(&settings);
    l3::RuntimeSession session(std::move(map),std::move(runtime_area),{},settings);
    auto p=profile();p.records[1].raw[4]=0;p.records[15].raw[4]=0; // Authored primary/auxiliary parameter0 representations.
    l3::SourceEventPollProfile poll;poll.decoder=1;session.bind_source_event_profile(poll,0);
    session.bind_source_event_phase5_profile(p);
    require(session.bind_source_event_phase5_resources()==l3::SourceEventPhase5ResourceStatus::Admitted,"attachment session resource owner rejected");
    const auto cache=l3::load_scene_e0_resources(session.active_area().ebd,p,session.source_event_phase5_resources());
    l3::ReferenceTables refs;refs.trig[0]={0,4096};refs.trig[1024]={4096,0};session.set_reference_tables(refs);
    require(session.initialize_fresh_source_event_subset_st00()&&session.source_prop_foreign_globals().admitted()&&
        session.source_prop_foreign_globals().queue_bound_admitted(),"actual fresh initialization did not admit foreign global owner");
    l3::SourceEventWork event;const uint32_t actor_id=0x800c3580u;
    for(size_t i=0;i<4;++i){event.raw[0x24+i]=uint8_t(actor_id>>(8*i));event.known[0x24+i]=1;}
    session.bind_source_event_state(event,{},std::nullopt,std::nullopt,std::nullopt);
    // Authored test owner: the constructor, packed bone matrix and coordinate
    // stores are explicit inputs. This is not a retail scheduler/pose claim.
    auto& context=const_cast<l3::SourceEventPhase5Context&>(session.source_event_phase5_context());
    context=constructor(p,session.source_event_phase5_resources());context.pool_e0.used[0]=0x80000000u;
    require(session.service_diagnostic_scene_e0_first_updates(cache).initialized_count==1,"attachment first callback held");
    auto& actor=context.pool40.slots[0];const auto store_actor=[&](size_t off,uint16_t value){actor.raw[off]=uint8_t(value);actor.raw[off+1]=uint8_t(value>>8);actor.known[off]=actor.known[off+1]=1;};
    actor.raw[1]=0x40;actor.raw[2]=5;actor.known[1]=actor.known[2]=1;
    for(size_t i=0;i<9;++i)store_actor(672+2*i,uint16_t(i%4==0?512:0));
    store_actor(408,100);store_actor(410,200);store_actor(412,300);
    auto& prop=context.pool_e0.slots[0];prop.raw[9]=1;prop.raw[12]=0;prop.known[9]=prop.known[12]=1;
    const size_t allocated=allocations;const auto updated=session.service_diagnostic_scene_e0_updates(cache);
    require(updated.updated_count==1&&updated.held_count==0&&updated.slots[0].committed&&updated.slots[0].source_function==0x80112d14u,"attachment session failed to reach original leaf");
    const auto draw=l3::scene_e0_draw_view(prop,cache,session.active_area().ebd,l3::SceneE0MeshTier::Near);
    require(draw.status==Status::Ready&&draw.view.mesh&&draw.view.position==std::array<int16_t,3>{{110,240,300}}&&
        draw.view.rotation==std::array<int16_t,9>{{4096,0,0,0,0,-4096,0,4096,0}}&&draw.view.modulation_word==0x02808080u,
        "attached source transform or alpha word did not reach E0 draw adapter");
    for(size_t i=44;i<56;++i)require(prop.known[i]==0,"wrapper invented attachment stack padding");
    const auto attached=prop;actor.known[408]=0;
    const auto missing=session.service_diagnostic_scene_e0_updates(cache);
    require(missing.updated_count==0&&missing.held_count==1&&missing.slots[0].status==Status::NeedsLinkedOwner&&
        prop.raw==attached.raw&&prop.known==attached.known,"missing parent pose changed attached prop");
    actor.known[408]=1;l3::SceneE0UpdateInputs inputs;inputs.references=&refs;inputs.event_work=&event;inputs.pool=&context.pool_e0;inputs.scene_context=&context;inputs.prop_identity=0x800bf118u;
    auto detached=attached;const auto stale=l3::update_scene_e0_80112b9c(detached,cache,session.active_area().ebd,inputs);
    require(!stale.committed&&stale.status==Status::NeedsLinkedOwner&&detached.raw==attached.raw&&detached.known==attached.known,"copied prop bypassed exact context slot owner");
    require(allocations==allocated,"attachment session/wrapper/draw allocated per frame");
    // Real RuntimeSession fresh lifecycle and wrapper release, with the exact
    // selected auxiliary slot identity. No external word is forced into it.
    context.pool_e0.used[0]=0x80000000u>>14;
    const auto aux_first=session.service_diagnostic_scene_e0_first_updates(cache);
    require(aux_first.initialized_count==1&&aux_first.slots[14].committed,"auxiliary first callback held");
    auto& auxiliary=context.pool_e0.slots[14];auxiliary.raw[9]=1;auxiliary.raw[12]=1;
    auxiliary.known[9]=auxiliary.known[12]=1;
    const auto aux_mask=*context.pool_e0.used[0];const auto aux_allocations=allocations;
    const auto released=session.service_diagnostic_scene_e0_updates(cache);
    require(released.updated_count==1&&released.held_count==0&&released.slots[14].committed&&
        released.slots[14].foreign_release_committed&&auxiliary.raw[0]==0&&*context.pool_e0.used[0]==aux_mask&&
        session.source_prop_foreign_globals().pending_8009898c()==std::optional<uint32_t>{0},
        "session foreign release did not atomically clear visibility/publish pending/preserve E0allocation");
    require(l3::scene_e0_draw_view(auxiliary,cache,session.active_area().ebd,l3::SceneE0MeshTier::Near).status==Status::NotScheduled&&
        allocations==aux_allocations,"released auxiliary remained drawable or allocated");
    session.register_location(1,session.active_map(),0,l3::TutorialRuntimeArea{});
    require(session.transition_to_location(1,{1,2,3},0)&&!session.source_prop_foreign_globals().admitted()&&
        !session.source_prop_foreign_globals().pending_8009898c()&&
        session.service_diagnostic_scene_e0_updates(cache).status==Status::NeedsResourceOwnership,
        "accepted unload retained foreign release/global resource owner");
    std::cout<<"authored attachment session -> current pool slot -> original bone3 transform -> alpha draw word; missing parent/copy owner atomic holds; no heap PASS\n";
    std::cout<<"fresh RuntimeSession -> auxiliary slot14 foreign release -> notdrawable, E0allocation retained -> unload invalidation; allocation-free callback PASS\n";
}
std::vector<uint8_t> read(const char* path){std::ifstream input(path,std::ios::binary);require(bool(input),"private owner file missing");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());}
void behavior_authored(){
    using Status=l3::SceneE0Status;const auto directory=l3::parse_ebd_directory(payload(),0x8016c000);const auto p=profile();
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);const auto cache=l3::load_scene_e0_resources(directory,p,witness);
    auto context=constructor(p,witness);auto baseline=context.pool_e0.slots[7];baseline.raw[3]=0;baseline.raw[8]=1;baseline.raw[9]=0;baseline.raw[10]=0;
    l3::ReferenceTables refs;refs.trig[0]={0,4096};refs.trig[9]={56,4096};
    l3::SourceEventWork event;event.known.fill(1);const uint32_t token=0x800bf168u;
    for(size_t i=0;i<4;++i)event.raw[0x40+i]=uint8_t(token>>(i*8));
    l3::SceneE0UpdateInputs inputs;inputs.references=&refs;inputs.event_work=&event;inputs.pool=&context.pool_e0;inputs.scratch_half_1f800002=1;
    const auto allocated=allocations;auto spin=baseline;
    require(l3::update_scene_e0_80112b9c(spin,cache,directory,inputs).committed&&spin.raw[10]==1&&spin.raw[24]==0&&spin.raw[25]==16,"source spin initialization differs");
    require(l3::update_scene_e0_80112b9c(spin,cache,directory,inputs).committed&&spin.raw[68]==0x2b&&spin.raw[69]==0x1a&&spin.raw[70]==9&&
        spin.raw[28]==56&&spin.raw[36]==200&&spin.raw[37]==255,"source spin packedlookup or wrapped accumulator differs");
    auto linked=baseline;linked.raw[3]=7;linked.raw[12]=0;
    require(l3::update_scene_e0_80112b9c(linked,cache,directory,inputs).committed&&linked.raw[56]==context.pool_e0.slots[1].raw[56],"source constructor-linked placement differs");
    for(size_t test=0;test<12;++test){auto work=baseline;auto in=inputs;Status status=Status::NeedsFields;
        switch(test){case 0:work.known[10]=0;break;
        case 1:in.references=nullptr;status=Status::NeedsReferences;break;
        case 2:work.raw[3]=1;work.raw[9]=1;in.scratch_half_1f800002.reset();status=Status::NeedsScratchOwner;break;
        case 3:work.raw[9]=1;status=Status::NeedsLinkedOwner;break;
        case 4:work.raw[3]=2;work.raw[9]=2;status=Status::NeedsLinkedOwner;break;
        case 5:work.raw[3]=3;work.raw[9]=1;work.raw[10]=1;status=Status::NeedsRngOrChildOwner;break;
        case 6:work.raw[3]=4;work.raw[9]=1;work.raw[10]=1;work.raw[13]=2;status=Status::NeedsResourceMutation;break;
        case 7:work.raw[3]=7;work.raw[12]=0;in.event_work=nullptr;status=Status::NeedsLinkedOwner;break;
        case 8:work.raw[3]=7;work.raw[12]=128;status=Status::NeedsLinkedOwner;break;
        case 9:work.raw[10]=2;status=Status::UnsupportedBehaviorState;break;
        case 10:work.raw[20]^=4;status=Status::NeedsResourceOwnership;break;
        default:work.raw[3]=8;status=Status::UnsupportedVariant;break;}
        const auto before=work;const auto result=l3::update_scene_e0_80112b9c(work,cache,directory,in);
        require(!result.committed&&result.status==status&&work.raw==before.raw&&work.known==before.known,"E0 variant dependency hold mutated source-prefix fields");
    }
    require(allocations==allocated,"E0 behavior callbacks allocated per frame");
    auto replacement=l3::parse_ebd_directory(payload(),0x8016c000);const auto held_allocated=allocations;
    for(size_t i=0;i<4096;++i){auto work=spin;const auto before=work;
        require(l3::update_scene_e0_80112b9c(work,cache,replacement,inputs).status==Status::NeedsResourceOwnership&&work.raw==before.raw&&work.known==before.known,
            "expired E0 behavior ownership mutated");}
    require(allocations==held_allocated,"E0 expired ownership holds allocated per frame");
    std::cout<<"E0 variant authored spin/link, 12 atomic dependency guards, 4096 expired owner holds PASS\n";
}
void behavior_fixture(const char* path,const char* references){
    const auto directory=l3::parse_ebd_directory(payload(),0x8016c000);const auto p=profile();const auto witness=l3::bind_source_event_phase5_resources(p,directory);
    const auto cache=l3::load_scene_e0_resources(directory,p,witness);const auto refs=l3::load_l3reference(read(references));
    std::ifstream input(path);require(bool(input),"private E0 oracle fixture missing");std::string line;std::getline(input,line);size_t cases=0;
    const auto digit=[](char c){return uint8_t(c<='9'?c-'0':c-'a'+10);};
    while(std::getline(input,line)){std::istringstream fields(line);unsigned seed,variant,selector,state,frame;std::string before,linked,after;
        require(bool(fields>>seed>>variant>>selector>>state>>frame>>before>>linked>>after)&&before.size()==160&&linked.size()==160&&after.size()==160,"E0 behavior oracle malformed");
        l3::SourceEventPhase5Bytes<80> work;work.known.fill(1);l3::SourceEventPhase5Pool<32,80> pool;pool.used[0]=0x40000000u;pool.slots[1].known.fill(1);
        std::array<uint8_t,80> expected{};
        for(size_t i=0;i<80;++i){work.raw[i]=uint8_t(digit(before[i*2])*16+digit(before[i*2+1]));expected[i]=uint8_t(digit(after[i*2])*16+digit(after[i*2+1]));
            pool.slots[1].raw[i]=uint8_t(digit(linked[i*2])*16+digit(linked[i*2+1]));}
        l3::SourceEventWork event;event.known.fill(1);const uint32_t token=0x800bf168u;
        for(size_t i=0;i<4;++i)event.raw[0x40+i]=uint8_t(token>>(i*8));
        l3::SceneE0UpdateInputs inputs;inputs.references=&refs;inputs.event_work=&event;inputs.pool=&pool;inputs.scratch_half_1f800002=uint16_t(frame);
        const auto allocated=allocations;const auto result=l3::update_scene_e0_80112b9c(work,cache,directory,inputs);
        if(!result.committed||work.raw!=expected){std::cerr<<"E0 oracle mismatch variant="<<variant<<" selector="<<selector<<" state="<<state<<" seed="<<seed<<" status="<<unsigned(result.status)<<'\n';
            for(size_t i=0;i<80;++i)if(work.raw[i]!=expected[i])std::cerr<<i<<":"<<unsigned(work.raw[i])<<"/"<<unsigned(expected[i])<<' ';std::cerr<<'\n';}
        require(result.committed&&work.raw==expected,"native E0 behavior differs from unchanged source instructions");
        require(allocations==allocated,"E0 oracle callback allocated");++cases;
    }
    require(cases==184,"E0 oracle branch coverage differs");std::cout<<cases<<" unchanged original E0 state1 source differential cases PASS\n";
}
void fixture(const char* path){
    const auto directory=l3::parse_ebd_directory(payload(),0x8016c000);const auto p=profile();
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);const auto cache=l3::load_scene_e0_resources(directory,p,witness);
    std::ifstream input(path);require(bool(input),"authored source fixture missing");std::string line;std::getline(input,line);size_t cases=0;
    const auto digit=[](char c){return uint8_t(c<='9'?c-'0':c-'a'+10);};
    while(std::getline(input,line)){std::istringstream fields(line);unsigned seed,variant;std::string before,after;
        require(bool(fields>>seed>>variant>>before>>after)&&before.size()==160&&after.size()==160,"authored source fixture malformed");
        l3::SourceEventPhase5Bytes<80> work;work.known.fill(1);std::array<uint8_t,80> expected{};
        for(size_t i=0;i<80;++i){work.raw[i]=uint8_t(digit(before[i*2])*16+digit(before[i*2+1]));expected[i]=uint8_t(digit(after[i*2])*16+digit(after[i*2+1]));}
        const auto result=l3::initialize_scene_e0_first_update_80112bdc(work,cache,directory);
        require(result.committed&&work.raw==expected,"native first E0 callback differs from unchanged source oracle");++cases;
    }
    require(cases==32,"authored source oracle coverage differs");std::cout<<cases<<" unchanged-source authored E0 initializer differential cases PASS\n";
}
void owner(const char* phase5,const char* ebd){
    const auto p=l3::load_source_event_phase5_profile(read(phase5));const auto directory=l3::load_l3ebd_directory(read(ebd));
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);const auto cache=l3::load_scene_e0_resources(directory,p,witness);
    auto state=constructor(p,witness);size_t ready=0;const auto allocated=allocations;
    for(size_t i=0;i<15;++i){auto& work=state.pool_e0.slots[i];require(l3::initialize_scene_e0_first_update_80112bdc(work,cache,directory).committed,"owner first callback held");
        for(size_t tier=0;tier<3;++tier)ready+=l3::scene_e0_draw_view(work,cache,directory,static_cast<l3::SceneE0MeshTier>(tier)).status==l3::SceneE0Status::Ready;}
    require(ready==45&&allocations==allocated,"owner draw windows held/allocated");
    std::cout<<"actual resident owner E0: ready="<<ready<<" unique_meshes="<<cache.resource_count()<<" decoded_bytes="<<cache.decoded_bytes()<<" PASS\n";
}
void owner_sequence(const char* phase5,const char* ebd,const char* references,const char* fixture_path){
    const auto p=l3::load_source_event_phase5_profile(read(phase5));const auto directory=l3::load_l3ebd_directory(read(ebd));
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);const auto cache=l3::load_scene_e0_resources(directory,p,witness);
    const auto refs=l3::load_l3reference(read(references));auto state=constructor(p,witness);
    l3::SourceEventWork event;event.known.fill(1);for(size_t i=1;i<16;++i){const uint32_t token=0x800bf118u+uint32_t(i-1)*80u;
        for(size_t j=0;j<4;++j)event.raw[0x24+i*4+j]=uint8_t(token>>(j*8));}
    l3::SceneE0UpdateInputs inputs;inputs.references=&refs;inputs.event_work=&event;inputs.pool=&state.pool_e0;
    std::ifstream input(fixture_path);require(bool(input),"private selected owner sequence missing");std::string line;std::getline(input,line);
    size_t count=0;unsigned previous_frame=0xffffffffu;const auto digit=[](char c){return uint8_t(c<='9'?c-'0':c-'a'+10);};
    while(std::getline(input,line)){std::istringstream fields(line);unsigned frame,slot;std::string raw;
        require(bool(fields>>frame>>slot>>raw)&&slot<15&&raw.size()==160,"selected owner sequence malformed");
        if(frame!=previous_frame){require(frame==(previous_frame==0xffffffffu?0:previous_frame+1),"selected owner sequence skipped frame");
            const auto allocated=allocations;for(size_t i=0;i<15;++i){const auto result=l3::update_scene_e0_80112b9c(state.pool_e0.slots[i],cache,directory,inputs);
                require(result.committed,"selected owner E0 repeated update held");}
            require(allocations==allocated,"selected owner E0 repeated updates allocated");previous_frame=frame;}
        for(size_t i=0;i<80;++i)if(state.pool_e0.slots[slot].raw[i]!=uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))){
            std::cerr<<"owner sequence frame="<<frame<<" slot="<<slot<<" offset="<<i<<'\n';throw std::runtime_error("selected owner E0 update differs from unchanged source sweep");}
        ++count;
    }
    require(count==1500&&previous_frame==99,"selected owner sequence coverage differs");
    std::cout<<"100 unchanged original E0 sweeps / 1500 full selected owner records / allocation-free updates PASS\n";
}
}

// 0x801133B4 / 0x80113600 / 0x801136A4 / 0x80113A6C: falling blocks and slide
// with debris/impact family-80 children, camera+0x0C jolts and E0 rand().
l3::SourceRngProfile block_rng_profile(uint32_t seed){
    l3::SourceRngProfile p;
    const auto digest=[](const char* hex){l3::SourceRngDigest d{};for(size_t i=0;i<32;++i){unsigned v=0;std::sscanf(hex+i*2,"%2x",&v);d[i]=uint8_t(v);}return d;};
    p.bios_sha256=digest("713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d");
    p.exe_sha256=digest("10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0");
    p.overlay_sha256=digest("9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c");
    p.trace_sha256.fill(23);p.validated_prefix_sha256=p.trace_sha256;
    p.scope=l3::SourceRngCaptureScope::WholeBoundedCapture;p.warnings=3;
    p.origin_seed32=seed;p.origin_game_entry_count=1;p.origin_vsync=1062;
    p.actual_rand_returns=1;p.trace_rows=5;p.validated_prefix_rows=5;
    p.captured_bytes=1200;p.validated_prefix_bytes=1200;p.final_rand_ordinal=1;p.observed_consumer_count=1;
    return p;
}
uint16_t block_lcg(uint32_t& s){s=s*1103515245u+12345u;return uint16_t((s>>16)&0x7fffu);}
void behavior_blocks(){
    using Status=l3::SceneE0Status;const auto directory=l3::parse_ebd_directory(payload(),0x8016c000);const auto p=profile();
    const auto witness=l3::bind_source_event_phase5_resources(p,directory);const auto cache=l3::load_scene_e0_resources(directory,p,witness);
    auto context=constructor(p,witness);l3::ReferenceTables refs;l3::SourceEventWork event;event.known.fill(1);
    context.pool80.used[0]=0x80000000u;context.pool80.used[1]=0;   // slot 0 busy: children go to slot 1, 2
    for(size_t i=0;i<64;++i){context.pool80.slots[i].raw[1]=uint8_t(0x80|i);context.pool80.slots[i].known[1]=1;}
    for(size_t i=0;i<4;++i){context.camera_800bc738.raw[0x0c+i]=0;context.camera_800bc738.known[0x0c+i]=1;}
    const size_t slot=7;const uint32_t identity=0x800bf118u+uint32_t(slot)*80u;
    l3::SceneE0UpdateInputs inputs;inputs.references=&refs;inputs.event_work=&event;inputs.pool=&context.pool_e0;
    inputs.scene_context=&context;inputs.prop_identity=identity;
    auto& work=context.pool_e0.slots[slot];
    const auto set16=[&](size_t at,int v){work.raw[at]=uint8_t(v);work.raw[at+1]=uint8_t(v>>8);work.known[at]=work.known[at+1]=1;};
    const auto get16=[&](const auto& b,size_t at){return int16_t(uint16_t(b.raw[at]|(b.raw[at+1]<<8)));};
    const auto set32=[&](size_t at,uint32_t v){for(size_t i=0;i<4;++i){work.raw[at+i]=uint8_t(v>>(8*i));work.known[at+i]=1;}};
    work.raw[3]=3;work.raw[8]=1;work.raw[9]=1;work.raw[10]=1;set16(56,100);set16(58,-400);set16(60,-50);set32(68,0);
    // No E0 rand() channel: hold without stores (existing contract).
    {const auto before=work;const auto r=l3::update_scene_e0_80112b9c(work,cache,directory,inputs);
     require(!r.committed&&r.status==Status::NeedsRngOrChildOwner&&work.raw==before.raw,"block without rng mutated");}
    l3::SourceRngService rng;const uint32_t seed=0x2468u;
    require(l3::install_source_rng_service(rng,block_rng_profile(seed),l3::SourceRngMode::CandidateOpenBios)==l3::SourceRngStatus::Ready,"rng install");
    uint32_t mirror=seed;size_t frames=0,debris=0;
    // Falling: +44 += 0x14E80, +3A += +44>>16 until >0; rand()&3==0 spawns 0x80113C98 debris.
    while(work.raw[10]==1&&frames<64){
        l3::SourceRngUpdateTransaction t;require(l3::begin_source_rng_update(rng,t,{frames,frames,l3::SourceRngUpdatePhase::E0ObjectCallback})==l3::SourceRngStatus::Ready,"begin");
        inputs.e0_rng=&t;const uint32_t vel=uint32_t(work.raw[68]|(work.raw[69]<<8)|(work.raw[70]<<16)|(uint32_t(work.raw[71])<<24))+0x14e80u;
        const int16_t y=int16_t(get16(work,58)+int16_t(vel>>16));const auto used_before=*context.pool80.used[0];
        const auto r=l3::update_scene_e0_80112b9c(work,cache,directory,inputs);
        require(r.committed&&l3::commit_source_rng_update(rng,t)==l3::SourceRngStatus::Ready,"falling block commit");
        require(get16(work,58)==y,"falling y integration");
        if(y>0){require(work.raw[10]==2&&context.camera_800bc738.raw[0x0e]==0x10,"landing jolt / substate");
            const auto& impact=context.pool80.slots[size_t(__builtin_clz(~used_before))];
            require(impact.raw[2]==4&&impact.raw[12]==4&&get16(impact,28)==100&&get16(impact,30)==0&&get16(impact,32)==-50,"landing impact 0x80113C30");}
        else{const uint16_t v=block_lcg(mirror);
            if((v&3u)==0){++debris;const size_t c=size_t(__builtin_clz(~used_before));
                const auto& child=context.pool80.slots[c];
                (void)block_lcg(mirror);   // 0x80113CC8: drawn, result unused
                require(child.raw[2]==3&&child.raw[12]==3&&child.raw[15]==0&&child.raw[0]==1&&child.raw[6]==3,"debris header");
                const uint16_t sx=block_lcg(mirror),sy=block_lcg(mirror),sz=block_lcg(mirror);
                require(get16(child,28)==int16_t(100+((sx&1u)?-386:386))&&get16(child,30)==int16_t(y+(sy&511)-207)&&
                    get16(child,32)==int16_t(-50+((sz&1u)?-64:64)),"debris placement");
                context.pool80.used[0]=used_before;}   // free it again for the next frame
        }
        ++frames;
    }
    require(work.raw[10]==2&&frames>1,"block never landed");
    // Landing impact child (subtype 4) at x/z, y 0.
    // Next state 0x801136A4 undoes the jolt.
    {l3::SourceRngUpdateTransaction t;require(l3::begin_source_rng_update(rng,t,{99,99,l3::SourceRngUpdatePhase::E0ObjectCallback})==l3::SourceRngStatus::Ready,"begin2");
     inputs.e0_rng=&t;require(l3::update_scene_e0_80112b9c(work,cache,directory,inputs).committed,"jolt undo");l3::rollback_source_rng_update(t);
     require(work.raw[10]==3&&context.camera_800bc738.raw[0x0e]==0&&context.camera_800bc738.raw[0x0f]==0,"jolt undo value");}
    std::cout<<"E0 falling block 0x80113600/0x801136A4: integration, debris 0x80113C98 placement ("<<debris<<"), landing jolt PASS\n";
}
int main(int argc,char** argv){try{if(argc==1){authored();session_integration();behavior_authored();behavior_blocks();attachment_session_integration();}else if(argc==2){authored();session_integration();behavior_authored();attachment_session_integration();fixture(argv[1]);}else if(argc==3&&std::string(argv[1])=="--fixture")fixture(argv[2]);else if(argc==4&&std::string(argv[1])=="--behavior-fixture")behavior_fixture(argv[2],argv[3]);else if(argc==6&&std::string(argv[1])=="--owner-sequence")owner_sequence(argv[2],argv[3],argv[4],argv[5]);else if(argc==3)owner(argv[1],argv[2]);else throw std::runtime_error("usage: test_scene_e0 [owner.l3p5 owner.l3ebd]");return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
