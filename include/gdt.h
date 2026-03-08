#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* ─── GDT Address ──────────────────────────────────────────────────────────── */
#define GDT_BASE_ADDRESS    0x00000800

/* ─── Segment selector indices ────────────────────────────────────────────── */
#define GDT_NULL            0   /* Null descriptor (required) */
#define GDT_KERNEL_CODE     1   /* Kernel code segment  */
#define GDT_KERNEL_DATA     2   /* Kernel data segment  */
#define GDT_KERNEL_STACK    3   /* Kernel stack segment */
#define GDT_USER_CODE       4   /* User code segment    */
#define GDT_USER_DATA       5   /* User data segment    */
#define GDT_USER_STACK      6   /* User stack segment   */
#define GDT_TSS             7   /* Task State Segment   */
#define GDT_ENTRIES         8

/* ─── Access byte flags ────────────────────────────────────────────────────── */
#define GDT_ACCESS_PRESENT      (1 << 7)    /* Segment present in memory */
#define GDT_ACCESS_RING0        (0 << 5)    /* Privilege level 0 (kernel) */
#define GDT_ACCESS_RING3        (3 << 5)    /* Privilege level 3 (user)   */
#define GDT_ACCESS_DESCRIPTOR   (1 << 4)    /* Code/data descriptor type  */
#define GDT_ACCESS_EXEC         (1 << 3)    /* Executable (code) segment  */
#define GDT_ACCESS_DC           (1 << 2)    /* Direction/Conforming bit   */
#define GDT_ACCESS_RW           (1 << 1)    /* Read/Write permission      */
#define GDT_ACCESS_ACCESSED     (1 << 0)    /* CPU sets this on access    */

/* ─── Granularity byte flags ───────────────────────────────────────────────── */
#define GDT_FLAG_GRANULARITY    (1 << 7)    /* 4KB page granularity       */
#define GDT_FLAG_32BIT          (1 << 6)    /* 32-bit protected mode      */
#define GDT_FLAG_64BIT          (1 << 5)    /* 64-bit (unused here)       */

/* ─── Segment descriptor structure (8 bytes) ──────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t limit_low;         /* Limit bits 0-15  */
    uint16_t base_low;          /* Base  bits 0-15  */
    uint8_t  base_mid;          /* Base  bits 16-23 */
    uint8_t  access;            /* Access byte      */
    uint8_t  granularity;       /* Flags + limit high bits 16-19 */
    uint8_t  base_high;         /* Base  bits 24-31 */
} gdt_entry_t;

/* ─── GDT Descriptor (loaded via lgdt) ────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t limit;             /* Size of GDT - 1  */
    uint32_t base;              /* Linear address of GDT */
} gdt_descriptor_t;

/* ─── Task State Segment (basic 32-bit TSS) ───────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t prev_tss;
    uint32_t esp0;              /* Stack pointer for ring 0 */
    uint32_t ss0;               /* Stack segment for ring 0 */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} tss_entry_t;

/* ─── Public API ───────────────────────────────────────────────────────────── */
void gdt_init(void);
void gdt_set_gate(int idx, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t granularity);
void gdt_print(void);

/* Defined in boot.asm */
extern void gdt_flush(uint32_t gdtr_ptr);
extern void tss_flush(void);

#endif /* GDT_H */
