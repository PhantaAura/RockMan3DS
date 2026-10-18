#define _POSIX_C_SOURCE 200809L
#include "mml_native_save_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

uint32_t mml_native_crc32(const uint8_t *d,size_t n) {
    uint32_t c=0xffffffffu;if(!d&&n)return 0;
    for(size_t i=0;i<n;i++){c^=d[i];for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}
    return ~c;
}
static uint32_t get32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t*p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static int alias(const char*a,const char*b){
    struct stat x,y;
    return !strcmp(a,b)||(stat(a,&x)==0&&stat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino);
}
/* Unique sibling temp and rename: an interrupted write never truncates the active file. */
static MMLNativeSaveResult replace(const char *p,const uint8_t *d,size_t n,uint32_t gen){
    size_t len=strlen(p);char*t=malloc(len+12);if(!t)return MML_NSAVE_IO;
    memcpy(t,p,len);memcpy(t+len,".tmp.XXXXXX",12);
    int fd=mkstemp(t);if(fd<0){free(t);return MML_NSAVE_IO;}
    FILE*f=fdopen(fd,"wb");if(!f){close(fd);remove(t);free(t);return MML_NSAVE_IO;}
    uint8_t h[28]={0};put32(h,MML_NATIVE_SAVE_MAGIC);h[4]=MML_NATIVE_SAVE_VERSION;h[6]=28;
    put32(h+8,gen);put32(h+12,(uint32_t)n);put32(h+16,mml_native_crc32(d,n));
    int ok=fwrite(h,1,sizeof(h),f)==sizeof(h)&&fwrite(d,1,n,f)==n&&fflush(f)==0&&fsync(fd)==0;
    if(fclose(f)!=0)ok=0;
    if(ok&&rename(t,p)!=0)ok=0;
    if(!ok)remove(t);
    free(t);return ok?MML_NSAVE_OK:MML_NSAVE_IO;
}
MMLNativeSaveResult mml_native_save_read_validated(const char*p,uint8_t*out,size_t cap,size_t*sz,uint32_t*gen){
    if(!p||!out)return MML_NSAVE_ARGUMENT;
    FILE*f=fopen(p,"rb");if(!f)return MML_NSAVE_IO;
    uint8_t h[28];
    if(fread(h,1,sizeof(h),f)!=sizeof(h)||get32(h)!=MML_NATIVE_SAVE_MAGIC||h[4]!=MML_NATIVE_SAVE_VERSION||h[5]||h[6]!=28||h[7]||get32(h+20)||get32(h+24)){
        fclose(f);return MML_NSAVE_FORMAT;
    }
    size_t n=get32(h+12);
    if(n>MML_NATIVE_SAVE_MAX_PAYLOAD){fclose(f);return MML_NSAVE_FORMAT;}
    if(n>cap){fclose(f);return MML_NSAVE_CAPACITY;}
    uint8_t*tmp=malloc(n?n:1);if(!tmp){fclose(f);return MML_NSAVE_IO;}
    int ok=fread(tmp,1,n,f)==n&&fgetc(f)==EOF&&!ferror(f);fclose(f);
    MMLNativeSaveResult result=!ok?MML_NSAVE_FORMAT:mml_native_crc32(tmp,n)!=get32(h+16)?MML_NSAVE_CRC:MML_NSAVE_OK;
    if(result==MML_NSAVE_OK){memcpy(out,tmp,n);if(sz)*sz=n;if(gen)*gen=get32(h+8);}
    free(tmp);return result;
}
MMLNativeSaveResult mml_native_save_write_atomic(const char*p,const char*b,const uint8_t*d,size_t n,uint32_t gen){
    if(!p||!d||n>MML_NATIVE_SAVE_MAX_PAYLOAD||(b&&alias(p,b)))return MML_NSAVE_ARGUMENT;
    if(b){
        uint8_t*old=malloc(MML_NATIVE_SAVE_MAX_PAYLOAD);if(!old)return MML_NSAVE_IO;
        size_t size=0;uint32_t generation=0;
        MMLNativeSaveResult r=mml_native_save_read_validated(p,old,MML_NATIVE_SAVE_MAX_PAYLOAD,&size,&generation);
        /* A corrupt primary must never overwrite the last valid backup. */
        if(r==MML_NSAVE_OK)r=replace(b,old,size,generation);
        else r=MML_NSAVE_OK;
        free(old);if(r!=MML_NSAVE_OK)return r;
    }
    return replace(p,d,n,gen);
}
MMLNativeSaveResult mml_native_save_read_with_backup(const char*p,const char*b,uint8_t*out,size_t cap,size_t*sz,uint32_t*gen,bool*used){
    if(used)*used=false;
    MMLNativeSaveResult r=mml_native_save_read_validated(p,out,cap,sz,gen);
    if(r==MML_NSAVE_OK||!b)return r;
    r=mml_native_save_read_validated(b,out,cap,sz,gen);
    if(r==MML_NSAVE_OK&&used)*used=true;
    return r;
}
