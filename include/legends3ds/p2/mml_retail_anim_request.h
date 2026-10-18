
#ifndef MML_RETAIL_ANIM_REQUEST_H
#define MML_RETAIL_ANIM_REQUEST_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {uint8_t sequence_a4,request_ac,previous_ad;} MMLRetailAnimState;
bool mml_retail_anim_request(MMLRetailAnimState *s,uint8_t request,bool force);
#endif
