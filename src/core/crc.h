/* Cliderness - core/crc.h : CRC-32 (IEEE) for save file verification */
#ifndef CLIDE_CORE_CRC_H
#define CLIDE_CORE_CRC_H

#include "core/types.h"

cl_u32 crc32_update(cl_u32 crc, const void *data, size_t n);
#define CRC32_INIT 0xFFFFFFFFUL
#define crc32_final(c) (CL_U32((c) ^ 0xFFFFFFFFUL))

#endif
