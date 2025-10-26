#include <exec/types.h>
#include <proto/exec.h>

APTR emu68_CachePreDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"))
{
    ULONG addr = (ULONG)vaddress;
    ULONG len = *length;
    len = (len + 31) & ~31;

    while(addr < (ULONG)vaddress + len) {
        asm volatile("cpushl dc, (%0)"::"a"(addr));
        addr += 32;
    }

    return vaddress;
}
