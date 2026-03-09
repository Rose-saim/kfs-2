/* KFS_2 - Custom Linker Script */

ENTRY(kernel_start)

SECTIONS
{
    . = 0x00100000;

    /* .multiboot DOIT être en tout premier — GRUB cherche le header dans les 8KB */
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

    . = ASSERT(. < 0x00100000 + 10 * 1024 * 1024, "Kernel exceeds 10MB limit!");
}
