MEMORY
{
    rom (rx) : org = 0x00000000, l = 64K
    ram (rw) : org = 0xdeaddead, l = 0M
}

ENTRY(_rom_start)

SECTIONS
{
    .text 0x0 : 
    {
        _rom_start = .;

        /* Everything from main.c will be copied to ram together with DiagArea */
        *main.c.o(.text .text.* .rodata .rodata.*)

        *(.text .text.* .rodata .rodata.*)
        *(.eh_frame)

        . = ALIGN(4096);
    } >rom = 0xff

    _rom_end = .;

    .data : {
        *(.data .data.*)
    } >ram

    .bss : {
        *(.bss)
    } >ram
}