#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "mml_package_v2.h"
#include "mml_renderer_recovery.h"
#include "mml_runtime_diag_v2.h"
#include "mml_native_save_v2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void put(uint8_t*p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static void table_crc(uint8_t*p){put(p+16,mml_native_crc32(p+20,32));}
int main(void){
    MMLRuntimeDiag diag;mml_diag_init(&diag,0,262144);diag.heap_budget=1024;mml_diag_memory(&diag,1025,0,0);assert(!mml_diag_budget_ok(&diag));
    mml_diag_memory(&diag,100,0,0);assert(mml_diag_budget_ok(&diag));mml_diag_phase(&diag,MML_PHASE_UPDATE,77);mml_diag_phase(&diag,MML_PHASE_UPDATE,10);assert(diag.phase_peak_us[MML_PHASE_UPDATE]==77);
    MMLRendererGate gate;mml_renderer_gate_init(&gate);
    assert(mml_renderer_gate_mask(&gate,MML_RF_ANIMATION)&&gate.true_0143_fallback&&gate.animation_enabled);
    assert(!mml_renderer_gate_mask(&gate,MML_RF_TERRAIN)&&gate.feature_mask==MML_RF_ANIMATION);
    assert(mml_renderer_gate_mask(&gate,MML_RF_UV|MML_RF_PLAYER)&&!gate.textures_enabled);
    assert(!mml_renderer_gate_set(&gate,(MMLRendererStage)-1));
    uint8_t backing[62]={0};uint8_t*p=backing+1; /* deliberately unaligned */
    put(p,MML_PKG_MAGIC);p[4]=2;p[6]=2;put(p+8,60);
    put(p+20,1);put(p+24,52);put(p+28,4);put(p+32,mml_native_crc32(p+52,4));
    put(p+36,2);put(p+40,56);put(p+44,4);put(p+48,mml_native_crc32(p+56,4));table_crc(p);
    assert(mml_package_v2_validate(p,60));
    const uint8_t*section=NULL;size_t bytes=0;
    assert(mml_package_v2_section(p,60,2,&section,&bytes)&&section==p+56&&bytes==4);
    p[59]=1;assert(!mml_package_v2_validate(p,60));p[59]=0;
    put(p+40,54);table_crc(p);assert(!mml_package_v2_validate(p,60));
    put(p+40,UINT32_MAX);table_crc(p);assert(!mml_package_v2_validate(p,60));
    put(p+40,56);put(p+36,1);table_crc(p);assert(!mml_package_v2_validate(p,60));
    assert(!mml_package_v2_validate(p,19));assert(!mml_package_v2_validate(NULL,60));
    char dir[]="/tmp/mml-save-test-XXXXXX";assert(mkdtemp(dir));char a[128],b[128];
    snprintf(a,sizeof(a),"%s/save",dir);snprintf(b,sizeof(b),"%s/backup",dir);
    uint8_t one[]={1,2,3},two[]={4,5,6},out[3]={9,9,9};size_t n=0;uint32_t gen=0;bool backup=false;
    assert(mml_native_save_write_atomic(a,b,one,3,1)==MML_NSAVE_OK);
    assert(mml_native_save_write_atomic(a,b,two,3,2)==MML_NSAVE_OK);
    FILE*f=fopen(a,"wb");assert(f);fputs("broken",f);fclose(f);
    assert(mml_native_save_read_validated(a,out,3,&n,&gen)==MML_NSAVE_FORMAT&&out[0]==9);
    assert(mml_native_save_read_with_backup(a,b,out,3,&n,&gen,&backup)==MML_NSAVE_OK&&backup&&gen==1&&out[0]==1);
    assert(mml_native_save_write_atomic(a,b,two,3,3)==MML_NSAVE_OK);
    assert(mml_native_save_read_validated(b,out,3,&n,&gen)==MML_NSAVE_OK&&gen==1);
    assert(mml_native_save_write_atomic(a,a,one,3,4)==MML_NSAVE_ARGUMENT);
    f=fopen(a,"ab");assert(f);fputc(0,f);fclose(f);
    assert(mml_native_save_read_validated(a,out,3,&n,&gen)==MML_NSAVE_FORMAT);
    unlink(a);unlink(b);rmdir(dir);puts("PASS: malformed packages and save recovery");return 0;
}
