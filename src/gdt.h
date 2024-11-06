// gdt.h
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Descripteur de segment GDT en 64 bits
struct GDTDescriptor {
    uint16_t limit_low;    // Limite basse des 16 bits
    uint16_t base_low;     // Adresse basse des 16 bits
    uint8_t  base_middle;  // Partie moyenne de l'adresse (bits 16-23)
    uint8_t  access;       // Bits d'accès
    uint8_t  granularity;  // Granularité et bits limités
    uint8_t  base_high;    // Partie haute de l'adresse (bits 24-31)
} __attribute__((packed));

// Structure pour pointer vers la GDT
struct GDTPointer {
    uint16_t limit;        // Limite de la table
    uint32_t base;         // Adresse de base de la GDT
} __attribute__((packed));

// Définitions des macros pour les flags des descripteurs de segments
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Type de descripteur (0 pour système, 1 pour code/données)
#define SEG_PRES(x)      ((x) << 0x07) // Présence
#define SEG_SAVL(x)      ((x) << 0x0C) // Utilisable par le système
#define SEG_LONG(x)      ((x) << 0x0D) // Mode long
#define SEG_SIZE(x)      ((x) << 0x0E) // Taille (0 pour 16-bit, 1 pour 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularité (0 pour 1B-1MB, 1 pour 4KB-4GB)
#define SEG_PRIV(x)     (((x) & 0x03) << 0x05)   // Niveau de privilège (0-3)

// Types de descripteurs de segments
#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR

// Fonctions pour configurer et charger la GDT
void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void load_gdt(struct GDTPointer* gdt_ptr);

#endif // GDT_H

