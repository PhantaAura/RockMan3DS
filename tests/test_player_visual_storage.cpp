#include "legends3ds/visual_bridge.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/st00_source.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

namespace {
bool count_allocations = false;
size_t allocation_count = 0;
}

void* operator new(std::size_t size) {
    if (count_allocations) ++allocation_count;
    if (void* p = std::malloc(size ? size : 1u)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
// Frozen pre-01412 bridge, independent of the new in-place function. This is an
// equivalence oracle for presentation/lifetime changes, not a retail pose oracle.
std::vector<l3::PosedPlayerVisualPrimitive> prior_pose(
    const std::vector<l3::PlayerVisualPrimitive>& local, const l3::PlayerPoseMatrices& pose) {
    std::vector<l3::PosedPlayerVisualPrimitive> out;
    out.reserve(local.size());
    for (const auto& src : local) {
        if (src.transform_id >= pose.world.size()) throw std::runtime_error("invalid transform");
        if (src.vertex_count != 3 && src.vertex_count != 4) throw std::runtime_error("invalid count");
        l3::PosedPlayerVisualPrimitive dst;
        dst.component_slot=src.component_slot; dst.part_id=src.part_id; dst.transform_id=src.transform_id;
        dst.vertex_count=src.vertex_count; dst.primitive_code=src.primitive_code;
        dst.r=src.r; dst.g=src.g; dst.b=src.b; dst.clut=src.clut; dst.tpage=src.tpage; dst.uv=src.uv;
        for (size_t i=0; i<src.vertex_count; ++i) {
            const auto& v=src.position[i];
            dst.position[i]=l3::player_pose_transform_point(pose.world[src.transform_id],
                {float(v.x),float(v.y),float(v.z)});
        }
        out.push_back(dst);
    }
    return out;
}

bool equal(const l3::PosedPlayerVisualPrimitive& a, const l3::PosedPlayerVisualPrimitive& b) {
    if (a.component_slot!=b.component_slot || a.part_id!=b.part_id || a.transform_id!=b.transform_id ||
        a.vertex_count!=b.vertex_count || a.primitive_code!=b.primitive_code || a.r!=b.r || a.g!=b.g ||
        a.b!=b.b || a.clut!=b.clut || a.tpage!=b.tpage || a.uv!=b.uv) return false;
    for (size_t i=0; i<4; ++i)
        if (a.position[i].x!=b.position[i].x || a.position[i].y!=b.position[i].y ||
            a.position[i].z!=b.position[i].z) return false;
    return true;
}

void check_frame(const std::vector<l3::PlayerVisualPrimitive>& local, const l3::PlayerPoseMatrices& matrices,
                 std::vector<l3::PosedPlayerVisualPrimitive>& scratch, const std::vector<uint32_t>& words) {
    const auto expected=prior_pose(local,matrices);
    const auto* storage=scratch.data();
    const auto capacity=scratch.capacity();
    allocation_count=0; count_allocations=true;
    l3::pose_player_visuals_into(local,matrices,scratch);
    count_allocations=false;
    assert(allocation_count==0 && scratch.data()==storage && scratch.capacity()==capacity);
    assert(scratch.size()==expected.size());
    for (size_t i=0;i<scratch.size();++i) assert(equal(scratch[i],expected[i]));
    std::vector<uint32_t> prior_words;
    for (const auto& p:expected) prior_words.push_back((uint32_t(p.clut)<<16u)|p.tpage);
    std::sort(prior_words.begin(),prior_words.end());
    prior_words.erase(std::unique(prior_words.begin(),prior_words.end()),prior_words.end());
    assert(words==prior_words);
}

unsigned synthetic_checks() {
    std::vector<l3::PlayerVisualPrimitive> local;
    for (unsigned part=0;part<l3::kPbdMainPosePartCount;++part) {
        for (unsigned count=3;count<=4;++count) {
            l3::PlayerVisualPrimitive p;
            p.component_slot=uint8_t(part%5); p.part_id=uint8_t(part+15); p.transform_id=uint8_t(part);
            p.vertex_count=uint8_t(count); p.primitive_code=uint8_t(count==3?0x25:0x2e);
            p.r=uint8_t(part*11); p.g=128; p.b=255;
            p.clut=uint16_t(0x7c00u+(part%3)); p.tpage=uint16_t(0x15u+(part%2));
            for (unsigned i=0;i<4;++i) {
                p.position[i]={int16_t(int(part*67)-int(i*109)),int16_t(-int(part*21+i*7)),int16_t(part*51+i*89),0};
                p.uv[i]={uint8_t(part+i*21),uint8_t(255-i*17-part)};
            }
            local.push_back(p);
        }
    }
    const auto words=l3::collect_player_visual_material_words(local);
    std::vector<l3::PosedPlayerVisualPrimitive> scratch(local.size());
    l3::PbdMainSkeleton sk;
    for (size_t part=0;part<sk.base_translation.size();++part)
        sk.base_translation[part]={int16_t(part*9),int16_t(-int(part*13)),int16_t(part*7)};
    for (unsigned frame=0;frame<128;++frame) {
        l3::PbdMainPoseBank bank;
        bank.root_translation={int16_t(int(frame)-64),int16_t(31-int(frame)),int16_t(frame*3)};
        for (size_t part=0;part<bank.part_rotation.size();++part)
            bank.part_rotation[part]={uint16_t(frame*37+part*11),uint16_t(frame*17+part*83),uint16_t(frame*23+part*41)};
        check_frame(local,l3::build_player_pose_matrices(sk,bank),scratch,words);
    }
    const auto unchanged=scratch;
    local[0].transform_id=uint8_t(l3::kPbdMainPosePartCount);
    bool threw=false;
    try { l3::pose_player_visuals_into(local,l3::PlayerPoseMatrices{},scratch); }
    catch (const std::runtime_error&) { threw=true; }
    assert(threw);
    for (size_t i=0;i<scratch.size();++i) assert(equal(scratch[i],unchanged[i]));
    local[0].transform_id=0; local.back().vertex_count=2; threw=false;
    try { l3::pose_player_visuals_into(local,l3::PlayerPoseMatrices{},scratch); }
    catch (const std::runtime_error&) { threw=true; }
    assert(threw);
    for (size_t i=0;i<scratch.size();++i) assert(equal(scratch[i],unchanged[i]));
    l3::pose_player_visuals_into({},l3::PlayerPoseMatrices{},scratch);
    assert(scratch.empty() && l3::collect_player_visual_material_words({}).empty());
    return 128;
}

unsigned actor_transform_checks() {
    const l3::PlayerPosePoint actor{137.f,-251.f,509.f};
    const std::array<l3::PlayerPosePoint,4> points{{
        {0.f,0.f,0.f},{8.f,-16.f,24.f},{-64.f,128.f,-256.f},{72.f,-96.f,112.f}
    }};
    const std::array<uint16_t,8> yaws{{0,1024,2048,3072,4095,4096,512,3584}};
    const std::array<uint16_t,3> raw_terms{{0,1024,4095}};
    unsigned cases=0;
    const auto near=[](float a,float b){return std::abs(a-b)<0.0001f;};
    for (uint16_t yaw:yaws) for (uint16_t raw:raw_terms) {
        const auto matrix=l3::build_player_actor_render_matrix(actor,yaw,raw);
        const auto rotation=l3::player_pose_rotation_xyz(0,
            uint16_t((uint32_t(yaw)-raw)&0x0fffu),0);
        // The actor's world position is never scaled, even at nonzero yaw.
        const auto origin=l3::player_pose_transform_point(matrix,{0,0,0});
        assert(origin.x==actor.x && origin.y==actor.y && origin.z==actor.z);
        for (const auto& point:points) {
            const auto expected=l3::player_pose_transform_point(rotation,
                {point.x/8.f,point.y/8.f,point.z/8.f});
            const auto actual=l3::player_pose_transform_point(matrix,point);
            assert(near(actual.x,actor.x+expected.x) && near(actual.y,actor.y+expected.y) &&
                   near(actual.z,actor.z+expected.z));
            ++cases;
        }
    }
    // Direct signed-axis fixtures: a positive quarter turn sends +Z to +X.
    const auto positive=l3::player_pose_transform_point(
        l3::build_player_actor_render_matrix({},1024),{0,0,8});
    const auto negative=l3::player_pose_transform_point(
        l3::build_player_actor_render_matrix({},3072),{0,0,8});
    assert(near(positive.x,1.f) && near(positive.z,0.f));
    assert(near(negative.x,-1.f) && near(negative.z,0.f));
    return cases+2;
}

void owner_checks(const std::string& root) {
    const std::array<const char*,5> names{{"body","head","right","left","feet"}};
    std::array<l3::PbdModel,l3::kPlayerComponentSlots> models;
    for (size_t i=0;i<models.size();++i)
        models[i]=l3::load_l3model(l3::read_file(root+"/player/"+names[i]+".l3model"),true);
    const auto comp=l3::load_l3player_composition(l3::read_file(root+"/player/player.l3pc"),true);
    const auto proven=l3::build_baseline_player_composition(models);
    assert(comp.attachments.size()==proven.attachments.size());
    for (size_t i=0;i<comp.attachments.size();++i) {
        const auto& a=comp.attachments[i]; const auto& b=proven.attachments[i];
        assert(a.component_slot==b.component_slot && a.mesh_index==b.mesh_index && a.part_id==b.part_id && a.transform_id==b.transform_id);
    }
    l3::PlayerVisualStats stats;
    const auto local=l3::build_player_visuals(models,comp,&stats);
    const auto words=l3::collect_player_visual_material_words(local);
    const auto sk=l3::load_l3skeleton(l3::read_file(root+"/player/main.l3skel"),true);
    const auto anim=l3::load_l3animation(l3::read_file(root+"/player/main.l3anim"),true);
    std::vector<l3::PosedPlayerVisualPrimitive> scratch(local.size());
    unsigned actions=0,frames=0;
    for (const auto& a:anim.actions) if(a.m_is_frame_table && !a.angle_frames.empty()) {
        ++actions;
        for (const auto& f:a.angle_frames) {
            check_frame(local,l3::build_player_pose_matrices(sk,l3::decode_pbd_main_pose_bank(f)),scratch,words);
            ++frames;
        }
    }
    unsigned resolved_bindings=0;
    for (unsigned area=0;area<5;++area) {
        const auto cache=l3::load_l3material_cache(l3::read_file(root+"/materials/player_area_0"+std::to_string(area)+".l3mc"),true);
        for (uint32_t word:words) {
            const auto* entry=l3::find_material_cache_entry(cache,word);
            assert(entry && entry->resolved); ++resolved_bindings;
        }
    }
    std::cout<<",\"owner_pack\":{\"status\":\"SOURCE_BACKED_PARTIAL\",\"components\":5,\"attachments\":"
        <<comp.attachments.size()<<",\"primitives\":"<<local.size()<<",\"triangles\":"<<stats.triangles
        <<",\"quads\":"<<stats.quads<<",\"vertex_references\":"<<stats.vertex_references
        <<",\"distinct_material_words\":"<<words.size()<<",\"resolved_area_material_bindings\":"<<resolved_bindings
        <<",\"frame_table_actions_checked\":"<<actions<<",\"pose_frames_checked\":"<<frames
        <<",\"pose_equivalence\":true,\"material_order_equivalence\":true,\"hot_pose_heap_allocations\":0"
        <<",\"retail_visual_parity\":\"UNKNOWN\"}";
}
}

int main(int argc,char** argv) {
    if(argc>2) { std::cerr<<"usage: test_player_visual_storage [owner tutorial root]\n"; return 2; }
    try {
        const unsigned frames=synthetic_checks();
        const unsigned actor_cases=actor_transform_checks();
        std::cout<<"{\"format\":\"player-visual-storage-check/1\",\"synthetic_frames_checked\":"<<frames
            <<",\"host_posed_primitive_bytes\":"<<sizeof(l3::PosedPlayerVisualPrimitive)
            <<",\"actor_transform_cases_checked\":"<<actor_cases
            <<",\"actor_world_translation_unscaled\":true,\"actor_local_render_scale\":0.125"
            <<",\"pose_equivalence\":true,\"material_order_equivalence\":true,\"hot_pose_heap_allocations\":0"
            <<",\"malformed_input_preserves_output\":true";
        if(argc==2) owner_checks(argv[1]);
        std::cout<<"}\n";
        return 0;
    } catch(const std::exception& e) { count_allocations=false; std::cerr<<e.what()<<'\n';return 1; }
}
