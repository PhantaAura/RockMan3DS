#include "legends3ds/source_profile_class60_06.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/l3map.h"
#include <cassert>
#include <stdexcept>
int main(){
 l3::Class60_06Profile p;for(size_t i=0;i<6;++i){p.normal_uv[i]={{uint16_t(256+i),uint16_t(65535-i)}};p.alternate_uv[i]={{uint16_t(i*37),uint16_t(i+512)}};}
 const auto b=l3::encode_source_profile_class60_06(p);const auto q=l3::load_source_profile_class60_06(b.data(),b.size());
 assert(p.normal_uv==q.normal_uv&&p.alternate_uv==q.alternate_uv&&b==l3::encode_source_profile_class60_06(q));
 for(size_t i=0;i<b.size();++i){auto bad=b;bad[i]^=1;bool rejected=false;try{(void)l3::load_source_profile_class60_06(bad.data(),bad.size());}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
 for(size_t n=0;n<b.size();++n){bool rejected=false;try{(void)l3::load_source_profile_class60_06(b.data(),n);}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
 auto extra=b;extra.push_back(0);bool rejected=false;try{(void)l3::load_source_profile_class60_06(extra.data(),extra.size());}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 p.source_stage=1;rejected=false;try{(void)l3::encode_source_profile_class60_06(p);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
}
