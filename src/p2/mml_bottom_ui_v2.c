
#include "mml_bottom_ui_v2.h"
void mml_bottom_ui_init(MMLBottomUI*u){if(!u)return;u->page=MML_UI_MAP;for(int i=0;i<MML_UI_PAGE_COUNT;i++)u->available[i]=true;u->dev_overlay=false;}static bool step(MMLBottomUI*u,int d){if(!u)return false;for(int i=0;i<MML_UI_PAGE_COUNT;i++){int n=((int)u->page+d+MML_UI_PAGE_COUNT)%MML_UI_PAGE_COUNT;u->page=(MMLBottomPage)n;if(u->available[n])return true;}return false;}bool mml_bottom_ui_next(MMLBottomUI*u){return step(u,1);}bool mml_bottom_ui_prev(MMLBottomUI*u){return step(u,-1);}
