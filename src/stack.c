#include "stack.h"
#include "printk.h"

/* Caracteres CP437 (page de code IBM PC - mode texte VGA)
 * \xc9 = top-left    \xbb = top-right
 * \xc8 = bot-left    \xbc = bot-right
 * \xcd = horizontal  \xba = vertical
 * \xcc = left-mid    \xb9 = right-mid
 * \xce = cross
 */

void print_stack_trace(void)
{
    stack_frame_t *frame;
    __asm__ volatile("mov %%ebp, %0" : "=r"(frame));

    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcb\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcb\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\xba  Frm  \xba  EBP        \xba  Return Addr \xba\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\xcc\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xb9\n");

    int depth = 0;
    while (frame && depth < STACK_MAX_FRAMES) {
        if ((uint32_t)frame < 0x1000 || (uint32_t)frame > 0x00200000)
            break;
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "\xba  #%02d   \xba  0x%08x  \xba  0x%08x  \xba\n",
            depth, (uint32_t)frame, frame->eip_ret);
        frame = frame->ebp_prev;
        depth++;
    }

    if (depth == 0)
        printk_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK,
            "\xba  (no valid frames found)              \xba\n");

    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xca\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xca\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc\n");
    printk_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK,
        "  Total frames: %d\n\n", depth);
}

void print_stack_dump(uint32_t esp, uint32_t size)
{
    size = (size + 15) & ~15u;

    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcb\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcb\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xba  Stack Dump 0x%08x            \xba  (%3u bytes)  \xba\n", esp, size);
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xcc\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xb9\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xba  Address       \xba  00 01 02 03  04 05 06 07  \xba  ASCII     \xba\n");
    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xcc\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xb9\n");

    for (uint32_t offset = 0; offset < size; offset += 8) {
        uint8_t *ptr = (uint8_t *)(esp + offset);
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "\xba  0x%08x   \xba  ", esp + offset);
        for (int b = 0; b < 8; b++) {
            if (b == 4) printk(" ");
            printk_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK, "%02x ", ptr[b]);
        }
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK, " \xba  ");
        for (int b = 0; b < 8; b++) {
            char c = (ptr[b] >= 0x20 && ptr[b] < 0x7F) ? ptr[b] : '.';
            printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK, "%c", c);
        }
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK, "  \xba\n");
    }

    printk_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK,
        "\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xca\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xca\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc\n\n");
}

void print_registers(void)
{
    uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi, eflags, cs, ds, ss;
    __asm__ volatile(
        "mov %%eax,%0\nmov %%ebx,%1\nmov %%ecx,%2\nmov %%edx,%3\n"
        :"=m"(eax),"=m"(ebx),"=m"(ecx),"=m"(edx));
    __asm__ volatile(
        "mov %%esp,%0\nmov %%ebp,%1\nmov %%esi,%2\nmov %%edi,%3\n"
        :"=m"(esp),"=m"(ebp),"=m"(esi),"=m"(edi));
    __asm__ volatile(
        "pushfl\npopl %0\nmov %%cs,%1\nmov %%ds,%2\nmov %%ss,%3\n"
        :"=m"(eflags),"=m"(cs),"=m"(ds),"=m"(ss));

    printk_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK,
        "\xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcb\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb\n"
        "\xba  EAX     \xba  0x%08x  \xba\n"
        "\xba  EBX     \xba  0x%08x  \xba\n"
        "\xba  ECX     \xba  0x%08x  \xba\n"
        "\xba  EDX     \xba  0x%08x  \xba\n"
        "\xcc\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xb9\n"
        "\xba  ESP     \xba  0x%08x  \xba\n"
        "\xba  EBP     \xba  0x%08x  \xba\n"
        "\xba  ESI     \xba  0x%08x  \xba\n"
        "\xba  EDI     \xba  0x%08x  \xba\n"
        "\xcc\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xce\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xb9\n"
        "\xba  EFLAGS  \xba  0x%08x  \xba\n"
        "\xba  CS      \xba  0x%08x  \xba\n"
        "\xba  DS      \xba  0x%08x  \xba\n"
        "\xba  SS      \xba  0x%08x  \xba\n"
        "\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xca\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc\n\n",
        eax,ebx,ecx,edx,esp,ebp,esi,edi,eflags,cs,ds,ss);
}
