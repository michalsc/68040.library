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
#define LIB_VERSION         2
#define LIB_REVISION        0
#define LIB_PRIORITY        119

/* ------------------------------------------------------- the DMA contract ---
 *
 * These two vectors replace exec's CachePreDMA/CachePostDMA .
 *
 * Direction.  DMA_ReadFromRAM (the device reads RAM, we wrote it) needs the
 * dirty lines written back but they may stay valid: clean only.  Otherwise
 * the device writes RAM and we will read it: clean+invalidate before arming,
 * invalidate afterwards so the CPU re-reads what the device left.
 *
 * The pre-arm op is MANDATORY. A dirty line covering a device-written buffer
 * can be evicted at ANY moment during the transfer — ordinary cache pressure
 * is enough — and its stale contents then land on top of the payload.
 * Nothing done after the transfer can undo that, so the buffer must carry no
 * dirty line into it.
 *
 * Alignment.  A device-write buffer should be cache-line aligned and span
 * whole lines.  These vectors clean the two end lines before invalidating, so
 * an unaligned buffer cannot damage whatever shares its boundary lines — the
 * same bargain a real 68040 CPUSHL makes.  What it can still lose is the
 * DMA'd bytes inside such a line, and a neighbour dirtying that line DURING
 * the transfer cannot be defended against at all.
 *
 * DMAF_NoSync (private; the NDK uses bits 1-3 for Continue/NoModify/
 * ReadFromRAM) suppresses the range op's trailing DSB so a batch pays ONE
 * barrier: issue N calls with NoSync, then close with a non-NoSync call or an
 * m68k NOP, which Emu68 translates to a bare dsb sy.  A pre-NoSync Emu68
 * ignores the bit at translation time and still emits the per-op barrier, so
 * setting it is always safe.
 *
 * SHARP EDGE: NoSync on an INVALIDATE is only legal if a dsb executes before
 * any CPU read of the region.
 * SHARP EDGE: a ZERO-LENGTH op emits nothing whatsoever — the handler skips
 * the barrier too — so it can never be used to close a NoSync batch. */
#define DMAF_NoSync         (1L<<4)

/* Emu68 range data-cache op (LINE-F, reserved cache SCOPE=00 — illegal on a
 * real 68040), maintaining [A0, A0+D1) in one pass.
 *
 *   Operation word  1111 0100 CC P 00 nnn :  CC = 01 data cache,
 *       P = CINV(0) invalidate / CPUSH(1) clean+invalidate, nnn = base reg An
 *   Extension word  0 ddd s N S 000000000  :  ddd = length reg Dn,
 *       s = word(0)/long(1) size, N = no-invalidate (clean only, with CPUSH),
 *       S = no-sync (suppress the trailing DSB, see DMAF_NoSync above)
 *
 * We always pass the base in A0 and a long length in D1, hence the fixed
 * nnn=000 / ddd=001 / s=1 baked into the constants below. */

/* Operation word: which maintenance, data cache, base register A0. */
#define EMU68_OP_CINV       0xF440  /* invalidate           (dc ivac)  */
#define EMU68_OP_CPUSH      0xF460  /* clean+invalidate     (dc civac) */

/* Extension word: start from the length register and add the flag bits. */
#define EMU68_EXT_D1L       0x1800  /* length in D1, long size         */
#define EMU68_EXT_NOINVAL   0x0400  /* N: clean only, line stays valid */
#define EMU68_EXT_NOSYNC    0x0200  /* S: suppress the trailing DSB    */

/* The two words are substituted with %c (print the constant bare), NOT
 * stringified.  Do not "simplify" this to `.short " #extw "` — the m68k
 * assembler treats `|` as a LINE COMMENT, so a stringified
 * `EMU68_EXT_D1L | EMU68_EXT_NOSYNC` assembles as plain 0x1800 with the flag
 * bits silently discarded.  %c makes the compiler fold the expression first. */
#define EMU68_RANGE_OP(opw, extw, addr, len)                            \
    do {                                                                \
        register ULONG _a0 asm("a0") = (ULONG)(addr);                   \
        register ULONG _d1 asm("d1") = (ULONG)(len);                    \
        asm volatile(".short %c0\n\t.short %c1"                         \
                     : : "i"(opw), "i"(extw), "a"(_a0), "d"(_d1)        \
                     : "memory");                                       \
    } while (0)

#define LIB_POSSIZE         (sizeof(struct Emu040Base))
#define LIB_NEGSIZE         (4*6)

APTR emu68_CachePreDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"));
void emu68_CachePostDMA(APTR vaddress asm("a0"), LONG *length asm("a1"), ULONG flags asm("d0"), struct ExecBase *SysBase asm("a6"));

#endif /* _BASE_H */
