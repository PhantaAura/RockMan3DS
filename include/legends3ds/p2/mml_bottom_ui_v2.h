
#ifndef MML_BOTTOM_UI_V2_H
#define MML_BOTTOM_UI_V2_H
#include <stdbool.h>
#include <stdint.h>
typedef enum{MML_UI_MAP=0,MML_UI_WEAPONS,MML_UI_ITEMS,MML_UI_STATUS,MML_UI_PAGE_COUNT}MMLBottomPage;
typedef struct{MMLBottomPage page;bool available[MML_UI_PAGE_COUNT];bool dev_overlay;}MMLBottomUI;
void mml_bottom_ui_init(MMLBottomUI*u);bool mml_bottom_ui_next(MMLBottomUI*u);bool mml_bottom_ui_prev(MMLBottomUI*u);
#endif
