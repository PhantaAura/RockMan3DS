
#include "mml_retail_anim_request.h"
bool mml_retail_anim_request(MMLRetailAnimState*s,uint8_t r,bool force){
 if(!s) return false;
 if(!force&&s->request_ac==r) return false;
 uint8_t old=s->request_ac;s->previous_ad=old;
 if((old==1&&r==12)||(old==12&&r==1))s->sequence_a4=0;
 else if(force||old!=r)s->sequence_a4=0;
 s->request_ac=r;return true;
}
