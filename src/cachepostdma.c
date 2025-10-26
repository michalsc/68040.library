#include <exec/types.h>
#include <proto/exec.h>

void emu68_CachePostDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"))
{
    ULONG addr = (ULONG)vaddress;
    ULONG len = *length;
    len = (len + 31) & ~31;

    /* Go through the cached block, line by line invalidating all of them one by one */

    while(addr < (ULONG)vaddress + len) {
        asm volatile("cinvl dc, (%0)"::"a"(addr));
        addr += 32;
    }
}
