/* src/kernel.c — KFS_2 : GDT & Stack */

#include "gdt.h"
#include "printk.h"
#include "stack.h"

#define MULTIBOOT_MAGIC 0x2BADB002

/* ─── Clavier PS/2 ───────────────────────────────────────────────────────── */
static uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void wait_key(void)
{
    printk_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK,
        "\n  [ Appuyez sur une touche... ]");
    while (inb(0x64) & 0x01) inb(0x60);
    while (!(inb(0x64) & 0x01));
    inb(0x60);
    terminal_clear();
}

/* ─── Demo pile ──────────────────────────────────────────────────────────── */
static void __attribute__((noinline)) level_c(void)
{
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));

    /* PAGE 3a : stack trace */
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "  Pile kernel — Stack Trace\n");
    printk_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK,
        "  kernel_main -> level_a -> level_b -> level_c\n\n");
    print_stack_trace();

    wait_key();

    /* PAGE 3b : stack dump */
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "  Pile kernel — Stack Dump\n\n");
    print_stack_dump(esp, 64);

    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\n[OK]   KFS_2 complet. Systeme en pause.\n");
}

static void __attribute__((noinline)) level_b(void) { level_c(); }
static void __attribute__((noinline)) level_a(void) { level_b(); }

/* ─── kernel_main ────────────────────────────────────────────────────────── */
void kernel_main(uint32_t mb_magic, uint32_t mb_info)
{
    (void)mb_info;
    terminal_init();

    /* ══ PAGE 1 : Demarrage ═════════════════════════════════════════════════ */
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "  KFS_2 - Global Descriptor Table & Stack\n\n");

    if (mb_magic != MULTIBOOT_MAGIC)
        printk_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK,
            "[WARN] Magic Multiboot incorrect : 0x%08x\n", mb_magic);
    else
        printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
            "[OK]   Magic Multiboot : 0x%08x\n", mb_magic);

    printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
        "[*]    Installation du GDT a 0x%08x...\n", GDT_BASE_ADDRESS);
    gdt_init();
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "[OK]   GDT charge - lgdt execute - GDTR mis a jour\n");

    wait_key();

    /* ══ PAGE 2 : Tableau GDT ═══════════════════════════════════════════════ */
    gdt_print();

    wait_key();

    /* ══ PAGE 3 : Pile kernel (geree dans level_c) ══════════════════════════ */
    level_a();

    for (;;)
        __asm__ volatile("cli; hlt");
}
