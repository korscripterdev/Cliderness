/* Cliderness - core/crc.c (nibble table: 16 entries, small for DOS) */
#include "core/crc.h"

static const cl_u32 g_tab[16] = {
    0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL,
    0x76DC4190UL, 0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL,
    0xEDB88320UL, 0xF00F9344UL, 0xD6D6A3E8UL, 0xCB61B38CUL,
    0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL, 0xBDBDF21CUL
};

cl_u32 crc32_update(cl_u32 crc, const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    crc = CL_U32(crc);
    while (n--) {
        crc ^= (cl_u32)*p++;
        crc = (crc >> 4) ^ g_tab[crc & 15];
        crc = (crc >> 4) ^ g_tab[crc & 15];
    }
    return CL_U32(crc);
}
