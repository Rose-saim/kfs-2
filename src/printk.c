/* src/printk.c
 * Kernel terminal output and printk implementation.
 * No stdlib — all code is self-contained.
 */

#include "printk.h"

/* ─── Internal state ───────────────────────────────────────────────────────── */
static uint16_t *vga_buf   = (uint16_t *)VGA_ADDRESS;
static uint8_t   term_col  = 0;
static uint8_t   term_row  = 0;
static uint8_t   term_color = 0;

/* ─── Helpers ──────────────────────────────────────────────────────────────── */
static inline uint8_t make_color(uint8_t fg, uint8_t bg)
{
    return fg | (bg << 4);
}

static inline uint16_t make_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void scroll(void)
{
    if (term_row < VGA_HEIGHT)
        return;

    /* Move every row up by one */
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buf[y * VGA_WIDTH + x] = vga_buf[(y + 1) * VGA_WIDTH + x];

    /* Clear last row */
    uint16_t blank = make_entry(' ', term_color);
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;

    term_row = VGA_HEIGHT - 1;
}

/* ─── Public terminal API ──────────────────────────────────────────────────── */
void terminal_init(void)
{
    term_col   = 0;
    term_row   = 0;
    term_color = make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    uint16_t blank = make_entry(' ', term_color);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_buf[i] = blank;
}

void terminal_clear(void)
{
    terminal_init();
}

void terminal_setcolor(uint8_t fg, uint8_t bg)
{
    term_color = make_color(fg, bg);
}

void terminal_putchar(char c)
{
    if (c == '\n') {
        term_col = 0;
        term_row++;
        scroll();
        return;
    }
    if (c == '\r') {
        term_col = 0;
        return;
    }
    if (c == '\t') {
        term_col = (term_col + 8) & ~7;
        if (term_col >= VGA_WIDTH) {
            term_col = 0;
            term_row++;
            scroll();
        }
        return;
    }

    vga_buf[term_row * VGA_WIDTH + term_col] = make_entry(c, term_color);
    if (++term_col >= VGA_WIDTH) {
        term_col = 0;
        if (++term_row >= VGA_HEIGHT)
            scroll();
    }
}

void terminal_write(const char *str)
{
    while (*str)
        terminal_putchar(*str++);
}

/* ─── Integer formatters ───────────────────────────────────────────────────── */
static void print_uint(unsigned int n, int base, int pad, char padchar)
{
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int  len = 0;

    if (n == 0) {
        buf[len++] = '0';
    } else {
        while (n) {
            buf[len++] = digits[n % base];
            n /= base;
        }
    }

    /* Padding */
    while (pad > len) {
        terminal_putchar(padchar);
        pad--;
    }

    /* Reverse print */
    for (int i = len - 1; i >= 0; i--)
        terminal_putchar(buf[i]);
}

static void print_int(int n)
{
    if (n < 0) {
        terminal_putchar('-');
        print_uint((unsigned int)(-n), 10, 0, ' ');
    } else {
        print_uint((unsigned int)n, 10, 0, ' ');
    }
}

/* ─── printk / printk_color ────────────────────────────────────────────────── */
static void vprintk(const char *fmt, va_list args)
{
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            terminal_putchar(*fmt);
            continue;
        }

        fmt++;

        /* Optional padding like %08x */
        char padchar = ' ';
        int  pad     = 0;
        if (*fmt == '0') { padchar = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') {
            pad = pad * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
            case 'd': print_int(va_arg(args, int));                         break;
            case 'u': print_uint(va_arg(args, unsigned int), 10, pad, padchar); break;
            case 'x': print_uint(va_arg(args, unsigned int), 16, pad, padchar); break;
            case 'X': print_uint(va_arg(args, unsigned int), 16, pad, padchar); break;
            case 'p':
                terminal_write("0x");
                print_uint(va_arg(args, unsigned int), 16, 8, '0');
                break;
            case 'c': terminal_putchar((char)va_arg(args, int));            break;
            case 's': terminal_write(va_arg(args, const char *));           break;
            case '%': terminal_putchar('%');                                 break;
            default:  terminal_putchar('%'); terminal_putchar(*fmt);        break;
        }
    }
}

void printk(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}

void printk_color(uint8_t fg, uint8_t bg, const char *fmt, ...)
{
    uint8_t saved = term_color;
    terminal_setcolor(fg, bg);

    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    term_color = saved;
}
