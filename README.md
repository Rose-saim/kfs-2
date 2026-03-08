# KFS_2 — Global Descriptor Table & Stack

## Overview

KFS_2 builds on KFS_1 by adding:

1. **GDT** placed at physical address `0x00000800`, with 6 required segments + null + TSS
2. **`printk`** — a minimal kernel printf supporting `%s %d %u %x %p %c %%`
3. **Stack inspection tools** — call-chain walker, raw hex dump, CPU register snapshot

---

## Project Structure

```
kfs2/
├── Makefile
├── linker.ld          ← Custom linker script (NOT the host's)
├── boot/
│   └── boot.asm       ← Multiboot header, stack, gdt_flush / tss_flush
├── include/
│   ├── gdt.h
│   ├── printk.h
│   └── stack.h
└── src/
    ├── kernel.c       ← kernel_main entry point
    ├── gdt.c          ← GDT setup & gdt_print()
    ├── printk.c       ← VGA terminal + printk
    └── stack.c        ← print_stack_trace, print_stack_dump, print_registers
```

---

## GDT Layout

| # | Segment      | Base | Limit | Ring |
|---|-------------|------|-------|------|
| 0 | Null        | 0    | 0     | —    |
| 1 | Kernel Code | 0    | 4 GB  | 0    |
| 2 | Kernel Data | 0    | 4 GB  | 0    |
| 3 | Kernel Stack| 0    | 4 GB  | 0    |
| 4 | User Code   | 0    | 4 GB  | 3    |
| 5 | User Data   | 0    | 4 GB  | 3    |
| 6 | User Stack  | 0    | 4 GB  | 3    |
| 7 | TSS         | &tss | …     | 0    |

GDT base address: **`0x00000800`**

---

## Build

```bash
# Requirements
sudo apt install gcc nasm binutils grub-pc-bin xorriso qemu-system-x86

# Build kernel binary
make

# Build bootable ISO
make iso

# Run in QEMU
make run

# Run with KVM (faster)
make run-kvm

# Debug with GDB
make debug
# then in another terminal:
# gdb kfs2.bin -ex "target remote localhost:1234"
```

---

## Compilation Flags

| Flag                 | Purpose                                     |
|----------------------|---------------------------------------------|
| `-m32`               | Target i386 (x86) 32-bit architecture       |
| `-fno-builtin`       | Disable GCC built-ins (no libc)             |
| `-fno-exceptions`    | Disable C++ exception tables                |
| `-fno-stack-protector` | No canary (requires libc)                 |
| `-fno-rtti`          | No run-time type info                       |
| `-nostdlib`          | Don't link standard startup files           |
| `-nodefaultlibs`     | Don't link libc or other default libs       |

---

## Stack Tools

### `print_stack_trace()`
Walks the EBP chain from current frame upwards, printing:
```
╔══════════════════════════════════════════════════╗
║             Kernel Stack Trace                  ║
╠══════╦════════════════╦════════════════════════╣
║ Frm  ║ EBP            ║ Return Address         ║
╠══════╬════════════════╬════════════════════════╣
║ #00  ║ 0x00104f20     ║ 0x001012a8             ║
║ #01  ║ 0x00104f40     ║ 0x001012c1             ║
```

### `print_stack_dump(esp, size)`
Hex + ASCII dump of raw stack memory.

### `print_registers()`
Captures and displays EAX–EDI, ESP, EBP, EFLAGS, CS, DS, SS.

---

## Memory Constraints

The linker script enforces that the kernel image stays **under 10 MB**:
```
. = ASSERT(. < 0x00100000 + 10 * 1024 * 1024, "Kernel exceeds 10MB limit!");
```
