// Owner-local map-edge audit (0.0.15.0 M2). Drives the real RuntimeSession on a
// cooked tutorial pack with deterministic pseudo-random walk/turn/jump input
// and reports map escapes, falls below the area and untranslated-response
// frames. Uses the same st00.l3init materialization as the 3DS runtime.
//   l3edge_audit TUTORIAL_DIR AREA X Y Z [SEEDS] [FRAMES]
// Output contains only counts and coordinates; no retail payload.
#include "legends3ds/runtime_session.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_animation.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/map_initialization.h"
#include <cstdio>
#include <filesystem>
#include <random>
#include <set>
#include <string>
using namespace l3;
int main(int argc,char** argv){
    try{
        if(argc<6){std::fprintf(stderr,"usage: l3edge_audit TUTORIAL_DIR AREA X Y Z [SEEDS] [FRAMES]\n");return 2;}
        const std::filesystem::path root=argv[1];const size_t ai=std::stoul(argv[2]);
        const RuntimeVec3i spawn{std::stoi(argv[3]),std::stoi(argv[4]),std::stoi(argv[5])};
        const int seeds=argc>6?std::stoi(argv[6]):100,frames=argc>7?std::stoi(argv[7]):3000;
        const auto mb=read_file(root/"st00.l3map"),rb=read_file(root/"st00.l3render");
        auto map=load_l3map(mb,true);const auto render=load_l3render(rb,true);
        const auto init=load_map_initialization(read_file(root/"st00.l3init"),true);
        const auto applied=apply_map_initialization(map,render,init,crc32(mb.data(),mb.size()),crc32(rb.data(),rb.size()));
        const auto anim=load_l3animation(read_file(root/"player/main.l3anim"),true);
        const auto refs=load_l3reference(read_file(root/"reference.l3ref"),true);
        std::vector<MmlMessageFile> banks;for(const char* n:{"st00_01.l3msg","st00_02.l3msg"})banks.push_back(load_l3message(read_file(root/"messages"/n),true));
        char ebd[64];std::snprintf(ebd,sizeof(ebd),"ebd/area_%02lu.l3ebd",(unsigned long)ai);
        L3QolSettings q{};l3_qol_settings_defaults(&q);
        static const uint16_t inputs[]={0x10,0x10|0x80,0x10|0x20,0x80,0x20,0x40,0x10|0x4000,0x10};
        int escapes=0,falls=0;long unsupported=0,nodata=0,deep=0;std::set<std::pair<int,int>> cells;
        for(int sd=0;sd<seeds;++sd){
            std::mt19937 rng{static_cast<uint32_t>(sd)};
            auto area=load_tutorial_runtime_area(read_file(root/"st00.l3flow"),read_file(root/ebd),ai,true);
            RuntimeSession s(map,area,banks,q);s.set_reference_tables(refs);
            s.reset_player(spawn,uint16_t(rng()%4096),100,100);PbdMainBodyTrack track{};
            uint16_t mask=0x10;int hold=0;bool escaped=false,fell=false;
            for(int f=0;f<frames;++f){
                if(hold--<=0){mask=inputs[rng()%8];hold=int(rng()%90);}
                RuntimeInputFrame in{};in.held_psx_mask=mask;in.pressed_psx_mask=(f%37==0)?0x4000:0;
                (void)runtime_animation_prepare(anim,track,s);s.tick(in);(void)runtime_animation_finish(anim,track,s);
                const auto p=s.player_position();const auto& c=s.player_contact();
                if(c.status==RuntimeContactStatus::Unsupported)++unsupported;
                if(c.status==RuntimeContactStatus::NoData)++nodata;
                deep+=long(c.deep_overlap_recoveries);
                cells.insert({p.x>>9,p.z>>9});
                if(!s.probe_player_map().in_area&&!escaped){escaped=true;++escapes;std::printf("ESCAPE seed=%d frame=%d pos=%d,%d,%d\n",sd,f,p.x,p.y,p.z);}
                if(p.y>600&&!fell){fell=true;++falls;std::printf("FALL seed=%d frame=%d pos=%d,%d,%d\n",sd,f,p.x,p.y,p.z);}
            }
        }
        std::printf("{\"area\":%lu,\"spawn\":[%d,%d,%d],\"map_init_writes\":%lu,\"seeds\":%d,\"frames\":%d,\"escapes\":%d,\"falls\":%d,\"unsupported_frames\":%ld,\"nodata_frames\":%ld,\"deep_overlap_recoveries\":%ld,\"visited_cells\":%lu}\n",
            (unsigned long)ai,spawn.x,spawn.y,spawn.z,(unsigned long)applied.write_count,seeds,frames,escapes,falls,unsupported,nodata,deep,(unsigned long)cells.size());
        return escapes||falls?1:0;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 2;}
}
