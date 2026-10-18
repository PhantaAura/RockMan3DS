
#ifndef MML_CUTSCENE_V2_H
#define MML_CUTSCENE_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mml_combat_v2.h"
#define MML_CUT_MAX_ACTORS 16
#define MML_CUT_MAX_AUDIO 24
typedef enum {MML_CUT_END=0,MML_CUT_CAMERA,MML_CUT_ACTOR_XFORM,MML_CUT_ANIM,MML_CUT_EXPRESSION,MML_CUT_MESSAGE,MML_CUT_AUDIO,MML_CUT_FADE,MML_CUT_TRANSITION,MML_CUT_HANDOFF} MMLCutOp;
typedef struct {uint32_t id;int32_t x,y,z;uint16_t yaw12;uint8_t anim,frame,expression,eye,mouth;} MMLCutActor;
typedef struct {MMLCutOp op;MMLEvidence evidence;uint32_t a,b,c;int32_t x,y,z;uint8_t payload[0x18];} MMLCutCue;
typedef struct {const MMLCutCue*code;size_t count,pc;MMLCutActor actors[MML_CUT_MAX_ACTORS];size_t actor_count;bool classic,halted,gameplay_frozen,handoff_pending;uint32_t handoff_token;uint32_t message_ids[16],audio_ids[MML_CUT_MAX_AUDIO];size_t message_count,audio_count;uint8_t transition_payload[0x18];bool transition_pending;} MMLCutscene;
void mml_cutscene_init(MMLCutscene*s,const MMLCutCue*code,size_t count,bool classic);bool mml_cutscene_step(MMLCutscene*s);bool mml_cutscene_ack_handoff(MMLCutscene*s,uint32_t token);
#endif
