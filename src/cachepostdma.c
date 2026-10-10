#include <exec/types.h>
#include <proto/exec.h>

#include "base.h"

/* Post-DMA: the device wrote RAM, so drop our copies and let the CPU re-read.
 * DMA contract, opcode constants and the alignment rules: see base.h.
 *
 * The range invalidate discards whole lines, which is what we want everywhere
 * except at the two ends: those may be shared with memory that has nothing to
 * do with this transfer, and discarding a line the neighbour has dirtied
 * throws its writes away.  So clean+invalidate exactly those two lines first
 * and only then invalidate the range.  Cleaning writes the neighbour's half
 * back to RAM; it also writes our stale half over the DMA'd bytes in that one
 * line, which is the trade a real 68040 CPUSHL has always made — the loss
 * stays inside the buffer whose owner chose not to align it. */
void emu68_CachePostDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"))
{
    if (flags & (DMA_ReadFromRAM | DMA_NoModify))
        return;

    ULONG len = (ULONG)*length;

    if (len == 0)
        return;

    /* A length of 1 maintains exactly the line holding that byte whatever the
     * line size is, so neither end needs to know it.  The second op syncs:
     * its DSB is what orders both cleans ahead of the invalidate below —
     * without it the ivac could discard the dirty line before the civac has
     * written it back, which is the whole thing we are avoiding. */
    EMU68_RANGE_OP(EMU68_OP_CPUSH, EMU68_EXT_D1L | EMU68_EXT_NOSYNC, vaddress, 1);
    EMU68_RANGE_OP(EMU68_OP_CPUSH, EMU68_EXT_D1L, (ULONG)vaddress + len - 1, 1);

    /* Both end lines are invalid by now, so their ivac below is a no-op. */
    if (flags & DMAF_NoSync)
        EMU68_RANGE_OP(EMU68_OP_CINV, EMU68_EXT_D1L | EMU68_EXT_NOSYNC, vaddress, len);
    else
        EMU68_RANGE_OP(EMU68_OP_CINV, EMU68_EXT_D1L, vaddress, len);
}
