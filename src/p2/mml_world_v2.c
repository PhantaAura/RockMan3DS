
#include "mml_world_v2.h"
#include <string.h>
void mml_world_init(MMLWorld*w){if(w)memset(w,0,sizeof(*w));}
MMLWorldObject*mml_world_find(MMLWorld*w,uint32_t id){if(!w)return 0;for(size_t i=0;i<w->object_count;i++)if(w->objects[i].gameplay_id==id)return &w->objects[i];return 0;}
bool mml_world_spawn(MMLWorld*w,const MMLWorldObject*o){if(!w||!o||w->object_count>=MML_WORLD_MAX_OBJECTS||mml_world_find(w,o->gameplay_id))return false;w->objects[w->object_count++]=*o;return true;}
bool mml_world_despawn(MMLWorld*w,uint32_t id){if(!w)return false;for(size_t i=0;i<w->object_count;i++)if(w->objects[i].gameplay_id==id){if(i+1<w->object_count)memmove(&w->objects[i],&w->objects[i+1],(w->object_count-i-1)*sizeof(w->objects[0]));w->object_count--;return true;}return false;}
bool mml_world_set_flag(MMLWorld*w,uint16_t id,bool v){if(!w||id>=MML_WORLD_MAX_FLAGS)return false;w->flags[id]=(uint8_t)(v?1:0);return true;}bool mml_world_get_flag(const MMLWorld*w,uint16_t id){return w&&id<MML_WORLD_MAX_FLAGS&&w->flags[id]!=0;}
bool mml_world_queue_message(MMLWorld*w,uint32_t id){if(!w||w->messages.count>=MML_WORLD_MAX_MESSAGES)return false;uint8_t p=(uint8_t)((w->messages.head+w->messages.count)%MML_WORLD_MAX_MESSAGES);w->messages.ids[p]=id;w->messages.count++;return true;}bool mml_world_pop_message(MMLWorld*w,uint32_t*id){if(!w||!id||!w->messages.count)return false;*id=w->messages.ids[w->messages.head];w->messages.head=(uint8_t)((w->messages.head+1)%MML_WORLD_MAX_MESSAGES);w->messages.count--;return true;}
void mml_world_clear_transient(MMLWorld*w){if(!w)return;size_t d=0;for(size_t i=0;i<w->object_count;i++)if(w->objects[i].persistent)w->objects[d++]=w->objects[i];w->object_count=d;}
bool mml_world_request_transition(MMLWorld*w,MMLTransitionKind k,const uint8_t p[0x18]){if(!w||!p||w->transition_count>=MML_WORLD_MAX_TRANSITIONS)return false;MMLTransitionRequest*r=&w->transitions[w->transition_count++];r->kind=k;memcpy(r->raw_payload,p,0x18);r->pending=true;return true;}
