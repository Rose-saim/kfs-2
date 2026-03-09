/* src/gdt.c — Global Descriptor Table */

#include "gdt.h"
#include "printk.h"

/* Accès direct à l'adresse physique 0x00000800.
 * Le volatile + pragma supprime les faux warnings GCC sur les bornes. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

#define GDT ((gdt_entry_t *)GDT_BASE_ADDRESS)

static gdt_descriptor_t gdtr;
static tss_entry_t      tss;

extern uint8_t stack_top[];

/* ─── Remplir une entrée du GDT ─────────────────────────────────────────── */
void gdt_set_gate(int idx, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t granularity)
{
    GDT[idx].base_low    = (base  & 0xFFFF);
    GDT[idx].base_mid    = (base  >> 16) & 0xFF;
    GDT[idx].base_high   = (base  >> 24) & 0xFF;
    GDT[idx].limit_low   = (limit & 0xFFFF);
    GDT[idx].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    GDT[idx].access      = access;
}

/* ─── TSS ────────────────────────────────────────────────────────────────── */
static void tss_init(void)
{
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;

    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss_entry_t); i++) p[i] = 0;

    tss.ss0        = 0x10;
    tss.esp0       = (uint32_t)stack_top;
    tss.iomap_base = sizeof(tss_entry_t);

    gdt_set_gate(GDT_TSS, base, limit, 0x89, 0x00);
}

/* ─── Initialisation complète du GDT ────────────────────────────────────── */
void gdt_init(void)
{
    /* 0 - Null */
    gdt_set_gate(GDT_NULL, 0, 0, 0, 0);

    /* 1 - Kernel Code : ring 0, exécutable + lisible, 4GB flat */
    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_EXEC    | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 2 - Kernel Data : ring 0, lecture/écriture, 4GB flat */
    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 3 - Kernel Stack : ring 0 */
    gdt_set_gate(GDT_KERNEL_STACK, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 4 - User Code : ring 3, exécutable + lisible */
    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_EXEC    | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 5 - User Data : ring 3 */
    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 6 - User Stack : ring 3 */
    gdt_set_gate(GDT_USER_STACK, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR |
        GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_32BIT);

    /* 7 - TSS */
    tss_init();

    /* Charger le GDT dans le registre GDTR du CPU */
    gdtr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtr.base  = GDT_BASE_ADDRESS;

    gdt_flush((uint32_t)&gdtr);
    tss_flush();
}

/* ─── Affichage lisible du GDT ───────────────────────────────────────────── */
static const char *seg_names[GDT_ENTRIES] = {
    "Null        ", "Kernel Code ", "Kernel Data ",
    "Kernel Stack", "User Code   ", "User Data   ",
    "User Stack  ", "TSS         "
};

static uint32_t entry_base(int i)
{
    return (uint32_t)GDT[i].base_low          |
           ((uint32_t)GDT[i].base_mid  << 16) |
           ((uint32_t)GDT[i].base_high << 24);
}

static uint32_t entry_limit(int i)
{
    uint32_t lim = (uint32_t)GDT[i].limit_low |
                   (((uint32_t)GDT[i].granularity & 0x0F) << 16);
    if (GDT[i].granularity & GDT_FLAG_GRANULARITY)
        lim = (lim << 12) | 0xFFF;
    return lim;
}

void gdt_print(void)
{
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "\n+===+==============+============+============+====+========+\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "|        Global Descriptor Table @ 0x%08x          |\n", GDT_BASE_ADDRESS);
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "+===+==============+============+============+====+========+\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "| # | Segment      | Base       | Limit      |DPL | Access |\n");
    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "+===+==============+============+============+====+========+\n");

    for (int i = 0; i < GDT_ENTRIES; i++) {
        uint8_t dpl = (GDT[i].access >> 5) & 0x03;
        printk_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK,
            "| %d | %s | 0x%08x | 0x%08x | %d  |  0x%02x  |\n",
            i, seg_names[i],
            entry_base(i), entry_limit(i),
            dpl, GDT[i].access);
    }

    printk_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK,
        "+===+==============+============+============+====+========+\n\n");
}

#pragma GCC diagnostic pop
