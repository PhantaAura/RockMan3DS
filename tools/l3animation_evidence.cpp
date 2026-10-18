#include "legends3ds/pbd_animation.h"
#include "legends3ds/st00_source.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
    try{
        if(argc!=2)return 2;
        const auto animation=l3::load_l3animation(l3::read_file(argv[1]),true);
        std::cout<<"{\"format\":\"animation-evidence/1\",\"request_mutator\":\"0x80041ddc\",\"mappings\":[";
        bool comma=false;
        for(unsigned request=0;request<256;++request){
            const auto*s=l3::find_pbd_animation_action(animation,static_cast<uint16_t>(request));
            const auto*m=l3::pbd_main_body_pose_action_for_request(animation,static_cast<uint8_t>(request));
            if(!s||!s->has_s_records||s->s_records.empty()||!m)continue;
            bool valid=true;
            for(const auto&r:s->s_records){
                if((r.word0&255u)>=m->angle_frames.size())valid=false;
                const auto ctl=static_cast<int8_t>(r.word1>>8);
                if(ctl<-1&&(static_cast<uint8_t>(ctl)&127u)>=s->s_records.size())valid=false;
            }
            if(comma)std::cout<<',';comma=true;
            std::cout<<"{\"request_id\":"<<request<<",\"s_action\":"<<s->action_index<<",\"m_action\":"<<m->action_index
                     <<",\"record_count\":"<<s->s_records.size()<<",\"m_frame_count\":"<<m->angle_frames.size()
                     <<",\"structural_valid\":"<<(valid?"true":"false")
                     <<",\"status\":\"SOURCE_BACKED_PARTIAL\",\"semantic_status\":\"UNKNOWN\",\"confidence\":\"structural mapping only\"}";
            if(!valid)throw std::runtime_error("invalid animation mapping");
        }
        std::cout<<"]}\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
