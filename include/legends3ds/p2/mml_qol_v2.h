
#ifndef MML_QOL_V2_H
#define MML_QOL_V2_H
#include <stdbool.h>
typedef struct{bool classic_mode,circle_pad_enhanced,faster_text,skip_viewed_cutscenes,safe_save_backups;}MMLQolConfig;
void mml_qol_classic_defaults(MMLQolConfig*q);bool mml_qol_classic_semantics_ok(const MMLQolConfig*q);
#endif
