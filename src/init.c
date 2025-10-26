/*
    Copyright © 2021 Michal Schulz <michal.schulz@gmx.de>
    https://github.com/michalsc

    This Source Code Form is subject to the terms of the
    Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/devicetree.h>

#include <libraries/configregs.h>
#include <libraries/configvars.h>

#include "base.h"

UBYTE rom_end;

ULONG L_Expunge(struct Library * LibraryBase asm("a6"))
{
    /* This is rom based device. no expunge here */
    if (LibraryBase->lib_OpenCnt > 0)
    {
        LibraryBase->lib_Flags |= LIBF_DELEXP;
    }

    return 0;
}

APTR L_ExtFunc(struct Library * LibraryBase asm("a6"))
{
    return LibraryBase;
}

struct Library * L_Open(ULONG version asm("d0"), struct Library * LibraryBase asm("a6"))
{
    LibraryBase->lib_OpenCnt++;
    LibraryBase->lib_Flags &= ~LIBF_DELEXP;

    return LibraryBase;
}

ULONG L_Close(struct Library * LibraryBase asm("a6"))
{
    LibraryBase->lib_OpenCnt--;
    
    return 0;
}

extern const char deviceName[];
extern const char deviceIdString[];

static void putch(UBYTE data asm("d0"), APTR ignore asm("a3"))
{
    *(UBYTE*)0xdeadbeef = data;
}



CONST_STRPTR FindToken(CONST_STRPTR string, CONST_STRPTR token)
{
    CONST_STRPTR ret = NULL;

    if (string)
    {
        do {
            while (*string == ' ' || *string == '\t') {
                string++;
            }

            if (*string == 0)
                break;

            for (int i=0; token[i] != 0; i++)
            {
                if (string[i] != token[i])
                {
                    break;
                }

                if (token[i] == '=') {
                    ret = string;
                    break;
                }

                if (string[i+1] == 0 || string[i+1] == ' ' || string[i+1] == '\t') {
                    ret = string;
                    break;
                }
            }

            if (ret)
                break;

            while(*string != 0) {
                if (*string != ' ' && *string != '\t')
                    string++;
                else break;
            }

        } while(!ret && *string != 0);
    }
    return ret;
}

APTR Init(struct ExecBase *SysBase asm("a6"))
{
    struct Emu040Base *LibraryBase = NULL;
    struct ExpansionBase *ExpansionBase = NULL;
    struct CurrentBinding binding;
    APTR DeviceTreeBase;

    APTR base_pointer = NULL;
    
    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    GetCurrentBinding(&binding, sizeof(binding));

    base_pointer = AllocMem(LIB_NEGSIZE + LIB_POSSIZE, MEMF_PUBLIC | MEMF_CLEAR);

    if (base_pointer)
    {
        ULONG relFuncTable[5];
        
        relFuncTable[0] = (ULONG)L_Open;
        relFuncTable[1] = (ULONG)L_Close;
        relFuncTable[2] = (ULONG)L_Expunge;
        relFuncTable[3] = (ULONG)L_ExtFunc;
        relFuncTable[4] =  -1;

        LibraryBase = (struct Emu040Base *)((UBYTE *)base_pointer + LIB_NEGSIZE);
        MakeFunctions(LibraryBase, relFuncTable, 0);
        
        LibraryBase->emu_Lib.lib_Node.ln_Type = NT_LIBRARY;
        LibraryBase->emu_Lib.lib_Node.ln_Pri = LIB_PRIORITY;
        LibraryBase->emu_Lib.lib_Node.ln_Name = (STRPTR)deviceName;

        LibraryBase->emu_Lib.lib_NegSize = LIB_NEGSIZE;
        LibraryBase->emu_Lib.lib_PosSize = LIB_POSSIZE;
        LibraryBase->emu_Lib.lib_Version = LIB_VERSION;
        LibraryBase->emu_Lib.lib_Revision = LIB_REVISION;
        LibraryBase->emu_Lib.lib_IdString = (STRPTR)deviceIdString;    

        SumLibrary((struct Library *)LibraryBase);
        AddLibrary((struct Library *)LibraryBase);

        LibraryBase->emu_CachePreDMA = emu68_CachePreDMA;
        LibraryBase->emu_CachePostDMA = emu68_CachePostDMA;

        {
            UWORD new_flags = SysBase->AttnFlags & 0x8000;
            int has_fpu = (SysBase->AttnFlags & (AFF_FPU40 | AFF_68881 | AFF_68882)) != 0;

            new_flags |= AFF_68010 | AFF_68020 | AFF_68030 | AFF_68040;

            if (has_fpu) {
                new_flags |= AFF_FPU40 | AFF_68881 | AFF_68882;
            }

            ULONG args[] = {
                SysBase->AttnFlags,
                new_flags
            };

            RawDoFmt("[68040] Found AttnFlags = %08lx. Setting to to %08lx\n", args, (APTR)putch, NULL);

            SysBase->AttnFlags = new_flags;
        }

        DeviceTreeBase = OpenResource("devicetree.resource");

        if (DeviceTreeBase)
        {
            const char *cmdline = DT_GetPropValue(DT_FindProperty(DT_OpenKey("/chosen"), "bootargs"));

            if (FindToken(cmdline, "vbr_move"))
            {
                RawDoFmt("[68040] VBR move requested\n", NULL, (APTR)putch, NULL);

                APTR vbr_old;
                APTR old_stack = SuperState();

                asm volatile("movec vbr, %0":"=r"(vbr_old));

                if (vbr_old == NULL) {
                    APTR vbr_new = AllocMem(256 * 4, MEMF_PUBLIC | MEMF_CLEAR | MEMF_REVERSE);
                    CopyMemQuick((APTR)vbr_old, vbr_new, 256 * 4);

                    asm volatile("movec %0, vbr"::"r"(vbr_new));
                    
                    RawDoFmt("[68040] VBR moved to %08lx\n", &vbr_new, (APTR)putch, NULL);
                }

                if (old_stack != NULL)
                    UserState(old_stack);
            }
        }

        binding.cb_ConfigDev->cd_Flags &= ~CDF_CONFIGME;
        binding.cb_ConfigDev->cd_Driver = LibraryBase;

        
        LibraryBase->emu_CachePreDMA_old = SetFunction((struct Library *)SysBase, -0x2fa, LibraryBase->emu_CachePreDMA);
        LibraryBase->emu_CachePostDMA_old = SetFunction((struct Library *)SysBase, -0x300, LibraryBase->emu_CachePostDMA);

        {
            ULONG args[] = {
                (ULONG)LibraryBase->emu_CachePreDMA_old, (ULONG)LibraryBase->emu_CachePreDMA
            };

            RawDoFmt("[68040] Patched CachePreDMA @ %08lx with %08lx\n", args, (APTR)putch, NULL);
        }
        {
            ULONG args[] = {
                (ULONG)LibraryBase->emu_CachePostDMA_old, (ULONG)LibraryBase->emu_CachePostDMA
            };

            RawDoFmt("[68040] Patched CachePostDMA @ %08lx with %08lx\n", args, (APTR)putch, NULL);
        }
    }

    CloseLibrary((struct Library*)ExpansionBase);


    return LibraryBase;
}