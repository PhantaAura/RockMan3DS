
#ifndef MML_PACKAGE_V2_H
#define MML_PACKAGE_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MML_PKG_MAGIC 0x324C4D4Du
#define MML_PKG_VERSION 2u
#define MML_PKG_MAX_SECTIONS 64u
#define MML_PKG_HEADER_BYTES 20u
#define MML_PKG_SECTION_BYTES 16u
/* manifest_crc covers the section table; crc32 covers each section.
 * source_profile_crc is an identifier, not an authentication guarantee.
 * Kinds: 1=metadata JSON, 2=L3MP, 3=render, 4=L3OT, 0x100+=EBD/MSG.
 * Distinct kinds required; unknown kinds may be skipped by readers. */
typedef struct {uint32_t kind,offset,size,crc32;} MMLPkgSection;
typedef struct {uint32_t magic;uint16_t version,section_count;uint32_t total_size,source_profile_crc,manifest_crc;} MMLPkgHeader;
bool mml_package_v2_validate(const uint8_t *data,size_t size);
bool mml_package_v2_section(const uint8_t *data,size_t size,uint32_t kind,
                            const uint8_t **payload,size_t *payload_size);
#endif
