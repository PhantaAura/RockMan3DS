
#ifndef MML_NATIVE_SAVE_V2_H
#define MML_NATIVE_SAVE_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MML_NATIVE_SAVE_MAGIC 0x3256534Du
#define MML_NATIVE_SAVE_VERSION 2u
#define MML_NATIVE_SAVE_MAX_PAYLOAD (64u*1024u)
typedef struct{uint32_t magic;uint16_t version,header_size;uint32_t generation,payload_size,payload_crc32,flags,reserved;}MMLNativeSaveHeader;
typedef enum{MML_NSAVE_OK=0,MML_NSAVE_ARGUMENT,MML_NSAVE_IO,MML_NSAVE_FORMAT,MML_NSAVE_CRC,MML_NSAVE_CAPACITY}MMLNativeSaveResult;
uint32_t mml_native_crc32(const uint8_t*d,size_t n);MMLNativeSaveResult mml_native_save_write_atomic(const char*p,const char*b,const uint8_t*d,size_t n,uint32_t gen);MMLNativeSaveResult mml_native_save_read_validated(const char*p,uint8_t*out,size_t cap,size_t*sz,uint32_t*gen);MMLNativeSaveResult mml_native_save_read_with_backup(const char*p,const char*b,uint8_t*out,size_t cap,size_t*sz,uint32_t*gen,bool*used);
#endif
