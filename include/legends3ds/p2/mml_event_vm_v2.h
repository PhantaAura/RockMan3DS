
#ifndef MML_EVENT_VM_V2_H
#define MML_EVENT_VM_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mml_world_v2.h"
typedef enum {MML_EVT_END=0,MML_EVT_SET_FLAG,MML_EVT_CLEAR_FLAG,MML_EVT_WAIT_FLAG,MML_EVT_MESSAGE,MML_EVT_OBJECT_STATE,MML_EVT_OBJECT_ACTIVE,MML_EVT_TRANSITION_AREA,MML_EVT_TRANSITION_STAGE} MMLEventOp;
typedef struct {MMLEventOp op;uint32_t a,b;uint8_t payload[0x18];} MMLEventInstr;
typedef struct {const MMLEventInstr*code;size_t count,pc;bool halted,blocked;uint32_t steps;} MMLEventVM;
void mml_event_vm_init(MMLEventVM*v,const MMLEventInstr*c,size_t n);bool mml_event_vm_step(MMLEventVM*v,MMLWorld*w);bool mml_event_vm_run(MMLEventVM*v,MMLWorld*w,uint32_t max_steps);
#endif
