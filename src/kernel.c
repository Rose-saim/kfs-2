/* src/kernel.c
 * KFS_2 — GDT & Stack
 * Kernel entry point.
 */

#include "gdt.h"
#include "printk.h"
#include "stack.h"

/* Multiboot magic expected from bootloader */
#define MULTIBOOT_MAGIC 0x2BADB002

/* ─── Banner ───────────────────────────────────────────────────────────────── */
static void print_banner(void)
{
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        " _  _______ ____    ____\n"
        "| |/ /  ___/ ___|  |___ \\\n"
        "| ' /| |_  \\___ \\    __) |\n"
        "| . \\|  _|  ___) |  / __/\n"
        "|_|\\_\\_|   |____/  |_____|\n\n");

    printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
        "  KFS_2  —  Global Descriptor Table & Stack\n");
    printk_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK,
        "  i386 protected mode kernel\n\n");
}

/* ─── Dummy functions to produce a visible call stack ──────────────────────── */
static void __attribute__((noinline)) level_c(void)
{
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "[stack] Capturing stack from level_c() ...\n");
    print_stack_trace();

    /* Also dump 64 bytes around current ESP */
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    print_stack_dump(esp, 64);
}

static void __attribute__((noinline)) level_b(void)
{
    level_c();
}

static void __attribute__((noinline)) level_a(void)
{
    level_b();
}

/* ─── kernel_main ──────────────────────────────────────────────────────────── */
void kernel_main(uint32_t mb_magic, uint32_t mb_info)
{
    (void)mb_info;  /* unused for now */

    /* 1. Init VGA terminal */
    terminal_init();
    print_banner();

    /* 2. Check multiboot magic */
    if (mb_magic != MULTIBOOT_MAGIC) {
        printk_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK,
            "[WARN] Bad multiboot magic: 0x%08x (expected 0x%08x)\n",
            mb_magic, MULTIBOOT_MAGIC);
    } else {
        printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
            "[OK]   Multiboot magic verified: 0x%08x\n", mb_magic);
    }

    /* 3. Install GDT at 0x00000800 */
    printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
        "[*]    Installing GDT at 0x%08x ...\n", GDT_BASE_ADDRESS);
    gdt_init();
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "[OK]   GDT loaded successfully.\n\n");

    /* 4. Print GDT contents */
    gdt_print();

    /* 5. Print CPU register snapshot */
    print_registers();

    /* 6. Demonstrate stack trace through a call chain */
    printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
        "[*]    Generating call chain: kernel_main -> level_a -> level_b -> level_c\n");
    level_a();

    /* 7. Done */
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\n[OK]   KFS_2 complete. System halted.\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}
