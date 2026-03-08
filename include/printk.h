#ifndef PRINTK_H
#define PRINTK_H

#include <stdint.h>
#include <stdarg.h>

/* VGA text buffer */
#define VGA_ADDRESS     0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25

/* VGA colors */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

void terminal_init(void);
void terminal_setcolor(uint8_t fg, uint8_t bg);
void terminal_putchar(char c);
void terminal_write(const char *str);
void terminal_clear(void);

/* printk: kernel printf — supports %s %d %u %x %p %c %% */
void printk(const char *fmt, ...);

/* Color helpers */
void printk_color(uint8_t fg, uint8_t bg, const char *fmt, ...);

#endif /* PRINTK_H */
