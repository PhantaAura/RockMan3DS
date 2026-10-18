
#include "mml_qol_v2.h"
void mml_qol_classic_defaults(MMLQolConfig*q){if(!q)return;q->classic_mode=true;q->circle_pad_enhanced=false;q->faster_text=false;q->skip_viewed_cutscenes=false;q->safe_save_backups=true;}bool mml_qol_classic_semantics_ok(const MMLQolConfig*q){if(!q)return false;if(!q->classic_mode)return true;return !q->circle_pad_enhanced&&!q->faster_text&&!q->skip_viewed_cutscenes;}
