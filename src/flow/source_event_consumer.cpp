#include "legends3ds/source_event_consumer.h"
#include "legends3ds/source_event_pipeline.h"

namespace l3 {
namespace {
uint16_t get16(const uint8_t* p) noexcept {return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
void clear_voice(SourceEventVoiceWork& voice) noexcept {
    for(const size_t offset: {size_t(0),size_t(1),size_t(4),size_t(5),size_t(0x1d)}){
        voice.raw[offset]=0;voice.known[offset]=1;
    }
}
bool driver_call(SourceEventDriverContext& driver,SourceEventDriverIntentQueue& queue,
    int16_t voice) noexcept {
    if(!driver.lock_800bc050)return false;
    SourceEventDriverIntent record;record.voice=voice;
    if(*driver.lock_800bc050==1){
        // The source rejects this call and the original player ignores -1.
        // Its actual native receipt is still required for transaction commit.
        record.source_return=-1;
    }else{
        for(unsigned i=0;i<2;++i)
            if(!driver.pending_800c5680[i]||!driver.active_8009ae40[i])return false;
        driver.lock_800bc050=1;
        driver.voice_800c0bfa=static_cast<uint16_t>(voice);
        clear_voice(driver.voices[static_cast<size_t>(voice)]);
        const unsigned half=voice<16?0:1,bit=static_cast<unsigned>(voice)&15;
        driver.pending_800c5680[half]=static_cast<uint16_t>(*driver.pending_800c5680[half]|(1u<<bit));
        for(unsigned i=0;i<2;++i){
            driver.active_8009ae40[i]=static_cast<uint16_t>(*driver.active_8009ae40[i]&~*driver.pending_800c5680[i]);
            record.pending[i]=*driver.pending_800c5680[i];record.active[i]=*driver.active_8009ae40[i];
        }
        driver.lock_800bc050=0;
    }
    queue.records[queue.count++]=record;
    return true;
}
}
void initialize_source_event_fresh_player_header_8003be6c(L3PlayerCompatWork& player,
    SourceEventPlayerKnown& known) noexcept {
    player.bytes[0]=3;player.bytes[6]=3;known[0]=1;known[6]=1;
    for(size_t i=2;i<6;++i){player.bytes[i]=0;known[i]=1;}
    for(size_t i=8;i<16;++i){player.bytes[i]=0;known[i]=1;}
}
void initialize_source_event_player_fields_8003bee8(L3PlayerCompatWork& player,
    SourceEventPlayerKnown& known) noexcept {
    for(const size_t offset:{size_t(0xbd),size_t(0x74),size_t(0x75),size_t(0x76),size_t(0x77),
        size_t(0x184),size_t(0x185),size_t(0x186),size_t(0x187),size_t(0x110),size_t(0x111),size_t(0x449)}){
        player.bytes[offset]=0;known[offset]=1;
    }
}
void initialize_source_event_driver_fields(SourceEventDriverContext& driver,
    SourceEventDriverIntentQueue& queue) noexcept {
    driver.lock_800bc050=0;
    for(unsigned i=0;i<2;++i){driver.pending_800c5680[i]=0;driver.active_8009ae40[i]=0;}
    // This queue is a native receipt owner; reset retains slot bytes.
    queue.count=0;queue.initialized=true;
}
int16_t initialize_source_event_busy_from_executable_boot() noexcept {return 0;}

SourceEventPlayerConsumerResult consume_source_event1_phase1_80042238(
    SourceEventWork& work,L3PlayerCompatWork& player,SourceEventPlayerKnown& known,
    SourceEventDriverContext& driver,SourceEventDriverIntentQueue& queue,
    std::optional<uint8_t> byte_800c1b93) noexcept {
    SourceEventPlayerConsumerResult out;
    if(!work.known[0]||!work.known[1]||!work.known[4])return out;
    if(!work.raw[0]||work.raw[1]!=1||work.raw[4]!=1){out.status=SourceEventPlayerConsumerStatus::WrongEventPhase;return out;}
    const auto need=[&](size_t offset) noexcept {
        if(known[offset])return false;
        out.status=SourceEventPlayerConsumerStatus::NeedsPlayerField;
        out.missing_player_offset=static_cast<uint16_t>(offset);return true;
    };
    if(need(0xbd)||need(0))return out;
    auto next_player=player;auto next_known=known;
    next_player.bytes[0x449]=255;next_known[0x449]=1;
    next_player.bytes[0xbd]|=0x40;next_known[0xbd]=1;
    bool ready=(player.bytes[0]&1)==0;
    if(!ready){
        if(need(8))return out;
        const auto state=player.bytes[8];
        ready=state==3;
        if(state==1){
            if(need(9))return out;
            bool test_busy=player.bytes[9]==0;
            if(player.bytes[9]==14){
                if(!byte_800c1b93){out.status=SourceEventPlayerConsumerStatus::NeedsSourceByte800c1b93;return out;}
                test_busy=*byte_800c1b93!=0;
            }
            if(test_busy){
                if(need(0x174))return out;
                bool busy=(player.bytes[0x174]&0xc0)!=0;
                if(!busy){if(need(0x177))return out;busy=player.bytes[0x177]!=0;}
                ready=!busy;
            }
            if(!ready){
                // Source AND mask00FFFF00 reads bytes09/0A, not retained0B.
                if(need(0xa))return out;
                if(get16(player.bytes+9)==0x020d){next_player.bytes[0xa]=6;next_known[0xa]=1;}
            }
        }
    }
    if(!ready){
        player=next_player;known=next_known;
        out.status=SourceEventPlayerConsumerStatus::NotReady;out.committed=true;return out;
    }
    if(!queue.initialized){out.status=SourceEventPlayerConsumerStatus::NeedsIntentQueue;return out;}
    if(queue.count>queue.records.size()){out.status=SourceEventPlayerConsumerStatus::MalformedIntentQueue;return out;}
    if(queue.records.size()-queue.count<2){out.status=SourceEventPlayerConsumerStatus::IntentQueueFull;return out;}
    auto next_driver=driver;auto next_queue=queue;
    if(!driver_call(next_driver,next_queue,22)||!driver_call(next_driver,next_queue,23)){
        out.status=SourceEventPlayerConsumerStatus::NeedsDriverField;return out;
    }
    next_player.bytes[0xb4]=0;next_player.bytes[0xb5]=0;
    next_known[0xb4]=1;next_known[0xb5]=1;
    next_player.bytes[0]&=0xfc;next_known[0]=1;
    player=next_player;known=next_known;driver=next_driver;queue=next_queue;
    work.raw[4]=2;work.known[4]=1;
    out.status=SourceEventPlayerConsumerStatus::Consumed;out.committed=true;
    out.player_ready=true;out.receipt_count=2;return out;
}
} // namespace l3
