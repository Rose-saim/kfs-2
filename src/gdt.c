/* src/gdt.c
 * Global Descriptor Table setup.
 *
 * The GDT is placed at physical address 0x00000800 as required.
 * It contains 8 entries:
 *   0 - Null descriptor
 *   1 - Kernel code   (ring 0, execute/read)
 *   2 - Kernel data   (ring 0, read/write)
 *   3 - Kernel stack  (ring 0, read/write, expand-down)
 *   4 - User code     (ring 3, execute/read)
 *   5 - User data     (ring 3, read/write)
 *   6 - User stack    (ring 3, read/write, expand-down)
 *   7 - TSS           (task state segment)
 */

#include "gdt.h"
#include "printk.h"

/* ─── GDT at fixed address 0x00000800 ─────────────────────────────────────── */
static gdt_entry_t     *gdt = (gdt_entry_t *)GDT_BASE_ADDRESS;
static gdt_descriptor_t gdtr;
static tss_entry_t      tss;

/* ─── Kernel stack (used by TSS for ring-0 stack on privilege change) ──────── */
extern uint8_t stack_top[];     /* defined in boot.asm bss */

/* ─── gdt_set_gate ─────────────────────────────────────────────────────────── */
void gdt_set_gate(int idx, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t granularity)
{
    gdt[idx].base_low    = (base  & 0xFFFF);
    gdt[idx].base_mid    = (base  >> 16) & 0xFF;
    gdt[idx].base_high   = (base  >> 24) & 0xFF;

    gdt[idx].limit_low   = (limit & 0xFFFF);
    /* Upper nibble of granularity holds limit bits 16-19 */
    gdt[idx].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);

    gdt[idx].access      = access;
}

/* ─── TSS setup ────────────────────────────────────────────────────────────── */
static void tss_init(void)
{
    uint32_t tss_base  = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(tss_entry_t) - 1;

    /* Zero out TSS */
    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss_entry_t); i++)
        p[i] = 0;

    tss.ss0   = 0x10;               /* Kernel data segment selector */
    tss.esp0  = (uint32_t)stack_top; /* Ring-0 stack pointer         */
    tss.iomap_base = sizeof(tss_entry_t);

    /* TSS descriptor: access=0x89 (present, ring0, TSS32 available) */
    gdt_set_gate(GDT_TSS, tss_base, tss_limit, 0x89, 0x00);
}

/* ─── gdt_init ─────────────────────────────────────────────────────────────── */
void gdt_init(void)
{
    /* ── Entry 0: Null descriptor (required by x86) ── */
    gdt_set_gate(GDT_NULL, 0, 0, 0, 0);

    /* ── Entry 1: Kernel Code ──
     * Base=0, Limit=4GB, Ring 0, Executable+Readable
     * Flags: 4KB granularity, 32-bit
     */
    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_EXEC    | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 2: Kernel Data ──
     * Base=0, Limit=4GB, Ring 0, Read/Write
     */
    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 3: Kernel Stack ──
     * Base=0, Limit=4GB, Ring 0, Read/Write
     * (same as data but semantically separate; grows down naturally)
     */
    gdt_set_gate(GDT_KERNEL_STACK, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 4: User Code ──
     * Base=0, Limit=4GB, Ring 3, Executable+Readable
     */
    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_EXEC    | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 5: User Data ──
     * Base=0, Limit=4GB, Ring 3, Read/Write
     */
    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 6: User Stack ──
     * Base=0, Limit=4GB, Ring 3, Read/Write
     */
    gdt_set_gate(GDT_USER_STACK, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* ── Entry 7: TSS ── */
    tss_init();

    /* ── Load the GDT ── */
    gdtr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtr.base  = GDT_BASE_ADDRESS;

    gdt_flush((uint32_t)&gdtr);
    tss_flush();
}

/* ─── gdt_print: human-friendly GDT dump ──────────────────────────────────── */
static const char *seg_names[GDT_ENTRIES] = {
    "Null        ",
    "Kernel Code ",
    "Kernel Data ",
    "Kernel Stack",
    "User Code   ",
    "User Data   ",
    "User Stack  ",
    "TSS         "
};

static uint32_t entry_base(int i)
{
    return (uint32_t)gdt[i].base_low         |
           ((uint32_t)gdt[i].base_mid  << 16) |
           ((uint32_t)gdt[i].base_high << 24);
}

static uint32_t entry_limit(int i)
{
    uint32_t lim = (uint32_t)gdt[i].limit_low |
                   (((uint32_t)gdt[i].granularity & 0x0F) << 16);
    if (gdt[i].granularity & GDT_FLAG_GRANULARITY)
        lim = (lim << 12) | 0xFFF;
    return lim;
}

void gdt_print(void)
{
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "\n╔══════════════════════════════════════════════════════════╗\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "║            Global Descriptor Table (GDT)                ║\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "║  Base address: 0x%08x   Entries: %d                  ║\n",
        GDT_BASE_ADDRESS, GDT_ENTRIES);
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "╠═══╦══════════════╦════════════╦════════════╦════╦════════╣\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "║ # ║ Segment      ║ Base       ║ Limit      ║DPL ║ Access ║\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "╠═══╬══════════════╬════════════╬════════════╬════╬════════╣\n");

    for (int i = 0; i < GDT_ENTRIES; i++) {
        uint8_t dpl = (gdt[i].access >> 5) & 0x03;
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "║ %d ║ %s ║ 0x%08x ║ 0x%08x ║ %d  ║  0x%02x  ║\n",
            i, seg_names[i],
            entry_base(i), entry_limit(i),
            dpl, gdt[i].access);
    }

    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "╚═══╩══════════════╩════════════╩════════════╩════╩════════╝\n");
}
