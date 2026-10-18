
#ifndef MML_WORLD_V2_H
#define MML_WORLD_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MML_WORLD_MAX_OBJECTS 96
#define MML_WORLD_MAX_FLAGS 256
#define MML_WORLD_MAX_MESSAGES 16
#define MML_WORLD_MAX_TRANSITIONS 4
typedef enum {MML_OBJ_SCENERY=0,MML_OBJ_INTERACTIVE,MML_OBJ_TRIGGER,MML_OBJ_ENEMY,MML_OBJ_NPC,MML_OBJ_TRANSITION} MMLObjectKind;
typedef struct {uint32_t gameplay_id,geometry_id;MMLObjectKind kind;bool active,persistent,targetable;uint16_t state;int32_t x,y,z;uint16_t yaw12;} MMLWorldObject;
typedef struct {uint32_t ids[MML_WORLD_MAX_MESSAGES];uint8_t head,count;} MMLMessageQueue;
typedef enum {MML_TRANSITION_AREA_ONLY=0,MML_TRANSITION_FULL_STAGE=1} MMLTransitionKind;
typedef struct {MMLTransitionKind kind;uint8_t raw_payload[0x18];bool pending;} MMLTransitionRequest;
typedef struct {MMLWorldObject objects[MML_WORLD_MAX_OBJECTS];size_t object_count;uint8_t flags[MML_WORLD_MAX_FLAGS];MMLMessageQueue messages;MMLTransitionRequest transitions[MML_WORLD_MAX_TRANSITIONS];size_t transition_count;} MMLWorld;
void mml_world_init(MMLWorld*w);MMLWorldObject*mml_world_find(MMLWorld*w,uint32_t id);bool mml_world_spawn(MMLWorld*w,const MMLWorldObject*o);bool mml_world_despawn(MMLWorld*w,uint32_t id);bool mml_world_set_flag(MMLWorld*w,uint16_t id,bool v);bool mml_world_get_flag(const MMLWorld*w,uint16_t id);bool mml_world_queue_message(MMLWorld*w,uint32_t id);bool mml_world_pop_message(MMLWorld*w,uint32_t*id);void mml_world_clear_transient(MMLWorld*w);bool mml_world_request_transition(MMLWorld*w,MMLTransitionKind kind,const uint8_t payload[0x18]);
#endif
