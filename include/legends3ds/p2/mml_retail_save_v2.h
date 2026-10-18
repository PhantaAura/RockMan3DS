
#ifndef MML_RETAIL_SAVE_V2_H
#define MML_RETAIL_SAVE_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MML_RETAIL_TRANSFER_SIZE 0x1F00u
typedef enum {MML_RETAIL_SAVE_OK=0,MML_RETAIL_SAVE_BAD_ARGUMENT,MML_RETAIL_SAVE_BAD_SIZE,MML_RETAIL_SAVE_BLOCK_A,MML_RETAIL_SAVE_BLOCK_B,MML_RETAIL_SAVE_BLOCK_C,MML_RETAIL_SAVE_BLOCK_D,MML_RETAIL_SAVE_BLOCK_E,MML_RETAIL_SAVE_BLOCK_F,MML_RETAIL_SAVE_MASTER} MMLRetailSaveResult;
MMLRetailSaveResult mml_retail_save_verify(const uint8_t*buf,size_t size);bool mml_retail_save_generate_checks(uint8_t*buf,size_t size,const uint32_t seeds[6]);
#endif
