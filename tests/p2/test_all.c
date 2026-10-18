
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mml_renderer_recovery.h"
#include "mml_player_material.h"
#include "mml_package_v2.h"
#include "mml_retail_motion.h"
#include "mml_h4_runtime.h"
#include "mml_retail_anim_request.h"
#include "mml_combat_v2.h"
#include "mml_world_v2.h"
#include "mml_event_vm_v2.h"
#include "mml_enemy_bridge_v2.h"
#include "mml_cutscene_v2.h"
#include "mml_retail_save_v2.h"
#include "mml_native_save_v2.h"
#include "mml_runtime_diag_v2.h"
#include "mml_bottom_ui_v2.h"
#include "mml_loader_guard_v2.h"
#include "mml_qol_v2.h"
static void h1(void){MMLRendererGate g;mml_renderer_gate_init(&g);assert(g.true_0143_fallback);assert(mml_renderer_gate_set(&g,MML_RENDER_PLAYER_TEXTURES)&&g.textures_enabled&&!g.alpha_enabled);MMLTPageAddr t=mml_decode_tpage(0x0005);MMLClutAddr c=mml_decode_clut(0x3C08);assert(t.x_word==320&&t.y==0&&t.bpp==4);assert(c.x_word==128&&c.y==240);}
static void h2(void){unsigned char b[sizeof(MMLPkgHeader)+sizeof(MMLPkgSection)+8]={0};MMLPkgHeader*h=(MMLPkgHeader*)b;h->magic=MML_PKG_MAGIC;h->version=MML_PKG_VERSION;h->section_count=1;h->total_size=sizeof(b);MMLPkgSection*s=(MMLPkgSection*)(b+sizeof(*h));s->offset=sizeof(MMLPkgHeader)+sizeof(MMLPkgSection);s->size=8;s->crc32=mml_native_crc32(b+s->offset,s->size);h->manifest_crc=mml_native_crc32(b+sizeof(*h),sizeof(*s));assert(mml_package_v2_validate(b,sizeof(b)));}
static void h3(void){MMLTurnState s={0xFF0,-8,99};mml_turn_update(&s,1,4,72);assert(s.turn_velocity==0&&s.turn_increment==99);int32_t x,y,z;mml_heading0_project(80,0,0,&x,&y,&z);assert(x==0&&y==0&&z==-80);}
static void h4(void){assert(mml_projection_heading12(0x10,0x20)==0xFF0);MMLTargetCandidate c[2]={{2,0x10,20,true,true},{1,0x10,20,true,true}};MMLLockOnState s={0};assert(mml_lockon_acquire(&s,0,0x40,100,c,2)&&s.target_id==1);}
static void h5(void){MMLRetailAnimState a={3,1,0};assert(mml_retail_anim_request(&a,12,false)&&a.request_ac==12&&a.sequence_a4==0);MMLCombatRuntime r;mml_combat_init(&r);MMLBusterConfig c={2,3,4,5,MML_EVIDENCE_UNKNOWN,MML_EVIDENCE_UNKNOWN,MML_EVIDENCE_UNKNOWN,MML_EVIDENCE_UNKNOWN};assert(!mml_combat_fire(&r,&c,true,1,0,0,0,0,1,0,0));}
static MMLWorldObject obj(uint32_t id,uint32_t geom,MMLObjectKind k){MMLWorldObject o={0};o.gameplay_id=id;o.geometry_id=geom;o.kind=k;o.active=true;o.targetable=k==MML_OBJ_ENEMY;return o;}
static void h6(void){MMLWorld w;mml_world_init(&w);MMLWorldObject a=obj(1,9,MML_OBJ_SCENERY),b=obj(2,9,MML_OBJ_ENEMY);assert(mml_world_spawn(&w,&a)&&mml_world_spawn(&w,&b));MMLEventInstr e[2]={{MML_EVT_SET_FLAG,3,0,{0}},{MML_EVT_END,0,0,{0}}};MMLEventVM v;mml_event_vm_init(&v,e,2);assert(mml_event_vm_run(&v,&w,4)&&mml_world_get_flag(&w,3));}
static void h7(void){MMLCutCue q[2]={{MML_CUT_HANDOFF,MML_EVIDENCE_SOURCE_BACKED,77,0,0,0,0,0,{0}},{MML_CUT_END,MML_EVIDENCE_SOURCE_BACKED,0,0,0,0,0,0,{0}}};MMLCutscene s;mml_cutscene_init(&s,q,2,true);assert(mml_cutscene_step(&s)&&s.handoff_pending);assert(!mml_cutscene_ack_handoff(&s,1));assert(mml_cutscene_ack_handoff(&s,77)&&!s.gameplay_frozen);}
static void h8(void){uint8_t b[MML_RETAIL_TRANSFER_SIZE];for(size_t i=0;i<sizeof(b);i++)b[i]=(uint8_t)(i*13+7);uint32_t seed[6]={1,2,3,4,5,6};assert(mml_retail_save_generate_checks(b,sizeof(b),seed));assert(mml_retail_save_verify(b,sizeof(b))==MML_RETAIL_SAVE_OK);b[0x400]^=1;assert(mml_retail_save_verify(b,sizeof(b))==MML_RETAIL_SAVE_BLOCK_E);}
static void h9(void){MMLRuntimeDiag d;mml_diag_init(&d,16667,256*1024);mml_diag_frame(&d,12000,0);assert(mml_diag_budget_ok(&d));MMLLoaderGuard g;mml_loader_begin(&g,10,1);assert(mml_loader_advance(&g,MML_LOAD_READ));assert(mml_loader_add_bytes(&g,10));assert(mml_loader_advance(&g,MML_LOAD_DECODE));assert(mml_loader_decoded(&g));assert(mml_loader_advance(&g,MML_LOAD_VALIDATE));assert(mml_loader_advance(&g,MML_LOAD_COMMIT));assert(mml_loader_commit(&g));MMLQolConfig q;mml_qol_classic_defaults(&q);assert(mml_qol_classic_semantics_ok(&q));}
int main(void){h1();h2();h3();h4();h5();h6();h7();h8();h9();puts("PASS: cumulative H1-H9 C runtime regression");return 0;}
