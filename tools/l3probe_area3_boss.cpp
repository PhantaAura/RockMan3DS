#include "legends3ds/actor_runtime.h"
#include "legends3ds/tutorial_runtime.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<uint8_t> read(const char* path){
    std::ifstream stream(path,std::ios::binary);
    if(!stream)throw std::runtime_error("cannot open owner-local cooked input");
    return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
}
int main(int argc,char** argv){
    if(argc!=4){std::cerr<<"usage: l3probe_area3_boss flow.l3flow area.l3ebd area_index\n";return 2;}
    try{
        const auto source=l3::load_tutorial_runtime_area(read(argv[1]),read(argv[2]),std::stoul(argv[3]));
        l3::ActorAreaRuntime actors;
        const auto result=l3::append_source_area3_boss_801076e8(actors,source.stage,source.ebd,3);
        std::cout<<"{\"schema\":\"source-area3-boss-probe/1\",\"status\":"<<unsigned(result.status)
            <<",\"actors\":"<<actors.actors.size();
        if(result.spec)std::cout<<",\"event_source_index\":"<<result.spec->event_source_index
            <<",\"callback\":\"0x801076e8\",\"initializer\":\"0x801078b8\"";
        if(result.actor_index<actors.actors.size()){
            const auto& boss=actors.actors[result.actor_index];
            std::cout<<",\"row\":"<<boss.row<<",\"parts\":"<<boss.resource->model.parts.size()
                <<",\"pose_request\":"<<unsigned(boss.pose->request)
                <<",\"pose_record\":"<<unsigned(boss.pose->record_index)
                <<",\"hp\":"<<(boss.source_area3_boss?boss.source_area3_boss->hp:0)
                <<",\"action\":"<<unsigned(boss.source_area3_boss?boss.source_area3_boss->action:0)
                <<",\"phase\":"<<unsigned(boss.source_area3_boss?boss.source_area3_boss->phase:0)
                <<",\"substate\":"<<unsigned(boss.source_area3_boss?boss.source_area3_boss->substate:0)
                <<",\"spawn_z\":"<<boss.source_origin[2]
                <<",\"decoded_bytes\":"<<actors.decoded_resource_bytes;
        }
        std::cout<<"}\n";return result.status==l3::SourceArea3BossStatus::SourceBackedPresentation?0:1;
    }catch(const std::exception& error){std::cerr<<"area3 boss probe failed: "<<error.what()<<'\n';return 1;}
}
