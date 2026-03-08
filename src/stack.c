/* src/stack.c
 * Kernel stack inspection tools.
 *
 * print_stack_trace()  — walk EBP chain and print return addresses
 * print_stack_dump()   — hex+ASCII dump of raw stack memory
 * print_registers()    — display current CPU register snapshot
 */

#include "stack.h"
#include "printk.h"

/* ─── print_stack_trace ────────────────────────────────────────────────────── */
void print_stack_trace(void)
{
    stack_frame_t *frame;

    /* Capture current EBP */
    __asm__ volatile("mov %%ebp, %0" : "=r"(frame));

    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\n╔══════════════════════════════════════════════════╗\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "║             Kernel Stack Trace                  ║\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "╠══════╦════════════════╦════════════════════════╣\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "║ Frm  ║ EBP            ║ Return Address         ║\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "╠══════╬════════════════╬════════════════════════╣\n");

    int depth = 0;
    while (frame && depth < STACK_MAX_FRAMES) {
        /* Safety: stop if EBP looks invalid (null or very low) */
        if ((uint32_t)frame < 0x1000)
            break;

        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "║ #%02d  ║ 0x%08x     ║ 0x%08x             ║\n",
            depth,
            (uint32_t)frame,
            frame->eip_ret);

        frame = frame->ebp_prev;
        depth++;
    }

    if (depth == 0)
        printk_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK,
            "║  (no valid frames found)                         ║\n");

    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "╚══════╩════════════════╩════════════════════════╝\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "  Total frames: %d\n\n", depth);
}

/* ─── print_stack_dump ─────────────────────────────────────────────────────── */
void print_stack_dump(uint32_t esp, uint32_t size)
{
    /* Round size up to 16-byte boundary */
    size = (size + 15) & ~15u;

    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\n╔══════════════════════════════════════════════════════════════╗\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "║  Stack Dump  [0x%08x .. 0x%08x]  (%u bytes)         ║\n",
        esp, esp + size - 1, size);
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "╠══════════════╦═════════════════════════════╦════════════════╣\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "║   Address    ║  00 01 02 03  04 05 06 07   ║    ASCII       ║\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "╠══════════════╬═════════════════════════════╬════════════════╣\n");

    for (uint32_t offset = 0; offset < size; offset += 8) {
        uint8_t *ptr = (uint8_t *)(esp + offset);

        /* Address */
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "║  0x%08x  ║  ", esp + offset);

        /* Hex bytes */
        for (int b = 0; b < 8; b++) {
            if (b == 4) printk(" ");
            printk_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK,
                "%02x ", ptr[b]);
        }

        /* ASCII */
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK, " ║  ");
        for (int b = 0; b < 8; b++) {
            char c = (ptr[b] >= 0x20 && ptr[b] < 0x7F) ? ptr[b] : '.';
            printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK, "%c", c);
        }
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "        ║\n");
    }

    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "╚══════════════╩═════════════════════════════╩════════════════╝\n\n");
}

/* ─── print_registers ──────────────────────────────────────────────────────── */
void print_registers(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t esp, ebp, esi, edi;
    uint32_t eflags, cs, ds, ss;

    __asm__ volatile(
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx)
    );
    __asm__ volatile(
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        "mov %%esi, %2\n"
        "mov %%edi, %3\n"
        : "=m"(esp), "=m"(ebp), "=m"(esi), "=m"(edi)
    );
    __asm__ volatile(
        "pushfl\n"
        "popl %0\n"
        "mov %%cs,  %1\n"
        "mov %%ds,  %2\n"
        "mov %%ss,  %3\n"
        : "=m"(eflags), "=m"(cs), "=m"(ds), "=m"(ss)
    );

    printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK,
        "\n╔══════════════════════════════════════╗\n");
    printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK,
        "║       CPU Register Snapshot          ║\n");
    printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK,
        "╠══════════╦═══════════════════════════╣\n");

    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  EAX     ║  0x%08x             ║\n", eax);
    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  EBX     ║  0x%08x             ║\n", ebx);
    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  ECX     ║  0x%08x             ║\n", ecx);
    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  EDX     ║  0x%08x             ║\n", edx);
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK, "║  ESP     ║  0x%08x             ║\n", esp);
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK, "║  EBP     ║  0x%08x             ║\n", ebp);
    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  ESI     ║  0x%08x             ║\n", esi);
    printk_color(VGA_COLOR_WHITE,      VGA_COLOR_BLACK, "║  EDI     ║  0x%08x             ║\n", edi);
    printk_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "║  EFLAGS  ║  0x%08x             ║\n", eflags);
    printk_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "║  CS      ║  0x%08x             ║\n", cs);
    printk_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "║  DS      ║  0x%08x             ║\n", ds);
    printk_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK, "║  SS      ║  0x%08x             ║\n", ss);

    printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK,
        "╚══════════╩═══════════════════════════╝\n\n");
}
