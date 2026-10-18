#include "legends3ds/runtime_session.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_animation.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static int16_t half(const L3PlayerCompatWork& w,size_t o) {
    return static_cast<int16_t>(uint16_t(w.bytes[o]) | (uint16_t(w.bytes[o+1])<<8));
}
static void vec(std::ostream& o,const l3::RuntimeVec3i& p) {o<<'['<<p.x<<','<<p.y<<','<<p.z<<']';}
int main(int argc,char** argv) {
    try {
        if(argc<4) {std::cerr<<"usage: l3replay TUTORIAL_DIR INPUT.tsv OUTPUT.json [--no-animation] [--area N] [--seed SEED.tsv]\n";return 2;}
        bool animate=true;size_t area_index=0;std::filesystem::path seed_path;
        for (int i=4;i<argc;++i) {
            const std::string option=argv[i];
            if (option=="--no-animation") animate=false;
            else if (option=="--area" && i+1<argc) area_index=std::stoul(argv[++i]);
            else if (option=="--seed" && i+1<argc) seed_path=argv[++i];
            else throw std::runtime_error("unknown or incomplete replay option");
        }
        const std::filesystem::path root=argv[1];
        const auto animation=l3::load_l3animation(l3::read_file(root/"player/main.l3anim"),true);
        l3::PbdMainBodyTrack track{};
        auto map=l3::load_l3map(l3::read_file(root/"st00.l3map"),true);
        char ebd_name[64];std::snprintf(ebd_name,sizeof(ebd_name),"ebd/area_%02lu.l3ebd",(unsigned long)area_index);
        auto area=l3::load_tutorial_runtime_area(l3::read_file(root/"st00.l3flow"),l3::read_file(root/ebd_name),area_index,true);
        std::vector<l3::MmlMessageFile> banks;
        for(const char* name:{"st00_01.l3msg","st00_02.l3msg"}) banks.push_back(l3::load_l3message(l3::read_file(root/"messages"/name),true));
        L3QolSettings q{};l3_qol_settings_defaults(&q);
        l3::RuntimeSession session(map,area,banks,q);
        session.set_reference_tables(l3::load_l3reference(l3::read_file(root/"reference.l3ref"),true));
        if(area_index>=map.areas.size()) throw std::runtime_error("area outside map");
        session.reset_player({map.areas[area_index].world_origin_x+256,0,map.areas[area_index].world_origin_z+256},0,100,100);
        if (!seed_path.empty()) {
            std::ifstream seed_file(seed_path);std::array<int64_t,25> values{};
            for (auto& v:values) if(!(seed_file>>v)) throw std::runtime_error("seed requires25 integer fields");
            std::string extra;if(seed_file>>extra)throw std::runtime_error("extra seed field");
            l3::RuntimePlayerReplaySeed seed{};size_t i=0;
            const auto bounded=[&](int64_t low,int64_t high) {
                const auto v=values[i++];if(v<low||v>high)throw std::runtime_error("seed field range");return v;
            };
            seed.position={int32_t(bounded(-32768,32767)),int32_t(bounded(-32768,32767)),int32_t(bounded(-32768,32767))};
            for(auto& v:seed.fixed_position)v=int32_t(bounded(INT32_MIN,INT32_MAX));
            seed.yaw=uint16_t(bounded(0,4095));
            for(auto& v:seed.state)v=uint8_t(bounded(0,255));
            for(auto& v:seed.velocity_work)v=int16_t(bounded(-32768,32767));
            seed.work_0b=uint8_t(bounded(0,255));seed.contact_state=uint8_t(bounded(0,255));
            seed.animation_request=uint8_t(bounded(0,255));seed.animation_pose=uint8_t(bounded(0,255));
            seed.animation_countdown=uint8_t(bounded(0,255));seed.animation_record_byte2=uint8_t(bounded(0,255));
            seed.animation_signed_control=int8_t(bounded(-128,127));
            seed.direction=int8_t(bounded(0,3));seed.previous_direction=int8_t(bounded(0,3));
            seed.direction_yaw_offset=int16_t(bounded(-32768,32767));seed.turn_accumulator=int16_t(bounded(-32768,32767));
            const auto hp=uint16_t(bounded(1,65535));
            session.reset_player(seed.position,seed.yaw,hp,hp);
            if(!session.seed_player_replay_diagnostic(seed))throw std::runtime_error("unsupported replay seed");
        }
        if (std::filesystem::exists(argv[3])) throw std::runtime_error("output exists; choose a new capture path");
        std::ifstream input(argv[2]);std::ofstream out(argv[3]);
        if(!input||!out) throw std::runtime_error("cannot open replay files");
        std::string line;uint32_t expected=0;out<<"[\n";
        while(std::getline(input,line)) {
            if(line.empty()||line[0]=='#') continue;
            std::istringstream row(line);uint32_t frame,held,fire,lock,advance;std::string extra;
            if(!(row>>frame>>held>>fire>>lock>>advance)||row>>extra||frame!=expected||held>65535||fire>1||lock>1||advance>1)
                throw std::runtime_error("invalid replay row; expected contiguous frame, held, fire, lock, advance");
            l3::RuntimeInputFrame in{};in.held_psx_mask=static_cast<uint16_t>(held);in.fire_pressed=fire;
            in.lock_held=lock;in.advance_dialogue=advance;
            // Source animation records carry control bytes consumed by gameplay.
            // --no-animation suppresses the presented frame only; it must not
            // remove those source-backed control updates from the simulation.
            (void)l3::runtime_animation_prepare(animation,track,session);
            const auto before=session.player_position();session.tick(in);const auto p=session.player_position();
            const auto& w=session.compat_player();
            const auto probe=session.probe_player_map();const auto& cam=session.camera();
            if(expected++) out<<",\n";
            out<<"{\"frame\":"<<frame<<",\"input\":["<<held<<','<<fire<<','<<lock<<','<<advance<<"],\"position\":";vec(out,p);
            out<<",\"position_delta\":";vec(out,{p.x-before.x,p.y-before.y,p.z-before.z});
            out<<",\"velocity_work\":["<<half(w,0x44)<<','<<half(w,0x46)<<','<<half(w,0x48)<<']';
            out<<",\"heading\":"<<session.player_yaw()<<",\"state\":["<<unsigned(w.bytes[8])<<','<<unsigned(w.bytes[9])<<','<<unsigned(w.bytes[10])<<']';
            out<<",\"animation_request\":"<<unsigned(w.bytes[0xac])<<",\"animation_frame\":";
            if(animate&&track.valid)out<<unsigned(w.bytes[0xa4]);else out<<"null";
            out<<",\"vertical_velocity\":"<<half(w,0x46)<<",\"grounded\":";
            const auto& contact=session.player_contact();
            if(contact.grounded)out<<(*contact.grounded?"true":"false");else out<<"null";
            out<<",\"floor_y\":";if(contact.floor_y)out<<*contact.floor_y;else out<<"null";
            out<<",\"contact_status\":"<<unsigned(contact.status)<<",\"contact_state_raw\":"<<unsigned(w.bytes[0x74]);
            out<<",\"work_0b_raw\":"<<unsigned(w.bytes[0x0b])<<",\"animation_control_raw\":["<<unsigned(w.bytes[0xa4])<<','<<unsigned(w.bytes[0xa5])<<','<<unsigned(w.bytes[0xa6])<<','<<int(static_cast<int8_t>(w.bytes[0xa7]))<<']';
            out<<",\"fixed_position\":[";
            for(unsigned i=0;i<3;++i){uint32_t v=0;for(unsigned j=0;j<4;++j)v|=uint32_t(w.bytes[0x1c+4*i+j])<<(8*j);if(i)out<<',';out<<static_cast<int32_t>(v);}out<<']';
            out<<",\"map_in_area\":"<<(probe.in_area?"true":"false")<<",\"contact_groups\":"<<probe.contact_groups.size();
            out<<",\"camera\":{\"owner\":"<<unsigned(cam.owner)<<",\"eye\":";vec(out,cam.current.eye);out<<",\"look_at\":";vec(out,cam.current.look_at);out<<'}';
            out<<",\"lock_target\":";if(auto id=session.locked_target_id())out<<*id;else out<<"null";
            out<<",\"buster_enabled\":"<<(l3_pl00_shot_is_enabled(&w)?"true":"false")<<",\"hp\":"<<session.hp()<<",\"projectiles\":[";
            bool comma=false;for(const auto& shot:session.projectiles()) {if(comma)out<<',';comma=true;out<<"{\"id\":"<<shot.serial<<",\"position\":";vec(out,shot.position);out<<",\"velocity\":";vec(out,shot.velocity);out<<",\"age\":"<<shot.age<<'}';}
            out<<"],\"event_state\":{\"dialogue_active\":"<<(session.dialogue().active?"true":"false")<<",\"script_wait\":"<<(session.dialogue().waiting_for_script?"true":"false")<<"}}";
            (void)l3::runtime_animation_finish(animation,track,session);
        }
        out<<"\n]\n";
        if(!out||!expected)throw std::runtime_error("empty replay or write failure");
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
