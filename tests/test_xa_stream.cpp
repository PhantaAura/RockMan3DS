#include "legends3ds/xa_stream.h"
#include "legends3ds/xa_transport.h"
#include "legends3ds/l3map.h"
#include "legends3ds/runtime_session.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>
using namespace l3;
static void require(bool value){if(!value){std::fprintf(stderr,"always-on integration check failed\n");std::abort();}}
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,size_t) noexcept{std::free(p);}
void put(std::vector<uint8_t>& b,size_t p,uint64_t x,size_t n=4){for(size_t i=0;i<n;++i)b[p+i]=uint8_t(x>>(8*i));}
void fix_header(std::vector<uint8_t>& b){put(b,96,0);put(b,96,crc32(b.data(),256));}
std::vector<uint8_t> authored(){
 const size_t pcm=256+48;std::vector<uint8_t> b(pcm+2*kXaSectorPcmBytes,0);
 std::memcpy(b.data(),"L3XA001\0",8);put(b,8,256);put(b,12,1);put(b,16,1);put(b,20,17);put(b,24,900);put(b,28,916);
 put(b,32,37800);put(b,36,2);put(b,40,16);put(b,44,1);put(b,48,2);put(b,52,24);put(b,56,4032,8);put(b,64,256,8);put(b,72,pcm,8);put(b,80,2*kXaSectorPcmBytes,8);put(b,88,b.size(),8);put(b,112,1);put(b,116,3);put(b,120,8);
 const uint8_t exe[]={0x10,0xd2,0x00,0x8f,0x28,0x37,0xc4,0x66,0xb8,0xa3,0xc3,0xf5,0x6f,0xf9,0xb0,0x94,0x60,0x96,0xef,0x29,0xd7,0xf2,0x6f,0x09,0x01,0x0d,0x78,0xed,0x4b,0x25,0x68,0xf0};std::copy(exe,exe+32,b.begin()+224);
 for(size_t i=0;i<2;++i){const auto off=256+i*24;put(b,off,903+i*8);put(b,off+4,2016);put(b,off+8,i*2016,8);put(b,off+16,crc32(b.data()+pcm+i*kXaSectorPcmBytes,kXaSectorPcmBytes));}
 put(b,100,crc32(b.data()+256,48));put(b,104,crc32(b.data()+pcm,2*kXaSectorPcmBytes));fix_header(b);return b;
}
void write(const char* path,const std::vector<uint8_t>& b){auto* f=std::fopen(path,"wb");require(f);require(std::fwrite(b.data(),1,b.size(),f)==b.size());std::fclose(f);}
void runtime_bridge(const XaStreamReader& reader){
 RuntimeMap map;RuntimeMapArea area;area.start_x=63;area.start_z=63;area.width=2;area.height=2;area.cell_count=4;map.areas.push_back(area);map.cells.assign(4,int16_t(-32768));
 L3QolSettings settings{};l3_qol_settings_defaults(&settings);RuntimeSession session(std::move(map),TutorialRuntimeArea{}, {},settings);
 SourceEventPollProfile poll;poll.decoder=1;poll.records.push_back({1540,1,0});session.bind_source_event_profile(poll,0);
 SourceEventPhase2Package phase;phase.profile={0x20000003,17,{{0x03000000,16}},true};session.bind_source_event_phase2_package(phase);
 SourceEventCommandPackage command;command.profile={17,900,true};auto& c=command.cold_context;
 c.byte_800988ec=0;c.byte_8009896c=0;c.byte_800988c0=0;c.byte_80098b42=0;c.byte_80098ab8=0;c.half_800ad142=0;c.half_800ad14e=0;c.word_800989b0=0;c.word_800989d8=0;c.callback_80091ffc=0;c.callback_80092000=0;c.mix_known.fill(1);c.response_known.fill(1);
 session.bind_source_event_command_package(command);require(session.initialize_fresh_source_event_subset_st00());
 XaNativeCdState state;state.retained_known.fill(1);XaPlaybackObservation backend;backend.ready=true;bool started=false,popped=false;
 for(size_t frame=0;frame<40;++frame){
  if(session.source_event_command_result().pending_request){
   auto reply=service_native_xa_cd_request(state,reader,*session.source_event_command_result().pending_request,backend);
   if(reply.request_start){started=true;require(!reply.receipt&&session.source_event_work()->phase04()==3);}
   if(started&&frame>20){backend.playing=true;backend.ever_started=true;}
   if(reply.receipt){SourceEventCommandInputs in;in.receipt_count=1;in.receipts[0]=*reply.receipt;require(session.deliver_source_event_cd_receipts(in));}
  }
  session.tick({});
  if(frame<=20&&started)require(session.source_event_command_queue().count==1&&session.source_event_work()->phase04()==3);
  if(session.source_event_command_result().popped){popped=true;require(backend.playing&&session.source_event_command_queue().count==0);require(session.source_event_cd_mix_intents().count==127);require(!session.acknowledge_source_event_cd_mix(128));require(session.acknowledge_source_event_cd_mix(127));require(session.source_event_cd_mix_intents().count==0);break;}
 }
 require(started&&popped); // Authored observed backend state, not an NDSP/hardware claim.
}
int main(int argc,char** argv){
 if(argc==3&&std::strcmp(argv[1],"--owner")==0){XaStreamReader reader;require(reader.open(argv[2]));std::array<int16_t,kXaFramesPerSector*2> pcm{};uint32_t combined=0;for(size_t i=0;i<reader.index().size();++i){require(reader.read_sector(i,pcm.data(),kXaFramesPerSector));combined^=crc32(reinterpret_cast<const uint8_t*>(pcm.data()),kXaSectorPcmBytes);}std::printf("id=%u,sectors=%lu,frames=%llu,index_resident=%u,PCM_all_sector_CRC=PASS,xor=%08x\n",reader.info().id,(unsigned long)reader.index().size(),(unsigned long long)reader.info().total_frames,reader.resident_index_bytes(),combined);reader.close();require(!reader.ready()&&reader.resident_index_bytes()==0);return 0;}
 const char* path="test_xa_stream_authored.tmp";auto original=authored();write(path,original);XaStreamReader reader;require(reader.open(path));require(reader.info().id==17&&reader.index().size()==2);require(reader.sector_for_frame(0)==903&&reader.sector_for_frame(2016)==911&&reader.sector_for_frame(4032)==916);
 std::array<int16_t,kXaFramesPerSector*2> pcm{};require(!reader.read_sector(2,pcm.data(),2016));require(!reader.read_sector(0,pcm.data(),2015));const auto before=allocations;for(size_t i=0;i<4096;++i)require(reader.read_sector(i%2,pcm.data(),2016));require(allocations==before);
 runtime_bridge(reader);
 XaNativeCdState state;state.retained_known.fill(1);XaPlaybackObservation backend{};SourceEventCdRequest request;request.command=14;request.parameter_bytes=1;request.parameters[0]=200;require(service_native_xa_cd_request(state,reader,request,backend).status==XaTransportStatus::NeedsBackend);
 backend.ready=true;auto r=service_native_xa_cd_request(state,reader,request,backend);require(r.receipt&&r.receipt->response[0]==2&&state.mode==200);
 request.command=13;request.parameter_bytes=2;request.parameters={1,3,0,0};require(bool(service_native_xa_cd_request(state,reader,request,backend).receipt));
 request.command=2;request.parameter_bytes=3;const auto bcd=source_event_sector_to_bcd_80077e88(900);request.parameters={bcd[0],bcd[1],bcd[2],0};require(bool(service_native_xa_cd_request(state,reader,request,backend).receipt));
 request.command=27;request.parameter_bytes=0;request.parameters={};r=service_native_xa_cd_request(state,reader,request,backend);require(r.request_start&&!r.receipt);r=service_native_xa_cd_request(state,reader,request,backend);require(!r.request_start&&!r.receipt);backend.ever_started=true;require(!service_native_xa_cd_request(state,reader,request,backend).receipt);backend.playing=true;require(service_native_xa_cd_request(state,reader,request,backend).receipt->response[0]==34);
 request.command=17;backend.played_frames=2016;r=service_native_xa_cd_request(state,reader,request,backend);require(bool(r.receipt));for(size_t i=0;i<5;++i)require(!r.receipt->response_known[i]);require(source_event_bcd_to_sector_80077f8c({r.receipt->response[5],r.receipt->response[6],r.receipt->response[7]})==911);
 backend.playing=false;request.command=1;r=service_native_xa_cd_request(state,reader,request,backend);require(r.receipt->response[0]==2);backend.failed=true;require(!service_native_xa_cd_request(state,reader,request,backend).receipt);request.command=9;backend.failed=false;require(service_native_xa_cd_request(state,reader,request,backend).status==XaTransportStatus::UnsupportedRequest);
 // A pause request cannot manufacture completion before the native backend
 // actually reports its paused state. Keep the old generic-wrapper rejection.
 request.source_function=0x8001d254u;
 const auto saved_mode=state.mode;const auto saved_response=state.retained_response;
 r=service_native_xa_cd_request(state,reader,request,backend);
 require(r.status==XaTransportStatus::NeedsPause&&r.request_pause&&!r.receipt);
 require(state.mode==saved_mode&&state.retained_response==saved_response);
 backend.paused=true;backend.playing=true;
 r=service_native_xa_cd_request(state,reader,request,backend);
 require(r.receipt&&r.receipt->synchronous_callback_code==2&&!r.request_pause&&r.receipt->response[0]==2);
 request.command=17;request.source_function=0x8001d2bcu;
 require(!service_native_xa_cd_request(state,reader,request,backend).receipt);
 request.command=9;request.source_function=0x8001d254u;backend.failed=true;
 require(!service_native_xa_cd_request(state,reader,request,backend).receipt);
 reader.close();require(!reader.ready()&&reader.resident_index_bytes()==0);
 for(int which=0;which<12;++which){auto b=original;switch(which){case 0:b[0]='X';break;case 1:put(b,48,4097);break;case 2:put(b,36,1);break;case 3:put(b,72,303,8);break;case 4:b[224]=0;break;case 5:b[256]=0;break;case 6:b.back()=1;break;case 7:b.pop_back();break;case 8:b.push_back(0);break;case 9:put(b,280,903);put(b,100,crc32(b.data()+256,48));break;case 10:put(b,256,904);put(b,280,912);put(b,100,crc32(b.data()+256,48));break;case 11:put(b,28,924);break;}fix_header(b);write(path,b);require(!reader.open(path)&&!reader.ready()&&reader.resident_index_bytes()==0);}
 write(path,original);require(reader.open(path));auto corrupted=original;corrupted.back()=1;write(path,corrupted);require(!reader.read_sector(1,pcm.data(),2016));reader.close();require(std::remove(path)==0);
}
