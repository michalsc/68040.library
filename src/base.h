#ifndef _BASE_H
#define _BASE_H

#include <exec/types.h>
#include <exec/libraries.h>

struct Emu040Base {
    struct Library  emu_Lib;
    APTR            emu_CachePreDMA;
    APTR            emu_CachePostDMA;

    APTR            emu_CachePreDMA_old;
    APTR            emu_CachePostDMA_old;
};

#define MANUFACTURER_ID     0x6d73
#define PRODUCT_ID          0x01
#define SERIAL_NUMBER       0x4c32 
#define LIB_VERSION         1
#define LIB_REVISION        3
#define LIB_PRIORITY        119

#define LIB_POSSIZE         (sizeof(struct Emu040Base))
#define LIB_NEGSIZE         (4*6)

APTR emu68_CachePreDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"));
void emu68_CachePostDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"));

#endif /* _BASE_H */
