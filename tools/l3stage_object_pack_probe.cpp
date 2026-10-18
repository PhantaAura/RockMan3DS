#include "legends3ds/stage_object_catalog.h"
#include "legends3ds/stage_runtime.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace fs=std::filesystem;
static std::vector<uint8_t> rf(const fs::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open "+p.string());return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
int main(int argc,char**argv){try{if(argc!=2){std::cerr<<"usage: l3stage_object_pack_probe OBJECT_DIR\n";return 2;}size_t files=0,tables=0,lists=0,objects=0,allocatable=0;for(const auto&de:fs::recursive_directory_iterator(argv[1])){if(!de.is_regular_file()||de.path().extension()!=".l3ot")continue;auto c=l3::load_l3stage_object_catalog(rf(de.path()),true);++files;tables+=c.tables.size();for(const auto&t:c.tables)for(const auto&l:t.lists){++lists;objects+=l.objects.records.size();auto os=l3::instantiate_stage_runtime_objects(l.objects);auto as=l3::assign_fresh_stage_pool_slots(os);for(const auto&a:as)if(a.pool_slot)++allocatable;}}std::cout<<"files="<<files<<" tables="<<tables<<" lists="<<lists<<" objects="<<objects<<" pool_allocatable="<<allocatable<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"error: "<<e.what()<<"\n";return 1;}}
