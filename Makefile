# KFS_2 — GDT & Stack
# Makefile
#
# Requirements:
#   gcc (cross-compile or native with -m32)
#   nasm
#   ld  (GNU linker — NOT using host .ld files)
#   grub-mkrescue + xorriso  (to build a bootable ISO)
#   qemu-system-i386 or kvm  (to run)

# ─── Toolchain ──────────────────────────────────────────────────────────────
CC      := gcc
AS      := nasm
LD      := ld
MKISO   := grub-mkrescue

# ─── Target ─────────────────────────────────────────────────────────────────
KERNEL  := kfs2.bin
ISO     := kfs2.iso

# ─── Flags ──────────────────────────────────────────────────────────────────
# -m32              : compile for i386
# -fno-builtin      : don't use GCC built-in functions (no libc)
# -fno-exception (sujet) = -fno-exceptions (GCC) : pas de tables d'exceptions C++
# -fno-stack-protector : no canary (no libc support)
# -fno-rtti         : no run-time type info
# -nostdlib         : don't link standard startup files
# -nodefaultlibs    : don't link default libraries (libc, etc.)

CFLAGS  := -m32 \
            -std=c11 \
            -Wall -Wextra \
            -O2 \
            -fno-builtin \
            -fno-exceptions \
            -fno-stack-protector \
            -fno-rtti \
            -nostdlib \
            -fno-omit-frame-pointer \
            -nodefaultlibs \
            -Iinclude

ASFLAGS := -f elf32

LDFLAGS := -m elf_i386 \
            -T linker.ld \
            --nmagic

# ─── Sources ─────────────────────────────────────────────────────────────────
C_SRCS  := src/kernel.c \
            src/gdt.c   \
            src/printk.c \
            src/stack.c

ASM_SRCS := boot/boot.asm

C_OBJS   := $(C_SRCS:.c=.o)
ASM_OBJS := $(ASM_SRCS:.asm=.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)

# ─── Rules ───────────────────────────────────────────────────────────────────
.PHONY: all clean fclean re iso run run-kvm

all: $(KERNEL)

# Link — use our own linker.ld (NOT the host's)
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "[LD]  $@"
	@size $@

# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC]  $<"

# Assemble ASM sources
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@
	@echo "[AS]  $<"

# ─── ISO (bootable GRUB image) ───────────────────────────────────────────────
iso: $(ISO)

$(ISO): $(KERNEL)
	@mkdir -p iso/boot/grub
	@cp $(KERNEL) iso/boot/
	@echo 'set timeout=-1'                          >  iso/boot/grub/grub.cfg
	@echo 'set default=0'                          >> iso/boot/grub/grub.cfg
	@echo 'menuentry "KFS_2 - GDT & Stack" {'     >> iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/kfs2.bin'           >> iso/boot/grub/grub.cfg
	@echo '    boot'                               >> iso/boot/grub/grub.cfg
	@echo '}'                                      >> iso/boot/grub/grub.cfg
	$(MKISO) -o $@ iso
	@echo "[ISO] $@"

# ─── Run with QEMU ───────────────────────────────────────────────────────────
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -m 64M

# ─── Run with KVM (faster) ───────────────────────────────────────────────────
run-kvm: $(ISO)
	qemu-system-i386 -enable-kvm -cdrom $(ISO) -m 64M

# ─── Debug with QEMU + GDB ───────────────────────────────────────────────────
debug: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -m 64M -s -S &
	@echo "QEMU paused. Connect GDB with:"
	@echo "  gdb $(KERNEL)"
	@echo "  (gdb) target remote localhost:1234"
	@echo "  (gdb) continue"

# ─── Clean ───────────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS)
	@echo "[CLEAN] object files removed"

fclean: clean
	rm -f $(KERNEL) $(ISO)
	rm -rf iso/
	@echo "[FCLEAN] binaries removed"

re: fclean all
