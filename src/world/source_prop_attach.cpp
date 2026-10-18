#include "legends3ds/source_prop_attach.h"
#include "legends3ds/source_event_pipeline.h"
#include <algorithm>
namespace l3 {
void invalidate_source_prop_foreign_globals(SourcePropForeignGlobals& state) noexcept {
    state.loaded_=false;state.queue_bound_=false;state.generation_=0;++state.revision_;
}
void initialize_source_prop_foreign_globals_from_executable_load(SourcePropForeignGlobals& state,
    uint64_t generation,bool fresh) noexcept {
    invalidate_source_prop_foreign_globals(state);
    if(!fresh||generation==0)return;
    state.loaded_=true;state.queue_bound_=true;state.generation_=generation;state.pending_=0;
}
SourcePropForeignReleasePrepared prepare_source_prop_foreign_release(SourcePropForeignGlobals& state,
    uint32_t identity,uint64_t generation,const SourceEventPhase5Pool<32,80>& roster) noexcept {
    SourcePropForeignReleasePrepared out;
    if(!state.loaded_)return out;
    if(!generation||generation!=state.generation_){out.status=SourcePropForeignGlobalsStatus::StaleSourceGeneration;return out;}
    // Unknown/type1 occupied records may execute the uncapped80112648 writer.
    // Revocation survives later disappearance; only a genuine fresh EXE load
    // may restore the invariant. It is not renewed at every area/frame.
    if(!roster.used[0]){state.queue_bound_=false;++state.revision_;out.status=SourcePropForeignGlobalsStatus::NeedsRoster;return out;}
    for(size_t i=0;i<32;++i)if(*roster.used[0]&(0x80000000u>>i)){
        const auto& r=roster.slots[i];
        if(!r.known[1]||r.raw[1]!=uint8_t(0xe0u|i)||!r.known[2]){
            state.queue_bound_=false;++state.revision_;out.status=SourcePropForeignGlobalsStatus::NeedsRoster;return out;
        }
        if(r.raw[2]==1){state.queue_bound_=false;++state.revision_;out.status=SourcePropForeignGlobalsStatus::UncheckedQueueProducerPresent;return out;}
    }
    if(!state.queue_bound_){out.status=SourcePropForeignGlobalsStatus::UncheckedQueueProducerPresent;return out;}
    if(identity<0x800bf118u||(identity-0x800bf118u)%80u||(identity-0x800bf118u)/80u>=32u){out.status=SourcePropForeignGlobalsStatus::UnknownForeignAddress;return out;}
    if(!(*roster.used[0]&(0x80000000u>>((identity-0x800bf118u)/80u)))){out.status=SourcePropForeignGlobalsStatus::NeedsRoster;return out;}
    const uint32_t delta=identity-0x800a3b40u,high=uint32_t((uint64_t(delta)*0x38e38e39u)>>32),offset=(high>>9)<<2;
    out.owner.prop_identity=identity;out.owner.pending_address=0x800988c8u+offset;out.owner.used_address=0x80098838u+offset;
    if(out.owner.pending_address!=0x8009898cu||out.owner.used_address!=0x800988fcu){out.status=SourcePropForeignGlobalsStatus::UnknownForeignAddress;return out;}
    out.owner.pending_word=state.pending_;out.owner.used_bits_value=0;
    out.owner.used_bits_known_mask=SourcePropForeignGlobals::counter_known_zero_mask;
    const uint32_t bit=0x80000000u>>((high>>4)&31u),combined=state.pending_|bit;
    if((~combined|out.owner.used_bits_known_mask)!=0xffffffffu){out.status=SourcePropForeignGlobalsStatus::NeedsKnownResult;return out;}
    out.owner.admitted=true;out.service=&state;out.source_generation=state.generation_;
    out.revision=state.revision_;out.pending_before=state.pending_;out.status=SourcePropForeignGlobalsStatus::Ready;return out;
}
bool commit_source_prop_foreign_release(SourcePropForeignGlobals& state,
    const SourcePropForeignReleasePrepared& receipt,bool complete) noexcept {
    if(!complete||receipt.status!=SourcePropForeignGlobalsStatus::Ready||receipt.service!=&state||
       !state.loaded_||!state.queue_bound_||receipt.source_generation!=state.generation_||
       receipt.revision!=state.revision_||receipt.pending_before!=state.pending_||!receipt.owner.pending_word||
       !receipt.owner.admitted||receipt.owner.pending_address!=0x8009898cu||receipt.owner.used_address!=0x800988fcu||
       receipt.owner.used_word||receipt.owner.used_bits_value||receipt.owner.used_bits_known_mask!=SourcePropForeignGlobals::counter_known_zero_mask||
       *receipt.owner.pending_word!=0)return false;
    state.pending_=*receipt.owner.pending_word;++state.revision_;return true;
}
namespace {
using Prop=SourceEventPhase5Bytes<80>;using Actor=SourceEventPhase5Bytes<1444>;
using Matrix=std::array<int16_t,9>;
uint16_t u16(const uint8_t* p) noexcept {return uint16_t(p[0]|(uint16_t(p[1])<<8));}
uint32_t u32(const uint8_t* p) noexcept {return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);}
int16_t s16(uint32_t v) noexcept {v&=65535u;return v<32768u?int16_t(v):int16_t(int32_t(v)-65536);}
int32_t s32(uint32_t v) noexcept {return v<0x80000000u?int32_t(v):int32_t(int64_t(v)-0x100000000ll);}
int32_t asr12(uint32_t v) noexcept {return s32((v>>12)|((v&0x80000000u)?0xfff00000u:0u));}
int32_t q12(int64_t v) noexcept {return int32_t(v>=0?v/4096:-((-v+4095)/4096));}
int16_t ir(int64_t v) noexcept {return int16_t(std::max<int32_t>(-32768,std::min<int32_t>(32767,q12(v))));}
uint32_t mul(int32_t a,int32_t b) noexcept {return uint32_t(a)*uint32_t(b);}
template<size_t N>bool known(const SourceEventPhase5Bytes<N>& w,size_t off,size_t count) noexcept {for(size_t i=0;i<count;++i)if(!w.known[off+i])return false;return true;}
void put(Prop& p,size_t off,size_t count,uint32_t v) noexcept {for(size_t i=0;i<count;++i){p.raw[off+i]=uint8_t(v>>(i*8));p.known[off+i]=1;}}
Matrix quarter_x(const ReferenceTables& refs) noexcept {
    // Same source8007B5A8 arithmetic as class40's integer pose helper, with
    // original authored angles1024/0/0. Products wrap32 before each ASR12.
    const int32_t sx=refs.trig[1024].sin_q12,cx=refs.trig[1024].cos_q12,
        sy=refs.trig[0].sin_q12,cy=refs.trig[0].cos_q12,sz=sy,cz=cy;
    Matrix m{};m[2]=s16(uint32_t(sy));m[5]=s16(uint32_t(asr12(0u-mul(cy,sx))));m[8]=s16(uint32_t(asr12(mul(cy,cx))));
    m[0]=s16(uint32_t(asr12(mul(cz,cy))));m[1]=s16(uint32_t(asr12(0u-mul(sz,cy))));
    auto t=asr12(mul(cz,-sy));m[3]=s16(uint32_t(asr12(mul(sz,cx)))-uint32_t(asr12(mul(t,sx))));m[6]=s16(uint32_t(asr12(mul(sz,sx)))+uint32_t(asr12(mul(t,cx))));
    t=asr12(mul(sz,-sy));m[4]=s16(uint32_t(asr12(mul(cz,cx)))+uint32_t(asr12(mul(t,sx))));m[7]=s16(uint32_t(asr12(mul(cz,sx)))-uint32_t(asr12(mul(t,cx))));return m;
}
Matrix compose(const Matrix& a,const Matrix& b) noexcept {Matrix out{};for(size_t row=0;row<3;++row)for(size_t col=0;col<3;++col){int64_t sum=0;for(size_t k=0;k<3;++k)sum+=int64_t(a[3*row+k])*b[3*k+col];out[3*row+col]=ir(sum);}return out;}
void scaled_matrix(Prop& p,const Matrix& matrix) noexcept {for(size_t i=0;i<9;++i){const int32_t v=asr12(mul(matrix[i],32768));put(p,24+i*2,i==8?4:2,uint32_t(v));}}
bool actor_need(const Actor& actor,size_t off,size_t count,SourcePropAttachResult& out) noexcept {for(size_t i=0;i<count;++i)if(!actor.known[off+i]){out.status=SourcePropAttachStatus::NeedsActorFields;out.missing_actor_offset=uint16_t(off+i);return false;}return true;}
}
SourcePropAttachResult update_source_prop_attach_80112d14(Prop& prop,uint32_t identity,const SourcePropAttachInputs& in) noexcept {
    SourcePropAttachResult out;
    if(identity<0x800bf118u || (identity-0x800bf118u)%80u || (identity-0x800bf118u)/80u>=32){out.status=SourcePropAttachStatus::InvalidIdentity;return out;}
    for(const auto& range:{std::pair<size_t,size_t>{0,4},{6,1},{9,1}})for(size_t i=0;i<range.second;++i)if(!prop.known[range.first+i]){out.status=SourcePropAttachStatus::NeedsPropFields;out.missing_prop_offset=uint16_t(range.first+i);return out;}
    const uint32_t slot=(identity-0x800bf118u)/80u;
    if(prop.raw[1]!=uint8_t(0xe0u|slot) || prop.raw[2]!=3 || prop.raw[3]!=0){out.status=SourcePropAttachStatus::UnsupportedProp;return out;}
    if(!(prop.raw[0]&prop.raw[6]&1u)){out.status=SourcePropAttachStatus::Inactive;return out;}
    const uint8_t state=prop.raw[9];if(state<1 || state>3){out.status=SourcePropAttachStatus::UnknownState;return out;}
    out.source_function=state==1?0x80112d14u:state==2?0x80112e80u:0x80112f70u;
    if(!in.references){out.status=SourcePropAttachStatus::NeedsReferences;return out;}
    if(!in.event_work || !std::all_of(in.event_work->known.begin()+0x24,in.event_work->known.begin()+0x28,[](uint8_t v){return v!=0;})){out.status=SourcePropAttachStatus::NeedsEventOwner;return out;}
    const uint32_t token=u32(in.event_work->raw.data()+0x24);
    if(!in.actors || token<0x800c3580u || (token-0x800c3580u)%1444u || (token-0x800c3580u)/1444u>=4 || !in.actors->used[0]){out.status=SourcePropAttachStatus::NeedsActorOwner;return out;}
    const size_t actor_slot=(token-0x800c3580u)/1444u;const auto& actor=in.actors->slots[actor_slot];
    if(!(*in.actors->used[0]&(0x80000000u>>actor_slot)) || !known(actor,1,2) || actor.raw[1]!=uint8_t(0x40u|actor_slot) || actor.raw[2]!=5){out.status=SourcePropAttachStatus::NeedsActorOwner;return out;}
    if(state<3 && !actor_need(actor,672,18,out))return out;
    if(state==1 && !actor_need(actor,408,6,out))return out;
    if(state==2 && (!actor_need(actor,416,6,out)||!actor_need(actor,440,6,out)))return out;
    if(state==3 && !actor_need(actor,384,6,out))return out;
    bool release=false;uint32_t pending_after=0;
    if(state==1){
        if(!prop.known[12]){out.status=SourcePropAttachStatus::NeedsPropFields;out.missing_prop_offset=12;return out;}
        release=prop.raw[12]!=0;
        if(release){
            const uint32_t delta=identity-0x800a3b40u,high=uint32_t((uint64_t(delta)*0x38e38e39u)>>32),offset=(high>>9)<<2;
            const uint32_t pending_address=0x800988c8u+offset,used_address=0x80098838u+offset,bit=0x80000000u>>((high>>4)&31u);
            if(!in.release_owner || !in.release_owner->admitted || !in.release_owner->pending_word){out.status=SourcePropAttachStatus::NeedsForeignReleaseOwner;return out;}
            const auto& owner=*in.release_owner;
            if(owner.prop_identity!=identity || owner.pending_address!=pending_address || owner.used_address!=used_address){out.status=SourcePropAttachStatus::ForeignReleaseMismatch;return out;}
            const uint32_t combined=*owner.pending_word|bit;
            if(owner.used_word)pending_after=combined&*owner.used_word;
            else {
                // A zero input bit fixes an AND result even when the counter's
                // corresponding bit is unknown. Require every output bitknown.
                if((owner.used_bits_value&~owner.used_bits_known_mask)||
                    (~combined|owner.used_bits_known_mask)!=0xffffffffu){out.status=SourcePropAttachStatus::NeedsForeignReleaseOwner;return out;}
                pending_after=combined&owner.used_bits_value;
            }
        }
    }
    auto next=prop;const auto quarter=quarter_x(*in.references);Matrix matrix{};
    if(state<3){Matrix bone{};for(size_t i=0;i<9;++i)bone[i]=s16(u16(actor.raw.data()+672+2*i));matrix=compose(bone,quarter);scaled_matrix(next,matrix);}
    if(state==1){
        const std::array<int16_t,3> local{{80,0,-320}};
        for(size_t row=0;row<3;++row){int64_t sum=0;for(size_t k=0;k<3;++k)sum+=int64_t(matrix[row*3+k])*local[k];put(next,56+2*row,2,uint32_t(uint16_t(ir(sum)))+u16(actor.raw.data()+408+2*row));}
        for(size_t i=44;i<56;++i)next.known[i]=0;
        out.invalidated_opaque_lanes=12;
    }else if(state==2){
        for(size_t i=0;i<3;++i){const int32_t sum=int32_t(s16(u16(actor.raw.data()+416+2*i)))+s16(u16(actor.raw.data()+440+2*i));put(next,56+2*i,2,uint32_t(sum/2));}
    }else{
        for(size_t i=24;i<56;++i)put(next,i,1,0);
        for(size_t i=0;i<9;++i)put(next,24+i*2,2,uint16_t(quarter[i]));
        for(size_t i=0;i<3;++i)put(next,56+2*i,2,uint32_t(u16(actor.raw.data()+384+2*i))-(i==0?20u:0u));
    }
    put(next,64,4,0x02808080u);
    if(release){put(next,0,1,0);in.release_owner->pending_word=pending_after;out.released=true;}
    prop=next;out.committed=true;return out;
}
} // namespace l3
