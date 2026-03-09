#!/bin/bash

OK=0
FAIL=0

check() {
    if eval "$2" > /dev/null 2>&1; then
        echo "[OK]   $1"
        OK=$((OK+1))
    else
        echo "[FAIL] $1"
        FAIL=$((FAIL+1))
    fi
}

echo "==============================="
echo "  KFS_2 - Verification"
echo "==============================="

# Outils
check "nasm installe"                "which nasm"
check "gcc installe"                 "which gcc"
check "ld installe"                  "which ld"
check "grub-mkrescue installe"       "which grub-mkrescue"
check "xorriso installe"             "which xorriso"
check "qemu-system-i386 installe"    "which qemu-system-i386"

# Fichiers sources
check "boot/boot.asm present"        "test -f boot/boot.asm"
check "src/kernel.c present"         "test -f src/kernel.c"
check "src/gdt.c present"            "test -f src/gdt.c"
check "src/printk.c present"         "test -f src/printk.c"
check "src/stack.c present"          "test -f src/stack.c"
check "include/gdt.h present"        "test -f include/gdt.h"
check "include/printk.h present"     "test -f include/printk.h"
check "include/stack.h present"      "test -f include/stack.h"
check "linker.ld present"            "test -f linker.ld"
check "Makefile present"             "test -f Makefile"

# Compilation
check "make fclean"                  "make fclean"
check "make compile"                 "make"

# Binaire
check "kfs2.bin genere"              "test -f kfs2.bin"
check "kfs2.bin non vide"            "test -s kfs2.bin"
check "format ELF 32bit"             "file kfs2.bin | grep -q 'ELF 32-bit'"
check "architecture i386"            "file kfs2.bin | grep -q '80386'"

# Header Multiboot dans les 8KB
check "header multiboot < 8KB"       "python3 -c \"
d=open('kfs2.bin','rb').read()
i=d.find(b'\\x02\\xb0\\xad\\x1b')
exit(0 if 0<=i<8192 else 1)\""

# GDT a 0x00000800
check "GDT_BASE_ADDRESS=0x800"       "grep -q '0x00000800' include/gdt.h"

# Flags de compilation
check "flag -fno-builtin"            "grep -q 'fno-builtin' Makefile"
check "flag -fno-stack-protector"    "grep -q 'fno-stack-protector' Makefile"
check "flag -nostdlib"               "grep -q 'nostdlib' Makefile"
check "flag -nodefaultlibs"          "grep -q 'nodefaultlibs' Makefile"
check "flag -m32"                    "grep -q 'm32' Makefile"
check "flag -fno-omit-frame-pointer" "grep -q 'fno-omit-frame-pointer' Makefile"

# Taille < 10MB
check "taille < 10MB"                "test $(stat -c%s kfs2.bin) -lt 10485760"

# ISO
check "make iso"                     "make iso"
check "kfs2.iso genere"              "test -f kfs2.iso"

# ── Lancement QEMU ──────────────────────────────────────────────────────────
echo ""
echo "  -- Test lancement QEMU --"
echo ""

# Notre kernel ecrit dans le buffer VGA (0xB8000), pas sur port serie.
# On verifie que QEMU demarre, charge le kernel via GRUB, et ne triple-fault pas.
# Strategie : QEMU tourne 4s puis on envoie 'q' pour quitter proprement.
# Si QEMU s'arrete AVANT les 4s tout seul => crash (triple fault).

QEMU_LOG=$(mktemp)

# Lance QEMU avec log des evenements machine
timeout 6s qemu-system-i386 \
    -cdrom kfs2.iso \
    -m 64M \
    -display none \
    -D "$QEMU_LOG" \
    -d guest_errors \
    2>/dev/null &

QEMU_PID=$!
sleep 4

# Si QEMU tourne encore apres 4s => le kernel n'a pas crashe
if kill -0 $QEMU_PID 2>/dev/null; then
    echo "[OK]   QEMU demarre et kernel stable (4s sans crash)"
    OK=$((OK+1))
    kill $QEMU_PID 2>/dev/null
    wait $QEMU_PID 2>/dev/null
else
    echo "[FAIL] QEMU s'est arrete avant 4s (crash / triple fault)"
    FAIL=$((FAIL+1))
fi

# Verifie qu'il n'y a pas d'erreurs guest dans le log QEMU
if [ -f "$QEMU_LOG" ] && [ -s "$QEMU_LOG" ]; then
    echo "[FAIL] Erreurs guest detectees dans QEMU :"
    cat "$QEMU_LOG"
    FAIL=$((FAIL+1))
else
    echo "[OK]   Aucune erreur guest QEMU"
    OK=$((OK+1))
fi

rm -f "$QEMU_LOG"

echo ""
echo "==============================="
echo "  OK: $OK  FAIL: $FAIL"
echo "==============================="
test $FAIL -eq 0
