/* KFS_2 - Custom Linker Script */
/* Target: i386 (x86), kernel loaded at 1MB */

ENTRY(kernel_start)

SECTIONS
{
    /* Load at 1MB physical address */
    . = 0x00100000;

    .multiboot :
    {
        *(.multiboot)
    }
   
    .text ALIGN(4096) :
    {
        *(.text)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata)
    }

    .data ALIGN(4096) :
    {
        *(.data)
    }

    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss)
    }

    /* Ensure total size stays under 10MB */
    . = ASSERT(. < 0x00100000 + 10 * 1024 * 1024, "Kernel exceeds 10MB limit!");
}
