#include <exec/types.h>
#include <proto/exec.h>

#include "base.h"

/* Pre-DMA: get every dirty line out of the way before the device runs.
 * DMA contract, opcode constants and the alignment rules: see base.h.
 *
 * DMA_ReadFromRAM means the device reads what we wrote, so the lines only
 * need writing back and may stay valid: clean only (NOINVAL).  Otherwise the
 * device writes RAM and we will read it back, so drop our copies as well.
 *
 * No end-line special case is needed here, unlike CachePostDMA: RAM holds no
 * device data yet, so cleaning a line shared with a neighbour writes both
 * halves back correctly and loses nothing. */
APTR emu68_CachePreDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"))
{
    ULONG len = (ULONG)*length;

    switch (flags & (DMA_ReadFromRAM | DMAF_NoSync))
    {
    case DMA_ReadFromRAM | DMAF_NoSync:
        EMU68_RANGE_OP(EMU68_OP_CPUSH,
                       EMU68_EXT_D1L | EMU68_EXT_NOINVAL | EMU68_EXT_NOSYNC,
                       vaddress, len);
        break;
    case DMA_ReadFromRAM:
        EMU68_RANGE_OP(EMU68_OP_CPUSH, EMU68_EXT_D1L | EMU68_EXT_NOINVAL,
                       vaddress, len);
        break;
    case DMAF_NoSync:
        EMU68_RANGE_OP(EMU68_OP_CPUSH, EMU68_EXT_D1L | EMU68_EXT_NOSYNC,
                       vaddress, len);
        break;
    default:
        EMU68_RANGE_OP(EMU68_OP_CPUSH, EMU68_EXT_D1L, vaddress, len);
        break;
    }

    /* Emu68 maps m68k memory 1:1, so the physical address is the one we were
     * given and the whole range is contiguous — *length needs no update and
     * callers never have to come back with DMA_Continue. */
    return vaddress;
}
