
#ifndef MML_LOADER_GUARD_V2_H
#define MML_LOADER_GUARD_V2_H
#include <stdbool.h>
#include <stdint.h>
typedef enum{MML_LOAD_IDLE=0,MML_LOAD_PREPARE,MML_LOAD_READ,MML_LOAD_DECODE,MML_LOAD_VALIDATE,MML_LOAD_COMMIT,MML_LOAD_DONE,MML_LOAD_FAILED}MMLLoadPhase;
typedef struct{MMLLoadPhase phase;uint32_t expected_bytes,read_bytes,expected_sections,decoded_sections,error;bool committed;}MMLLoaderGuard;
void mml_loader_begin(MMLLoaderGuard*g,uint32_t bytes,uint32_t sections);bool mml_loader_advance(MMLLoaderGuard*g,MMLLoadPhase next);bool mml_loader_add_bytes(MMLLoaderGuard*g,uint32_t n);bool mml_loader_decoded(MMLLoaderGuard*g);bool mml_loader_commit(MMLLoaderGuard*g);void mml_loader_fail(MMLLoaderGuard*g,uint32_t error);
#endif
